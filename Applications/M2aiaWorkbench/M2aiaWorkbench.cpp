/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <mitkBaseApplication.h>
#include <QPixmap>
#include <QTimer>
#include <QVariant>

#if defined __GNUC__ && !defined __clang__
#  include <QDir>
#  include <QFileInfo>
#  include <QString>
#  include <QStringList>
#endif

int main(int argc, char **argv)
{
  mitk::BaseApplication app(argc, argv);

  app.setSingleMode(true);
  app.setApplicationName("M2aia Workbench");
  app.setOrganizationName("University of Applied Sciences");

  #if defined __GNUC__ && !defined __clang__
    auto library = QFileInfo(argv[0]).dir().path() + "/../lib/plugins/liborg_blueberry_core_expressions.so";

    if (!QFileInfo(library).exists())
      library = "liborg_blueberry_core_expressions";

    app.setPreloadLibraries(QStringList() << library);
  #endif
	
  app.setProperty(mitk::BaseApplication::PROP_PRODUCT, "org.mitk.gui.qt.m2.application.workbench");

  return app.run();
}
