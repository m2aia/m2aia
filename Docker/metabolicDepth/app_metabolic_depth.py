#!/usr/bin/env python3
"""Metabolic Depth Analysis for Mass Spectrometry Imaging Data using Multi-GASTON.

This script applies the Multi-GASTON neural network method to learn metabolic depth
(isodepth) from mass spectrometry imaging data in imzML format. The method learns
spatial gradients in metabolite abundance patterns.
"""

import argparse
import logging
import sys
import os
from pathlib import Path
import numpy as np
import pandas as pd

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser(
    prog="MetabolicDepth",
    description="Apply Multi-GASTON to learn metabolic depth from mass spectrometry imaging data.",
    epilog="Based on: https://github.com/raphael-group/Multi-GASTON",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
)

# Input/Output arguments
parser.add_argument("-i", "--imzml", required=True,
                    help="Path to input imzML file")
parser.add_argument("-m", "--mask", required=False, default=None,
                    help="Path to mask image file (optional)")
parser.add_argument("-l", "--mask_label", required=False, default=None, type=int,
                    help="Mask label value to use (if mask provided)")
parser.add_argument("-c", "--centroids", required=True,
                    help="Path to centroids CSV file (m/z values)")
parser.add_argument("--out_depthmap", required=True,
                    help="Path to output metabolic depth map (NRRD format)")
parser.add_argument("--out_contours", required=True,
                    help="Path to output contour segmentation (NRRD format)")
parser.add_argument("--output", required=True,
                    help="Path to output non autoload data (e.g., intermediate results, logs)")
# Preprocessing arguments
parser.add_argument("--normalization", default="TIC",
                    help="Normalization method (e.g., TIC, None)")
parser.add_argument("--intensity_transform", default="Log",
                    help="Intensity transformation method (e.g., Log, Sqrt, None)")
parser.add_argument("--range_pooling", default="Mean",
                    help="Range pooling method")
parser.add_argument("--smoothing", default="None",
                    help="Smoothing method")
parser.add_argument("--baseline_correction", default="None",
                    help="Baseline correction method")
parser.add_argument("--smoothing_value", default=2, type=int,
                    help="Smoothing parameter value")
parser.add_argument("--baseline_correction_value", default=50, type=int,
                    help="Baseline correction parameter value")
parser.add_argument("--tolerance", default=75, type=float,
                    help="Mass tolerance in ppm")

# Multi-GASTON neural network parameters
parser.add_argument("-K", "--n_isodepths", default=1, type=int,
                    help="Number of isodepths to learn (number of distinct gradients expected)")
parser.add_argument("--met_dep_arch", default="400,400,400", type=str,
                    help="Architecture for metabolic depth network (comma-separated layer sizes)")
parser.add_argument("--abundance_arch", default="10,10,10,10", type=str,
                    help="Architecture for 1-D expression function (comma-separated layer sizes)")
parser.add_argument("--num_epochs", default=12000, type=int,
                    help="Number of training epochs")
parser.add_argument("--checkpoint", default=3000, type=int,
                    help="Save model after number of epochs = multiple of checkpoint")
parser.add_argument("--optimizer", default="adam", type=str,
                    help="Optimizer to use (adam, sgd, etc.)")
parser.add_argument("--num_restarts", default=10, type=int,
                    help="Number of random initializations to train")
parser.add_argument("-r", "--random_state", default=None, type=int,
                    help="Random seed for reproducibility")

args = parser.parse_args()


def validate_inputs(args):
    """Validate input arguments and file existence."""
    imzml_path = Path(args.imzml)
    if not imzml_path.exists():
        logger.error(f"ImzML file not found: {args.imzml}")
        sys.exit(1)
    
    centroids_path = Path(args.centroids)
    if not centroids_path.exists():
        logger.error(f"Centroids file not found: {args.centroids}")
        sys.exit(1)
    
    # Validate mask inputs
    if args.mask is not None:
        mask_path = Path(args.mask)
        if not mask_path.exists():
            logger.error(f"Mask file not found: {args.mask}")
            sys.exit(1)
        if args.mask_label is None:
            logger.error("--mask_label must be provided when using a mask")
            sys.exit(1)
    

    # Use parent directory for intermediate outputs
    # Validate neural network parameters
    if args.n_isodepths < 1:
        logger.error(f"n_isodepths must be >= 1, got {args.n_isodepths}")
        sys.exit(1)
    
    if args.num_epochs < 1:
        logger.error(f"num_epochs must be >= 1, got {args.num_epochs}")
        sys.exit(1)
    
    if args.tolerance <= 0:
        logger.error(f"tolerance must be > 0, got {args.tolerance}")
        sys.exit(1)
    
    logger.info(f"Input validation passed")


