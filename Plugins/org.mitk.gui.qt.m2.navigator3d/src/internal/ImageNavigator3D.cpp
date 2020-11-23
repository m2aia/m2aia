/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "ImageNavigator3D.h"

// Qt
#include <QMessageBox>

// mitk image
#include <mitkImage.h>
#include <mitkBaseRenderer.h>
#include <mitkStepper.h>
#include <QmitkRenderWindow.h>
#include <mitkRotationOperation.h>
#include <mitkInteractionConst.h>
#include <mitkCameraController.h>

const std::string ImageNavigator3D::VIEW_ID = "org.mitk.views.imagenavigator3d";

void ImageNavigator3D::SetFocus()
{

}


void ImageNavigator3D::CreateQtPartControl(QWidget *parent)
{


	// create GUI widgets from the Qt Designer's .ui file
	m_Controls.setupUi(parent);
	connect(m_Controls.anterior, &QPushButton::clicked, []() {
		vtkRenderWindow* renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4");
		auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController();
		controller->SetViewToAnterior();
	});

	connect(m_Controls.posterior, &QPushButton::clicked, []() {
		vtkRenderWindow* renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4");
		auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController();
		controller->SetViewToPosterior();
	});

	connect(m_Controls.sinister, &QPushButton::clicked, []() {
		vtkRenderWindow* renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4");
		auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController();
		controller->SetViewToSinister();
	});

	connect(m_Controls.dexter, &QPushButton::clicked, []() {
		vtkRenderWindow* renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4");
		auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController();
		controller->SetViewToDexter();
	});

	connect(m_Controls.cranial, &QPushButton::clicked, []() {
		vtkRenderWindow* renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4");
		auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController();
		controller->SetViewToCranial();
	});

	connect(m_Controls.caudal, &QPushButton::clicked, []() {
		vtkRenderWindow* renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4");
		auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController();
		controller->SetViewToCaudal();
	});


}

void ImageNavigator3D::ChangeOrientation()
{


}



void ImageNavigator3D::RenderWindowPartActivated(mitk::IRenderWindowPart* /*renderWindowPart*/)
{
	
}


void ImageNavigator3D::OnRefetch()
{
	
}


