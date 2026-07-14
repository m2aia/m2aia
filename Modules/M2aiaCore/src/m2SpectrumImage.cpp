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
#include <itksys/SystemTools.hxx>
#include <mitkDataNode.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkIOUtil.h>
#include <mitkLevelWindowProperty.h>
#include <mitkLookupTableProperty.h>
#include <mitkOperation.h>
#include <mitkStringProperty.h>
#include <signal/m2PeakDetection.h>

#include <itkCastImageFilter.h>

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

mitk::Image::Pointer m2::SpectrumImage::GetNormalizationImage()
{
    return GetNormalizationImage(m_NormalizationStrategy);
}

mitk::Image::Pointer m2::SpectrumImage::GetNormalizationImage(m2::NormalizationStrategyType type)
{
  
  auto it = m_NormalizationImages.find(type);
  if (it != m_NormalizationImages.end()){
      auto image = it->second;
      if (image.IsNotNull())
        return image;
  }

  std::string inputLocation;
  auto m2aiaDataPathProp = this->GetProperty("m2aia.IO.path");
  auto dataPathProp = this->GetProperty("path");
  if (m2aiaDataPathProp)
    inputLocation = m2aiaDataPathProp->GetValueAsString();
  else if (dataPathProp)
    inputLocation = dataPathProp->GetValueAsString();
  else{
    MITK_ERROR << "Cannot determine input location for normalization image. Maybe you forgot to set the property 'm2aia.IO.path' or 'path'. Returning nullptr.";
  }
  
  const std::string inputDir = itksys::SystemTools::GetFilenamePath(inputLocation);
  const std::string inputStem = itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation);
  const std::string sidecarBase = inputDir + "/" + inputStem + "/" + inputStem;
  const std::string legacyBase = inputDir + "/" + inputStem;
  const std::string typeName = m2::NormalizationStrategyTypeNames.at(to_underlying(type));
  std::string fileName = sidecarBase + "." + typeName + ".nrrd";

  if (!itksys::SystemTools::FileExists(fileName))
    fileName = legacyBase + "." + typeName + ".nrrd";

  if (!itksys::SystemTools::FileExists(fileName))
  {

    this->InitializeNormalizationImage(type);
    return this->GetNormalizationImage(type);
  }

  try
  {
    auto dataVector = mitk::IOUtil::Load(fileName);
    if (dataVector.empty())
      return nullptr;

    auto *loadedImage = dynamic_cast<mitk::Image *>(dataVector[0].GetPointer());
    if (loadedImage == nullptr)
      return nullptr;

    auto rawImage = mitk::Image::Pointer(loadedImage);

    // Cast to NormImagePixelType (float) to ensure a uniform pixel type in the cache.
    mitk::Image::Pointer imagePointer;
    AccessByItk(rawImage, ([&imagePointer](auto itkSrc) {
      using SrcType = typename std::remove_pointer<decltype(itkSrc)>::type;
      using DstType = itk::Image<m2::NormImagePixelType, SrcType::ImageDimension>;
      auto caster = itk::CastImageFilter<SrcType, DstType>::New();
      caster->SetInput(itkSrc);
      caster->Update();
      mitk::CastToMitkImage(caster->GetOutput(), imagePointer);
    }));

    SetImageGeometryInformation(this, imagePointer);
    SetNormalizationImage(imagePointer, type);
    return imagePointer;
  }
  catch (const std::exception &e)
  {
    MITK_INFO("SpectrumImage") << "GetNormalizationImage: exception while loading normalization image from '"
                                 << fileName << "': " << e.what() << ". Returning nullptr.";
    return nullptr;
  }
}

void m2::SpectrumImage::SetNormalizationImage(mitk::Image::Pointer image, m2::NormalizationStrategyType type)
{
  image->GetGeometry()->SetOrigin(GetGeometry()->GetOrigin());
  image->GetGeometry()->SetSpacing(GetGeometry()->GetSpacing());
  image->GetGeometry()->SetIndexToWorldTransformByVtkMatrixWithoutChangingSpacing(GetGeometry()->GetVtkMatrix());

  m_NormalizationImages[type] = image;
}


std::vector<double> &m2::SpectrumImage::GetSkylineSpectrum()
{
  return m_SpectraArtifacts[(SpectrumType::Maximum)];
}

std::vector<double> &m2::SpectrumImage::GetMeanSpectrum()
{
  return m_SpectraArtifacts[(SpectrumType::Mean)];
}

std::vector<double> &m2::SpectrumImage::GetSumSpectrum()
{
  return m_SpectraArtifacts[(SpectrumType::Sum)];
}

std::vector<double> &m2::SpectrumImage::GetXAxis()
{
  return m_XAxis;
}

const std::vector<double> &m2::SpectrumImage::GetXAxis() const
{
  return m_XAxis;
}

void m2::SpectrumImage::GetImage(double, double, const mitk::Image *, mitk::Image *) const
{
  MITK_WARN("SpectrumImage") << "Get image is not implemented in derived class!";
}

m2::SpectrumImage::~SpectrumImage() {}
m2::SpectrumImage::SpectrumImage() : mitk::Image() {}
