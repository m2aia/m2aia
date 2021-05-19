#include <m2MultiModalMaskExportHelper.h>

#include <algorithm>
#include <eigen3/Eigen/Dense>
#include <iterator>
#include <mitkImageAccessByItk.h>

#include <m2ImzMLSpectrumImage.h>
#include <m2PeakDetection.h>
#include <m2Pooling.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>

void m2::MultiModalMaskExportHelper::AddMaskNode(mitk::DataNode::Pointer maskNode)
{
  m_MaskImages.push_back(maskNode);
}

void m2::MultiModalMaskExportHelper::AddMsiDataNode(mitk::DataNode::Pointer msiDataNode)
{
  m_MsiDataNodes.push_back(msiDataNode);
}

void m2::MultiModalMaskExportHelper::SetFilePath(std::string path)
{
  m_FilePath = path;
}

void m2::MultiModalMaskExportHelper::SetLowerMzBound(double lowerBound)
{
  m_LowerMzBound = lowerBound;
}

void m2::MultiModalMaskExportHelper::SetUpperMzBound(double upperBound)
{
  m_UpperMzBound = upperBound;
}

void m2::MultiModalMaskExportHelper::SetExportOption(bool exportFullSpectra, bool exportBackground)
{
  m_ExportFullSpectra = exportFullSpectra;
  m_ExportBackground = exportBackground;
}

void m2::MultiModalMaskExportHelper::InitalizeLayerLabelMap(mitk::Image::Pointer labelImage,
                                                            mitk::LabelSetImage::Pointer maskImage,
                                                            unsigned int layer)
{
  AccessFixedDimensionByItk(labelImage,
                            ([&, this](auto input) {
                              mitk::Vector3D vectorIndex;
                              mitk::Vector3D vectorWorld;

                              std::map<unsigned int, std::vector<mitk::Vector3D>> *layerMapEntry =
                                &m_LayerLabelMap[layer];

                              for (unsigned int x = 0; x < input->GetLargestPossibleRegion().GetSize(0); x++)
                              {
                                for (unsigned int y = 0; y < input->GetLargestPossibleRegion().GetSize(1); y++)
                                {
                                  for (unsigned int z = 0; z < input->GetLargestPossibleRegion().GetSize(2); z++)
                                  {
                                    unsigned int labelValue = static_cast<unsigned int>(input->GetPixel({x, y, z}));
                                    if (labelValue > 0)
                                    {
                                      vectorIndex.SetElement(0, x);
                                      vectorIndex.SetElement(1, y);
                                      vectorIndex.SetElement(2, z);

                                      std::vector<mitk::Vector3D> *labelEntry = &layerMapEntry->operator[](labelValue);
                                      maskImage->GetGeometry()->IndexToWorld(vectorIndex, vectorWorld);
                                      labelEntry->push_back(vectorWorld);
                                    }
                                  }
                                }
                              }
                            }),
                            3)
}

void WriteSpectraPeaksToCsv(m2::SpectrumImageBase::Pointer msImage,
                            std::vector<unsigned int> indexValues,
                            std::string intensitiesFileName,
                            std::string massAxisFileName)
{
  std::ofstream output(intensitiesFileName);

  auto massIndicesMask = msImage->GetPeaks();

  for (auto imzlIndex : indexValues)
  {
    std::vector<double> intensities, mzs, intsMasked;
    msImage->ReceiveSpectrum(imzlIndex, mzs, intensities);

    for (auto p : massIndicesMask)
    {
      auto i = p.massAxisIndex;
      double val = 0;
      if (msImage->GetTolerance() == 0)
      {
        val = intensities[i];
      }
      else
      {
        auto tol = msImage->GetTolerance() * 10e-6 * mzs[i];
        auto subRes = m2::Signal::Subrange(mzs, mzs[i] - tol, mzs[i] + tol);
        std::vector<double>::iterator s = std::next(std::begin(intensities), subRes.first);
        std::vector<double>::iterator e = std::next(s, subRes.second);
        auto poolingStrategy = msImage->GetRangePoolingStrategy();
        val = m2::Signal::RangePooling<double>(s, e, poolingStrategy);
      }
      intsMasked.push_back(val);
    }

    auto intensitiesIt = intsMasked.begin();

    std::string outputString = std::to_string(*intensitiesIt);
    std::replace(outputString.begin(), outputString.end(), ',', '.');

    output.write(outputString.c_str(), outputString.size());
    ++intensitiesIt;

    while (intensitiesIt != intsMasked.end())
    {
      std::string doubleFormat = std::to_string(*intensitiesIt);
      std::replace(doubleFormat.begin(), doubleFormat.end(), ',', '.');
      std::string outputString = "," + doubleFormat;

      output.write(outputString.c_str(), outputString.size());
      ++intensitiesIt;
    }

    output << '\n';
  }
  output.close();

  std::ofstream mzOutput(massAxisFileName);

  auto mzIt = massIndicesMask.begin();
  std::string doubleFormat = std::to_string(mzIt->mass);
  std::replace(doubleFormat.begin(), doubleFormat.end(), ',', '.');

  mzOutput.write(doubleFormat.c_str(), doubleFormat.size());

  ++mzIt;
  while (mzIt != massIndicesMask.end())
  {
    //    std::string outputStr = "," + std::to_string(mzIt->mass);
    std::string doubleFormat = std::to_string(mzIt->mass);
    std::replace(doubleFormat.begin(), doubleFormat.end(), ',', '.');
    std::string outputString = "," + doubleFormat;

    mzOutput.write(outputString.c_str(), outputString.size());
    ++mzIt;
  }
  mzOutput.close();
}

