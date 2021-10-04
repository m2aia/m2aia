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
#include <functional>
#include <itkCastImageFilter.h>
#include <itkMetaDataObject.h>
#include <m2SpectrumImageBase.h>
#include <mitkBaseData.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkPointSet.h>
#include <set>

namespace m2
{
  /**
   * MS3DComposeDataObject represents a Composition of multiple mitk::MSDataObjects
   *
   **/
  class M2AIACORE_EXPORT TransformixMSDataObjectStack : public SpectrumImageBase
  {
  public:
    mitkClassMacro(TransformixMSDataObjectStack, SpectrumImageBase);

    mitkNewMacro1Param(Self, std::string);

  protected:
    using TransformationVector = std::vector<std::string>;
    std::vector<std::vector<SpectrumImageBase::Pointer>> m_MSDataObjectReferences;
    std::vector<std::vector<TransformationVector>> m_MSDataObjectTransformations;
    double m_ZSpacing = 0.0;
    unsigned m_StackSize = 0;
    const std::string m_Transformix;
    itkNewMacro(Self);

    auto Transform(
      const mitk::Image *const inImage,
      const std::vector<std::string> &transformations,
      std::function<void(std::string &)> adaptor = [](std::string &) {}) const -> mitk::Image::Pointer;

    void CopyWarpedImageToStack(mitk::Image *warped, mitk::Image *stack, unsigned i) const;

    static void ReplaceParameter(std::string &paramFileString, std::string what, std::string by);

  public:
    TransformixMSDataObjectStack() = default;
    explicit TransformixMSDataObjectStack(std::string transformixPath);

    bool IsPathToTransformixValid();
    void Resize(unsigned num);

    void Insert(unsigned i,
                m2::SpectrumImageBase::Pointer data,
                TransformixMSDataObjectStack::TransformationVector transformations);
    void InitializeImages(unsigned i, double zSpacing);

    void InitializeGeometry() override {}
    void InitializeProcessor() override {}
    void InitializeImageAccess() override {}

    // Inherited via IMSImageDataAccess
    // virtual void ReceiveSpectrum(unsigned int index, std::vector<double> &mzs, std::vector<double> &ints) const
    // override;
    virtual void GetImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;
    virtual void GetSpectrum(unsigned int, std::vector<float> &, std::vector<float> &, unsigned int) const override{}
  };

} // namespace m2
