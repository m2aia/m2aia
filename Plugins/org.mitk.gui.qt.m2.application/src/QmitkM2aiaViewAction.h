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

#include <QAction>
#include <berryIViewDescriptor.h>
#include <berryIWorkbenchPage.h>
#include <berryIWorkbenchWindow.h>
#include <berryUIException.h>

namespace m2
{
  class QmitkM2aiaViewAction : public QAction
  {
    Q_OBJECT

  private:
    berry::IWorkbenchWindow *m_Window;

    berry::IViewDescriptor::Pointer m_Desc;

  public:
	  QmitkM2aiaViewAction(berry::IWorkbenchWindow::Pointer window, berry::IViewDescriptor::Pointer desc)
      : QAction(nullptr)
    {
      this->setParent(static_cast<QWidget *>(window->GetShell()->GetControl()));
      this->setText(desc->GetLabel());
      this->setToolTip(desc->GetLabel());
      this->setIconVisibleInMenu(true);

      QIcon icon = desc->GetImageDescriptor();
      this->setIcon(icon);

      m_Window = window.GetPointer();
      m_Desc = desc;

      this->connect(this, SIGNAL(triggered(bool)), this, SLOT(Run()));
    }
  protected slots:
    void Run()
    {
      berry::IWorkbenchPage::Pointer page = m_Window->GetActivePage();
      if (page.IsNotNull())
      {
        try
        {
          page->ShowView(m_Desc->GetId());
        }
        catch (const berry::PartInitException &e)
        {
          BERRY_ERROR << "Error: " << e.what();
        }
      }
    }
  };
} // namespace m2
