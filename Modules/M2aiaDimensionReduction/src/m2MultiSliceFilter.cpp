#include <itkIntensityWindowingImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkRescaleIntensityImageFilter.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2MultiSliceFilter.h>
#include <m2RGBColorMixer.hpp>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>

void m2::MultiSliceFilter::SetColorVector(std::vector<std::array<ColorType, 4>> colorVector)
{
  this->m_ColorVector = colorVector;
}
void m2::MultiSliceFilter::SetScaleOptions(std::vector<std::array<float, 2>> options)
{
  m_ScaleOptions = options;
}

void m2::MultiSliceFilter::SetVisualizationOption(VisualizationOptions option)
{
  m_VisualizationOption = option;
}

void m2::MultiSliceFilter::GenerateData()
{
  auto input = this->GetIndexedInputs();
  this->GetValidIndices();

  auto itkScalarImage = itk::Image<double, 3>::New();
  auto itkIndexImage = itk::Image<double, 3>::New();

  initializeItkDoubleImage(itkScalarImage);
  initializeItkDoubleImage(itkIndexImage);

  std::vector<itk::Image<double, 3>::Pointer> scaledImages;

  unsigned int currentImage = 0;
  std::vector<itk::Image<RGBPixel, 3>::Pointer> colorImages;
  itk::Image<double, 3>::Pointer optionScaledImage;

  for (auto const &image : input)
  {
    itk::Image<double, 3>::Pointer itkImage;
    mitk::CastToItkImage(dynamic_cast<mitk::Image *>(image.GetPointer()), itkImage);

    auto colorImage = itk::Image<RGBPixel, 3>::New();
    this->initializeColorImage(colorImage);
    colorImages.push_back(colorImage);

    auto rescaler = itk::RescaleIntensityImageFilter<itk::Image<double, 3>>::New();
    rescaler->SetInput(itkImage);
    rescaler->SetOutputMaximum(1);
    rescaler->SetOutputMinimum(0);
    rescaler->Update();
    auto scaledImage = rescaler->GetOutput();

    auto windowRescaler = itk::IntensityWindowingImageFilter<itk::Image<double, 3>>::New();
    windowRescaler->SetInput(scaledImage);
    windowRescaler->SetOutputMaximum(1);
    windowRescaler->SetOutputMinimum(0);
    windowRescaler->SetWindowMaximum(1 - (1 - m_ScaleOptions.at(currentImage)[1]));
    windowRescaler->SetWindowMinimum(0 + m_ScaleOptions.at(currentImage)[0]);
    windowRescaler->Update();

    optionScaledImage = windowRescaler->GetOutput();
    scaledImages.push_back(optionScaledImage);

    for (auto index : m_ValidIndices)
    {
      auto pixelValue = optionScaledImage->GetPixel(index);
      RGBPixel pixel;
      pixel.SetRed(MapIntensityValue(m_ColorVector.at(currentImage)[0], pixelValue, 0, 1));
      pixel.SetGreen(MapIntensityValue(m_ColorVector.at(currentImage)[1], pixelValue, 0, 1));
      pixel.SetBlue(MapIntensityValue(m_ColorVector.at(currentImage)[2], pixelValue, 0, 1));
      colorImage->SetPixel(index, pixel);

      if (m_VisualizationOption == m2::MultiSliceFilter::MAXIMUM)
      {
        if (currentImage == 0)
        {
          itkScalarImage->SetPixel(index, pixelValue);
          itkIndexImage->SetPixel(index, currentImage);
        }
        else
        {
          if (pixelValue > itkScalarImage->GetPixel(index))
          {
            itkScalarImage->SetPixel(index, pixelValue);
            itkIndexImage->SetPixel(index, currentImage);
          }
        }
      }
    }
    ++currentImage;
  }

  if (m_VisualizationOption == m2::MultiSliceFilter::MAXIMUM)
  {
    for (unsigned int i = 0; i < input.size(); ++i)
    {
      std::vector<itk::Index<3>> indices;
      double maxValue = 0;
      double minValue = 1;

      for (auto index : this->m_ValidIndices)
      {
        if (itkIndexImage->GetPixel(index) == i)
        {
          indices.push_back(index);
          auto scalarImageValue = itkScalarImage->GetPixel(index);
          if (scalarImageValue > maxValue)
          {
            maxValue = scalarImageValue;
          }
          if (scalarImageValue < minValue)
          {
            minValue = scalarImageValue;
          }
        }
      }

      /*Iterate over all indices, which have their origin in the same image and scale the values.*/
      for (auto index : indices)
      {
        auto pixelValue = itkScalarImage->GetPixel(index);
        auto newPixelValue = (pixelValue - minValue) / (maxValue - minValue);
        itkScalarImage->SetPixel(index, newPixelValue);
      }
    }
  }

  auto outputImage = itk::Image<itk::RGBPixel<OutputColorType>, 3>::New();
  this->initializeItkImage(outputImage);

  for (auto index : m_ValidIndices)
  {
    if (m_VisualizationOption == VisualizationOptions::MAXIMUM)
    {
      auto maxIndex = itkIndexImage->GetPixel(index);
      auto image = colorImages.at(maxIndex);
      RGBPixel pixel = image->GetPixel(index);
      pixel.SetRed(pixel.GetRed() * 255);
      pixel.SetGreen(pixel.GetGreen() * 255);
      pixel.SetBlue(pixel.GetBlue() * 255);

      outputImage->SetPixel(index, pixel);
    }
    else
    {
      std::vector<RGBPixel> pixelList;
      for (auto image : colorImages)
      {
        auto pixel = image->GetPixel(index);
        pixelList.push_back(pixel);
      }
      RGBPixel newColor = m2::ColorMixer::m2RGBColorMixer<RGBPixel>::MixRGBColors(pixelList);
      itk::RGBPixel<OutputColorType> colorPixel;
      colorPixel.SetRed(newColor.GetRed() * 255);
      colorPixel.SetGreen(newColor.GetGreen() * 255);
      colorPixel.SetBlue(newColor.GetBlue() * 255);
      //colorPixel.SetAlpha((newColor.GetBlue() + newColor.GetGreen() + newColor.GetRed()) * 255);
      outputImage->SetPixel(index, colorPixel);
    }

    itkIndexImage->SetPixel(index, itkIndexImage->GetPixel(index) + 1);
  }

  mitk::Image::Pointer mitkOutputImage = this->GetOutput();

  mitk::CastToMitkImage(outputImage, mitkOutputImage);

  mitkOutputImage->SetSpacing(this->GetInput()->GetGeometry()->GetSpacing());
  mitkOutputImage->SetOrigin(this->GetInput()->GetGeometry()->GetOrigin());
  if (m_VisualizationOption == m2::MultiSliceFilter::MAXIMUM)
  {
    mitk::Image::Pointer mitkIndexImage;

    auto intenistyFilter = itk::RescaleIntensityImageFilter<itk::Image<double, 3>>::New();
    intenistyFilter->SetInput(itkIndexImage);
    intenistyFilter->SetOutputMaximum(255);
    intenistyFilter->SetOutputMinimum(0);
    intenistyFilter->Update();
    auto scaledIndexImage = intenistyFilter->GetOutput();

    mitk::CastToMitkImage(scaledIndexImage, mitkIndexImage);
    mitkIndexImage->SetSpacing(this->GetInput()->GetGeometry()->GetSpacing());
    mitkIndexImage->SetOrigin(this->GetInput()->GetGeometry()->GetOrigin());

    m_IndexImage = mitkIndexImage;
  }
}

