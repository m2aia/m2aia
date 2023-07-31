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

#include <M2aiaCoreExports.h>
#include <itkMetaDataObject.h>

#include <mitkDockerHelper.h>

#include <m2CoreCommon.h>
#include <m2SpectrumImageBase.h>
#include <m2IntervalVector.h>

namespace m2
{

  class M2AIACORE_EXPORT SpectrumImageHelper
  {
  public:

    /**
     * @brief Add default arguments for a SpectrumImage to a docker 
     * floats.
    */
    static void AddArguments(mitk::DockerHelper & helper);
  
    /**
     * @brief Get the Intensity Data representing a matrix of shape [#intervals, #pixels] as a contiguous array of
     * floats.
     *
     * @param image: SpectrumImage
     * @param intervals: list of intervals (e.g. a peak center).
     * @return std::vector<float>
     */
    static std::vector<float> GetIntensityData(const m2::SpectrumImageBase * image, const std::vector<m2::Interval> &intervals);

    /**
     * @brief Get the shape of the assumed data matrix generated with GetIntensityData(...)
     *
     * @param image: SpectrumImage
     * @param intervals: list of intervals (e.g. a peak center). Same as used in GetIntensityData(...).
     * @return std::vector<unsigned long>
     */
    static std::vector<unsigned long> GetIntensityDataShape(const m2::SpectrumImageBase * image, const std::vector<m2::Interval> &intervals);

  };
}