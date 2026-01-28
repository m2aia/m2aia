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
#include <m2PlotData.h>
#include <string>
#include <vector>
#include <map>

namespace m2
{
  /**
   * @brief Dynamic tabular data reader that can handle multiple table formats
   * 
   * This reader automatically detects the type of data in a CSV/tabular file and
   * creates appropriate data nodes:
   * - Centroid lists (m/z and intensity data) -> IntervalVector
   * - General plot data (XY data, time series, etc.) -> PlotData
   * 
   * File Format Support:
   * - CSV files (.csv)
   * - Tab-separated files (.tsv, .txt)
   * - Custom delimiters
   * 
   * Auto-detection:
   * - Checks for centroid-specific columns (m/z, mz, mass, intensity)
   * - Checks for label column to split into multiple datasets
   * - Falls back to general plot data for other formats
   * 
   * Example formats:
   * 
   * Centroid list:
   * m/z, intensity
   * 400.5, 1000
   * 401.2, 2000
   * 
   * Centroid list with labels:
   * m/z, intensity, label
   * 400.5, 1000, 1
   * 401.2, 2000, 1
   * 500.3, 1500, 2
   * 
   * Plot data:
   * time, signal, error
   * 0.0, 1.5, 0.1
   * 1.0, 2.3, 0.2
   */
  class M2AIACOREIO_EXPORT TabularDataIO : public mitk::AbstractFileIO
  {
  public:
    TabularDataIO();

    std::string TABULAR_MIMETYPE_NAME()
    {
      static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".csv.tabular";
      return name;
    }

    mitk::CustomMimeType TABULAR_MIMETYPE()
    {
      mitk::CustomMimeType mimeType(TABULAR_MIMETYPE_NAME());
      mimeType.AddExtension("csv");
      mimeType.AddExtension("tsv");
      mimeType.AddExtension("txt");
      mimeType.AddExtension("dat");
      mimeType.SetCategory("Tabular Data");
      mimeType.SetComment("CSV/Tabular data (centroid lists, plot data, etc.)");
      return mimeType;
    }

    std::vector<mitk::BaseData::Pointer> DoRead() override;
    ConfidenceLevel GetReaderConfidenceLevel() const override;
    
    void Write() override;
    ConfidenceLevel GetWriterConfidenceLevel() const override;

  private:
    TabularDataIO *IOClone() const override;

    /**
     * @brief Represents parsed CSV data
     */
    struct TableData
    {
      std::vector<std::string> headers;
      std::vector<std::vector<std::string>> rows;
    };

    /**
     * @brief Parse a CSV/TSV file
     */
    TableData ParseFile(const std::string& path, char delimiter = ',');

    /**
     * @brief Detect delimiter in file
     */
    char DetectDelimiter(const std::string& path);

    /**
     * @brief Check if data represents centroid list
     */
    bool IsCentroidData(const TableData& data);

    /**
     * @brief Create IntervalVector from centroid data
     */
    std::vector<mitk::BaseData::Pointer> CreateCentroidLists(const TableData& data, const std::string& filename);

    /**
     * @brief Create PlotData from general tabular data
     */
    std::vector<mitk::BaseData::Pointer> CreatePlotData(const TableData& data, const std::string& filename);

    /**
     * @brief Find column index by name (case-insensitive)
     */
    int FindColumn(const std::vector<std::string>& headers, const std::vector<std::string>& possibleNames);

    /**
     * @brief Strip whitespace from string
     */
    std::string Strip(const std::string& str);

    /**
     * @brief Convert string to lowercase
     */
    std::string ToLower(const std::string& str);

  }; 

} // namespace m2
