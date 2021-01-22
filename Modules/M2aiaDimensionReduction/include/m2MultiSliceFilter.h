/*===================================================================

The Mass Spectrometry Imaging Interaction Toolkit (MSITK)

Copyright (c) Jonas Cordes, Hochschule Mannheim.
Division of Medical Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once

#include <M2aiaCoreExports.h>
#include <m2MassSpecVisualizationFilter.h>
#include <mitkImage.h>
#include <mitkImageToImageFilter.h>

namespace m2
{
  class M2AIACORE_EXPORT MultiSliceFilter : public MassSpecVisualizationFilter
  {
    using ColorType = float;
    using OutputColorType = unsigned char;
    using RGBPixel = itk::RGBPixel<ColorType>;

  public:
    mitkClassMacro(MultiSliceFilter, MassSpecVisualizationFilter) itkFactorylessNewMacro(Self) itkCloneMacro(Self)
      itkGetMacro(IndexImage, mitk::Image::Pointer) void SetColorVector(std::vector<std::array<ColorType, 4>>);

    enum VisualizationOptions
    {
      MAXIMUM,
      MIXCOLOR
    };

    void SetScaleOptions(std::vector<std::array<float, 2>>);
    void SetVisualizationOption(VisualizationOptions option);


  protected:
    using RGBAPixel = itk::RGBAPixel<unsigned char>;
    MultiSliceFilter() = default;
    //    ~m2PcaImageFilter() override;
    void GenerateData() override;
    std::array<ColorType, 4> MapScalarToSpecificRGB(float scalar, int position);

  private:
    std::vector<std::array<ColorType, 4>> m_ColorVector;
    mitk::Image::Pointer m_IndexImage;
    std::vector<std::array<float, 2>> m_ScaleOptions;
    std::vector<std::array<float, 2>> m_MinMaxImageValues;
    std::vector<std::array<float, 2>> m_MinMaxScaleImageValues;

    ColorType MapIntensityValue(ColorType maxValue, float scalar, float min, float max);
    void initializeItkDoubleImage(itk::Image<double, 3>::Pointer);
    void initializeColorImage(itk::Image<RGBPixel, 3>::Pointer);

    VisualizationOptions m_VisualizationOption;
  };
} // namespace mitk
