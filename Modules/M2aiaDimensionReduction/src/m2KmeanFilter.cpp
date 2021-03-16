#include <m2KmeanFilter.h>

#include <itkRescaleIntensityImageFilter.h>

#include <vnl/vnl_matrix.h>

#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <m2ImzMLSpectrumImage.h>

void mitk::m2KmeanFilter::GenerateData()
{
  MITK_INFO << "Hello start filter";
  this->GetValidIndices();
  unsigned int numberOfInputs = this->GetIndexedInputs().size();
  vnl_matrix<double> pixelValues(m_ValidIndices.size(), numberOfInputs);

  std::map<int, std::vector<int>> pixelCentroidAffiliations;
  std::map<int, vnl_vector<double>> centroidsMean;

  for (unsigned int i = 0; i < m_NumberOfCluster; ++i)
  {
    vnl_vector<double> vector(numberOfInputs);
    centroidsMean.insert({i, vector});
    std::vector<int> pixelRowPosition;
    pixelCentroidAffiliations.insert({i, pixelRowPosition});
  }

  std::vector<ImageType::Pointer> normalizedImages;
  for (auto inputImage : this->GetIndexedInputs())
  {
    ImageType::Pointer itkImage;
    mitk::CastToItkImage(dynamic_cast<mitk::Image *>(inputImage.GetPointer()), itkImage);
    ZTransformation(itkImage);

    auto intensityRescale = itk::RescaleIntensityImageFilter<ImageType>::New();
    intensityRescale->SetOutputMaximum(1);
    intensityRescale->SetOutputMinimum(0);
    intensityRescale->SetInput(itkImage);
    intensityRescale->Update();
    normalizedImages.push_back(intensityRescale->GetOutput());
  }

  unsigned i = 0;
  unsigned j = 0;

  for (auto image : normalizedImages)
  {
    j = 0;
    for (auto index : m_ValidIndices)
    {
      pixelValues(j, i) = (image->GetPixel(index));
      ++j;
    }
    ++i;
  }

  int w = m_ValidIndices.size() / m_NumberOfCluster;
  for (unsigned int i = 0; i < m_NumberOfCluster; ++i)
  {
    auto vec = pixelValues.get_row(w * i);

    centroidsMean.at(i) = vec;
  }

  bool clusteringNotFinished = true;
  std::map<int, vnl_vector<double>> newCentroidsMean;
  for (unsigned int i = 0; i < centroidsMean.size(); ++i)
  {
    vnl_vector<double> newCentroidVector(numberOfInputs, 0);
    newCentroidsMean.insert({i, newCentroidVector});
  }

  /*Remove all pixels form the clusters and recalculate the cluster affiliation.*/
  while (clusteringNotFinished)
  {
    unsigned int numberOfNoChange = 0;
    for (unsigned int i = 0; i < newCentroidsMean.size(); ++i)
    {
      newCentroidsMean.at(i) *= 0;
      pixelCentroidAffiliations.at(i).clear();
    }

    for (unsigned int i = 0; i < pixelValues.rows(); ++i)
    {
      auto pixelVector = pixelValues.get_row(i);
      int centroidAffiliation = 0;
      double centroidDistance = pow(2, (centroidsMean.at(0) - pixelVector).magnitude());
      for (unsigned int k = 1; k < m_NumberOfCluster; ++k)
      {
        double pixelCentroidDistance = pow(2, (centroidsMean.at(k) - pixelVector).magnitude());
        if (pixelCentroidDistance < centroidDistance)
        {
          centroidDistance = pixelCentroidDistance;
          centroidAffiliation = k;
        }
      }

      pixelCentroidAffiliations.at(centroidAffiliation).push_back(i);
    }

    for (unsigned int i = 0; i < pixelCentroidAffiliations.size(); ++i)
    {
      std::vector<int> affiliations = pixelCentroidAffiliations.at(i);
      for (unsigned int j = 0; j < affiliations.size(); ++j)
      {
        newCentroidsMean.at(i) += pixelValues.get_row(affiliations.at(j));
      }
      newCentroidsMean.at(i) /= affiliations.size();
      if (fabs(centroidsMean.at(i).magnitude() - newCentroidsMean.at(i).magnitude()) < 0.00001)
      {
        ++numberOfNoChange;
      }
      else
      {
        clusteringNotFinished = true;
      }
      MITK_INFO << fabs(centroidsMean.at(i).magnitude() - newCentroidsMean.at(i).magnitude());
      centroidsMean.at(i) = newCentroidsMean.at(i);
    }
    if (numberOfNoChange == m_NumberOfCluster)
    {
      clusteringNotFinished = false;
    }
  }
  MITK_INFO << "Finish";
  auto result = itk::Image<unsigned char, 3>::New();
  initializeItkImage(result);
  result->Allocate();

  char colorWidhth = 255 / m_NumberOfCluster;
  std::vector<unsigned char> centroidColor;
  for (unsigned int i = 1; i <= m_NumberOfCluster; ++i)
  {
    centroidColor.push_back(colorWidhth * i);
  }

  for (unsigned int i = 0; i < pixelCentroidAffiliations.size(); ++i)
  {
    for (unsigned int j = 0; j < pixelCentroidAffiliations.at(i).size(); ++j)
    {
      int centroidAffiliation = pixelCentroidAffiliations.at(i).at(j);
      result->SetPixel(m_ValidIndices.at(centroidAffiliation), centroidColor.at(i));
    }
  }

  mitk::Image::Pointer kmeanImage = this->GetOutput();

  MITK_INFO << "Cast";
  mitk::CastToMitkImage(result, kmeanImage);
  MITK_INFO << "Cast to MITK Done";
  kmeanImage->SetSpacing(this->GetInput()->GetGeometry()->GetSpacing());
  kmeanImage->SetOrigin(this->GetInput()->GetGeometry()->GetOrigin());
  mitk::IOUtil::Save(kmeanImage, "/home/marven/Pictures/kmean.nrrd");
}

