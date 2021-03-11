/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#ifndef MITKMSImageBase
#define MITKMSImageBase

#include <M2aiaCoreExports.h>
#include <itkMetaDataObject.h>
#include <m2Common.h>
#include <m2ISpectrumDataAccess.h>
#include <m2IonImageReference.h>
#include <m2MassValue.h>
#include <mitkBaseData.h>

namespace m2
{
  /**
   * SpectrumImageBase is a mitk::Image class extended to work with spectral image acquistion modalities.
   *
   * Spectrum image is a composition of:
   * - data access utilities (SpectrumDataAccess)
   * - list of meta data (property list)
   * - additional (required) images (e.g. IndexImage, NormalizationImage for visualization)
   * - overview spectra (e.g. MeanSpectrum or SkylineSpectrum).
   *
   * Data initialization is implementation dependent of the different spectrum image format types (e.g. ImzML, FSM)
   *
   * To be consistent with other derived SpectrumImage types the following properties are recommended:
   * - Provide an index image, where each pixel represents the index of the spectrum reference vector.
   * - Provide a normalization image, so that normalization constants can be generated once during initialization.
   * - Provide a mask image, where each pixel represents a Boolean/Label value that guarantees valid or invalid image
   *regions.
   * - Provide overview spectra (Skyline, Mean)
   **/

  class M2AIACORE_EXPORT SpectrumImageBase : public ISpectrumDataAccess, public mitk::Image
  {
  public:
    mitkClassMacro(SpectrumImageBase, mitk::Image);
    itkNewMacro(Self);

    itkSetEnumMacro(NormalizationStrategy, NormalizationStrategyType);
    itkGetEnumMacro(NormalizationStrategy, NormalizationStrategyType);

    itkSetEnumMacro(IonImageGrabStrategy, ImagingStrategyType);
    itkGetEnumMacro(IonImageGrabStrategy, ImagingStrategyType);

    itkSetEnumMacro(SmoothingStrategy, SmoothingType);
    itkGetEnumMacro(SmoothingStrategy, SmoothingType);

    itkSetEnumMacro(BaselineCorrectionStrategy, BaselineCorrectionType);
    itkGetEnumMacro(BaselineCorrectionStrategy, BaselineCorrectionType);

    itkSetMacro(BaseLinecorrectionHalfWindowSize, unsigned int);
    itkGetConstReferenceMacro(BaseLinecorrectionHalfWindowSize, unsigned int);

    itkSetMacro(SmoothingHalfWindowSize, unsigned int);
    itkGetConstReferenceMacro(SmoothingHalfWindowSize, unsigned int);

    // itkSetMacro(PeakPickingHalfWindowSize, unsigned int);
    // itkGetConstReferenceMacro(PeakPickingHalfWindowSize, unsigned int);

    // itkSetMacro(PeakPickingSNR, double);
    // itkGetConstReferenceMacro(PeakPickingSNR, double);

    itkSetMacro(BinningTolerance, double);
    itkGetConstReferenceMacro(BinningTolerance, double);

    itkSetMacro(Tolerance, double);
    itkGetConstReferenceMacro(Tolerance, double);

    mitk::Image::Pointer GetNormalizationImage();
    mitk::Image::Pointer GetMaskImage();
    mitk::Image::Pointer GetIndexImage();

    virtual void InitializeImageAccess(){};
    virtual void InitializeGeometry(){};

    using ImageArtifactMapType = std::map<std::string, mitk::BaseData::Pointer>;
    using SpectrumArtifactDataType = double;
    using SpectrumArtifactVectorType = std::vector<SpectrumArtifactDataType>;
    using SpectrumArtifactMapType = std::map<m2::OverviewSpectrumType, SpectrumArtifactVectorType>;
    using IonImageReferenceVectorType = std::vector<IonImageReference::Pointer>;

    SpectrumArtifactVectorType &SkylineSpectrum();
    SpectrumArtifactVectorType &SumSpectrum();
    SpectrumArtifactVectorType &MeanSpectrum();
    SpectrumArtifactVectorType &PeakIndicators();
    SpectrumArtifactVectorType &MassAxis();
    const SpectrumArtifactVectorType &MassAxis() const;

    itkGetMacro(ImageArtifacts, ImageArtifactMapType &);
    itkGetConstReferenceMacro(ImageArtifacts, ImageArtifactMapType);

    itkGetMacro(SpectraArtifacts, SpectrumArtifactMapType &);
    itkGetConstReferenceMacro(SpectraArtifacts, SpectrumArtifactMapType);

    itkGetMacro(IonImageReferenceVector, IonImageReferenceVectorType &);
    itkGetConstReferenceMacro(IonImageReferenceVector, IonImageReferenceVectorType);

