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
   * Writes/Reads spectrum artifacts (interval vector)
   * 
   * To read centroid data you can create a .csv file with at least two columns.
   * The first two columns have to be set to the centroid m/z value and the intensity.
   * Additional a column "label" ca be specified to split a single csv file in multiple interval vectors.
   * 
   * example.csv:
   * 
   * m/z, intensity, ..., label, ...
   * 400, 100, ..., 1, ...
   * 412, 100, ..., 1, ...
   * 444, 100, ..., 2, ...
   * 422, 100, ..., 2, ...
   * 
   */

  
  class M2AIACOREIO_EXPORT IntervalVectorIO : public mitk::AbstractFileIO
  {
  public:
    IntervalVectorIO();

    std::string INTERVALVECTOR_MIMETYPE_NAME()
    {
      static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".csv.spectrum";
      return name;
    }

    mitk::CustomMimeType INTERVALVECTOR_MIMETYPE()
    {
      mitk::CustomMimeType mimeType(INTERVALVECTOR_MIMETYPE_NAME());
      mimeType.AddExtension("csv");
      mimeType.AddExtension("centroids");
      mimeType.SetCategory("Centroids");
      mimeType.SetComment("Spectrum or list of centroids");
      return mimeType;
    }

    std::vector<mitk::BaseData::Pointer> DoRead() override;
    ConfidenceLevel GetReaderConfidenceLevel() const override;
    
    void Write() override;
    ConfidenceLevel GetWriterConfidenceLevel() const override;
    
  private:
    IntervalVectorIO *IOClone() const override;

  }; 

} // namespace m2


