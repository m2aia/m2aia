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
#include <m2SpectrumImageProcessor.h>
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
    void GetSpectrum(unsigned int, std::vector<float> &, std::vector<float> &, unsigned int) const override {}
    void GetIntensities(unsigned int, std::vector<float> &, unsigned int) const override {}

  private:
    SpectrumVectorType m_Spectra;
    m2::SpectrumFormat m_ImportMode = m2::SpectrumFormat::ContinuousProfile;
    using m2::SpectrumImageBase::InternalClone;
    bool m_ImageAccessInitialized = false;
    bool m_ImageGeometryInitialized = false;

    FsmSpectrumImage();
    ~FsmSpectrumImage() override;
    class FsmProcessor;
    std::unique_ptr<m2::ProcessorBase> m_Processor;
  };

  class FsmSpectrumImage::FsmProcessor : public m2::ProcessorBase
  {
  private:
    friend class FsmSpectrumImage;
    m2::FsmSpectrumImage *p;

  public:
    explicit FsmProcessor(m2::FsmSpectrumImage *owner) : p(owner) {}
    
    void GetYValues(unsigned int id, std::vector<float> & data, unsigned int /*source*/ = 0) 
    {
      data = p->m_Spectra[id].data;
    }
    
    void GetYValues(unsigned int id, std::vector<double> & data, unsigned int /*source*/ = 0) 
    {
      const auto & d = p->m_Spectra[id].data;
      data.resize(d.size());
      std::copy(std::begin(d), std::end(d), std::begin(data));
    }
    
    void GetXValues(unsigned int /*id*/, std::vector<float> & data, unsigned int /*source*/ = 0) 
    {
      const auto & d = p->GetXAxis();
      data.resize(d.size());
      std::copy(std::begin(d), std::end(d), std::begin(data));
    }
    
    void GetXValues(unsigned int /*id*/, std::vector<double> & data, unsigned int /*source*/ = 0) 
    {
      data = p->GetXAxis();
    }
    
    void GetImagePrivate(double mz, double tol, const mitk::Image *mask, mitk::Image *image) override;
    void InitializeImageAccess() override;
    void InitializeGeometry() override;
  };

  

} // namespace m2
