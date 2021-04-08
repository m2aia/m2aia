#include <m2MultiModalMaskExportHelper.h>

#include <algorithm>
#include <eigen3/Eigen/Dense>
#include <iterator>

#include <m2ImzMLMassSpecImage.h>
#include <m2PeakDetection.hpp>

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

void m2::MultiModalMaskExportHelper::SetExportOption(bool isExportFullSpectra)
{
  m_ExportFullSpectra = isExportFullSpectra;
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

void WriteSpectraPeaksToCsv(m2::MSImageBase::Pointer msImage,
                       std::vector<unsigned int> indexValues,
                       std::string intensitiesFileName,
                       std::string massAxisFileName)
{
  std::ofstream output(intensitiesFileName);

  auto massIndicesMask = msImage->GetPeaks();

  for (auto imzlIndex : indexValues)
  {
    std::vector<double> intensities, mzs, intsMasked;
    msImage->GrabSpectrum(imzlIndex, mzs, intensities);

    for (auto p : massIndicesMask)
    {
      auto i = p.massAxisIndex;
      double val = 0;
      if (msImage->GetMassPickingTolerance() == 0)
      {
        val = intensities[i];
      }
      else
      {
        auto tol = msImage->GetMassPickingTolerance() * 10e-6 * mzs[i];
        auto subRes = m2::Peaks::Subrange(mzs, mzs[i] - tol, mzs[i] + tol);
        auto s = std::next(std::begin(intensities), subRes.first);
        auto e = std::next(s, subRes.second);
        switch (msImage->GetIonImageGrabStrategy())
        {
          case m2::IonImageGrabStrategyType::None:
            break;
          case m2::IonImageGrabStrategyType::Sum:
            val = std::accumulate(s, e, double(0));
            break;
          case m2::IonImageGrabStrategyType::Mean:
            val = std::accumulate(s, e, double(0)) / double(std::distance(s, e));
            break;
          case m2::IonImageGrabStrategyType::Maximum:
            val = *std::max_element(s, e);
            break;
          case m2::IonImageGrabStrategyType::Median:
          {
            const unsigned int _N = std::distance(s, e);
            double median = 0;
            if ((_N % 2) == 0)
            {
              std::nth_element(s, s + _N * 0.5, e);
              std::nth_element(s, s + _N * 0.5 + 1, e);
              median = 0.5 * (*std::next(s, _N * 0.5) + *std::next(s, _N * 0.5 + 1));
            }
            else
            {
              std::nth_element(s, s + ((_N + 1) * 0.5), e);
              median = 0.5 * (*std::next(s, _N * 0.5) + *std::next(s, _N * 0.5 + 1));
            }
            val = median;
            break;
          }
        }
      }
      intsMasked.push_back(val);
    }

    auto intensitiesIt = intsMasked.begin();

    std::string outputString = std::to_string(*intensitiesIt);
    output.write(outputString.c_str(), outputString.size());
    ++intensitiesIt;

    while (intensitiesIt != intsMasked.end())
    {
      std::string outputString = "," + std::to_string(*intensitiesIt);
      output.write(outputString.c_str(), outputString.size());

      ++intensitiesIt;
    }

    output << '\n';
  }
  output.close();

  std::ofstream mzOutput(massAxisFileName);

  auto mzIt = massIndicesMask.begin();

  std::string mzOutputStr = std::to_string(mzIt->mass);
  mzOutput.write(mzOutputStr.c_str(), mzOutputStr.size());

  ++mzIt;
  while (mzIt != massIndicesMask.end())
  {
      std::string outputStr = "," + std::to_string(mzIt->mass);
      mzOutput.write(outputStr.c_str(), outputStr.size());
      ++mzIt;
  }
  mzOutput.close();
}

/*Get the indices of the mz values with respect to the mz bounds*/
void m2::MultiModalMaskExportHelper::GetValidMzIndices(m2::MSImageBase::Pointer msImage,
                                                       std::vector<unsigned int> *validIndices)
{
  int counter = 0;

  if (m_UpperMzBound != m_LowerMzBound)
  {
    for (auto mz : msImage->MassAxis())
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
    validIndices->resize(msImage->MassAxis().size());
    for (auto it = msImage->MassAxis().begin(); it < msImage->MassAxis().end(); ++it)
    {
      validIndices->push_back(counter);
      ++counter;
    }
  }
}

void m2::MultiModalMaskExportHelper::WriteSpectraToCsv(m2::MSImageBase::Pointer msImage,
                                                       std::vector<unsigned int> indexValues,
                                                       std::vector<unsigned int> validMzIndices,
                                                       std::string intensitiesFileName,
                                                       std::string massAxisFileName)
{
  std::ofstream output(intensitiesFileName);

  std::vector<double> intensities;

  for (auto imzlIndex : indexValues)
  {
    msImage->GrabIntensity(imzlIndex, intensities);

    auto indicesIt = validMzIndices.begin();

    std::string outputString = std::to_string(intensities.at(*indicesIt));
    output.write(outputString.c_str(), outputString.size());
    ++indicesIt;

    while (indicesIt != validMzIndices.end())
    {
      std::string outputString = "," + std::to_string(intensities.at(*indicesIt));
      output.write(outputString.c_str(), outputString.size());

      ++indicesIt;
    }
    output.write(std::string("\n").c_str(), 1);
  }

  output.close();

  std::ofstream massAxisOutput(massAxisFileName);
  std::vector<double> massAxis = msImage->MassAxis();

  auto mzIt = validMzIndices.begin();
  std::string outputString = std::to_string(massAxis.at(*mzIt));
  massAxisOutput.write(outputString.c_str(), outputString.size());

  ++mzIt;
  while (mzIt != validMzIndices.end())
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
          unsigned int labelCounter = 1;
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