m2::MultiSliceFilter::ColorType m2::MultiSliceFilter::MapIntensityValue(ColorType maxValue,
                                                                        float scalar,
                                                                        float min,
                                                                        float max)
{
  auto newPixelVal = (scalar - min) * ((maxValue - 0) / (max - min)) + 0;
  return newPixelVal;
}

void m2::MultiSliceFilter::initializeColorImage(itk::Image<RGBPixel, 3>::Pointer image)
{
  itk::Image<RGBPixel, 3>::IndexType start;
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;

  itk::Image<RGBPixel, 3>::SizeType size;
  auto dimensions = this->GetInput()->GetDimensions();
  size[0] = dimensions[0];
  size[1] = dimensions[1];
  size[2] = dimensions[2];
  itk::Image<RGBPixel, 3>::RegionType region;
  region.SetSize(size);
  region.SetIndex(start);

  image->SetRegions(region);
  image->Allocate();

  itk::ImageRegionIterator<itk::Image<RGBPixel, 3>> imageIterator(image, image->GetRequestedRegion());
  imageIterator.GoToBegin();
  auto pixel = RGBPixel();
  pixel.SetRed(0);
  pixel.SetBlue(0);
  pixel.SetGreen(0);

  while (!imageIterator.IsAtEnd())
  {
    imageIterator.Set(pixel);
    ++imageIterator;
  }
}

void m2::MultiSliceFilter::initializeItkDoubleImage(itk::Image<double, 3>::Pointer image)
{
  using ImageType = itk::Image<double, 3>;

  ImageType::IndexType start;
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;

  ImageType::SizeType size;
  size[0] = this->GetInput()->GetDimensions()[0];
  size[1] = this->GetInput()->GetDimensions()[1];
  size[2] = this->GetInput()->GetDimensions()[2];

  ImageType::RegionType region;
  region.SetSize(size);
  region.SetIndex(start);

  image->SetRegions(region);
  image->Allocate();

  itk::ImageRegionIterator<itk::Image<double, 3>> imageIterator(image, image->GetRequestedRegion());
  imageIterator.GoToBegin();

  while (!imageIterator.IsAtEnd())
  {
    imageIterator.Set(0);
    ++imageIterator;
  }
}
