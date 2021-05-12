set(H_FILES 

  include/m2MassSpecVisualizationFilter.h
  include/m2PcaImageFilter.h
  include/m2TSNEImageFilter.h
  include/m2KmeanFilter.h
  include/m2MultiSliceFilter.h
  include/m2RGBColorMixer.hpp
  
  include/tsne/sptree.h
  include/tsne/vptree.h
  include/tsne/tsne.h
)

set(CPP_FILES
  sptree.cpp
  tsne.cpp
  m2MassSpecVisualizationFilter.cpp
  m2MultiSliceFilter.cpp
  m2KmeanFilter.cpp
  m2PcaImageFilter.cpp
  m2TSNEImageFilter.cpp
)

set(RESOURCE_FILES)
