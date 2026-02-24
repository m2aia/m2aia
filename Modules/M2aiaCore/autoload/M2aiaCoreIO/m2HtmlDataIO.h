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
#include <m2HtmlData.h>

namespace m2
{
  /**
   * @brief File IO for reading and writing HTML files as HtmlData nodes
   * 
   * This IO class handles loading HTML files into m2::HtmlData objects
   * for display in the M2aia plot view.
   * 
   * Supported file extensions: .html, .htm
   */
  class HtmlDataIO : public mitk::AbstractFileIO
  {
  public:
    HtmlDataIO();

    // AbstractFileReader
    using AbstractFileReader::Read;

    // AbstractFileWriter
    using AbstractFileWriter::Write;
    void Write() override;

    mitk::IFileIO::ConfidenceLevel GetWriterConfidenceLevel() const override;

  protected:
    std::vector<itk::SmartPointer<mitk::BaseData>> DoRead() override;

  private:
    HtmlDataIO *IOClone() const override;
  };

  /**
   * @brief MIME type for HTML files
   */
  class HtmlDataIOMimeType : public mitk::CustomMimeType
  {
  public:
    HtmlDataIOMimeType()
      : CustomMimeType("application/html")
    {
      this->AddExtension("html");
      this->AddExtension("htm");
      this->SetCategory("HTML Document");
      this->SetComment("HTML file for visualization");
    }

    bool AppliesTo(const std::string &path) const override
    {
      bool result = CustomMimeType::AppliesTo(path);
      return result;
    }

    HtmlDataIOMimeType *Clone() const override
    {
      return new HtmlDataIOMimeType(*this);
    }
  };

  inline mitk::CustomMimeType HTML_MIMETYPE()
  {
    return HtmlDataIOMimeType();
  }
}
