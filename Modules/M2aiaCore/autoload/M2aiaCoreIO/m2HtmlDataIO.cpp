/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <m2HtmlDataIO.h>
#include <mitkIOUtil.h>
#include <itksys/SystemTools.hxx>
#include <fstream>
#include <sstream>

namespace m2
{
  HtmlDataIO::HtmlDataIO() 
    : AbstractFileIO(HtmlData::GetStaticNameOfClass(), HTML_MIMETYPE(), "HTML Document (M²aia)")
  {
    AbstractFileWriter::SetRanking(10);
    AbstractFileReader::SetRanking(10);
    this->RegisterService();
  }

  mitk::IFileIO::ConfidenceLevel HtmlDataIO::GetWriterConfidenceLevel() const
  {
    if (dynamic_cast<const m2::HtmlData *>(this->GetInput()))
      return Supported;
    else
      return Unsupported;
  }

  void HtmlDataIO::Write()
  {
    ValidateOutputLocation();

    const auto *htmlData = dynamic_cast<const m2::HtmlData *>(this->GetInput());
    
    if (!htmlData)
    {
      MITK_ERROR << "Input is not an HtmlData object";
      return;
    }

    std::string outputPath = GetOutputLocation();
    
    // Use HtmlData's built-in save method
    if (!const_cast<m2::HtmlData*>(htmlData)->SaveToFile(outputPath))
    {
      MITK_ERROR << "Failed to write HTML file: " << outputPath;
    }
  }

  std::vector<itk::SmartPointer<mitk::BaseData>> HtmlDataIO::DoRead()
  {
    std::vector<itk::SmartPointer<mitk::BaseData>> result;

    std::string inputPath = GetInputLocation();
    
    // Check if file exists
    if (!itksys::SystemTools::FileExists(inputPath))
    {
      MITK_ERROR << "File does not exist: " << inputPath;
      return result;
    }

    // Read the HTML file
    std::ifstream file(inputPath);
    if (!file.is_open())
    {
      MITK_ERROR << "Failed to open HTML file: " << inputPath;
      return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string htmlContent = buffer.str();

    // Create HtmlData object
    auto htmlData = HtmlData::New();
    htmlData->SetHtmlContent(htmlContent);

    // Set the title from filename
    std::string filename = itksys::SystemTools::GetFilenameName(inputPath);
    std::string filenameWithoutExt = itksys::SystemTools::GetFilenameWithoutLastExtension(filename);
    htmlData->SetTitle(filenameWithoutExt);

    result.push_back(htmlData.GetPointer());

    return result;
  }

  HtmlDataIO *HtmlDataIO::IOClone() const
  {
    return new HtmlDataIO(*this);
  }
}
