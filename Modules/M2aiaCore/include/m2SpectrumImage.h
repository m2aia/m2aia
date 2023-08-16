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
#include <signal/m2SignalCommon.h>
#include <m2ISpectrumImageDataAccess.h>
#include <m2SpectrumInfo.h>
#include <m2ElxRegistrationHelper.h>

#include <mitkImage.h>
#include <mitkProperties.h>

#include <random>

#include <itkMultiThreaderBase.h>



namespace m2
{

  class M2AIACORE_EXPORT SpectrumImage : public ISpectrumImageDataAccess, public mitk::Image
  {
  public:
    using ImageArtifactMapType = std::map<std::string, mitk::BaseData::Pointer>;
    using SpectrumArtifactDataType = double;
    using SpectrumArtifactVectorType = std::vector<SpectrumArtifactDataType>;
    using SpectrumArtifactMapType = std::map<m2::SpectrumType, SpectrumArtifactVectorType>;
    using TransformParameterVectorType = std::vector<std::string>;

    mitkClassMacro(SpectrumImage, mitk::Image);

    itkSetEnumMacro(NormalizationStrategy, NormalizationStrategyType);
    itkGetEnumMacro(NormalizationStrategy, NormalizationStrategyType);

    itkSetEnumMacro(IntensityTransformationStrategy, IntensityTransformationType);
    itkGetEnumMacro(IntensityTransformationStrategy, IntensityTransformationType);

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

    itkSetMacro(Tolerance, double);
    itkGetConstReferenceMacro(Tolerance, double);

    itkGetConstReferenceMacro(CurrentX, double);

    itkSetMacro(UseToleranceInPPM, bool);
    itkGetConstReferenceMacro(UseToleranceInPPM, bool);

    itkGetConstReferenceMacro(NumberOfThreads, unsigned int);
    itkSetMacro(NumberOfThreads, unsigned int);

    itkGetConstReferenceMacro(NumberOfValidPixels, unsigned int);
    itkSetMacro(NumberOfValidPixels, unsigned int);

    itkGetMacro(ImageArtifacts, ImageArtifactMapType &);
    itkGetConstReferenceMacro(ImageArtifacts, ImageArtifactMapType);

    itkGetMacro(SpectraArtifacts, SpectrumArtifactMapType &);
    itkGetConstReferenceMacro(SpectraArtifacts, SpectrumArtifactMapType);

    mitk::Image::Pointer GetNormalizationImage();
    mitk::Image::Pointer GetMaskImage();
    mitk::Image::Pointer GetIndexImage();

    mitk::Image::Pointer GetNormalizationImage() const;
    mitk::Image::Pointer GetMaskImage() const;
    mitk::Image::Pointer GetIndexImage() const;

    SpectrumArtifactVectorType &GetSkylineSpectrum();
    SpectrumArtifactVectorType &GetSumSpectrum();
    SpectrumArtifactVectorType &GetMeanSpectrum();
    SpectrumArtifactVectorType &GetXAxis();
    const SpectrumArtifactVectorType &GetXAxis() const;
    
    
    itkGetConstMacro(UseExternalMask, bool);
    itkSetMacro(UseExternalMask, bool);
    itkBooleanMacro(UseExternalMask);

    itkGetConstMacro(UseExternalIndices, bool);
    itkSetMacro(UseExternalIndices, bool);
    itkBooleanMacro(UseExternalIndices);

    itkGetConstMacro(UseExternalNormalization, bool);
    itkSetMacro(UseExternalNormalization, bool);
    itkBooleanMacro(UseExternalNormalization);

    itkSetEnumMacro(ImageGeometryInitialized, bool);
    itkGetEnumMacro(ImageGeometryInitialized, bool);

    itkSetEnumMacro(ImageAccessInitialized, bool);
    itkGetEnumMacro(ImageAccessInitialized, bool);

    
    virtual void InitializeImageAccess() = 0;
    virtual void InitializeGeometry() = 0;
    virtual void InitializeProcessor() = 0;

    void GetImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;
    void InsertImageArtifact(const std::string &key, mitk::Image *img);

    template <class T>
    void SetPropertyValue(const std::string &key, const T &value);

    template <class T>
    const T GetPropertyValue(const std::string &key, T def = T()) const;

    void ApplyGeometryOperation(mitk::Operation *);
    void ApplyMoveOriginOperation(const mitk::Vector3D &v);

    inline void SaveModeOn() const { this->m_InSaveMode = true; }
    inline void SaveModeOff() const { this->m_InSaveMode = false; }
    double ApplyTolerance(double xValue) const;