    itkGetObjectMacro(CurrentIonImageReference, IonImageReference);
    itkGetConstObjectMacro(CurrentIonImageReference, IonImageReference);
    itkSetObjectMacro(CurrentIonImageReference, IonImageReference);

    using PeaksVectorType = std::vector<m2::MassValue>;
    itkGetMacro(Peaks, PeaksVectorType &);
    itkGetConstReferenceMacro(Peaks, PeaksVectorType);

    void GetIntensities(unsigned int index, std::vector<double> &ints, unsigned int sourceIndex = 0) const override;
    void GetXValues(unsigned int index, std::vector<double> &mzs, unsigned int sourceIndex = 0) const override;
    void GetSpectrum(unsigned int index,
                     std::vector<double> &mzs,
                     std::vector<double> &ints,
                     unsigned int sourceIndex = 0) const override;
    void GenerateImageData(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;

    template <class T>
    void SetPropertyValue(const std::string &key, const T &value);

    template <class T>
    const T GetPropertyValue(const std::string &key) const;

    void ApplyGeometryOperation(mitk::Operation *);
    void ApplyMoveOriginOperation(const std::array<int, 2> &v);

    inline void SaveModeOn() const { this->m_InSaveMode = true; }
    inline void SaveModeOff() const { this->m_InSaveMode = false; }

    itkGetConstReferenceMacro(NumberOfThreads, unsigned int);
    itkSetMacro(NumberOfThreads, unsigned int);

  protected:
    bool mutable m_InSaveMode = false;
    double m_Tolerance = 10;
    double m_BinningTolerance = 50;

    unsigned int m_BaseLinecorrectionHalfWindowSize = 100;
    unsigned int m_SmoothingHalfWindowSize = 4;
    unsigned int m_NumberOfThreads = 10;

    PeaksVectorType m_Peaks;

    ImageArtifactMapType m_ImageArtifacts;
    SpectrumArtifactMapType m_SpectraArtifacts;

    BaselineCorrectionType m_BaselineCorrectionStrategy;
    SmoothingType m_SmoothingStrategy;

    /**
     * @brief Vector of ion images associated with this ims file. E.g. peaks or individual picked ion
     * images.
     */
    IonImageReferenceVectorType m_IonImageReferenceVector;

    /**
     * @brief Information about the represented ion image distribution captured by this image instance.
     * This property is updated by GenerateImageData.
     *
     */
    IonImageReference::Pointer m_CurrentIonImageReference;

    NormalizationStrategyType m_NormalizationStrategy = m2::NormalizationStrategyType::TIC;
    m2::ImagingStrategyType m_IonImageGrabStrategy = m2::ImagingStrategyType::Sum;

    ~SpectrumImageBase() override;
    bool m_IsDataAccessInitialized = false;

    /*
     * ProcessorBase
     * This nasted class has to be inherited by all MSDataObject sub-types
     *
     * */
    class ProcessorBase;
    std::unique_ptr<ProcessorBase> m_Processor;

    std::function<double(double)> m_Offset = [](double) { return 0; };
    std::function<double(double)> m_InverseOffset = [](double) { return 0; };
    SpectrumArtifactVectorType m_MassAxis;
  };

  template <class T>
  inline void m2::SpectrumImageBase::SetPropertyValue(const std::string &key, const T &value)
  {
    auto dd = this->GetPropertyList();
    auto prop = dd->GetProperty(key);
    using TargetProperty = mitk::GenericProperty<T>;

    auto entry = dynamic_cast<TargetProperty *>(prop);
    if (!entry)
    {
      auto entry = TargetProperty::New(value);
      dd->SetProperty(key, entry);
    }
    else
    {
      entry->SetValue(value);
    }
  }

  template <class T>
  inline const T m2::SpectrumImageBase::GetPropertyValue(const std::string &key) const
  {
    auto dd = this->GetPropertyList();
    const mitk::GenericProperty<T> *entry = dynamic_cast<mitk::GenericProperty<T> *>(dd->GetProperty(key));
    if (entry)
    {
      return entry->GetValue();
    }
    else
    {
      MITK_WARN << "No meta data object found! " << key;
      return T(0);
    }
  }

  class SpectrumImageBase::ProcessorBase
  {
  public:
    virtual void GrabIonImagePrivate(double mz, double tol, const Image *mask, Image *image) const = 0;
    virtual void GrabIntensityPrivate(unsigned long int index,
                                      std::vector<double> &ints,
                                      unsigned int sourceIndex = 0) const = 0;
    virtual void GrabMassPrivate(unsigned long int index,
                                 std::vector<double> &mzs,
                                 unsigned int sourceIndex = 0) const = 0;
    virtual void InitializeImageAccess() = 0;
    virtual void InitializeGeometry() = 0;
    virtual ~ProcessorBase() = default;
  };

} // namespace m2

#endif // MITKMSImageBase
