/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef MatchPointBrowserConstants_h
#define MatchPointBrowserConstants_h

#include <QString>
#include <string>

#include "org_mitk_m2_core_helper_Export.h"

/**
 * \class MatchPointBrowserConstants
 * \brief Structure to define a namespace for constants used privately within this view.
 */
struct MITK_M2_CORE_HELPER_EXPORT m2BrowserConstants
{
  /**
   * \brief The name of the preferences node containing the list of directories to scan for
   * MatchPoint Deployed Registration Algorithms (MDRA).
   */
  static const std::string MDAR_DIRECTORIES_NODE_NAME;

  /**
   * \brief The name of the preferences node containing the additional MatchPoint Deployed Registration Algorithms (MDRA)
   * that should be explicitly checked/loaded.
   */
  static const std::string MDAR_FILES_NODE_NAME;

  /**
   * \brief The name of the preferences node containing whether we are producing debug output.
   */
  static const std::string DEBUG_OUTPUT_NODE_NAME;

  /**
   * \brief The name of the preferences node containing a boolean describing whether
   * we are loading MDRAs from the application directory.
   */
  static const std::string LOAD_FROM_APPLICATION_DIR;

  /**
   * \brief The name of the preferences node containing a boolean describing whether
   * we are loading MDRAs from the users home directory.
   */
  static const std::string LOAD_FROM_HOME_DIR;

  /**
   * \brief The name of the preferences node containing a boolean describing whether
   * we are loading MDRAs from the applications current working directory.
   */
  static const std::string LOAD_FROM_CURRENT_DIR;

  /**
   * \brief The name of the preferences node containing a boolean describing whether
   * we are loading MDRAs from the directory specified in MDRA_MODULE_LOAD_PATH.
   */
  static const std::string LOAD_FROM_AUTO_LOAD_DIR;

  /**
   * \brief The View ID = org.mitk.gui.qt.algorithm.browser, and should match that in plugin.xml.
   */
  static const std::string VIEW_ID;

};

#endif // MatchPointBrowserConstants_h
