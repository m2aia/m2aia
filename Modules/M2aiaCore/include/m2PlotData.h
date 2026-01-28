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

#include <M2aiaCoreExports.h>
#include <mitkBaseData.h>
#include <mitkDataNode.h>
#include <mitkProperties.h>
#include <string>
#include <vector>
#include <map>

namespace m2
{
  /**
   * @brief Represents a single data series for plotting (e.g., XY data)
   * 
   * This class stores tabular plot data with support for multiple columns.
   * Common use cases:
   * - XY scatter plots
   * - Line plots
   * - Time series data
   * - Spectroscopic data
   */
  class M2AIACORE_EXPORT PlotData : public mitk::BaseData
  {
  public:
    mitkClassMacro(PlotData, mitk::BaseData);
    itkCloneMacro(Self);
    itkFactorylessNewMacro(Self);

    using ValueVector = std::vector<double>;
    using ColumnMap = std::map<std::string, ValueVector>;
    using StringVector = std::vector<std::string>;
    using StringColumnMap = std::map<std::string, StringVector>;

    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return false; }
    void SetRequestedRegion(const itk::DataObject *) override {}

    /**
     * @brief Add a data column with a name
     * @param columnName The name of the column (e.g., "X", "Y", "Intensity")
     * @param data The data values for this column
     */
    void AddColumn(const std::string& columnName, const ValueVector& data);

    /**
     * @brief Get data from a specific column
     * @param columnName The name of the column
     * @return The data vector, or empty if column doesn't exist
     */
    ValueVector GetColumn(const std::string& columnName) const;

    /**
     * @brief Check if a column exists
     */
    bool HasColumn(const std::string& columnName) const;

    /**
     * @brief Get all column names
     */
    std::vector<std::string> GetColumnNames() const;

    /**
     * @brief Get the number of rows (assumes all columns have same length)
     */
    size_t GetNumberOfRows() const;

    /**
     * @brief Get the number of columns
     */
    size_t GetNumberOfColumns() const;

    /**
     * @brief Clear all data
     */
    void Clear();

    /**
     * @brief Check if the plot data is empty
     */
    virtual bool IsEmpty() const override;

    /**
     * @brief Set a description for the plot data
     */
    void SetDescription(const std::string& description);

    /**
     * @brief Get the description
     */
    std::string GetDescription() const;

    /**
     * @brief Get all data as a map
     */
    const ColumnMap& GetData() const { return m_Data; }

    /**
     * @brief Add a string column (for categorical data, labels, etc.)
     * @param columnName The name of the column
     * @param data The string values for this column
     */
    void AddStringColumn(const std::string& columnName, const StringVector& data);

    /**
     * @brief Get string data from a specific column
     * @param columnName The name of the column
     * @return The string vector, or empty if column doesn't exist
     */
    StringVector GetStringColumn(const std::string& columnName) const;

    /**
     * @brief Check if a string column exists
     */
    bool HasStringColumn(const std::string& columnName) const;

    /**
     * @brief Get all string column names
     */
    std::vector<std::string> GetStringColumnNames() const;

    /**
     * @brief Get all string data as a map
     */
    const StringColumnMap& GetStringData() const { return m_StringData; }

    /**
     * @brief Set the X-axis column name (for plotting)
     */
    void SetXAxisColumn(const std::string& columnName);

    /**
     * @brief Get the X-axis column name
     */
    std::string GetXAxisColumn() const { return m_XAxisColumn; }

    /**
     * @brief Set the Y-axis column name (for plotting)
     */
    void SetYAxisColumn(const std::string& columnName);

    /**
     * @brief Get the Y-axis column name
     */
    std::string GetYAxisColumn() const { return m_YAxisColumn; }

  protected:
    PlotData();
    ~PlotData() override = default;

  private:
    ColumnMap m_Data;
    StringColumnMap m_StringData;
    std::string m_Description;
    std::string m_XAxisColumn;
    std::string m_YAxisColumn;
  };

} // namespace m2
