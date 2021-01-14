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

#include <MitkM2aiaCoreExports.h>
#include <array>
#include <itkMetaDataObject.h>
#include <m2IMassSpecDataAccess.h>
#include <m2IonImageReference.h>
#include <map>
#include <mitkBaseData.h>
#include <mitkDataNode.h>
#include <mitkImage.h>

#include <m2MassValue.h>
#include <set>

namespace m2
{
  enum class ImzMLFormatType : unsigned
  {
    NotSet = 0,
    ContinuousProfile = 1,
    ProcessedProfile = 2,   //?
    ContinuousCentroid = 4, // Masked
    ProcessedCentroid = 8,
    ProcessedMonoisotopicCentroid = 16,
    ContinuousMonoisotopicCentroid = 32
  };

  enum class NormalizationStrategyType
  {
    None,
    TIC,
    Median,
    InFile,
    Sum
  };

  enum class OverviewSpectrumType
  {
    Sum,
    Mean,
    Maximum,
    Variance,
    Median,
    AreaMean,
    AreaMaximum,
    AreaSum,
    AreaMedian,
    PeakIndicators,
    RangeIndicator,
    None
  };

  enum class IonImageGrabStrategyType
  {
    None,
    Mean,
    Maximum,
    Median,
    Sum
  };

  enum class BaselineCorrectionType
  {
    None,
    TopHat,
    Median    
  };

  enum class OutputDataType
  {
    Float,
    Double
  };

  using IonImagePixelType = double;
  using NormImagePixelType = double;
  using MaskImagePixelType = unsigned short;
  using IndexType = unsigned;
  using WorldCoordinateType = float;
  using IndexImagePixelType = IndexType;

  enum class TransformationMethod
  {
    None,
    Transformix
  };

} // namespace m2

inline m2::ImzMLFormatType operator|(m2::ImzMLFormatType lhs, m2::ImzMLFormatType rhs)
{
  return static_cast<m2::ImzMLFormatType>(static_cast<std::underlying_type<m2::ImzMLFormatType>::type>(lhs) |
                                          static_cast<std::underlying_type<m2::ImzMLFormatType>::type>(rhs));
}

inline m2::ImzMLFormatType operator|=(m2::ImzMLFormatType lhs, m2::ImzMLFormatType rhs)
{
  return lhs | rhs;
}

inline m2::ImzMLFormatType operator&(m2::ImzMLFormatType lhs, m2::ImzMLFormatType rhs)
{
  return static_cast<m2::ImzMLFormatType>(static_cast<std::underlying_type<m2::ImzMLFormatType>::type>(lhs) &
                                          static_cast<std::underlying_type<m2::ImzMLFormatType>::type>(rhs));
}

inline bool any(m2::ImzMLFormatType lhs)
{
  return static_cast<std::underlying_type<m2::ImzMLFormatType>::type>(lhs) != 0;
}

namespace m2
{
  /**
   * MSDataObject represents the mitk::BaseData class for all mass spectrometry image (MSI) data types.
   *
   * MS images are composed of
   * data access utilities (GrabIonImage of GrabSpectrum) and
   * list of meta data (property list) and
   * image artifacts (e.g. IndexImage for image based access and IonImage for visualization) as well as a
   * spectral artifacts (e.g. MeanSpectrum or SkylineSpectrum).
   *
   * Data initialization is implementation dependent of the different MS imaging format types (ImzML or Bruker)
   * that are inherit from MSDataObject.
   *
   * MSDataObject should be used to start filter pipelines and so has to contain accesible image and spectral
   * information.
   *
   * IndexImage: A postion in this image contains an index that is used to access the binary data. For example:
   * The inherited type may offer a container with byte-offsets in the binary file. The index links the spatial location
   * with the structur of offset information.
   * IonImage: The IonImage is used for visualization of pixel-wise normalized, accumulated and processed mz-ranges.
   * NormalizationImage: Stores interim normalization factors for each voxel.
   * MaskImage: Region of spectral information.
   * MeanSpectrum: mean intensity over all spectra in the image.
   * MaxSpectrum: max intensity over all spectra in the image.
   *
   **/
  class MITKM2AIACORE_EXPORT MSImageBase : public IMSImageDataAccess, public mitk::Image
  {
  public:
    mitkClassMacro(MSImageBase, mitk::Image);
    itkNewMacro(Self);

