#!/usr/bin/env python3
"""UMAP dimensionality reduction for mass spectrometry imaging data.

This script applies UMAP (Uniform Manifold Approximation and Projection) to
mass spectrometry imaging data in imzML format, reducing the dimensionality
of spectral data to a lower-dimensional embedding.
"""

import argparse
import logging
import sys
from pathlib import Path
import numpy as np

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser(
    prog="UMAP",
    description="Apply UMAP dimensionality reduction to mass spectrometry imaging data.",
    epilog="Based on: https://github.com/lmcinnes/umap",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter
)

# Input/Output arguments
parser.add_argument("-i", "--imzml", required=True,
                    help="Path to directory containing input imzML file(s)")
parser.add_argument("-c", "--centroids", required=True,
                    help="Path to centroids CSV file (m/z values)")
parser.add_argument("-o", "--out", required=True,
                    help="Path to output directory for UMAP images")

# Preprocessing arguments
parser.add_argument("--normalization", default="None",
                    help="Normalization method")
parser.add_argument("--intensity_transform", default="None",
                    help="Intensity transformation method")
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

# UMAP algorithm arguments
parser.add_argument("-e", "--n_epochs", default=None, type=int,
                    help="Number of training epochs (None=automatic)")
parser.add_argument("-nn", "--n_neighbors", default=15, type=int,
                    help="Number of nearest neighbors")
parser.add_argument("-nc", "--n_components", default=2, type=int,
                    help="Dimension of the target embedding space")
parser.add_argument("-j", "--n_jobs", default=-1, type=int,
                    help="Number of parallel jobs (-1=all cores)")
parser.add_argument("-m", "--metric", default="euclidean", type=str,
                    help="Distance metric to use")
parser.add_argument("-mkwds", "--metric_kwds", default=None, type=str,
                    help="Additional metric keyword arguments (JSON string)")
parser.add_argument("-om", "--output_metric", default="euclidean", type=str,
                    help="Output space distance metric")
parser.add_argument("-omkwds", "--output_metric_kwds", default=None, type=str,
                    help="Additional output metric keyword arguments (JSON string)")
parser.add_argument("-lr", "--learning_rate", default=1.0, type=float,
                    help="Learning rate for optimization")
parser.add_argument("-in", "--init", default="spectral", type=str,
                    help="Initialization method (spectral, random, pca)")
parser.add_argument("-md", "--min_dist", default=0.1, type=float,
                    help="Minimum distance between embedded points")
parser.add_argument("-s", "--spread", default=1.0, type=float,
                    help="Effective scale of embedded points")
parser.add_argument("-lm", "--low_memory", default=False, action="store_true",
                    help="Use low memory mode")
parser.add_argument("--set_op_mix_ratio", default=1.0, type=float,
                    help="Fuzzy set union vs intersection ratio")
parser.add_argument("--local_connectivity", default=1.0, type=float,
                    help="Local connectivity requirement")
parser.add_argument("--repulsion_strength", default=1.0, type=float,
                    help="Repulsion strength for negative samples")
parser.add_argument("--negative_sample_rate", default=5, type=int,
                    help="Number of negative samples per positive")
parser.add_argument("--transform_queue_size", default=4.0, type=float,
                    help="Queue size for transform operations")
parser.add_argument("--a", default=None, type=float,
                    help="Specific parameter a for embedding")
parser.add_argument("--b", default=None, type=float,
                    help="Specific parameter b for embedding")
parser.add_argument("-r", "--random_state", default=None, type=int,
                    help="Random seed for reproducibility")
parser.add_argument("--angular_rp_forest", default=False, action="store_true",
                    help="Use angular random projection forest")
parser.add_argument("--target_n_neighbors", default=-1, type=int,
                    help="Number of nearest neighbors for target")
parser.add_argument("--target_metric", default="categorical", type=str,
                    help="Distance metric for target space")
parser.add_argument("--target_metric_kwds", default=None, type=str,
                    help="Additional target metric arguments (JSON string)")
parser.add_argument("--target_weight", default=0.5, type=float,
                    help="Weight for target in supervised learning")
parser.add_argument("--transform_seed", default=42, type=int,
                    help="Random seed for transform operations")
parser.add_argument("--transform_mode", default="embedding", type=str,
                    help="Transform mode (embedding or graph)")
parser.add_argument("--force_approximation_algorithm", default=False, action="store_true",
                    help="Force use of approximation algorithm")
parser.add_argument("--verbose", default=False, action="store_true",
                    help="Enable verbose output")
parser.add_argument("--tqdm_kwds", default=None, type=str,
                    help="Additional tqdm keyword arguments (JSON string)")
parser.add_argument("--unique", default=False, action="store_true",
                    help="Only consider unique data points")
parser.add_argument("-d", "--densmap", default=False, action="store_true",
                    help="Enable density-preserving UMAP")
parser.add_argument("-dl", "--dens_lambda", default=2.0, type=float,
                    help="Density regularization strength")
