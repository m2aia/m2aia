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
#include <mitkImage.h>
#include <mitkItkImageIO.h>

#include <m2ImzMLSpectrumImage.h>


namespace m2
{
  /**
   * Writes/Reads ImzML images
   * @ingroup Process
   */
  // The export macro should be removed. Currently, the unit
  // tests directly instantiate this class.

  class M2AIACOREIO_EXPORT ImzMLImageIO : public mitk::AbstractFileIO
  {
  public:
    // typedef mitk::MassSpectrometryImage InputType;
    ImzMLImageIO();

    std::string IMZML_MIMETYPE_NAME()
    {
      static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".image.imzML";
      return name;
    }

    mitk::CustomMimeType IMZML_MIMETYPE()
    {
      mitk::CustomMimeType mimeType(IMZML_MIMETYPE_NAME());
      mimeType.AddExtension("imzML");
      mimeType.SetCategory("Images");
      mimeType.SetComment("imzML");
      return mimeType;
    }

    // -------------- AbstractFileReader -------------

    /**
     * @brief Reads a number of m2::ImzMLSpectrumImage from the file system
     * @return a vector of m2::ImzMLSpectrumImage
     */
    std::vector<mitk::BaseData::Pointer> DoRead() override;
    ConfidenceLevel GetReaderConfidenceLevel() const override;

    // -------------- AbstractFileWriter -------------
    /**
     * @brief Writes a m2::ImzMLSpectrumImage  as its *.imzML and *.ibd file to disk
     *
     */
    void Write() override;

    ConfidenceLevel GetWriterConfidenceLevel() const override;
    std::string GetIBDOutputPath() const;
    std::string GetImzMLOutputPath() const;
    void WriteContinuousProfile(m2::ImzMLSpectrumImage::SourceListType &sourceList) const;
    void WriteContinuousCentroid(m2::ImzMLSpectrumImage::SourceListType &sourceList) const;
    void WriteProcessedProfile(m2::ImzMLSpectrumImage::SourceListType &sourceList) const;
    void WriteProcessedCentroid(m2::ImzMLSpectrumImage::SourceListType &sourceList) const;

  private:
    void EvaluateSpectrumFormatType(m2::SpectrumImageBase *);
    void LoadAssociatedData(m2::ImzMLSpectrumImage *);
    ImzMLImageIO *IOClone() const override;
  };
} // namespace m2


