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

#include <M2aiaDimensionReductionExports.h>
#include <array>
#include <itkRGBPixel.h>
#include <vector>
namespace m2
{
  namespace ColorMixer
  {
    template <class T>
    class M2AIADIMENSIONREDUCTION_EXPORT m2RGBColorMixer
    {
    public:
      using RGBPixel = T;
      static RGBPixel MixRGBColors(std::vector<RGBPixel> colors)
      {
        int numberOfColors = colors.size();
        double redColorSum = 0;
        double greenColorSum = 0;
        double blueColorSum = 0;

        for (auto const color : colors)
        {
          redColorSum += color.GetRed();
          greenColorSum += color.GetGreen();
          blueColorSum += color.GetBlue();
        }

        RGBPixel mixedColorPixel;
        mixedColorPixel.SetRed(redColorSum / double(numberOfColors));
        mixedColorPixel.SetGreen(greenColorSum / double(numberOfColors));
        mixedColorPixel.SetBlue(blueColorSum / double(numberOfColors));

        return mixedColorPixel;
      }

    protected:
    };
  } // namespace ColorMixer
} // namespace m2
