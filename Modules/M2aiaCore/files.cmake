set(H_FILES 
  include/m2CoreCommon.h
  include/m2Process.hpp 
  include/m2ISpectrumDataAccess.h
  include/m2SpectrumImageBase.h
  include/m2SpectrumImageStack.h

  include/m2IonImageReference.h
  include/m2ImzMLSpectrumImage.h
  include/m2ImzMLXMLParser.h
  include/m2FsmSpectrumImage.h
  include/m2Timer.h

  include/m2ElxUtil.h
  include/m2ElxRegistrationHelper.h

  include/mitkMassSpecImageInteractor.h
  include/mitkImage3DSliceToImage2DFilter.h
  include/mitkImage2DToImage3DSliceFilter.h
  include/m2SubdivideImage2DFilter.h

  include/signal/m2Baseline.h
  include/signal/m2EstimateFwhm.h
  include/signal/m2MedianAbsoluteDeviation.h
  include/signal/m2Morphology.h
  include/signal/m2Normalization.h
  include/signal/m2PeakDetection.h
  include/signal/m2Pooling.h
  include/signal/m2RunningMedian.h
  include/signal/m2SignalCommon.h
  include/signal/m2Smoothing.h
  
  

)

set(CPP_FILES
  m2IonImageReference.cpp
  m2SpectrumImageBase.cpp
  m2SpectrumImageStack.cpp
  m2CoreObjectFactory.cpp  
  m2ElxUtil.cpp
  m2ElxRegistrationHelper.cpp
  m2ImzMLXMLParser.cpp
  m2ImzMLSpectrumImage.cpp
  m2FsmSpectrumImage.cpp

  mitkImage3DSliceToImage2DFilter.cpp
  mitkImage2DToImage3DSliceFilter.cpp
  m2SubdivideImage2DFilter.cpp
)

set(RESOURCE_FILES
#  Interactions/msiInteraction.xml
#  Interactions/msiInteractionConfig.xml
)
