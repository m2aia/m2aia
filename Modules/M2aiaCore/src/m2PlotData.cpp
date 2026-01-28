/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <m2PlotData.h>
#include <algorithm>

namespace m2
{
  PlotData::PlotData()
    : m_Description("Plot Data"),
      m_XAxisColumn(""),
      m_YAxisColumn("")
  {
  }

  void PlotData::AddColumn(const std::string& columnName, const ValueVector& data)
  {
    m_Data[columnName] = data;
  }

  PlotData::ValueVector PlotData::GetColumn(const std::string& columnName) const
  {
    auto it = m_Data.find(columnName);
    if (it != m_Data.end())
    {
      return it->second;
    }
    return ValueVector();
  }

  bool PlotData::HasColumn(const std::string& columnName) const
  {
    return m_Data.find(columnName) != m_Data.end();
  }

  std::vector<std::string> PlotData::GetColumnNames() const
  {
    std::vector<std::string> names;
    names.reserve(m_Data.size());
    for (const auto& pair : m_Data)
    {
      names.push_back(pair.first);
    }
    return names;
  }

  size_t PlotData::GetNumberOfRows() const
  {
    if (m_Data.empty())
      return 0;
    return m_Data.begin()->second.size();
  }

  size_t PlotData::GetNumberOfColumns() const
  {
    return m_Data.size();
  }

  void PlotData::Clear()
  {
    m_Data.clear();
    m_StringData.clear();
    m_Description.clear();
    m_XAxisColumn.clear();
    m_YAxisColumn.clear();
  }

  bool PlotData::IsEmpty() const
  {
    return m_Data.empty() && m_StringData.empty();
  }

  void PlotData::SetDescription(const std::string& description)
  {
    m_Description = description;
    this->SetProperty("m2aia.plotdata.description", mitk::StringProperty::New(description));
  }

  std::string PlotData::GetDescription() const
  {
    return m_Description;
  }

  void PlotData::SetXAxisColumn(const std::string& columnName)
  {
    m_XAxisColumn = columnName;
    this->SetProperty("m2aia.plotdata.xaxis", mitk::StringProperty::New(columnName));
  }

  void PlotData::SetYAxisColumn(const std::string& columnName)
  {
    m_YAxisColumn = columnName;
    this->SetProperty("m2aia.plotdata.yaxis", mitk::StringProperty::New(columnName));
  }

  void PlotData::AddStringColumn(const std::string& columnName, const StringVector& data)
  {
    m_StringData[columnName] = data;
  }

  PlotData::StringVector PlotData::GetStringColumn(const std::string& columnName) const
  {
    auto it = m_StringData.find(columnName);
    if (it != m_StringData.end())
    {
      return it->second;
    }
    return StringVector();
  }

  bool PlotData::HasStringColumn(const std::string& columnName) const
  {
    return m_StringData.find(columnName) != m_StringData.end();
  }

  std::vector<std::string> PlotData::GetStringColumnNames() const
  {
    std::vector<std::string> names;
    names.reserve(m_StringData.size());
    for (const auto& pair : m_StringData)
    {
      names.push_back(pair.first);
    }
    return names;
  }

} // namespace m2
