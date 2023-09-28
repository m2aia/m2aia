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
#include <m2SpectrumImage.h>
#include <m2ISpectrumImageSource.h>
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
  class M2AIACORE_EXPORT FsmSpectrumImage final : public SpectrumImage
  {
  public:
    mitkClassMacro(FsmSpectrumImage, SpectrumImage);
    itkNewMacro(Self);

    itkSetEnumMacro(ImageGeometryInitialized, bool);
    itkGetEnumMacro(ImageGeometryInitialized, bool);

    itkSetEnumMacro(ImageAccessInitialized, bool);
    itkGetEnumMacro(ImageAccessInitialized, bool);

    void GetImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;
    

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

    itkGetMacro(Spectra, SpectrumVectorType &);
    itkGetConstReferenceMacro(Spectra, SpectrumVectorType);

    void InitializeImageAccess() override;
    void InitializeGeometry() override;
    void InitializeProcessor() override;
    void InitializeNormalizationImage(m2::NormalizationStrategyType) override {}
    void GetSpectrum(unsigned int, std::vector<double> &, std::vector<double> &) const override {}
    void GetIntensities(unsigned int, std::vector<double> &) const override {}
    void GetSpectrumFloat(unsigned int, std::vector<float> &, std::vector<float> &) const override {}
    void GetIntensitiesFloat(unsigned int, std::vector<float> &) const override {}

  private:
    SpectrumVectorType m_Spectra;
    m2::SpectrumFormat m_ImportMode = m2::SpectrumFormat::ContinuousProfile;
    using m2::SpectrumImage::InternalClone;
    bool m_ImageAccessInitialized = false;
    bool m_ImageGeometryInitialized = false;

    FsmSpectrumImage();
    ~FsmSpectrumImage() override;
    class FsmProcessor;
    std::unique_ptr<m2::ISpectrumImageSource> m_Processor;
  };

  class FsmSpectrumImage::FsmProcessor : public m2::ISpectrumImageSource
  {
  private:
    friend class FsmSpectrumImage;
    m2::FsmSpectrumImage *p;

  public:
    explicit FsmProcessor(m2::FsmSpectrumImage *owner) : p(owner) {}
    
    void GetYValues(unsigned int id, std::vector<float> & data) 
    {
      data = p->m_Spectra[id].data;
    }
    
    void GetYValues(unsigned int id, std::vector<double> & data) 
    {
      const auto & d = p->m_Spectra[id].data;
      data.resize(d.size());
      std::copy(std::begin(d), std::end(d), std::begin(data));
    }
    
    void GetXValues(unsigned int /*id*/, std::vector<float> & data) 
    {
      const auto & d = p->GetXAxis();
      data.resize(d.size());
      std::copy(std::begin(d), std::end(d), std::begin(data));
    }
    
    void GetXValues(unsigned int /*id*/, std::vector<double> & data) 
    {
      data = p->GetXAxis();
    }
    
    void GetImagePrivate(double mz, double tol, const mitk::Image *mask, mitk::Image *image) override;
    void InitializeImageAccess() override;
    void InitializeGeometry() override;
  };

  

} // namespace m2
