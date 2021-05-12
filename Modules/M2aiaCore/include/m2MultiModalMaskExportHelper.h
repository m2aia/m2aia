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

#include <MitkM2aiaCoreExports.h>

#include <itkIndex.h>
#include <m2MSImageBase.h>
#include <mitkDataNode.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>
#include <algorithm>
#include <eigen3/Eigen/Dense>
#include <iterator>
#include <itkMacro.h>
#include <m2ImzMLMassSpecImage.h>
#include <m2PeakDetection.hpp>

#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
namespace m2
{
  class MITKM2AIACORE_EXPORT MultiModalMaskExportHelper : public itk::LightObject
  {
  public:
    mitkClassMacroItkParent(MultiModalMaskExportHelper, itk::LightObject);

    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    void AddMaskNode(mitk::DataNode::Pointer);
    void Export();
    void AddMsiDataNode(mitk::DataNode::Pointer);
    void SetFilePath(std::string);
    void SetLowerMzBound(double);
    void SetUpperMzBound(double);
    void SetExportOption(bool, bool);

  protected:
    using ImageType = itk::Image<double, 3>;

    MultiModalMaskExportHelper() = default;
    std::vector<mitk::DataNode::Pointer> m_MaskImages;
    std::vector<itk::Index<3>> m_MaskIndices;
    std::vector<mitk::DataNode::Pointer> m_MsiDataNodes;
    std::string m_FilePath;
    // key: layer, value: labels with their respecitve indices
    std::map<unsigned int, std::map<unsigned int, std::vector<mitk::Vector3D>>> m_LayerLabelMap;

    bool m_ExportFullSpectra = true;
    bool m_ExportBackground = false;

    double m_LowerMzBound;
    double m_UpperMzBound;

    void GetValidMzIndices(m2::MSImageBase::Pointer, std::vector<unsigned int> *);
    void InitalizeLayerLabelMap(mitk::Image::Pointer, mitk::LabelSetImage::Pointer, unsigned int);
    void WriteSpectraToCsv(
      m2::MSImageBase::Pointer, std::vector<unsigned int>, std::vector<unsigned int>, std::string, std::string);
    void ExportMaskImage(mitk::Image::Pointer, m2::MSImageBase::Pointer);

    template <typename T, typename F>
    void IterateImageApplyLambda(F &&lambda, mitk::Image::Pointer mask, std::vector<T> * vector)
    {
        AccessFixedDimensionByItk(mask,
                                ([&, this](auto input) {
                                  for (unsigned int x = 0; x < input->GetLargestPossibleRegion().GetSize(0); x++)
                                  {
                                    for (unsigned int y = 0; y < input->GetLargestPossibleRegion().GetSize(1); y++)
                                    {
                                      for (unsigned int z = 0; z < input->GetLargestPossibleRegion().GetSize(2); z++)
                                      {
                                        if (input->GetPixel({x, y, z}) > 0)
                                        {
                                          lambda(vector, {x, y, z}, input);
                                          // maskValues.push_back(input->GetPixel({x, y, z}));
                                        }
                                      }
                                    }
                                  }
                                }),
                                3);
    }
  };
} // namespace m2