    itkSetEnumMacro(NormalizationStrategy, NormalizationStrategyType);
    itkGetEnumMacro(NormalizationStrategy, NormalizationStrategyType);

    itkSetEnumMacro(IonImageGrabStrategy, IonImageGrabStrategyType);
    itkGetEnumMacro(IonImageGrabStrategy, IonImageGrabStrategyType);


	itkSetEnumMacro(BaselineCorrectionStrategy, BaselineCorrectionType);
    itkGetEnumMacro(BaselineCorrectionStrategy, BaselineCorrectionType);
    itkSetMacro(BaseLinecorrectionHalfWindowSize, unsigned int);
    itkGetConstReferenceMacro(BaseLinecorrectionHalfWindowSize, unsigned int);

    itkSetMacro(SmoothingHalfWindowSize, unsigned int);
    itkGetConstReferenceMacro(SmoothingHalfWindowSize, unsigned int);

    itkSetMacro(PeakPickingHalfWindowSize, unsigned int);
    itkGetConstReferenceMacro(PeakPickingHalfWindowSize, unsigned int);

    itkSetMacro(PeakPickingSNR, double);
    itkGetConstReferenceMacro(PeakPickingSNR, double);

    itkSetMacro(PeakPickingBinningTolerance, double);
    itkGetConstReferenceMacro(PeakPickingBinningTolerance, double);

    itkSetMacro(MassPickingTolerance, double);
    itkGetConstReferenceMacro(MassPickingTolerance, double);

    itkSetMacro(UpperMZBound, double);
    itkGetConstReferenceMacro(UpperMZBound, double);

    itkSetMacro(LowerMZBound, double);
    itkGetConstReferenceMacro(LowerMZBound, double);

    mitk::Image::Pointer GetNormalizationImage();
    mitk::Image::Pointer GetMaskImage();
    mitk::Image::Pointer GetIndexImage();  

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

    void GrabIntensity(unsigned int index, std::vector<double> &ints, unsigned int sourceIndex = 0) const override;
    void GrabMass(unsigned int index, std::vector<double> &mzs, unsigned int sourceIndex = 0) const override;
    void GrabSpectrum(unsigned int index,
                      std::vector<double> &mzs,
                      std::vector<double> &ints,
                      unsigned int sourceIndex = 0) const override;
    void GrabIonImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;

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
    bool mutable m_UsePeakPickingOnGrabSpectrum = false;
    double m_LowerMZBound = -1;
    double m_UpperMZBound = -1;
    double m_MassPickingTolerance = 0.0;
    double m_PeakPickingSNR = 10;
    double m_PeakPickingBinningTolerance = 50;
    unsigned int m_PeakPickingHalfWindowSize = 20;
    unsigned int m_BaseLinecorrectionHalfWindowSize = 100;
    unsigned int m_SmoothingHalfWindowSize = 4;
    unsigned int m_NumberOfThreads = 10;

    PeaksVectorType m_Peaks;

    ImageArtifactMapType m_ImageArtifacts;
    SpectrumArtifactMapType m_SpectraArtifacts;

	BaselineCorrectionType m_BaselineCorrectionStrategy;
    /**
     * @brief Vector of ion images associated with this ims file. E.g. peaks or individual picked ion
     * images.
     */
    IonImageReferenceVectorType m_IonImageReferenceVector;

    /**
     * @brief Information about the represented ion image distribution captured by this image instance.
     * This property is updated by GrabIonImage.
     *
     */
    IonImageReference::Pointer m_CurrentIonImageReference;

    m2::NormalizationStrategyType m_NormalizationStrategy = m2::NormalizationStrategyType::TIC;
    m2::IonImageGrabStrategyType m_IonImageGrabStrategy = m2::IonImageGrabStrategyType::Sum;


    ~MSImageBase() override;
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
  inline void m2::MSImageBase::SetPropertyValue(const std::string &key, const T &value)
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
  inline const T m2::MSImageBase::GetPropertyValue(const std::string &key) const
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

  class MSImageBase::ProcessorBase
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