try:
    import m2aia as m2
    import SimpleITK as sitk
    from multi_gaston import data_processing, multi_gaston
    import torch
except ImportError as e:
    logger.error(f"Failed to import required module: {e}")
    logger.error("Please ensure m2aia, SimpleITK, and multi-gaston are installed")
    sys.exit(1)

logger.info("Starting Metabolic Depth Analysis")

# Check for GPU availability
if torch.cuda.is_available():
    device = torch.device("cuda")
    logger.info(f"GPU available: {torch.cuda.get_device_name(0)}")
    logger.info(f"CUDA version: {torch.version.cuda}")
else:
    device = torch.device("cpu")
    logger.info("GPU not available, using CPU")

validate_inputs(args)

# Load centroids
try:
    logger.info(f"Loading centroids from: {args.centroids}")
    centroids_df = pd.read_csv(args.centroids)
    # Assume first column contains m/z values
    centroids = centroids_df.iloc[:, 0].values.astype(np.float32)
    
    if centroids.size == 0:
        logger.error("Centroids file is empty")
        sys.exit(1)
    
    # Get metabolite names if available in the file
    if centroids_df.shape[1] > 1:
        metabolite_names = centroids_df.iloc[:, 1].values
    else:
        metabolite_names = np.array([f"mz_{mz:.4f}" for mz in centroids])
    
    logger.info(f"Loaded {centroids.shape[0]} centroids")
except Exception as e:
    logger.error(f"Failed to load centroids file: {e}")
    sys.exit(1)

# Load ImzML data
try:
    logger.info(f"Loading imzML file: {args.imzml}")
    reader = m2.ImzMLReader(str(args.imzml))
    reader.SetSmoothing(args.smoothing, args.smoothing_value)
    reader.SetBaselineCorrection(args.baseline_correction, args.baseline_correction_value)
    reader.SetNormalization(args.normalization)
    reader.SetPooling(args.range_pooling)
    reader.SetIntensityTransformation(args.intensity_transform)
    reader.Load()
    logger.info("ImzML file loaded successfully")
except Exception as e:
    logger.error(f"Failed to load imzML file: {e}")
    sys.exit(1)

# Get or load mask
try:
    if args.mask is not None:
        logger.info(f"Loading mask from: {args.mask}")
        mask_sitk = sitk.ReadImage(str(args.mask))
        mask_array = sitk.GetArrayFromImage(mask_sitk)
        # Apply label filtering
        mask = (mask_array == args.mask_label).astype(np.uint8)
        logger.info(f"Using mask label {args.mask_label}")
    else:
        logger.info("No mask provided, using default mask from imzML")
        mask = reader.GetMaskArray()
        mask = (mask == args.mask_label).astype(np.uint8)
    
    num_pixels = np.sum(mask)
    
    if num_pixels == 0:
        logger.error("Mask is empty - no valid pixels found")
        sys.exit(1)
    
    logger.info(f"Processing {num_pixels} valid pixels")
except Exception as e:
    logger.error(f"Failed to process mask: {e}")
    sys.exit(1)

# Extract spatial coordinates and metabolite abundances
try:
    logger.info("Extracting spatial coordinates and metabolite abundances...")
    
    # Get image dimensions
    image_shape = mask.shape
    
    # Extract coordinates of valid pixels
    coords_list = []
    abundances_list = []
    
    # Iterate through all pixels
    for z in range(image_shape[0]):
        for y in range(image_shape[1]):
            for x in range(image_shape[2]):
                if mask[z, y, x] == 1:
                    # For 2D MSI data, typically z=0, so we use x,y coordinates
                    # Adjust based on your data orientation
                    coords_list.append([x, y])
    
    coords = np.array(coords_list, dtype=np.float32)
    logger.info(f"Extracted coordinates: {coords.shape}")
    
    # Extract metabolite abundances for each centroid
    logger.info("Extracting metabolite abundances...")
    abundances = np.zeros((num_pixels, len(centroids)), dtype=np.float32)
    
    for i, mz_value in enumerate(centroids):
        if (i + 1) % 100 == 0 or i == len(centroids) - 1:
            logger.info(f"  Processing centroid {i+1}/{len(centroids)}...")
        ion_image = reader.GetArray(mz_value, args.tolerance)
        abundances[:, i] = ion_image[mask == 1]
    
    logger.info(f"Metabolite abundance matrix: {abundances.shape}")
    
