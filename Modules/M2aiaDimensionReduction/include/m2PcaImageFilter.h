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

#include <M2aiaDimensionReductionExports.h>
#include <eigen3/Eigen/Dense>
#include <itkBarrier.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2MassSpecVisualizationFilter.h>
#include <mitkImage.h>
#include <mitkImageToImageFilter.h>
#include <vector>

namespace m2
{
  class M2AIADIMENSIONREDUCTION_EXPORT PcaImageFilter : public m2::MassSpecVisualizationFilter
  {
  public:
    mitkClassMacro(PcaImageFilter, MassSpecVisualizationFilter);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    void initMatrix();
    Eigen::MatrixXf GetEigenImageMatrix();
    Eigen::VectorXf GetMeanImage();

  protected:
    Eigen::MatrixXf m_DataMatrix;
    Eigen::MatrixXf m_EigenImageMatrix;
    Eigen::VectorXf m_MeanImage;
    PcaImageFilter()
    {
      OutputImageType::Pointer output0 = static_cast<OutputImageType *>(this->MakeOutput(0).GetPointer());
      OutputImageType::Pointer output1 = static_cast<OutputImageType *>(this->MakeOutput(1).GetPointer());
      Superclass::SetNumberOfRequiredOutputs(2);
      Superclass::SetNthOutput(0, output0.GetPointer());
      Superclass::SetNthOutput(1, output1.GetPointer());
    };
    void GenerateData() override;

  private:
  };
} // namespace m2
