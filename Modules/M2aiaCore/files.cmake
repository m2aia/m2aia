set(H_FILES 
  include/m2CoreCommon.h
  include/m2Process.hpp 
  include/m2ISpectrumDataAccess.h
  include/m2SpectrumImageBase.h

  include/m2IonImageReference.h
  include/m2ImzMLSpectrumImage.h
  include/m2ImzMLXMLParser.h
  include/m2FsmSpectrumImage.h

  include/mitkMassSpecImageInteractor.h

  include/mitkTransformixMSDataObjectStack.h
  include/mitkImage3DSliceToImage2DFilter.h
  include/mitkImage2DToImage3DSliceFilter.h
  
  include/mitkTimer.h
  include/m2ElxUtil.h
  include/m2ElxRegistrationHelper.h

)

set(CPP_FILES
  m2IonImageReference.cpp
  m2SpectrumImageBase.cpp
  m2CoreObjectFactory.cpp
  m2MassValue.cpp
  m2ElxUtil.cpp
  m2ElxRegistrationHelper.cpp
  
  mitkTransformixMSDataObjectStack.cpp

  m2ImzMLXMLParser.cpp
  m2ImzMLSpectrumImage.cpp

  m2FsmSpectrumImage.cpp

  mitkImage3DSliceToImage2DFilter.cpp
  mitkImage2DToImage3DSliceFilter.cpp
)

set(RESOURCE_FILES
#  Interactions/msiInteraction.xml
#  Interactions/msiInteractionConfig.xml
)
