/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <m2PeakDetection.h>
#include <m2SpectrumImageBase.h>
#include <mitkDataNode.h>
#include <mitkLevelWindowProperty.h>
#include <mitkLookupTableProperty.h>
#include <mitkOperation.h>

void m2::SpectrumImageBase::ApplyMoveOriginOperation(const std::array<int, 2> &v)
{
  auto geometry = this->GetGeometry();
  auto pos = geometry->GetOrigin();
  auto space = geometry->GetSpacing();
  pos[0] = pos[0] + v.at(0) * space[0];
  pos[1] = pos[1] + v.at(1) * space[1];
  geometry->SetOrigin(pos);

  for (auto kv : m_ImageArtifacts)
  {
    geometry = kv.second->GetGeometry();
    pos = geometry->GetOrigin();
    space = geometry->GetSpacing();
    pos[0] = pos[0] + v.at(0) * space[0];
    pos[1] = pos[1] + v.at(1) * space[1];
    geometry->SetOrigin(pos);
  }
}

void m2::SpectrumImageBase::ApplyGeometryOperation(mitk::Operation *op)
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

m2::SpectrumImageBase::SpectrumArtifactVectorType &m2::SpectrumImageBase::SkylineSpectrum()
{
  return m_SpectraArtifacts[m2::OverviewSpectrumType::Maximum];
}

m2::SpectrumImageBase::SpectrumArtifactVectorType &m2::SpectrumImageBase::PeakIndicators()
{
  return m_SpectraArtifacts[m2::OverviewSpectrumType::PeakIndicators];
}

m2::SpectrumImageBase::SpectrumArtifactVectorType &m2::SpectrumImageBase::MeanSpectrum()
{
  return m_SpectraArtifacts[m2::OverviewSpectrumType::Mean];
}

m2::SpectrumImageBase::SpectrumArtifactVectorType &m2::SpectrumImageBase::SumSpectrum()
{
  return m_SpectraArtifacts[m2::OverviewSpectrumType::Sum];
}

m2::SpectrumImageBase::SpectrumArtifactVectorType &m2::SpectrumImageBase::GetXAxis()
{
  return m_XAxis;
}

const m2::SpectrumImageBase::SpectrumArtifactVectorType &m2::SpectrumImageBase::GetXAxis() const
{
  return m_XAxis;
}

mitk::Image::Pointer m2::SpectrumImageBase::GetNormalizationImage()
{
  if (m_ImageArtifacts.find("NormalizationImage") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("NormalizationImage").GetPointer());
  return nullptr;
}

mitk::Image::Pointer m2::SpectrumImageBase::GetMaskImage()
{
  if (m_ImageArtifacts.find("mask") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("mask").GetPointer());
  return nullptr;
}

mitk::Image::Pointer m2::SpectrumImageBase::GetIndexImage()
{
  if (m_ImageArtifacts.find("index") != m_ImageArtifacts.end())
    return dynamic_cast<mitk::Image *>(m_ImageArtifacts.at("index").GetPointer());
  return nullptr;
}

void m2::SpectrumImageBase::GenerateImageData(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const
{
  GenerateImageStart.Send();
  m_Processor->CreateIonImagePrivate(mz, tol, mask, img);
  GenerateImageEnd.Send();
}

void m2::SpectrumImageBase::ReceiveIntensities(unsigned int index,
                                               std::vector<double> &ints,
                                               unsigned int sourceIndex) const
{
  m_Processor->GrabIntensityPrivate(index, ints, sourceIndex);
}

void m2::SpectrumImageBase::ReceivePositions(unsigned int index,
                                             std::vector<double> &mzs,
                                             unsigned int sourceIndex) const
{
  m_Processor->GrabMassPrivate(index, mzs, sourceIndex);
}

void m2::SpectrumImageBase::ReceiveSpectrum(unsigned int index,
                                            std::vector<double> &mzs,
                                            std::vector<double> &ints,
                                            unsigned int sourceIndex) const
{
  ReceiveSpectrumStart.Send();
  m_Processor->GrabMassPrivate(index, mzs, sourceIndex);
  m_Processor->GrabIntensityPrivate(index, ints, sourceIndex);
  ReceiveSpectrumEnd.Send();
}

m2::SpectrumImageBase::~SpectrumImageBase() {}

m2::SpectrumImageBase::SpectrumImageBase() : mitk::Image() {}