    void SetElxRegistrationHelper(const std::shared_ptr<m2::ElxRegistrationHelper> &d) { m_ElxRegistrationHelper = d; }

    const SpectrumInfo &GetSpectrumType() const { return m_SpectrumType; }
    SpectrumInfo &GetSpectrumType() { return m_SpectrumType; }
    void SetSpectrumType(const SpectrumInfo &other) { m_SpectrumType = other; }

  protected:
    bool mutable m_InSaveMode = false;
    double m_Tolerance = 10;
    double m_BinningTolerance = 50;
    int m_NumberOfBins = 2000;
    double mutable m_CurrentX = -1;

    bool m_UseExternalMask = false;
    bool m_UseExternalIndices = false;
    bool m_UseExternalNormalization = false;
    bool m_UseToleranceInPPM = true;
    
        /// @brief Image access is only valid if this was set to true from the image source
    bool m_ImageAccessInitialized = false;
    
    /// @brief Image access is only valid if this was set to true from the image I/O
    bool m_ImageGeometryInitialized = false;

    std::shared_ptr<m2::ElxRegistrationHelper> m_ElxRegistrationHelper;

    unsigned int m_NumberOfValidPixels = 0;
    unsigned int m_BaseLineCorrectionHalfWindowSize = 100;
    unsigned int m_SmoothingHalfWindowSize = 4;
    unsigned int m_NumberOfThreads = 24;
    // unsigned int m_NumberOfThreads = 2;

    ImageArtifactMapType m_ImageArtifacts;
    SpectrumArtifactMapType m_SpectraArtifacts;

    SpectrumInfo m_SpectrumType;
    SpectrumInfo m_ExportSpectrumType;

    BaselineCorrectionType m_BaselineCorrectionStrategy = m2::BaselineCorrectionType::None;
    SmoothingType m_SmoothingStrategy = m2::SmoothingType::None;
    IntensityTransformationType m_IntensityTransformationStrategy = m2::IntensityTransformationType::None;

    NormalizationStrategyType m_NormalizationStrategy = NormalizationStrategyType::TIC;
    RangePoolingStrategyType m_RangePoolingStrategy = RangePoolingStrategyType::Sum;

    SpectrumImage();
    ~SpectrumImage() override;

    SpectrumArtifactVectorType m_XAxis;
  };

  itkEventMacroDeclaration(InitializationFinishedEvent, itk::AnyEvent);

  /**
   * Clone and add properties:
   * - spectrum.plot.color
   * - spectrum.marker.color
   * - spectrum.marker.size
   */
  inline void CopyNodeProperties(const mitk::DataNode *sourceNode, mitk::DataNode *targetNode)
  {
    if (const auto plotColorProp = sourceNode->GetProperty("spectrum.plot.color"))
      targetNode->SetProperty("spectrum.plot.color", plotColorProp->Clone());

    if (const auto markerColorProp = sourceNode->GetProperty("spectrum.marker.color"))
      targetNode->SetProperty("spectrum.marker.color", markerColorProp->Clone());

    if (const auto markerSizeProp = sourceNode->GetProperty("spectrum.marker.size"))
      targetNode->SetProperty("spectrum.marker.size", markerSizeProp->Clone());
  }

  /**
   * Create default properties:
   * - spectrum.plot.color (=randomColor)
   * - spectrum.marker.color (=spectrum.plot.color)
   * - spectrum.marker.size (=2)
   */
  inline void DefaultNodeProperties(const mitk::DataNode *node, bool override = true)
  {
    std::random_device rd;
    std::mt19937 e2(rd());
    std::uniform_real_distribution<> dist(0, 1);
    mitk::Color mitkColor;
    mitkColor.Set(dist(e2), dist(e2), dist(e2));
    if (override || !node->GetPropertyList()->GetProperty("spectrum.plot.color"))
      node->GetPropertyList()->SetProperty("spectrum.plot.color", mitk::ColorProperty::New(mitkColor));
    if (override || !node->GetPropertyList()->GetProperty("spectrum.marker.color"))
      node->GetPropertyList()->SetProperty("spectrum.marker.color", mitk::ColorProperty::New(mitkColor));
    if (override || !node->GetPropertyList()->GetProperty("spectrum.marker.size"))
      node->GetPropertyList()->SetProperty("spectrum.marker.size", mitk::IntProperty::New(2));    
  }

} // namespace m2

template <class T>
inline void m2::SpectrumImage::SetPropertyValue(const std::string &key, const T &value)
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
inline const T m2::SpectrumImage::GetPropertyValue(const std::string &key, T def) const
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
    return def;
  }
}
