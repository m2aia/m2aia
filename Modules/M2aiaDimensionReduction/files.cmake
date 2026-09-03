set(H_FILES 

  include/m2MassSpecVisualizationFilter.h
  include/m2SpectralFeatureMatrix.h
  include/m2EmbeddingFilterBase.h
  include/m2EmbeddingResult.h
  include/m2TruncatedSvd.h
  include/m2NmfImageFilter.h
  include/m2IcaImageFilter.h
  include/m2PlsDaImageFilter.h
  include/m2PcaImageFilter.h
  include/m2TSNEImageFilter.h
  include/m2MultiSliceFilter.h
  include/m2RGBColorMixer.hpp
  include/m2KMeansImageFilter.h
  
  include/tsne/sptree.h
  include/tsne/vptree.h
  include/tsne/tsne.h
)

set(CPP_FILES
  sptree.cpp
  tsne.cpp
  m2MassSpecVisualizationFilter.cpp
  m2SpectralFeatureMatrix.cpp
  m2EmbeddingFilterBase.cpp
  m2EmbeddingResult.cpp
  m2TruncatedSvd.cpp
  m2NmfImageFilter.cpp
  m2IcaImageFilter.cpp
  m2PlsDaImageFilter.cpp
  m2MultiSliceFilter.cpp
  m2PcaImageFilter.cpp
  m2TSNEImageFilter.cpp
  m2KMeansImageFilter.cpp
)

set(RESOURCE_FILES)
