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
#include <algorithm>
#include <array>
#include <m2SpectrumImageBase.h>
#include <mitkDataNode.h>
#include <mitkITKImageImport.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkStringProperty.h>
#include <mitkVectorProperty.h>
#include <numeric>
#include <string>
#include <thread>

namespace m2
{
  class M2AIACORE_EXPORT FsmSpectrumImage final : public SpectrumImageBase
  {
  public:
    mitkClassMacro(FsmSpectrumImage, SpectrumImageBase);
    itkNewMacro(Self);

    itkSetEnumMacro(ImageGeometryInitialized, bool);
    itkGetEnumMacro(ImageGeometryInitialized, bool);

    itkSetEnumMacro(ImageAccessInitialized, bool);
    itkGetEnumMacro(ImageAccessInitialized, bool);

    void GenerateImageData(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;

    struct SpectrumData
    {
      uint32_t id;
      itk::Index<3> index;
      std::vector<float> data;
      struct
      {
        float x, y, z;
      } world;

      m2::NormImagePixelType normalize = -1.0;
    };

    using SpectrumVectorType = std::vector<SpectrumData>;
    struct Source
    {
      /**
       * @brief _Spectra
       * Each single spectrum is represented as a BinarySpectrumMetaData object.
       */
      SpectrumVectorType _Spectra;

      itk::Offset<3> _offset = {0, 0, 0};
      m2::SpectrumFormatType ImportMode = m2::SpectrumFormatType::ContinuousProfile;
    };
    using SourceListType = std::vector<Source>;

    inline SourceListType &GetSpectrumImageSourceList() noexcept { return m_SourcesList; }
    inline const SourceListType &GetSpectrumImageSourceList() const noexcept { return m_SourcesList; }
    inline Source &GetSpectrumImageSource(unsigned int i = 0)
    {
      if (i >= m_SourcesList.size())
        mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
      return m_SourcesList[i];
    }

    inline const Source &GetSpectrumImageSource(unsigned int i = 0) const
    {
      if (i >= m_SourcesList.size())
        mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
      return m_SourcesList[i];
    }

    void InitializeImageAccess();
    void InitializeGeometry();

  private:
    using m2::SpectrumImageBase::InternalClone;
    template <class MassAxisType, class IntensityType>

    class FsmProcessor;
    void InitializeProcessor();

    SourceListType m_SourcesList;

    bool m_ImageAccessInitialized = false;
    bool m_ImageGeometryInitialized = false;

    FsmSpectrumImage();
    ~FsmSpectrumImage() override;
  };

  template <class MassAxisType, class IntensityType>
  class FsmSpectrumImage::FsmProcessor : public m2::SpectrumImageBase::ProcessorBase
  {
  private:
    friend class FsmSpectrumImage;
    m2::FsmSpectrumImage *p;

  public:
    explicit FsmProcessor(m2::FsmSpectrumImage *owner) : p(owner) {}
    void GrabIonImagePrivate(double mz, double tol, const mitk::Image *mask, mitk::Image *image) const override;
    void GrabIntensityPrivate(unsigned long int index,
                              std::vector<double> &ints,
                              unsigned int sourceIndex = 0) const override;
    void GrabMassPrivate(unsigned long int index,
                         std::vector<double> &mzs,
                         unsigned int sourceIndex = 0) const override;
    void InitializeImageAccess() override;
    void InitializeGeometry() override;
  };

} // namespace m2
