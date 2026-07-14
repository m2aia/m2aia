/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <m2TabularDataIO.h>
#include <mitkIOUtil.h>
#include <mitkLocaleSwitch.h>
#include <itksys/SystemTools.hxx>
#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace m2
{
  TabularDataIO::TabularDataIO() 
    : AbstractFileIO("TabularData", TABULAR_MIMETYPE(), "Tabular Data (M²aia)")
  {
    // Higher ranking than IntervalVectorIO so it gets priority
    AbstractFileWriter::SetRanking(20);
    AbstractFileReader::SetRanking(20);
    this->RegisterService();
  }

  TabularDataIO::TabularDataIO(const std::string &baseDataType)
    : AbstractFileIO(baseDataType, TABULAR_MIMETYPE(), "Tabular Data (M²aia)")
  {
    AbstractFileWriter::SetRanking(20);
    AbstractFileReader::SetRanking(20);
    this->RegisterService();
  }

  mitk::IFileIO::ConfidenceLevel TabularDataIO::GetWriterConfidenceLevel() const
  {
    // Check if input is PlotData or IntervalVector
    const auto *plotData = dynamic_cast<const m2::PlotData *>(this->GetInput());
    const auto *intervalVector = dynamic_cast<const m2::IntervalVector *>(this->GetInput());
    MITK_INFO << "TabularDataIO::GetWriterConfidenceLevel - PlotData: " << (plotData != nullptr) << ", IntervalVector: " << (intervalVector != nullptr);
    if (plotData || intervalVector)
      return Supported;
    else
      return Unsupported;
  }

  void TabularDataIO::Write()
  {
    mitk::LocaleSwitch localeSwitch("C");
    ValidateOutputLocation();

    std::ofstream file(GetOutputLocation());
    
    if (!file.is_open()) {
        MITK_ERROR << "Failed to open file for writing: " << GetOutputLocation();
        return;
    }

    // Check if input is PlotData
    if (const auto *plotData = dynamic_cast<const m2::PlotData *>(this->GetInput()))
    {
      // Write PlotData
      auto numericColumns = plotData->GetColumnNames();
      auto stringColumns = plotData->GetStringColumnNames();
      
      if (numericColumns.empty() && stringColumns.empty())
      {
        MITK_WARN << "PlotData has no columns to write";
        return;
      }

      // Combine column names (string columns first, then numeric)
      std::vector<std::string> allColumns;
      allColumns.insert(allColumns.end(), stringColumns.begin(), stringColumns.end());
      allColumns.insert(allColumns.end(), numericColumns.begin(), numericColumns.end());

      // Write header
      file << boost::algorithm::join(allColumns, ",") << "\n";

      // Write data rows
      size_t numRows = plotData->GetNumberOfRows();
      if (numRows == 0 && !stringColumns.empty())
      {
        // Get row count from string columns if numeric columns are empty
        auto firstStringCol = plotData->GetStringColumn(stringColumns[0]);
        numRows = firstStringCol.size();
      }

      for (size_t row = 0; row < numRows; ++row)
      {
        std::vector<std::string> rowData;
        
        // Add string column values
        for (const auto& colName : stringColumns)
        {
          auto column = plotData->GetStringColumn(colName);
          if (row < column.size())
          {
            rowData.push_back(column[row]);
          }
          else
          {
            rowData.push_back("");
          }
        }
        
        // Add numeric column values
        for (const auto& colName : numericColumns)
        {
          auto column = plotData->GetColumn(colName);
          if (row < column.size())
          {
            rowData.push_back(std::to_string(column[row]));
          }
          else
          {
            rowData.push_back("");
          }
        }
        
        file << boost::algorithm::join(rowData, ",") << "\n";
      }
    }
    // Check if input is IntervalVector (fallback to existing implementation)
    else if (const auto *intervalVector = dynamic_cast<const m2::IntervalVector *>(this->GetInput()))
    {
      file << "center,max,min,mean\n";
      const auto intervals = intervalVector->GetIntervals();
      for(size_t i = 0 ; i < intervals.size(); ++i){
        if(i==0)
          file << intervals[i].x.max()+0.005 << "," << intervals[i].y.max() << "," << intervals[i].y.min() << "," << intervals[i].y.mean() << "\n";
        else if(i == intervals.size() -1)
          file << intervals[i].x.min()-0.005 << "," << intervals[i].y.max() << "," << intervals[i].y.min() << "," << intervals[i].y.mean() << "\n";
        else
          file << intervals[i].x.mean() << "," << intervals[i].y.max() << "," << intervals[i].y.min() << "," << intervals[i].y.mean() << "\n";
      }
    }

    file.close();
  }

  mitk::IFileIO::ConfidenceLevel TabularDataIO::GetReaderConfidenceLevel() const
  {
    if (AbstractFileIO::GetReaderConfidenceLevel() == Unsupported)
      return Unsupported;
    return Supported;
  }

  std::vector<mitk::BaseData::Pointer> TabularDataIO::DoRead()
  {  
    auto path = this->GetInputLocation();
    
    MITK_INFO << "Reading tabular data from: " << path;

    // Detect delimiter
    char delimiter = DetectDelimiter(path);
    MITK_INFO << "Detected delimiter: '" << delimiter << "'";

    // Parse file
    TableData data = ParseFile(path, delimiter);
    
    if (data.headers.empty() || data.rows.empty())
    {
      MITK_ERROR << "File is empty or invalid: " << path;
      return {};
    }

    MITK_INFO << "Parsed " << data.rows.size() << " rows with " << data.headers.size() << " columns";
    MITK_INFO << "Columns: " << boost::algorithm::join(data.headers, ", ");

    // Get filename for naming nodes
    std::string filename = itksys::SystemTools::GetFilenameName(path);

    // Determine data type and create appropriate nodes
    if (IsCentroidData(data))
    {
      MITK_INFO << "Detected centroid data";
      return CreateCentroidLists(data, filename);
    }
    else
    {
      MITK_INFO << "Detected general plot data";
      return CreatePlotData(data, filename);
    }
  }

  TabularDataIO::TableData TabularDataIO::ParseFile(const std::string& path, char delimiter)
  {
    TableData result;
    std::ifstream file(path);

    if (!file.is_open()) {
        MITK_ERROR << "Failed to open file: " << path;
        return result;
    }

    std::string line;
    
    // Read header
    if(std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, delimiter)) {
            result.headers.push_back(Strip(cell));
        }
    }

    // Read data rows
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;
        
        while (std::getline(ss, cell, delimiter)) {
            row.push_back(Strip(cell));
        }
        
        if (!row.empty())
        {
          result.rows.push_back(row);
        }
    }

    file.close();
    return result;
  }

  char TabularDataIO::DetectDelimiter(const std::string& path)
  {
    std::ifstream file(path);
    if (!file.is_open()) {
        return ',';  // default
    }

    std::string firstLine;
    if (std::getline(file, firstLine))
    {
      // Count occurrences of common delimiters
      int commaCount = std::count(firstLine.begin(), firstLine.end(), ',');
      int tabCount = std::count(firstLine.begin(), firstLine.end(), '\t');
      int semicolonCount = std::count(firstLine.begin(), firstLine.end(), ';');

      // Return the most common delimiter
      if (tabCount > commaCount && tabCount > semicolonCount)
        return '\t';
      else if (semicolonCount > commaCount && semicolonCount > tabCount)
        return ';';
      else
        return ',';
    }

    file.close();
    return ',';  // default
  }

  bool TabularDataIO::IsCentroidData(const TableData& data)
  {
    // Check for centroid-specific columns
    // Common column names for centroids: m/z, mz, mass, center, intensity, int
    
    std::vector<std::string> mzNames = {"m/z", "mz", "mass", "center", "m_z"};
    std::vector<std::string> intensityNames = {"intensity", "int", "i", "y", "height", "max", "mean"};

    int mzCol = FindColumn(data.headers, mzNames);
    int intensityCol = FindColumn(data.headers, intensityNames);

    // If we find both m/z and intensity columns, it's likely centroid data
    return (mzCol >= 0 && intensityCol >= 0);
  }

  std::vector<mitk::BaseData::Pointer> TabularDataIO::CreateCentroidLists(
    const TableData& data, const std::string& filename)
  {
    // Find m/z and intensity columns
    std::vector<std::string> mzNames = {"m/z", "mz", "mass", "center", "m_z"};
    std::vector<std::string> intensityNames = {"intensity", "int", "i", "y", "height", "max", "mean"};
    
    int mzCol = FindColumn(data.headers, mzNames);
    int intensityCol = FindColumn(data.headers, intensityNames);

    if (mzCol < 0 || intensityCol < 0)
    {
      MITK_ERROR << "Could not find m/z or intensity columns";
      return {};
    }

    MITK_INFO << "Using columns: " << data.headers[mzCol] << " (m/z) and " 
              << data.headers[intensityCol] << " (intensity)";

    // Check for label column
    std::vector<std::string> labelNames = {"label", "class", "group", "id"};
    int labelCol = FindColumn(data.headers, labelNames);

    if (labelCol >= 0)
    {
      MITK_INFO << "Found label column: " << data.headers[labelCol];
      
      // Group data by label
      std::map<std::string, m2::IntervalVector::Pointer> labeledResults;
      
      for (const auto& row : data.rows)
      {
        if (row.size() <= static_cast<size_t>(std::max(mzCol, std::max(intensityCol, labelCol))))
          continue;

        std::string label = row[labelCol];
        if (labeledResults[label].IsNull())
        {
          labeledResults[label] = m2::IntervalVector::New();
        }

        try
        {
          double mz = std::stod(row[mzCol]);
          double intensity = std::stod(row[intensityCol]);
          m2::Interval interval(mz, intensity);
          labeledResults[label]->GetIntervals().push_back(interval);
        }
        catch (const std::exception& e)
        {
          MITK_WARN << "Failed to parse row: " << e.what();
        }
      }

      // Convert to result vector and finalize
      std::vector<mitk::BaseData::Pointer> results;
      for (auto& kv : labeledResults)
      {
        auto& intervalVector = kv.second;
        
        // Set type and info after all intervals are added
        intervalVector->SetType(m2::SpectrumFormat::Centroid);
        intervalVector->SetInfo("Centroid list (label: " + kv.first + ") from: " + filename);
        
        results.push_back(intervalVector);
      }
      
      MITK_INFO << "Created " << results.size() << " centroid lists from labeled data";
      return results;
    }
    else
    {
      // No label column, create single IntervalVector
      auto intervalVector = m2::IntervalVector::New();
      intervalVector->SetType(m2::SpectrumFormat::Centroid);
      intervalVector->SetInfo("Centroid list from: " + filename);

      for (const auto& row : data.rows)
      {
        if (row.size() <= static_cast<size_t>(std::max(mzCol, intensityCol)))
          continue;

        try
        {
          double mz = std::stod(row[mzCol]);
          double intensity = std::stod(row[intensityCol]);
          m2::Interval interval(mz, intensity);
          intervalVector->GetIntervals().push_back(interval);
        }
        catch (const std::exception& e)
        {
          MITK_WARN << "Failed to parse row: " << e.what();
        }
      }

      MITK_INFO << "Created centroid list with " << intervalVector->GetIntervals().size() << " intervals";
      return {intervalVector};
    }
  }

  std::vector<mitk::BaseData::Pointer> TabularDataIO::CreatePlotData(
    const TableData& data, const std::string& filename)
  {
    auto plotData = m2::PlotData::New();
    plotData->SetDescription("Plot data from: " + filename);

    // First pass: detect which columns are numeric and which are strings
    std::vector<bool> isNumeric(data.headers.size(), true);
    
    for (size_t col = 0; col < data.headers.size(); ++col)
    {
      // Check if all values in this column can be converted to numbers
      for (const auto& row : data.rows)
      {
        if (col < row.size() && !row[col].empty())
        {
          try
          {
            (void)std::stod(row[col]);
          }
          catch (const std::exception&)
          {
            isNumeric[col] = false;
            break;
          }
        }
      }
    }

    // Second pass: add columns based on their type
    for (size_t col = 0; col < data.headers.size(); ++col)
    {
      if (isNumeric[col])
      {
        // Add as numeric column
        std::vector<double> columnData;
        columnData.reserve(data.rows.size());

        for (const auto& row : data.rows)
        {
          if (col < row.size())
          {
            try
            {
              double value = std::stod(row[col]);
              columnData.push_back(value);
            }
            catch (const std::exception&)
            {
              columnData.push_back(std::numeric_limits<double>::quiet_NaN());
            }
          }
        }

        plotData->AddColumn(data.headers[col], columnData);
      }
      else
      {
        // Add as string column
        std::vector<std::string> columnData;
        columnData.reserve(data.rows.size());

        for (const auto& row : data.rows)
        {
          if (col < row.size())
          {
            columnData.push_back(row[col]);
          }
          else
          {
            columnData.push_back("");
          }
        }

        plotData->AddStringColumn(data.headers[col], columnData);
      }
    }

    // Try to auto-detect X and Y axis (only from numeric columns)
    auto numericColumns = plotData->GetColumnNames();
    if (numericColumns.size() >= 1)
    {
      // First numeric column is often X
      plotData->SetXAxisColumn(numericColumns[0]);
      if (numericColumns.size() >= 2)
      {
        // Second numeric column is often Y
        plotData->SetYAxisColumn(numericColumns[1]);
      }
    }

    MITK_INFO << "Created plot data with " << plotData->GetNumberOfColumns() 
              << " numeric columns, " << plotData->GetStringColumnNames().size()
              << " string columns and " << plotData->GetNumberOfRows() << " rows";

    return {plotData};
  }

  int TabularDataIO::FindColumn(const std::vector<std::string>& headers, 
                                 const std::vector<std::string>& possibleNames)
  {
    for (size_t i = 0; i < headers.size(); ++i)
    {
      std::string headerLower = ToLower(headers[i]);
      for (const auto& name : possibleNames)
      {
        if (headerLower == ToLower(name))
        {
          return static_cast<int>(i);
        }
      }
    }
    return -1;  // Not found
  }

  std::string TabularDataIO::Strip(const std::string& str)
  {
    size_t start = 0;
    size_t end = str.length();

    // Find the index of the first non-whitespace character
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    // Find the index of the last non-whitespace character
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    // Return the stripped substring
    return str.substr(start, end - start);
  }

  std::string TabularDataIO::ToLower(const std::string& str)
  {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
  }

  TabularDataIO *TabularDataIO::IOClone() const 
  { 
    return new TabularDataIO(*this); 
  }

} // namespace m2
