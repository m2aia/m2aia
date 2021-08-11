/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once

#include <M2aiaCoreExports.h>
#include <itkMetaDataObject.h>
#include <m2CoreCommon.h>
#include <m2ISpectrumDataAccess.h>
#include <m2IonImageReference.h>
#include <m2MassValue.h>
#include <m2SignalCommon.h>
#include <mitkBaseData.h>
#include <mitkImage.h>

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
    using ImageArtifactMapType = std::map<std::string, mitk::BaseData::Pointer>;
    using SpectrumArtifactDataType = double;
    using SpectrumArtifactVectorType = std::vector<SpectrumArtifactDataType>;
    using SpectrumArtifactMapType = std::map<m2::OverviewSpectrumType, SpectrumArtifactVectorType>;
    using IonImageReferenceVectorType = std::vector<IonImageReference::Pointer>;
    using PeaksVectorType = std::vector<m2::MassValue>;
    using TransformParameterVectorType = std::vector<std::string>;

    mitkClassMacro(SpectrumImageBase, mitk::Image);
    // itkNewMacro(Self);

    itkSetEnumMacro(NormalizationStrategy, NormalizationStrategyType);
    itkGetEnumMacro(NormalizationStrategy, NormalizationStrategyType);

    itkSetEnumMacro(RangePoolingStrategy, RangePoolingStrategyType);
    itkGetEnumMacro(RangePoolingStrategy, RangePoolingStrategyType);

    itkSetEnumMacro(SmoothingStrategy, SmoothingType);
    itkGetEnumMacro(SmoothingStrategy, SmoothingType);

    itkSetEnumMacro(BaselineCorrectionStrategy, BaselineCorrectionType);
    itkGetEnumMacro(BaselineCorrectionStrategy, BaselineCorrectionType);

    itkSetMacro(BaseLineCorrectionHalfWindowSize, unsigned int);
    itkGetConstReferenceMacro(BaseLineCorrectionHalfWindowSize, unsigned int);

    itkSetMacro(SmoothingHalfWindowSize, unsigned int);
    itkGetConstReferenceMacro(SmoothingHalfWindowSize, unsigned int);

    itkSetEnumMacro(ExportMode, SpectrumFormatType);
    itkGetEnumMacro(ExportMode, SpectrumFormatType);

    itkSetEnumMacro(ImportMode, SpectrumFormatType);
    itkGetEnumMacro(ImportMode, SpectrumFormatType);

    itkSetEnumMacro(XOutputType, NumericType);
    itkGetEnumMacro(XOutputType, NumericType);

    itkSetEnumMacro(YOutputType, NumericType);
    itkGetEnumMacro(YOutputType, NumericType);

    itkSetEnumMacro(XInputType, NumericType);
    itkGetEnumMacro(XInputType, NumericType);

    itkSetEnumMacro(YInputType, NumericType);
    itkGetEnumMacro(YInputType, NumericType);

    itkSetMacro(BinningTolerance, double);
    itkGetConstReferenceMacro(BinningTolerance, double);

    itkSetMacro(NumberOfBins, int);
    itkGetConstReferenceMacro(NumberOfBins, int);

    itkSetMacro(Tolerance, double);
    itkGetConstReferenceMacro(Tolerance, double);

    itkSetMacro(UseToleranceInPPM, bool);
    itkGetConstReferenceMacro(UseToleranceInPPM, bool);
    
    itkGetMacro(Peaks, PeaksVectorType &);
    itkGetConstReferenceMacro(Peaks, PeaksVectorType);

    itkGetConstReferenceMacro(NumberOfThreads, unsigned int);
    itkSetMacro(NumberOfThreads, unsigned int);

    itkGetMacro(ImageArtifacts, ImageArtifactMapType &);
    itkGetConstReferenceMacro(ImageArtifacts, ImageArtifactMapType);

    itkGetMacro(SpectraArtifacts, SpectrumArtifactMapType &);
    itkGetConstReferenceMacro(SpectraArtifacts, SpectrumArtifactMapType);

    itkGetMacro(IonImageReferenceVector, IonImageReferenceVectorType &);
    itkGetConstReferenceMacro(IonImageReferenceVector, IonImageReferenceVectorType);

    itkGetMacro(Transformations, TransformParameterVectorType &);
    itkGetConstReferenceMacro(Transformations, TransformParameterVectorType);
    void SetTransformations(const TransformParameterVectorType &v) { m_Transformations = v; }

    itkGetObjectMacro(CurrentIonImageReference, IonImageReference);
    itkGetConstObjectMacro(CurrentIonImageReference, IonImageReference);
    itkSetObjectMacro(CurrentIonImageReference, IonImageReference);

    mitk::Image::Pointer GetNormalizationImage();
    mitk::Image::Pointer GetMaskImage();
    mitk::Image::Pointer GetIndexImage();

    itkGetConstMacro(UseExternalMask, bool);
    itkSetMacro(UseExternalMask, bool);
    itkBooleanMacro(UseExternalMask);

    itkGetConstMacro(UseExternalIndices, bool);
    itkSetMacro(UseExternalIndices, bool);
    itkBooleanMacro(UseExternalIndices);

    itkGetConstMacro(UseExternalNormalization, bool);
    itkSetMacro(UseExternalNormalization, bool);
    itkBooleanMacro(UseExternalNormalization);

    virtual void InitializeImageAccess() = 0;
    virtual void InitializeGeometry() = 0;
    virtual void InitializeProcessor() = 0;

    SpectrumArtifactVectorType &SkylineSpectrum();
    SpectrumArtifactVectorType &SumSpectrum();
    SpectrumArtifactVectorType &MeanSpectrum();
    SpectrumArtifactVectorType &PeakIndicators();
    SpectrumArtifactVectorType &GetXAxis();
    const SpectrumArtifactVectorType &GetXAxis() const;

    void ReceiveIntensities(unsigned int index, std::vector<double> &ints, unsigned int sourceIndex = 0) const override;
    void ReceivePositions(unsigned int index, std::vector<double> &mzs, unsigned int sourceIndex = 0) const override;
    void ReceiveSpectrum(unsigned int index,
                         std::vector<double> &mzs,
                         std::vector<double> &ints,
                         unsigned int sourceIndex = 0) const override;
    void UpdateImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;
    void InsertImageArtifact(const std::string & key, mitk::Image * img);

    template <class T>
    void SetPropertyValue(const std::string &key, const T &value);

    template <class T>
    const T GetPropertyValue(const std::string &key) const;

    void ApplyGeometryOperation(mitk::Operation *);
    void ApplyMoveOriginOperation(const mitk::Vector3D &v);

    inline void SaveModeOn() const { this->m_InSaveMode = true; }
    inline void SaveModeOff() const { this->m_InSaveMode = false; }
    double ApplyTolerance(double);

  protected:
    bool mutable m_InSaveMode = false;
    double m_Tolerance = 10;
    double m_BinningTolerance = 50;
    int m_NumberOfBins = 2000;

    bool m_UseExternalMask = false;
    bool m_UseExternalIndices = false;
    bool m_UseExternalNormalization = false;
    bool m_UseToleranceInPPM = false;

    // if UseTransformationsOn()
    bool m_UseTransformations = true;
    TransformParameterVectorType m_Transformations;

    unsigned int m_BaseLineCorrectionHalfWindowSize = 100;
    unsigned int m_SmoothingHalfWindowSize = 4;
    unsigned int m_NumberOfThreads = itk::MultiThreader::GetGlobalDefaultNumberOfThreads();
    // unsigned int m_NumberOfThreads = 2;
    PeaksVectorType m_Peaks;

    ImageArtifactMapType m_ImageArtifacts;
    SpectrumArtifactMapType m_SpectraArtifacts;

    BaselineCorrectionType m_BaselineCorrectionStrategy;
    SmoothingType m_SmoothingStrategy;

    NumericType m_YOutputType = NumericType::Float;
    NumericType m_XOutputType = NumericType::Float;
    NumericType m_YInputType;
    NumericType m_XInputType;
    SpectrumFormatType m_ExportMode = SpectrumFormatType::ContinuousProfile;
    SpectrumFormatType m_ImportMode;
    /**
     * @brief Vector of ion images associated with this ims file. E.g. peaks or individual picked ion
     * images.
     */
    IonImageReferenceVectorType m_IonImageReferenceVector;

    /**
     * @brief Information about the represented ion image distribution captured by this image instance.
     * This property is updated by UpdateImage.
     *
     */
    IonImageReference::Pointer m_CurrentIonImageReference;

    NormalizationStrategyType m_NormalizationStrategy = NormalizationStrategyType::TIC;
    RangePoolingStrategyType m_RangePoolingStrategy = RangePoolingStrategyType::Sum;

    SpectrumImageBase();
    ~SpectrumImageBase() override;
    bool m_IsDataAccessInitialized = false;

    /*
     * ProcessorBase
     * This nasted class has to be inherited by all MSDataObject sub-types
     *
     * */
    class ProcessorBase;
    std::unique_ptr<ProcessorBase> m_Processor;
    SpectrumArtifactVectorType m_XAxis;
  };

} // namespace m2

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

class m2::SpectrumImageBase::ProcessorBase
{
public:
  virtual void UpdateImagePrivate(double mz, double tol, const Image *mask, Image *image) const = 0;
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
