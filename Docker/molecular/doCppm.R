#suppressPackageStartupMessages(library("argparse"))
# library(nat)
# library(moleculaR)

timeNow <- function() {
  format(Sys.time(), format = "%H:%M:%S")
}

doCppm <- function(path_ionDir, 
                  path_mask, 
                  path_out,
                  pval = 0.05,
                  verbose = TRUE) {
  
  ionImages <- list.files(path = path_ionDir, pattern = "nrrd", full.names = TRUE)
  
  stopifnot(length(ionImages) > 0)
  stopifnot(tools::file_ext(path_mask) == "nrrd")
  stopifnot(is.numeric(pval))
  
  # create window object
  mask <- nat::read.nrrd(path_mask)[,,1] == 1
  win <- spatstat.geom::as.polygonal(spatstat.geom::owin(mask=mask))
  
  # save spp objects into a list
  sppList <- vector("list", length(ionImages))
  
  # loop through each ion image file
  for(i in 1 : length(ionImages)){
    
    stopifnot(tools::file_ext(ionImages[i]) == "nrrd")
    
    if(verbose) {
      cat(timeNow(), "reading", ionImages[i], "\n")
    }
    
    im <- nat::read.nrrd(ionImages[i])
    
    if(i == 1){ # save the header of the first image
      in_header <- attr(im, "header")
    }
    
    ii <- im[,,1]
    im <- spatstat.geom::as.im(im[,,1])
    sppList[[i]] <- moleculaR::im2spp(im, win = win)
    
    
  }
  
  # combine SPPs
  spp <- moleculaR::superimposeAnalytes(sppList, spWin = win)
  
  
  
  if(verbose) {
    cat(timeNow(), "computing molecular probability map...\n")
  }
  
  
  mpm <- moleculaR::probMap(spp, pvalThreshold = pval)
  
  # create a 3D array with z = 1 to hold hot- and coldspots; 0 = background, 1 = hotspot, 2 = coldspot
  combined_arr <- array(data = 0, dim = c(dim(im), 1))
  
  # hotspot
  if(mpm$hotspotpp$n > 0){ # if no hotspot detected then ignore 
    coords <- as.matrix(cbind(spatstat.geom::coords(mpm$hotspotpp), z = 1))
    coords <- coords[ , c(2, 1, 3)] # spatstat has different image axes 
    combined_arr[coords] <- 1
    rm(coords)
  }
  
  
  # coldspot
  if(mpm$coldspotpp$n > 0){ # if no coldspot detected then ignore 
    coords <- as.matrix(cbind(spatstat.geom::coords(mpm$coldspotpp), z = 1))
    coords <- coords[ , c(2, 1, 3)] # spatstat has different image axes 
    combined_arr[coords] <- 2
    rm(coords)
  }
  
  
  if(verbose) {
    cat(timeNow(), "writing results...\n")
  }
  
  # add dicom label attributes
  attr(in_header, "DICOM_0008_0060") <- "{\"values\":[{\"z\":0, \"t\":0, \"value\":\"SEG\"}]}" 
  attr(in_header, "DICOM_0008_103E") <- "{\"values\":[{\"z\":0, \"t\":0, \"value\":\"MITK Segmentation\"}]}" 
  
  
  # Copy data from original array to new array
  nat::write.nrrd(combined_arr, file = path_out, header = in_header, dtype = "ushort")
}

### call from command line
args = commandArgs(trailingOnly=TRUE)
if (length(args)==0) {
  stop("At least one argument must be supplied (input file)\n", call.=FALSE)
}

# create parser object
parser <- argparse::ArgumentParser()
parser$add_argument("-v", "--verbose", action="store_true", default=TRUE,
                    help="Print extra output [default]")

# Inputs
parser$add_argument("--ionimage", type="character", help="directory path where the ion images (*.nrrd) are.")
parser$add_argument("--mask", type="character", help="path to the mask image file (*.nrrd).")
parser$add_argument("--pval", type="double", help="P-value for probability calculation", default = 0.05)

# Outputs
parser$add_argument("--out", type = "character", help="Hot and cold spot output path (*.nrrd)")

args <- parser$parse_args()

doCppm(path_ionDir = args$ionimage,
      path_mask = args$mask, 
      path_out = args$out, 
      pval = args$pval,
      verbose = args$verbose)

