/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/


#include "org_mitk_biomarker_roc_Activator.h"
#include "BiomarkerRoc.h"

namespace mitk
{
  void org_mitk_biomarker_roc_Activator::start(ctkPluginContext *context)
  {
    BERRY_REGISTER_EXTENSION_CLASS(BiomarkerRoc, context)
  }

  void org_mitk_biomarker_roc_Activator::stop(ctkPluginContext *context) { Q_UNUSED(context) }
}
