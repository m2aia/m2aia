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
  class M2AIACORE_EXPORT SpectrumContainerImage final : public SpectrumImage
  {
  public:
    public:
    typedef SpectrumContainerImage Self;
    typedef SpectrumImage Superclass;
    typedef itk::SmartPointer<Self> Pointer;
    typedef itk::SmartPointer<const Self> ConstPointer;
    virtual std::vector<std::string> GetClassHierarchy() const override { return mitk::GetClassHierarchy<Self>(); }

    // Overwrite those methods to make MITK recognize this as default mitk::Image
    static const char *GetStaticNameOfClass() { return "Image"; }
    const char *GetNameOfClass() const override { return "Image"; }

    itkNewMacro(Self);

    // mitkClassMacro(SpectrumContainerImage, SpectrumImage);
    // itkNewMacro(Self);

    itkSetEnumMacro(ImageGeometryInitialized, bool);
    itkGetEnumMacro(ImageGeometryInitialized, bool);

    itkSetEnumMacro(ImageAccessInitialized, bool);
    itkGetEnumMacro(ImageAccessInitialized, bool);

    /**
     * @brief Modality of this container image.
     *   MSI – Mass Spectrometry Imaging (m/z axis)
     *   MIR – Mid-InfraRed Spectral Imaging (wavenumber axis, cm⁻¹)
     * The modality controls which pre-processing and image-generation
     * steps are applied (e.g. absorbance conversion, derivative imaging).
     */
    itkSetEnumMacro(Modality, m2::ModalityType);
    itkGetEnumMacro(Modality, m2::ModalityType);

    // ── MIR-specific processing controls ──────────────────────────────────
    // All three flags are only evaluated when Modality == MIR.  They default
    // to off (None / false) so that existing code is unaffected; set them
    // explicitly after constructing or loading a MIR container image.

    /**
     * @brief Step 2 – atmospheric band suppression strategy.
     *
     *  None                – skip (default)
     *  LinearInterpolation – bridge CO₂ and H₂O bands with a linear ramp
     */
    itkSetEnumMacro(MIRAtmosphericCorrectionStrategy, m2::MIRAtmosphericCorrectionType);
    itkGetEnumMacro(MIRAtmosphericCorrectionStrategy, m2::MIRAtmosphericCorrectionType);

    /**
     * @brief Step 3 – polynomial scattering (baseline) correction strategy.
     *
     *  None              – skip (default)
     *  PolynomialDegree2 – subtract least-squares quadratic fit (EMSC-lite)
     */
    itkSetEnumMacro(MIRScatteringCorrectionStrategy, m2::MIRScatteringCorrectionType);
    itkGetEnumMacro(MIRScatteringCorrectionStrategy, m2::MIRScatteringCorrectionType);

    /**
     * @brief Step 4 – per-spectrum vector (L₂) normalisation.
     *
     *  false – skip (default)
     *  true  – divide each spectrum by its Euclidean norm
     */
    itkSetMacro(MIRVectorNormalization, bool);
    itkGetConstReferenceMacro(MIRVectorNormalization, bool);

    /**
     * @brief Step 5 – spectral derivative.
     *
     *  None   – skip (default)
     *  First  – centred first-order finite difference
     *  Second – centred second-order finite difference
     */
    itkSetEnumMacro(MIRDerivativeStrategy, m2::MIRDerivativeType);
    itkGetEnumMacro(MIRDerivativeStrategy, m2::MIRDerivativeType);

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
    };

    using SpectrumVectorType = std::vector<SpectrumData>;

    itkGetMacro(Spectra, SpectrumVectorType &);
    itkGetConstReferenceMacro(Spectra, SpectrumVectorType);

    void InitializeImageAccess() override;
    void InitializeGeometry() override;
    void InitializeProcessor() override;
    void InitializeNormalizationImage(m2::NormalizationStrategyType) override;
    void GetSpectrum(unsigned int id, std::vector<double> & xs, std::vector<double> & ys) const override {GetYValues(id, ys);GetXValues(id, xs);}
    void GetIntensities(unsigned int  id, std::vector<double> & ys) const override {GetYValues(id, ys);}
    void GetSpectrumFloat(unsigned int id, std::vector<float> & xs, std::vector<float> & ys) const override {GetYValues(id, ys);GetXValues(id, xs);}
    void GetIntensitiesFloat(unsigned int id, std::vector<float> & ys) const override {GetYValues(id, ys);}

  private:
    SpectrumVectorType m_Spectra;
    m2::SpectrumFormat m_ImportMode = m2::SpectrumFormat::ContinuousProfile;
    /// @brief Imaging modality – defaults to MIR because the container uses a cm⁻¹ x-axis.
    m2::ModalityType m_Modality = m2::ModalityType::MIR;
    /// @brief Step 2: atmospheric band suppression (default: off).
    m2::MIRAtmosphericCorrectionType m_MIRAtmosphericCorrectionStrategy =
      m2::MIRAtmosphericCorrectionType::None;
    /// @brief Step 3: polynomial scattering correction (default: off).
    m2::MIRScatteringCorrectionType  m_MIRScatteringCorrectionStrategy =
      m2::MIRScatteringCorrectionType::None;
    /// @brief Step 4: per-spectrum vector (L₂) normalisation (default: off).
    bool m_MIRVectorNormalization = false;
    /// @brief Step 5: spectral derivative (default: off).
    m2::MIRDerivativeType m_MIRDerivativeStrategy = m2::MIRDerivativeType::None;
    using m2::SpectrumImage::InternalClone;
    bool m_ImageAccessInitialized = false;
    bool m_ImageGeometryInitialized = false;

    SpectrumContainerImage();
    ~SpectrumContainerImage() override;


    void GetYValues(unsigned int id, std::vector<float> & data) const
    {
      data = m_Spectra[id].data;
    }
    
    void GetYValues(unsigned int id, std::vector<double> & data) const
    {
      const auto & d = m_Spectra[id].data;
      data.resize(d.size());
      std::copy(std::begin(d), std::end(d), std::begin(data));
    }
    
    void GetXValues(unsigned int /*id*/, std::vector<float> & data) const
    {
      const auto & d = GetXAxis();
      data.resize(d.size());
      std::copy(std::begin(d), std::end(d), std::begin(data));
    }
    
    void GetXValues(unsigned int /*id*/, std::vector<double> & data) const
    {
      data = GetXAxis();
    }
    
  };
 

} // namespace m2