except Exception as e:
    logger.error(f"Failed to extract data from imzML: {e}")
    sys.exit(1)

# Prepare data for Multi-GASTON
# Multi-GASTON expects:
# - S: N x 2 spatial coordinate matrix
# - A: N x M metabolite abundance matrix
# where N = number of spatial locations, M = number of metabolites
try:
    logger.info("Preparing data for Multi-GASTON...")
    
    # S is already the coordinates (N x 2)
    S = coords
    
    # A is the abundance matrix (N x M)
    A = abundances
    
    logger.info(f"Spatial coordinates shape: {S.shape}")
    logger.info(f"Abundance matrix shape: {A.shape}")
    
    # Get dimensions
    X_dim = int(np.max(S[:, 0]) - np.min(S[:, 0])) + 1
    Y_dim = int(np.max(S[:, 1]) - np.min(S[:, 1])) + 1
    
    logger.info(f"Data dimensions: {X_dim} x {Y_dim}")
    
    # Transform to torch tensors and normalize
    S_torch, A_torch = data_processing.tensor_transform_inputs(S, A)
    
    logger.info(f"Data transformed to torch tensors and normalized")
    logger.info(f"Device available: {device}")
    
except Exception as e:
    logger.error(f"Failed to prepare data for Multi-GASTON: {e}")
    sys.exit(1)

# Parse architecture strings
try:
    met_dep_arch = [int(x) for x in args.met_dep_arch.split(',')]
    abundance_arch = [int(x) for x in args.abundance_arch.split(',')]
    logger.info(f"Metabolic depth network architecture: {met_dep_arch}")
    logger.info(f"Abundance network architecture: {abundance_arch}")
except Exception as e:
    logger.error(f"Failed to parse architecture strings: {e}")
    sys.exit(1)

# Train Multi-GASTON neural network
try:
    logger.info("Training Multi-GASTON neural network...")
    logger.info(f"Parameters: K={args.n_isodepths}, epochs={args.num_epochs}, restarts={args.num_restarts}")
    
    # Create output directory for intermediate results
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Determine seed list
    if args.random_state is not None:
        seed_list = [args.random_state + i for i in range(args.num_restarts)]
    else:
        seed_list = range(args.num_restarts)
    
    for i, seed in enumerate(seed_list, 1):
        logger.info(f"Training model with seed {seed} ({i}/{args.num_restarts})...")
        (output_dir / f"rep{seed}").mkdir(exist_ok=True)  # Ensure the directory exists for checking files
        
        # multi_gaston saves files with rep{seed} prefix directly in SAVE_PATH
        # e.g., rep0coordinate.txt, rep0loss_list.npy, etc.
        mod, loss_list, lasso_loss, _ = multi_gaston.train(
            S_torch, A_torch,
            S_hidden_list=met_dep_arch,
            A_hidden_list=abundance_arch,
            epochs=args.num_epochs,
            checkpoint=args.checkpoint,
            K=args.n_isodepths,
            SAVE_PATH=str((output_dir / f"rep{seed}")) + "/",
            optim=args.optimizer,
            seed=seed
        )
        
        final_loss = loss_list[-1] if len(loss_list) > 0 else float('inf')
        logger.info(f"  Seed {seed} training completed. Final loss: {final_loss}")
        
        # Verify that required output files were created
        # multi_gaston saves: rep{seed}coordinate.txt, rep{seed}loss_list.npy
        coordinate_file = output_dir / f"rep{seed}"/ f"coordinate.txt"
        loss_file = output_dir / f"rep{seed}"/ f"loss_list.npy"
        
        if not coordinate_file.exists():
            logger.warning(f"  rep{seed}coordinate.txt not found at {coordinate_file}")
        if not loss_file.exists():
            logger.warning(f"  rep{seed}loss_list.npy not found at {loss_file}")
    
    logger.info("All models trained successfully")
    
except Exception as e:
    logger.error(f"Failed to train Multi-GASTON model: {e}")
    sys.exit(1)

