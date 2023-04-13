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
#include <mitkDataStorage.h>

namespace m2
{
  class M2AIACOREIO_EXPORT MicroscopyTiffImageIO : public mitk::AbstractFileIO
  {
  public:
    MicroscopyTiffImageIO();

    std::string MICROSCOPYTIFF_MIMETYPE_NAME()
    {
      static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".image.microscopy";
      return name;
    }

    mitk::CustomMimeType MICROSCOPYTIFF_MIMETYPE()
    {
      mitk::CustomMimeType mimeType(MICROSCOPYTIFF_MIMETYPE_NAME());
      mimeType.AddExtension("tif");	  
      mimeType.AddExtension("tiff");
      mimeType.SetCategory("MicroscopyTiff");
      mimeType.SetComment("MicroscopyTiff");
      return mimeType;
    }

    std::vector<mitk::BaseData::Pointer> DoRead() override;

    using mitk::AbstractFileReader::Read;
    mitk::DataStorage::SetOfObjects::Pointer Read(mitk::DataStorage &ds) override;

    ConfidenceLevel GetReaderConfidenceLevel() const override;
    void Write() override;

    ConfidenceLevel GetWriterConfidenceLevel() const override;

  private:
    // MicroscopyTiffImageIO(const MicroscopyTiffImageIO &other);
    MicroscopyTiffImageIO *IOClone() const override;
    const std::string A_IMPORT_VOLUME = "a. Import as volume";
    const std::string B_SPLIT_CHANNELS = "b. Split channels";
    const std::string C_REMOVE_DUPLICATES = "c. No duplicates on split";
    const std::string D_THICKNESS = "d. Slice thickness (µm)";
    const std::string E_USE_USER_DEFS = "e. User pixel size";
    const std::string F_PIXEL_SIZE_X = "f. Pixel size x (µm)";
    const std::string G_PIXEL_SIZE_Y = "g. Pixel size y (µm)";
  };
} // namespace m2
