/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <m2SpectrumImage.h>
#include <mitkDataNode.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkLevelWindowProperty.h>
#include <mitkLookupTableProperty.h>
#include <mitkOperation.h>
#include <signal/m2PeakDetection.h>

namespace m2
{
  itkEventMacroDefinition(InitializationFinishedEvent, itk::AnyEvent);
} // namespace m2

double m2::SpectrumImage::ApplyTolerance(double xValue) const
{
  if (this->GetUseToleranceInPPM())
    return m2::PartPerMillionToFactor(this->GetTolerance()) * xValue;
  else
    return this->GetTolerance();
}

void m2::SpectrumImage::InsertImageArtifact(const std::string &key, mitk::Image *img)
{
  m_ImageArtifacts[key] = img;
  const unsigned int DIMS = 3;

  if (img->GetDimension() != DIMS)
  {
    mitkThrow() << "SpectrumBaseImage related image artifacts require 3 dimensions.";
  }
  auto aD = this->GetDimensions();
  auto bD = img->GetDimensions();

  if (!std::equal(aD, aD + DIMS, bD))
  {
    mitkThrow() << "SpectrumBaseImage related image artifacts require identical image dimensions.";
  }

  auto aS = this->GetGeometry()->GetSpacing();
  auto bS = img->GetGeometry()->GetSpacing();

  if (!std::equal(
        aS.GetDataPointer(), aS.GetDataPointer() + DIMS, bS.GetDataPointer(), [](const auto &a, const auto &b) {
          return itk::Math::FloatAlmostEqual(a, b);
        }))
  {
    mitkThrow() << "SpectrumBaseImage related image artifacts require identical spacings.";
  }

  // if spacing and dimensions are equal, copy origin and vtkMatrix to the new image artifact.
  img->SetClonedTimeGeometry(this->GetTimeGeometry());
}

void m2::SpectrumImage::ApplyMoveOriginOperation(const mitk::Vector3D &v)
{
  auto geometry = this->GetGeometry();
  geometry->Translate(v);

  for (auto kv : m_ImageArtifacts)
  {
    geometry = kv.second->GetGeometry();
    geometry->Translate(v);
  }
}

void m2::SpectrumImage::ApplyGeometryOperation(mitk::Operation *op)
{
  auto manipulatedGeometry = this->GetGeometry()->Clone();
  manipulatedGeometry->ExecuteOperation(op);
  this->GetGeometry()->SetIdentity();
  this->GetGeometry()->Compose(manipulatedGeometry->GetIndexToWorldTransform());

  for (auto kv : m_ImageArtifacts)
  {
    auto manipulatedGeometry = kv.second->GetGeometry()->Clone();
    manipulatedGeometry->ExecuteOperation(op);
    kv.second->GetGeometry()->SetIdentity();
    kv.second->GetGeometry()->Compose(manipulatedGeometry->GetIndexToWorldTransform());
  }
}

m2::SpectrumImage::SpectrumArtifactVectorType &m2::SpectrumImage::GetSkylineSpectrum()
{
  return m_SpectraArtifacts[(SpectrumType::Maximum)];
}

m2::SpectrumImage::SpectrumArtifactVectorType &m2::SpectrumImage::GetMeanSpectrum()
{
  return m_SpectraArtifacts[(SpectrumType::Mean)];
}

m2::SpectrumImage::SpectrumArtifactVectorType &m2::SpectrumImage::GetSumSpectrum()
{
  return m_SpectraArtifacts[(SpectrumType::Sum)];
}

m2::SpectrumImage::SpectrumArtifactVectorType &m2::SpectrumImage::GetXAxis()
{
  return m_XAxis;
}

const m2::SpectrumImage::SpectrumArtifactVectorType &m2::SpectrumImage::GetXAxis() const
{
  return m_XAxis;
}

mitk::Image::Pointer m2::SpectrumImage::GetNormalizationImage()
{
  if (m_ImageArtifacts.find("NormalizationImage") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("NormalizationImage").GetPointer());
  return nullptr;
}

mitk::Image::Pointer m2::SpectrumImage::GetMaskImage()
{
  if (m_ImageArtifacts.find("mask") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("mask").GetPointer());
  return nullptr;
}

mitk::Image::Pointer m2::SpectrumImage::GetIndexImage()
{
  if (m_ImageArtifacts.find("index") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("index").GetPointer());
  return nullptr;
}

mitk::Image::Pointer m2::SpectrumImage::GetNormalizationImage() const
{
  if (m_ImageArtifacts.find("NormalizationImage") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("NormalizationImage").GetPointer());
  return nullptr;
}

mitk::Image::Pointer m2::SpectrumImage::GetMaskImage() const
{
  if (m_ImageArtifacts.find("mask") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("mask").GetPointer());
  return nullptr;
}

mitk::Image::Pointer m2::SpectrumImage::GetIndexImage() const
{
  if (m_ImageArtifacts.find("index") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("index").GetPointer());
  return nullptr;
}

void m2::SpectrumImage::GetImage(double, double, const mitk::Image *, mitk::Image *) const
{
  MITK_WARN("SpectrumImage") << "Get image is not implemented in derived class!";
}

m2::SpectrumImage::~SpectrumImage() {}
m2::SpectrumImage::SpectrumImage() : mitk::Image() {}
