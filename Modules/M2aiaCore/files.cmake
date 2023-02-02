set(H_FILES 
  include/m2CoreCommon.h
  include/m2Process.hpp 
  include/m2ISpectrumDataAccess.h
  include/m2SpectrumImageBase.h
  include/m2SpectrumImageStack.h
  include/m2SpectrumImageDataInteractor.h

  include/m2IonImageReference.h
  include/m2ImzMLSpectrumImage.h
  include/m2ImzMLParser.h
  include/m2FsmSpectrumImage.h
  include/m2Timer.h
  include/m2SpectrumInfo.h

  include/m2ElxUtil.h
  include/m2ElxRegistrationHelper.h
  include/m2SubdivideImage2DFilter.h

  include/m2DockerHelper.h
  include/m2HelperUtils.h

  include/signal/m2Baseline.h
  include/signal/m2EstimateFwhm.h
  include/signal/m2MedianAbsoluteDeviation.h
  include/signal/m2Morphology.h
  include/signal/m2Normalization.h
  include/signal/m2Binning.h
  include/signal/m2PeakDetection.h
  include/signal/m2Pooling.h
  include/signal/m2RunningMedian.h
  include/signal/m2SignalCommon.h
  include/signal/m2Smoothing.h
  include/signal/m2Transformer.h
)

set(CPP_FILES
  m2IonImageReference.cpp
  m2SpectrumImageBase.cpp
  m2SpectrumImageStack.cpp
  m2CoreObjectFactory.cpp 
  m2DockerHelper.cpp
  m2ElxUtil.cpp
  m2ElxRegistrationHelper.cpp
  m2ImzMLParser.cpp
  m2ImzMLSpectrumImage.cpp
  m2FsmSpectrumImage.cpp
  m2SubdivideImage2DFilter.cpp
  m2SpectrumImageDataInteractor.cpp
)

set(RESOURCE_FILES
  Interactions/SpectrumDataInteractor.xml
  Interactions/SpectrumDataInteractorConfig.xml
)