/*Get the indices of the mz values with respect to the mz bounds*/
void m2::MultiModalMaskExportHelper::GetValidMzIndices(m2::SpectrumImageBase::Pointer msImage,
                                                       std::vector<unsigned int> *validIndices)
{
  unsigned int counter = 0;

  if (m_UpperMzBound != m_LowerMzBound)
  {
    for (auto mz : msImage->GetXAxis())
    {
      if (mz <= m_UpperMzBound && mz >= m_LowerMzBound)
      {
        validIndices->push_back(counter);
      }
      ++counter;
    }
  }
  else
  {
    for (auto it = msImage->GetXAxis().begin(); it < msImage->GetXAxis().end(); ++it)
    {
      validIndices->push_back(counter);
      ++counter;
    }
  }
}
/**
 * This method exports spectra at predefined positions as a csv file
 *
 * @param msImage, the mass spectrum image which used to grab the spectra
 * @param indexValues, the positions where the spectra should be grabed
 * @param validMzIndices, the indices of the mz values that are in a valid mz range
 * @param intensitiesFileName, the name of the csv file for the intensities
 * @param massAxisFileName, the name of the csv file for the mass axis/
 * */
void m2::MultiModalMaskExportHelper::WriteSpectraToCsv(m2::SpectrumImageBase::Pointer msImage,
                                                       std::vector<unsigned int> indexValues,
                                                       std::vector<unsigned int> validMzIndices,
                                                       std::string intensitiesFileName,
                                                       std::string massAxisFileName)
{
  std::ofstream output(intensitiesFileName);

  std::vector<double> intensities;

  for (auto imzlIndex : indexValues)
  {
    msImage->ReceiveIntensities(imzlIndex, intensities);

    auto indicesIt = validMzIndices.begin();

    std::string intialOutputString = std::to_string(intensities.at(*indicesIt));
    std::replace(intialOutputString.begin(), intialOutputString.end(), ',', '.');

    output.write(intialOutputString.c_str(), intialOutputString.size());
    ++indicesIt;

    while (indicesIt != validMzIndices.end())
    {
      std::string doubleFormat = std::to_string(intensities.at(*indicesIt));
      std::replace(doubleFormat.begin(), doubleFormat.end(), ',', '.');
      std::string outputString = "," + doubleFormat;

      output.write(outputString.c_str(), outputString.size());

      ++indicesIt;
    }
    output.write(std::string("\n").c_str(), 1);
  }

  output.close();

  std::ofstream massAxisOutput(massAxisFileName);
  std::vector<double> massAxis = msImage->GetXAxis();

  auto mzIt = validMzIndices.begin();
  std::string intialOutputString = std::to_string(massAxis.at(*mzIt));

  std::replace(intialOutputString.begin(), intialOutputString.end(), ',', '.');
  massAxisOutput.write(intialOutputString.c_str(), intialOutputString.size());
  ++mzIt;

  while (mzIt != validMzIndices.end())
  {
    std::string doubleFormat = std::to_string(massAxis.at(*mzIt));
    std::replace(doubleFormat.begin(), doubleFormat.end(), ',', '.');
    std::string outputString = "," + doubleFormat;

    massAxisOutput.write(outputString.c_str(), outputString.size());

    ++mzIt;
  }
  massAxisOutput.close();
}