# Generate output NRRD files
try:
    logger.info("Generating output files...")
    
    # Find the best trial based on loss
    best_seed = None
    best_loss = float('inf')
    
    for seed in seed_list:
        # Load loss from numpy array file
        loss_file = output_dir / f"rep{seed}"/ f"loss_list.npy"
        if loss_file.exists():
            try:
                loss_array = np.load(str(loss_file))
                if len(loss_array) > 0:
                    final_loss = float(loss_array[-1])
                    if final_loss < best_loss:
                        best_loss = final_loss
                        best_seed = seed
            except Exception as e:
                logger.warning(f"Could not load loss file for seed {seed}: {e}")
    
    if best_seed is None:
        logger.error("Could not determine best model - no loss files found")
        logger.error(f"Expected loss files: {output_dir}/rep<seed>/loss_list.npy")
        # List what files actually exist
        existing_files = list(output_dir.glob("rep*/loss_list.npy"))
        logger.error(f"Found loss files: {[f.name for f in existing_files]}")
        sys.exit(1)
    
    logger.info(f"Best model: seed {best_seed} with loss {best_loss}")
    
    # Load best isodepth coordinate file
    best_isodepth_file = output_dir / f"rep{best_seed}/coordinate.txt"
    if not best_isodepth_file.exists():
        logger.error(f"Best isodepth file not found: {best_isodepth_file}")
        # List what coordinate files do exist
        existing_coord_files = list(output_dir.glob("rep*/coordinate.txt"))
        logger.error(f"Found coordinate files: {[f.name for f in existing_coord_files]}")
        sys.exit(1)
    
    best_isodepth = np.loadtxt(str(best_isodepth_file))
    
    # Reconstruct the full 3D image with isodepth values
    isodepth_image = np.zeros(image_shape + (args.n_isodepths,), dtype=np.float32)
    
    pixel_idx = 0
    for z in range(image_shape[0]):
        for y in range(image_shape[1]):
            for x in range(image_shape[2]):
                if mask[z, y, x] == 1:
                    isodepth_image[z, y, x] = best_isodepth[pixel_idx]
                    pixel_idx += 1
    
    # Save continuous depth values as NRRD
    isodepth_sitk = sitk.GetImageFromArray(isodepth_image)
    isodepth_sitk.SetSpacing(mask_sitk.GetSpacing())
    sitk.WriteImage(isodepth_sitk, args.out_depthmap)
    logger.info(f"Metabolic depth image saved to {args.out_depthmap}")
    
    # Create multi-label segmentation with contour levels
    num_contour_levels = 10
    logger.info(f"Creating multi-label segmentation with {num_contour_levels} contour levels")
    
    # Get min and max isodepth values in masked region
    valid_isodepth_values = best_isodepth[np.isfinite(best_isodepth)]
    if len(valid_isodepth_values) == 0:
        logger.error("No valid isodepth values found")
        sys.exit(1)
    
    min_depth = np.min(valid_isodepth_values)
    max_depth = np.max(valid_isodepth_values)
    
    # Create contour boundaries
    contour_boundaries = np.linspace(min_depth, max_depth, num_contour_levels + 1)
    logger.info(f"Isodepth range: [{min_depth:.4f}, {max_depth:.4f}]")
    
    # Create multi-label image (0 = background, 1-N = contour levels)
    multilabel_image = np.zeros(image_shape + (args.n_isodepths,), dtype=np.uint16)
    
    pixel_idx = 0
    for z in range(image_shape[0]):
        for y in range(image_shape[1]):
            for x in range(image_shape[2]):
                if mask[z, y, x] == 1:
                    depth_values = best_isodepth[pixel_idx]
                    # convert to list if necessary
                    if not isinstance(depth_values, np.ndarray):
                        depth_values = np.array([depth_values])
                    # Assign to contour level (1-based indexing, 0 is background)
                    for level in range(num_contour_levels):
                        for i, depth_value in enumerate(depth_values): 
                            if depth_value >= contour_boundaries[level] and depth_value <= contour_boundaries[level + 1]:
                                multilabel_image[z, y, x, i] = level + 1
                                break
                    pixel_idx += 1
    
    # Save as NRRD multi-label segmentation
    multilabel_sitk = sitk.GetImageFromArray(multilabel_image)
    multilabel_sitk.SetSpacing(mask_sitk.GetSpacing())
    sitk.WriteImage(multilabel_sitk, args.out_contours)
    logger.info(f"Multi-label contour segmentation saved to {args.out_contours}")
    logger.info(f"Labels: 0 = background, 1-{num_contour_levels} = contour levels (ascending depth)")
    
    logger.info("Metabolic Depth Analysis completed successfully!")
    
except Exception as e:
    logger.error(f"Failed to generate output files: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