parser.add_argument("-df", "--dens_frac", default=0.3, type=float,
                    help="Fraction of epochs for density optimization")
parser.add_argument("-dvs", "--dens_var_shift", default=0.1, type=float,
                    help="Variance shift for density estimation")
parser.add_argument("-do", "--output_dens", default=False, action="store_true",
                    help="Output density information")
parser.add_argument("--disconnection_distance", default=None, type=float,
                    help="Distance threshold for disconnection")

args = parser.parse_args()


def validate_inputs(args):
    """Validate input arguments and file existence."""
    # Check if imzml path is a directory
    imzml_path = Path(args.imzml)
    if not imzml_path.exists():
        logger.error(f"ImzML path not found: {args.imzml}")
        sys.exit(1)
    
    # If it's a directory, find all .imzML files
    if imzml_path.is_dir():
        imzml_files = sorted(imzml_path.glob('*.imzML'))
        if not imzml_files:
            logger.error(f"No .imzML files found in directory: {args.imzml}")
            sys.exit(1)
        # Convert Path objects to strings
        imzml_files = [str(f) for f in imzml_files]
    else:
        # Single file
        imzml_files = [str(imzml_path)]
    
    # Store the list of files back in args
    args.imzml_files = imzml_files
    
    centroids_path = Path(args.centroids)
    if not centroids_path.exists():
        logger.error(f"Centroids file not found: {args.centroids}")
        sys.exit(1)
    
    # Validate/create output directory
    output_path = Path(args.out)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Validate UMAP parameters
    if args.n_components < 1:
        logger.error(f"n_components must be >= 1, got {args.n_components}")
        sys.exit(1)
    
    if args.n_neighbors < 2:
        logger.error(f"n_neighbors must be >= 2, got {args.n_neighbors}")
        sys.exit(1)
    
    if not 0 <= args.min_dist <= args.spread:
        logger.error(f"min_dist ({args.min_dist}) must be between 0 and spread ({args.spread})")
        sys.exit(1)
    
    if args.tolerance <= 0:
        logger.error(f"tolerance must be > 0, got {args.tolerance}")
        sys.exit(1)
    
    logger.info(f"Input validation passed")


try:
    import m2aia as m2
    import umap
    import SimpleITK as sitk
except ImportError as e:
    logger.error(f"Failed to import required module: {e}")
    logger.error("Please ensure m2aia, umap-learn, and SimpleITK are installed")
    sys.exit(1)

logger.info("Starting UMAP analysis")

validate_inputs(args)

logger.info(f"Processing {len(args.imzml_files)} image(s)")
logger.info(f"UMAP parameters: n_neighbors={args.n_neighbors}, n_components={args.n_components}, "
            f"metric={args.metric}, min_dist={args.min_dist}")

# Load centroids (common for all images)
try:
    logger.info(f"Loading centroids from: {args.centroids}")
    centroids = np.genfromtxt(args.centroids, dtype=np.float32, delimiter=',', skip_header=1)
    
    if centroids.size == 0:
        logger.error("Centroids file is empty")
        sys.exit(1)
    
    if centroids.ndim == 1:
        centroids = centroids.reshape(-1, 1)
    
    logger.info(f"Loaded {centroids.shape[0]} centroids")
except Exception as e:
    logger.error(f"Failed to load centroids file: {e}")
    sys.exit(1)

# Load all images and extract spectral data
readers = []
masks = []
data_matrices = []
pixel_counts = []

for idx, imzml_file in enumerate(args.imzml_files):
    logger.info(f"Loading image {idx+1}/{len(args.imzml_files)}: {imzml_file}")
    
    try:
        # Load ImzML data
        reader = m2.ImzMLReader(str(imzml_file))
        reader.SetSmoothing(args.smoothing, args.smoothing_value)
        reader.SetBaselineCorrection(args.baseline_correction, args.baseline_correction_value)
        reader.SetNormalization(args.normalization)
        reader.SetPooling(args.range_pooling)
        reader.SetIntensityTransformation(args.intensity_transform)
        reader.Load()
        readers.append(reader)
        logger.info(f"  Image {idx+1} loaded successfully")
        
        # Get the mask
        mask = reader.GetMaskArray()
        num_pixels = np.sum(mask)
        
        if num_pixels == 0:
            logger.error(f"  Image {idx+1}: Mask is empty - no valid pixels found")
            sys.exit(1)
        
        masks.append(mask)
        pixel_counts.append(num_pixels)
        logger.info(f"  Image {idx+1}: Processing {num_pixels} valid pixels")
        
        # Extract spectral data for centroids
        logger.info(f"  Extracting spectral data for image {idx+1}...")
        data_matrix = np.zeros((centroids.shape[0], num_pixels), dtype=np.float32)
        
        for i, mz_value in enumerate(centroids[:, 0]):
            if (i + 1) % 100 == 0:
                logger.info(f"    Processing centroid {i+1}/{centroids.shape[0]}...")
            data_matrix[i] = reader.GetArray(mz_value, args.tolerance)[mask == 1]
        
        data_matrices.append(data_matrix)
        logger.info(f"  Image {idx+1} data matrix: {data_matrix.shape}")
        
    except Exception as e:
        logger.error(f"Failed to process image {idx+1} ({imzml_file}): {e}")
        sys.exit(1)