void mitk::m2KmeanFilter::initializeItkImage(itk::Image<unsigned char, 3>::Pointer itkImage)
{
  itk::Image<unsigned char, 3>::IndexType start;
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;

  itk::Image<unsigned char, 3>::SizeType size;
  auto dimensions = this->GetInput()->GetDimensions();
  size[0] = dimensions[0];
  size[1] = dimensions[1];
  size[2] = dimensions[2];
  itk::Image<unsigned char, 3>::RegionType region;
  region.SetSize(size);
  region.SetIndex(start);

  itkImage->SetRegions(region);
  itkImage->Allocate();

  itk::ImageRegionIterator<itk::Image<unsigned char, 3>> imageIterator(itkImage, itkImage->GetRequestedRegion());
  imageIterator.GoToBegin();

  while (!imageIterator.IsAtEnd())
  {
    imageIterator.Set(0);
    ++imageIterator;
  }
}

void mitk::m2KmeanFilter::ZTransformation(itk::Image<double, 3>::Pointer image)
{
  auto loopOverMaskImage = [this](auto lambda) {
    for (auto index : this->m_ValidIndices)
    {
      lambda(index);
    }
  };

  double sqrtSum = 0;
  double sum = 0;

  loopOverMaskImage([&](itk::Index<3> index) {
    auto value = image->GetPixel(index);
    sum += value;
  });
  double mean = sum / this->m_ValidIndices.size();
  loopOverMaskImage([&](itk::Index<3> index) {
    auto value = image->GetPixel(index);
    sqrtSum += (value - mean) * (value - mean);
  });
  double stdev = std::sqrt((1.0 / double((this->m_ValidIndices.size()))) * sqrtSum);
  loopOverMaskImage([&](itk::Index<3> index) {
    auto value = image->GetPixel(index);
    double normalizedPixel = (value - mean) / stdev;
    image->SetPixel(index, normalizedPixel);
  });
}
