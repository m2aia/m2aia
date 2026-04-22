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
#include <string>

namespace m2
{
  /**
   * @brief Represents HTML content for visualization
   * 
   * This class stores HTML content that can be displayed in web views.
   * Common use cases:
   * - Interactive plots (Plotly, etc.)
   * - Analysis results with HTML reports
   * - Volcano plots
   * - Any custom HTML visualization
   */
  class M2AIACORE_EXPORT HtmlData : public mitk::BaseData
  {
  public:
    mitkClassMacro(HtmlData, mitk::BaseData);
    itkCloneMacro(Self);
    itkFactorylessNewMacro(Self);

    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return false; }
    void SetRequestedRegion(const itk::DataObject *) override {}

    /**
     * @brief Set the HTML content
     * @param html The HTML content as a string
     */
    void SetHtmlContent(const std::string& html);

    /**
     * @brief Get the HTML content
     * @return The HTML content string
     */
    std::string GetHtmlContent() const;

    /**
     * @brief Set a title/description for the HTML data
     */
    void SetTitle(const std::string& title);

    /**
     * @brief Get the title
     */
    std::string GetTitle() const;

    /**
     * @brief Check if the HTML data is empty
     */
    virtual bool IsEmpty() const override;

    /**
     * @brief Clear all data
     */
    void Clear();

    /**
     * @brief Load HTML content from a file
     * @param filePath Path to the HTML file
     * @return true if successful, false otherwise
     */
    bool LoadFromFile(const std::string& filePath);

    /**
     * @brief Save HTML content to a file
     * @param filePath Path to save the HTML file
     * @return true if successful, false otherwise
     */
    bool SaveToFile(const std::string& filePath) const;

  protected:
    HtmlData();
    ~HtmlData() override = default;

  private:
    std::string m_HtmlContent;
    std::string m_Title;
  };

} // namespace m2