/*
This method exports spectra in form of csv files
*/
void m2::MultiModalMaskExportHelper::Export()
{
  for (auto maskNode : m_MaskImages)
  {
    if (mitk::LabelSetImage::Pointer maskImage = dynamic_cast<mitk::LabelSetImage *>(maskNode->GetData()))
    {
      auto numberOfLayers = maskImage->GetNumberOfLayers();
      if (numberOfLayers > 1)
      {
        for (unsigned int layer = 0; layer < maskImage->GetNumberOfLayers(); ++layer)
        {
          mitk::Image::Pointer layerImage = maskImage->GetLayerImage(layer);
          InitalizeLayerLabelMap(layerImage, maskImage, layer);
        }
      }
      else
      {
        InitalizeLayerLabelMap(dynamic_cast<mitk::Image *>(maskImage.GetPointer()), maskImage, 0);
      }

      for (const mitk::DataNode::Pointer &msiDataNode : m_MsiDataNodes)
      {
        std::string nodeName = msiDataNode->GetName();
        const m2::SpectrumImageBase::Pointer msiImage = dynamic_cast<m2::SpectrumImageBase *>(msiDataNode->GetData());
        const mitk::Image::Pointer indexImage = msiImage->GetIndexImage();

        ExportMaskImage(dynamic_cast<mitk::Image *>(maskNode->GetData()), msiImage);

        unsigned int layerCounter = 0;

        // iterate over each layer and export the spectra for the different labels as csv
        for (const auto &layer : m_LayerLabelMap)
        {
          std::vector<unsigned int> indexValues;
          unsigned int labelCounter = 1;
          for (const auto &labelWordlCoordPair : layer.second)
          {
            AccessFixedDimensionByItk(indexImage,
                                      ([&, this](auto input) {
                                        for (const auto &vectorWoldCord : labelWordlCoordPair.second)
                                        {
                                          mitk::Vector3D vectorIndex;
                                          dynamic_cast<mitk::Image *>(msiDataNode->GetData())
                                            ->GetGeometry()
                                            ->WorldToIndex(vectorWoldCord, vectorIndex);

                                          indexValues.push_back(input->GetPixel({(long int) vectorIndex[0], (long int)vectorIndex[1], (long int)vectorIndex[2]}));
                                        }
                                      }),
                                      3);
            std::string intensitiesFileName = m_FilePath + "_" + nodeName + "_Mask-" + maskNode->GetName() + "_Layer-" +
                                              std::to_string(layerCounter) + "_Label-" + std::to_string(labelCounter) +
                                              ".csv";

            std::string massAxisFileName = m_FilePath + "_" + nodeName + "_Mass-axis.csv";

            std::vector<unsigned int> validMzIndices;

            GetValidMzIndices(msiImage, &validMzIndices);
            if (m_ExportFullSpectra)
            {
              WriteSpectraToCsv(msiImage, indexValues, validMzIndices, intensitiesFileName, massAxisFileName);
            }
            else
            {
              WriteSpectraPeaksToCsv(msiImage, indexValues, intensitiesFileName, massAxisFileName);
            }
            ++labelCounter;
          }
          ++layerCounter;
        }
      }
    }
    m_LayerLabelMap.clear();
  }
}

void m2::MultiModalMaskExportHelper::ExportMaskImage(mitk::Image::Pointer mask, m2::SpectrumImageBase::Pointer msImage)
{
  std::vector<unsigned int> maskValues;
  auto maskI = msImage->GetMaskImage();
  if (m_ExportBackground)
  {
    std::vector<itk::Index<3>> validIndices;
    auto lambda = [&](std::vector<itk::Index<3>> * vec, itk::Index<3> index, auto /*input*/) { vec->push_back(index); };

    IterateImageApplyLambda<itk::Index<3>>(lambda, maskI, &validIndices);
    AccessFixedDimensionByItk(mask,
                              ([&, this](auto input) {
                                for (const auto &index : validIndices)
                                {
                                  maskValues.push_back(input->GetPixel(index));
                                }
                              }),
                              3)
  }
  else
  {
    auto lambda = [&](std::vector<unsigned int> *vec, itk::Index<3> index, auto input) {
      vec->push_back(input->GetPixel(index));
    };
    IterateImageApplyLambda<unsigned int>(lambda, mask, &maskValues);
  }

  std::ofstream massAxisOutput(m_FilePath + "_" + "Mask.csv");

  auto mzIt = maskValues.begin();
  std::string outputString = std::to_string(*mzIt);
  massAxisOutput.write(outputString.c_str(), outputString.size());

  ++mzIt;
  while (mzIt != maskValues.end())
  {
    std::string outputString = "," + std::to_string(*mzIt);
    massAxisOutput.write(outputString.c_str(), outputString.size());
    ++mzIt;
  }
  massAxisOutput.close();
}
