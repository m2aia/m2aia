/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <m2HtmlData.h>
#include <mitkStringProperty.h>
#include <fstream>
#include <sstream>

namespace m2
{
  HtmlData::HtmlData()
    : m_Title("HTML Visualization")
  {
  }

  void HtmlData::SetHtmlContent(const std::string& html)
  {
    m_HtmlContent = html;
    this->Modified();
  }

  std::string HtmlData::GetHtmlContent() const
  {
    return m_HtmlContent;
  }

  void HtmlData::SetTitle(const std::string& title)
  {
    m_Title = title;
    this->SetProperty("m2aia.htmldata.title", mitk::StringProperty::New(title));
  }

  std::string HtmlData::GetTitle() const
  {
    return m_Title;
  }

  bool HtmlData::IsEmpty() const
  {
    return m_HtmlContent.empty();
  }

  void HtmlData::Clear()
  {
    m_HtmlContent.clear();
    m_Title.clear();
  }

  bool HtmlData::LoadFromFile(const std::string& filePath)
  {
    std::ifstream file(filePath);
    if (!file.is_open())
      return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_HtmlContent = buffer.str();
    
    this->Modified();
    return true;
  }

  bool HtmlData::SaveToFile(const std::string& filePath) const
  {
    std::ofstream file(filePath);
    if (!file.is_open())
      return false;

    file << m_HtmlContent;
    return true;
  }

} // namespace m2
