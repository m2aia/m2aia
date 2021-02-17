set(H_FILES 
  include/m2Process.hpp
  include/m2MassValue.h

  include/m2NoiseEstimators.hpp
  include/m2PeakDetection.hpp
  include/m2Smoothing.hpp
  include/m2Morphology.hpp
  include/m2Calibration.hpp
  include/m2RunningMedian.hpp


  include/m2ImzMLEngine.h
  include/m2ImzMLTemplate.h
  
  include/m2IMassSpecDataAccess.h
  include/m2MSImageBase.h
  include/m2IonImageReference.h
  include/m2ImzMLMassSpecImage.h
  include/m2ImzMLXMLParser.h  

  include/mitkMassSpecImageInteractor.h


  include/mitkTransformixMSDataObjectStack.h
  include/mitkImage3DSliceToImage2DFilter.h
  include/mitkImage2DToImage3DSliceFilter.h
  
  include/tsne/sptree.h
  include/tsne/vptree.h
  include/tsne/tsne.h

  include/mitkTimer.h

  include/m2MassSpecVisualizationFilter.h

  include/m2PcaImageFilter.h
  include/m2TSNEImageFilter.h
  include/m2KmeanFilter.h
  
  include/m2MultiSliceFilter.h
  include/m2MultiModalMaskExportHelper.h

  include/m2RGBColorMixer.hpp
)

set(CPP_FILES
  m2IonImageReference.cpp
  m2MSImageBase.cpp
  m2CoreObjectFactory.cpp

  m2ImzMLEngine.cpp
  m2MassValue.cpp
  mitkTransformixMSDataObjectStack.cpp

  m2ImzMLXMLParser.cpp
  m2ImzMLMassSpecImage.cpp 

  mitkImage3DSliceToImage2DFilter.cpp
  mitkImage2DToImage3DSliceFilter.cpp

  m2PcaImageFilter.cpp
  m2TSNEImageFilter.cpp
  m2KmeanFilter.cpp
 
  m2MassSpecVisualizationFilter.cpp
  m2MultiSliceFilter.cpp
  m2MultiModalMaskExportHelper.cpp

  sptree.cpp
  tsne.cpp


)

set(RESOURCE_FILES
#  Interactions/msiInteraction.xml
#  Interactions/msiInteractionConfig.xml
)
