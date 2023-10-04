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
#include <m2CoreCommon.h>


namespace m2
{
  /**
   * @brief SpectrumType holds information about the import/export spectrum type definition.
   * E.g. for an ImzML Image the values are determined during the ImzML xml parsing.
   * 
   */
  class M2AIACORE_EXPORT SpectrumInfo
  {
      public:
        /**
         * @brief Numeric storage/representation type of x values.
         */
        NumericType XAxisType = NumericType::Float;

        /**
         * @brief Unit of the x axis. This is used for plotting of spectra in the spectrum view.
         */
        std::string XAxisLabel = "No unit string defined for x values!";

        /**
         * @brief If true, spectrum (import/)export will use min/max limits.

         */
        bool UseLimits = false;

        /**
         * @brief The minimum limit used for (import/)export of spectral data. This is used e.g. to export subspectra.
         */
        double XLimMin = 0;

        /**
         * @brief The maximum limit used for (import/)export of spectral data. This is used e.g. to export subspectra.
         */
        double XLimMax = 0;
        
        /**
         * @brief Numeric storage/representation type of y values.
         */
        NumericType YAxisType = NumericType::Float;

        /**
         * @brief Unit of the x axis. This is used for plotting of spectra in the spectrum view.
         */
        std::string YAxisLabel = "Intensity";
        
        /**
         * @brief Spectrum profile format type. Combines [Continuous/Processed] and [Profile/Centroid].
         */
        SpectrumFormat Format = m2::SpectrumFormat::None;
  };

  /**
   * @brief Print a SpectrumType instance in a unified way.
   * 
   * @param type 
   * @return std::string 
   */
  inline std::string to_string(const SpectrumInfo & type ){
    return "SpectrumType:\n\tX axis type:" + m2::to_string(type.XAxisType) +
    "\n\tX label: " + type.XAxisLabel +
    "\n\tY axis type: " + m2::to_string(type.YAxisType) +
    "\n\tSpectrum format: " + m2::to_string(type.Format);
  }
}

