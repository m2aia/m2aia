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
#include <m2SpectrumImageBase.h>
#include <m2ElxRegistrationHelper.h>
#include <memory>

namespace m2
{
  /**
   * MS3DComposeDataObject represents a Composition of multiple mitk::MSDataObjects
   *
   **/
  class M2AIACORE_EXPORT SpectrumImageStack : public SpectrumImageBase
  {
  public:
    mitkClassMacro(SpectrumImageStack, SpectrumImageBase);
    mitkNewMacro1Param(Self, double);

  protected:
  // key: slice index
  // value: transformer
    std::map<unsigned int, std::shared_ptr<m2::ElxRegistrationHelper>> m_SliceTransformers;
    void CopyWarpedImageToStackImage(mitk::Image *warped, mitk::Image *stack, unsigned i) const;
    explicit SpectrumImageStack(double spacingZ);
    SpectrumImageStack() = delete;
    double m_SpacingZ = 0;

  public:
    void Insert(unsigned int sliceId ,std::shared_ptr<m2::ElxRegistrationHelper> transformer);

    const std::map<unsigned int, std::shared_ptr<m2::ElxRegistrationHelper>> & GetSliceTransformers() const {
      return m_SliceTransformers;
    }

    void InitializeGeometry() override;
    void InitializeProcessor() override;
    void InitializeImageAccess() override;

    

    virtual void GetImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;
    virtual void GetSpectrum(unsigned int, std::vector<float> &, std::vector<float> &, unsigned int) const override{}
    virtual void GetIntensities(unsigned int, std::vector<float> &, unsigned int) const override{}
  };

} // namespace m2
