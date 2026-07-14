/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <M2aiaCoreIOExports.h>
#include <mitkAbstractFileIO.h>
#include <mitkIOMimeTypes.h>
#include <m2IntervalVector.h>

namespace m2
{
  /**
   * @brief Reader/Writer for SEF (Spectral Evidence Format) annotation files (.sef).
   *
   * SEF files are JSON documents with the following structure:
   * @code
   * {
   *   "version": "2",
   *   "peaklist": {
   *     "metaInformation": { "numberOfIntervals": <N> },
   *     "intervals": [
   *       {
   *         "lower":         <double>,
   *         "upper":         <double>,
   *         "name":          <string>,
   *         "color":         <string>,          // hex, e.g. "#eee685"
   *         "mobilityLower": <double>,          // optional
   *         "mobilityUpper": <double>           // optional
   *       }, ...
   *     ]
   *   }
   * }
   * @endcode
   *
   * Each file is read as a single m2::IntervalVector. Interval bounds are stored
   * via m2::Interval::FromBounds(), so x.min()/x.max() return the m/z lower/upper
   * bounds, x.mean() returns the centre, and the extended fields (color,
   * mobilityLower, mobilityUpper, description) carry the per-interval metadata.
   */
  class M2AIACOREIO_EXPORT SefFileIO : public mitk::AbstractFileIO
  {
  public:
    SefFileIO();

    std::string SEF_MIMETYPE_NAME()
    {
      static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".sef.annotation";
      return name;
    }

    mitk::CustomMimeType SEF_MIMETYPE()
    {
      mitk::CustomMimeType mimeType(SEF_MIMETYPE_NAME());
      mimeType.AddExtension("sef");
      mimeType.AddExtension("centroids");
      mimeType.SetCategory("Feature Lists");
      mimeType.SetComment("SEF interval annotation list");
      return mimeType;
    }

    std::vector<mitk::BaseData::Pointer> DoRead() override;
    ConfidenceLevel GetReaderConfidenceLevel() const override;

    void Write() override;
    ConfidenceLevel GetWriterConfidenceLevel() const override;

  private:
    SefFileIO *IOClone() const override;
  };

} // namespace m2
