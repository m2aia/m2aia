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

#include <mitkImage.h>
#include <mitkLabelSetImage.h>
#include <itkIndex.h>
#include <m2MSImageBase.h>
#include <mitkDataNode.h>

namespace m2
{
  class MITKM2AIACORE_EXPORT MultiModalMaskExportHelper : public itk::LightObject
  {
  public:
    mitkClassMacroItkParent(MultiModalMaskExportHelper,  itk::LightObject);

    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);



    void AddMaskNode(mitk::DataNode::Pointer);
    void Export();
    void AddMsiDataNode(mitk::DataNode::Pointer);
    void SetFilePath(std::string);
    void SetLowerMzBound(double);
    void SetUpperMzBound(double);

  protected:
    using ImageType = itk::Image<double, 3>;

    MultiModalMaskExportHelper() = default;
    std::vector<mitk::DataNode::Pointer> m_MaskImages;
    std::vector<itk::Index<3>> m_MaskIndices;
    std::vector<mitk::DataNode::Pointer> m_MsiDataNodes;
    std::string m_FilePath;
    std::map<unsigned int, std::map<unsigned int, std::vector<mitk::Vector3D>>> m_LayerLabelMap;
    double m_LowerMzBound;
    double m_UpperMzBound;

    void InitalizeLayerLabelMap(mitk::Image::Pointer, mitk::LabelSetImage::Pointer, unsigned int);
    void WriteSpectraToCsv(m2::MSImageBase::Pointer, std::vector<unsigned int>, std::string, std::string);
  };
} // namespace mitk