# Concatenate all data matrices for common UMAP calculation
try:
    logger.info("Concatenating data from all images...")
    combined_data = np.concatenate([dm.T for dm in data_matrices], axis=0)
    total_pixels = sum(pixel_counts)
    logger.info(f"Combined data matrix: {combined_data.shape} ({total_pixels} total pixels)")
except Exception as e:
    logger.error(f"Failed to concatenate data matrices: {e}")
    sys.exit(1)

# Initialize UMAP model
try:
    logger.info("Initializing UMAP model...")
    model = umap.UMAP(
        n_neighbors=args.n_neighbors,
        n_components=args.n_components,
        metric=args.metric,
        metric_kwds=args.metric_kwds,
        output_metric=args.output_metric,
        output_metric_kwds=args.output_metric_kwds,
        n_epochs=args.n_epochs,
        learning_rate=args.learning_rate,
        init=args.init,
        min_dist=args.min_dist,
        spread=args.spread,
        low_memory=args.low_memory,
        n_jobs=args.n_jobs,
        set_op_mix_ratio=args.set_op_mix_ratio,
        local_connectivity=args.local_connectivity,
        repulsion_strength=args.repulsion_strength,
        negative_sample_rate=args.negative_sample_rate,
        transform_queue_size=args.transform_queue_size,
        a=args.a,
        b=args.b,
        random_state=args.random_state,
        angular_rp_forest=args.angular_rp_forest,
        target_n_neighbors=args.target_n_neighbors,
        target_metric=args.target_metric,
        target_metric_kwds=args.target_metric_kwds,
        target_weight=args.target_weight,
        transform_seed=args.transform_seed,
        transform_mode=args.transform_mode,
        force_approximation_algorithm=args.force_approximation_algorithm,
        verbose=args.verbose,
        tqdm_kwds=args.tqdm_kwds,
        unique=args.unique,
        densmap=args.densmap,
        dens_lambda=args.dens_lambda,
        dens_frac=args.dens_frac,
        dens_var_shift=args.dens_var_shift,
        output_dens=args.output_dens,
        disconnection_distance=args.disconnection_distance)
except Exception as e:
    logger.error(f"Failed to initialize UMAP model: {e}")
    sys.exit(1)

# Apply UMAP transformation on combined data
try:
    logger.info("Applying UMAP transformation on combined data...")
    logger.info(f"Input shape: {combined_data.shape}")
    combined_embedding = model.fit_transform(combined_data)
    logger.info(f"UMAP embedding computed: {combined_embedding.shape}")
except Exception as e:
    logger.error(f"UMAP transformation failed: {e}")
    sys.exit(1)

# Split embedding back to individual images and save
try:
    logger.info("Splitting embedding and creating output images...")
    
    output_dir = Path(args.out)
    
    # Split the combined embedding back to individual images
    start_idx = 0
    for img_idx, (mask, num_pixels, imzml_file) in enumerate(zip(masks, pixel_counts, args.imzml_files)):
        logger.info(f"Processing output for image {img_idx+1}/{len(args.imzml_files)}...")
        
        # Extract embedding for this image
        end_idx = start_idx + num_pixels
        embedding = combined_embedding[start_idx:end_idx, :]
        start_idx = end_idx
        
        # Reshape embedding back to image space
        image_shape = mask.shape
        output_image = np.zeros((image_shape[0], image_shape[1], image_shape[2], args.n_components), dtype=np.float32)
        
        # Normalize each component to [0, 255] range
        for component_idx in range(args.n_components):
            component_data = embedding[:, component_idx]
            
            # Normalize to 0-255 range
            min_val = np.min(component_data)
            max_val = np.max(component_data)
            
            if max_val > min_val:
                normalized = (component_data - min_val) / (max_val - min_val) * 255.0
            else:
                logger.warning(f"  Image {img_idx+1}: Component {component_idx} has constant values, setting to 128")
                normalized = np.full_like(component_data, 128.0)
            
            output_image[..., component_idx][mask == 1] = normalized
        
        # Convert to uint8 for image output
        output_image = output_image.astype(np.uint8)
        logger.info(f"  Image {img_idx+1} output shape: {output_image.shape}")
        
        # Create numbered output filename
        output_file = output_dir / f"umap_{img_idx}.nrrd"
        
        # Write output image
        logger.info(f"  Writing output to: {output_file}")
        output_sitk = sitk.GetImageFromArray(output_image, True)
        sitk.WriteImage(output_sitk, str(output_file))
    
    logger.info("UMAP analysis completed successfully!")
except Exception as e:
    logger.error(f"Failed to create output images: {e}")
    sys.exit(1)
