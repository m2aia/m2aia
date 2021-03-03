set(H_FILES 
  include/m2Process.hpp 
  include/m2IMassSpecDataAccess.h
  include/m2MSImageBase.h
  

  include/m2IonImageReference.h
  include/m2ImzMLMassSpecImage.h
  include/m2ImzMLXMLParser.h  

  include/mitkMassSpecImageInteractor.h

  include/mitkTransformixMSDataObjectStack.h
  include/mitkImage3DSliceToImage2DFilter.h
  include/mitkImage2DToImage3DSliceFilter.h
  
  include/mitkTimer.h
  
)

set(CPP_FILES
  m2IonImageReference.cpp
  m2MSImageBase.cpp
  m2CoreObjectFactory.cpp
  m2MassValue.cpp
  
  mitkTransformixMSDataObjectStack.cpp

  m2ImzMLXMLParser.cpp
  m2ImzMLMassSpecImage.cpp 

  mitkImage3DSliceToImage2DFilter.cpp
  mitkImage2DToImage3DSliceFilter.cpp
)

set(RESOURCE_FILES
#  Interactions/msiInteraction.xml
#  Interactions/msiInteractionConfig.xml
)
