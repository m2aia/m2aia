#include <m2MultiModalMaskExportHelper.h>

#include <algorithm>
#include <eigen3/Eigen/Dense>
#include <iterator>
#include <m2ImzMLMassSpecImage.h>
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

void m2::MultiModalMaskExportHelper::WriteSpectraToCsv(m2::MSImageBase::Pointer msImage,
                                                       std::vector<unsigned int> indexValues,
                                                       std::string intensitiesFileName,
                                                       std::string massAxisFileName)
{
  std::ofstream output(intensitiesFileName);
  std::ostringstream oss;

  std::vector<int> validVectorIndices;
  int counter = 0;

  if (m_UpperMzBound != m_LowerMzBound)
  {
    for (auto mz : msImage->MassAxis())
    {
      if (mz <= m_UpperMzBound && mz >= m_LowerMzBound)
      {
        validVectorIndices.push_back(counter);
      }
      ++counter;
    }
  }
  else
  {
    validVectorIndices.resize(msImage->MassAxis().size());
    int i = 0;

    for (auto it = msImage->MassAxis().begin(); it < msImage->MassAxis().end(); ++it)
    {
      validVectorIndices.push_back(i);
      ++i;
    }
  }

  std::vector<double> intensities;

  for (auto imzlIndex : indexValues)
  {
    msImage->GrabIntensity(imzlIndex, intensities);

    auto indicesIt = validVectorIndices.begin();

    std::string outputString = std::to_string(intensities.at(*indicesIt));
    output.write(outputString.c_str(), outputString.size());
    ++indicesIt;

    while (indicesIt != validVectorIndices.end())
    {
      std::string outputString = "," + std::to_string(intensities.at(*indicesIt));
      output.write(outputString.c_str(), outputString.size());

      ++indicesIt;
    }
    output.write(std::string(" \n").c_str(), 2);
  }

  output.close();

  std::ofstream massAxisOutput(massAxisFileName);
  std::vector<double> massAxis = msImage->MassAxis();

  auto mzIt = validVectorIndices.begin();
  std::string outputString = std::to_string(massAxis.at(*mzIt));
  massAxisOutput.write(outputString.c_str(), outputString.size());

  ++mzIt;

  while (mzIt != validVectorIndices.end())
  {
    std::string outputString = "," + std::to_string(massAxis.at(*mzIt));
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
        const m2::MSImageBase::Pointer msiImage = dynamic_cast<m2::MSImageBase *>(msiDataNode->GetData());
        const mitk::Image::Pointer indexImage = msiImage->GetIndexImage();

        unsigned int layerCounter = 0;

        // iterate over each layer and export the spectra for the different labels as csv
        for (const auto &layer : m_LayerLabelMap)
        {
          std::vector<unsigned int> indexValues;
          unsigned int labelCounter = 0;
          for (const auto &labelWordlCoordPair : layer.second)
          {
            AccessFixedDimensionByItk(indexImage,
                                      ([&, this](auto input) {
                                        for (const auto &vectorWoldCord : labelWordlCoordPair.second)
                                        {
                                          itk::Index<3> vectorIndex;
                                          dynamic_cast<mitk::Image *>(msiDataNode->GetData())
                                            ->GetGeometry()
                                            ->WorldToIndex(vectorWoldCord, vectorIndex);
                                          indexValues.push_back(input->GetPixel(vectorIndex));
                                        }
                                      }),
                                      3);
            std::string intensitiesFileName = m_FilePath + "_" + nodeName + "_Mask-" + maskNode->GetName() + "_Layer-" +
                                              std::to_string(layerCounter) + "_Label-" + std::to_string(labelCounter) +
                                              ".csv";

            std::string massAxisFileName = m_FilePath + "_" + nodeName + "_Mass-axis.csv";

            WriteSpectraToCsv(msiImage, indexValues, intensitiesFileName, massAxisFileName);
            ++labelCounter;
          }
          ++layerCounter;
        }
      }
    }
    m_LayerLabelMap.clear();
  }
}
