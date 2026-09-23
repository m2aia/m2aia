/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2PointCloudWidget.h"

#include <QMouseEvent>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCubeAxesActor.h>
#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarsToColors.h>
#include <vtkTextProperty.h>
#include <vtkUnsignedCharArray.h>

#include <algorithm>

namespace
{
  /** The camera style of the two dimensional plot: the trackball without the two gestures that
      would tilt the plane out of the screen. Panning and zooming stay as they are. */
  class m2PlaneInteractorStyle : public vtkInteractorStyleTrackballCamera
  {
  public:
    static m2PlaneInteractorStyle *New();
    vtkTypeMacro(m2PlaneInteractorStyle, vtkInteractorStyleTrackballCamera);

    void Rotate() override {}
    void Spin() override {}
  };

  vtkStandardNewMacro(m2PlaneInteractorStyle);

  /** A colour that reads on the dark background of the scene. */
  constexpr double FOREGROUND[3] = {0.85, 0.85, 0.85};
} // namespace

m2PointCloudWidget::m2PointCloudWidget(QWidget *parent) : QVTKOpenGLNativeWidget(parent)
{
  vtkNew<vtkGenericOpenGLRenderWindow> window;
  this->setRenderWindow(window);

  m_Renderer = vtkSmartPointer<vtkRenderer>::New();
  m_Renderer->SetBackground(0.16, 0.16, 0.18);
  m_Renderer->SetBackground2(0.09, 0.09, 0.10);
  m_Renderer->GradientBackgroundOn();
  window->AddRenderer(m_Renderer);

  m_PolyData = vtkSmartPointer<vtkPolyData>::New();
  m_PolyData->SetPoints(vtkSmartPointer<vtkPoints>::New());
  m_PolyData->SetVerts(vtkSmartPointer<vtkCellArray>::New());

  m_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  m_Mapper->SetInputData(m_PolyData);
  m_Mapper->SetScalarModeToUsePointData();
  m_Mapper->SetColorModeToMapScalars();
  m_Mapper->ScalarVisibilityOff();

  m_Actor = vtkSmartPointer<vtkActor>::New();
  m_Actor->SetMapper(m_Mapper);
  m_Actor->GetProperty()->SetPointSize(3);
  m_Actor->GetProperty()->SetRenderPointsAsSpheres(true);
  m_Actor->GetProperty()->SetColor(0.55, 0.75, 0.95);
  m_Renderer->AddActor(m_Actor);

  m_Axes = vtkSmartPointer<vtkCubeAxesActor>::New();
  m_Axes->SetCamera(m_Renderer->GetActiveCamera());
  m_Axes->SetFlyModeToOuterEdges();
  m_Axes->DrawXGridlinesOn();
  m_Axes->DrawYGridlinesOn();
  m_Axes->DrawZGridlinesOn();
  m_Axes->SetGridLineLocation(vtkCubeAxesActor::VTK_GRID_LINES_FURTHEST);
  for (int axis = 0; axis < 3; ++axis)
  {
    m_Axes->GetTitleTextProperty(axis)->SetColor(FOREGROUND[0], FOREGROUND[1], FOREGROUND[2]);
    m_Axes->GetLabelTextProperty(axis)->SetColor(FOREGROUND[0], FOREGROUND[1], FOREGROUND[2]);
  }
  m_Axes->GetXAxesLinesProperty()->SetColor(0.45, 0.45, 0.48);
  m_Axes->GetYAxesLinesProperty()->SetColor(0.45, 0.45, 0.48);
  m_Axes->GetZAxesLinesProperty()->SetColor(0.45, 0.45, 0.48);
  m_Axes->GetXAxesGridlinesProperty()->SetColor(0.28, 0.28, 0.31);
  m_Axes->GetYAxesGridlinesProperty()->SetColor(0.28, 0.28, 0.31);
  m_Axes->GetZAxesGridlinesProperty()->SetColor(0.28, 0.28, 0.31);
  m_Axes->VisibilityOff();
  m_Renderer->AddActor(m_Axes);

  m_ScalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
  m_ScalarBar->SetOrientationToVertical();
  m_ScalarBar->SetNumberOfLabels(5);
  m_ScalarBar->SetWidth(0.09);
  m_ScalarBar->SetHeight(0.55);
  m_ScalarBar->SetPosition(0.89, 0.06);
  m_ScalarBar->GetTitleTextProperty()->SetColor(FOREGROUND[0], FOREGROUND[1], FOREGROUND[2]);
  m_ScalarBar->GetLabelTextProperty()->SetColor(FOREGROUND[0], FOREGROUND[1], FOREGROUND[2]);
  m_ScalarBar->GetAnnotationTextProperty()->SetColor(FOREGROUND[0], FOREGROUND[1], FOREGROUND[2]);
  m_ScalarBar->GetTitleTextProperty()->SetShadow(0);
  m_ScalarBar->GetLabelTextProperty()->SetShadow(0);
  m_ScalarBar->GetAnnotationTextProperty()->SetShadow(0);
  m_ScalarBar->VisibilityOff();
  m_Renderer->AddViewProp(m_ScalarBar);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  if (auto *interactor = this->interactor())
    interactor->SetInteractorStyle(style);

  UpdateCamera(true);
}

m2PointCloudWidget::~m2PointCloudWidget() = default;

std::size_t m2PointCloudWidget::GetNumberOfPoints() const
{
  return static_cast<std::size_t>(m_PolyData->GetNumberOfPoints());
}

void m2PointCloudWidget::SetPoints(const std::vector<float> &x,
                                   const std::vector<float> &y,
                                   const std::vector<float> &z)
{
  const auto count = std::min(x.size(), y.size());
  // a cloud without a third coordinate is a cloud in the plane; it is drawn at z = 0 so that the
  // two and the three dimensional case share every actor
  const bool hasZ = z.size() == x.size();

  vtkNew<vtkPoints> points;
  points->SetDataTypeToFloat();
  points->SetNumberOfPoints(static_cast<vtkIdType>(count));

  vtkNew<vtkCellArray> vertices;
  vertices->AllocateEstimate(static_cast<vtkIdType>(count), 1);

  for (std::size_t i = 0; i < count; ++i)
  {
    const auto id = static_cast<vtkIdType>(i);
    points->SetPoint(id, x[i], y[i], hasZ ? z[i] : 0.0f);
    vertices->InsertNextCell(1, &id);
  }

  m_PolyData->SetPoints(points);
  m_PolyData->SetVerts(vertices);
  m_PolyData->GetPointData()->SetScalars(nullptr);
  m_PolyData->Modified();

  m_Mapper->ScalarVisibilityOff();
  m_ScalarBar->VisibilityOff();
  m_Axes->SetVisibility(count > 0);

  UpdateAxes();
  UpdateCamera(true);
}

void m2PointCloudWidget::SetScalars(const std::vector<float> &values,
                                    vtkScalarsToColors *table,
                                    const QString &title)
{
  const auto count = static_cast<std::size_t>(m_PolyData->GetNumberOfPoints());
  if (!table || values.size() != count || count == 0)
  {
    // nothing that could be coloured; the uniform colour of the actor stays visible
    m_PolyData->GetPointData()->SetScalars(nullptr);
    m_Mapper->ScalarVisibilityOff();
    m_ScalarBar->VisibilityOff();
    RequestRender();
    return;
  }

  vtkNew<vtkFloatArray> scalars;
  scalars->SetName(title.toUtf8().constData());
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(static_cast<vtkIdType>(count));
  std::copy(values.begin(), values.end(), scalars->GetPointer(0));

  m_PolyData->GetPointData()->SetScalars(scalars);
  m_PolyData->Modified();

  m_Mapper->SetLookupTable(table);
  m_Mapper->UseLookupTableScalarRangeOn();
  m_Mapper->SetColorModeToMapScalars();
  m_Mapper->ScalarVisibilityOn();

  m_ScalarBar->SetLookupTable(table);
  m_ScalarBar->SetTitle(title.toUtf8().constData());
  m_ScalarBar->VisibilityOn();

  RequestRender();
}

void m2PointCloudWidget::SetColors(const std::vector<unsigned char> &rgb,
                                   vtkScalarsToColors *legend,
                                   const QString &legendTitle)
{
  const auto count = static_cast<std::size_t>(m_PolyData->GetNumberOfPoints());
  if (rgb.size() != count * 3 || count == 0)
  {
    // nothing that could be coloured; the uniform colour of the actor stays visible
    m_PolyData->GetPointData()->SetScalars(nullptr);
    m_Mapper->ScalarVisibilityOff();
    m_ScalarBar->VisibilityOff();
    RequestRender();
    return;
  }

  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetName("Colors");
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(static_cast<vtkIdType>(count));
  std::copy(rgb.begin(), rgb.end(), colors->GetPointer(0));

  m_PolyData->GetPointData()->SetScalars(colors);
  m_PolyData->Modified();

  // the colours are the colours, not values to be looked up
  m_Mapper->SetColorModeToDirectScalars();
  m_Mapper->ScalarVisibilityOn();

  // the bar only says what the colours mean; it is shown when the caller has something to say
  if (legend)
  {
    m_ScalarBar->SetLookupTable(legend);
    m_ScalarBar->SetTitle(legendTitle.toUtf8().constData());
    m_ScalarBar->VisibilityOn();
  }
  else
  {
    m_ScalarBar->VisibilityOff();
  }

  RequestRender();
}

void m2PointCloudWidget::SetUniformColor(const QColor &color)
{
  m_PolyData->GetPointData()->SetScalars(nullptr);
  m_Mapper->ScalarVisibilityOff();
  m_ScalarBar->VisibilityOff();
  m_Actor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
  RequestRender();
}

void m2PointCloudWidget::SetAxisTitles(const QString &x, const QString &y, const QString &z)
{
  m_Axes->SetXTitle(x.toUtf8().constData());
  m_Axes->SetYTitle(y.toUtf8().constData());
  m_Axes->SetZTitle(z.toUtf8().constData());
  RequestRender();
}

void m2PointCloudWidget::SetPointSize(int size)
{
  m_Actor->GetProperty()->SetPointSize(std::max(1, size));
  RequestRender();
}

void m2PointCloudWidget::SetProjection2D(bool enabled)
{
  if (m_Projection2D == enabled)
    return;

  m_Projection2D = enabled;

  if (auto *interactor = this->interactor())
  {
    if (enabled)
    {
      vtkNew<m2PlaneInteractorStyle> style;
      interactor->SetInteractorStyle(style);
    }
    else
    {
      vtkNew<vtkInteractorStyleTrackballCamera> style;
      interactor->SetInteractorStyle(style);
    }
  }

  // the third axis carries no information in the plane and its labels would only be noise
  m_Axes->SetZAxisVisibility(enabled ? 0 : 1);
  m_Axes->SetDrawZGridlines(enabled ? 0 : 1);

  UpdateCamera(true);
}

void m2PointCloudWidget::ResetCamera()
{
  UpdateCamera(true);
}

void m2PointCloudWidget::Clear()
{
  SetPoints({}, {}, {});
}

void m2PointCloudWidget::RequestRender()
{
  // rendering needs a context of its own; before the widget has one there is nothing to do but
  // ask for the first paint, which renders the scene anyway
  if (this->isValid() && this->renderWindow())
    this->renderWindow()->Render();
  else
    this->update();
}

void m2PointCloudWidget::UpdateAxes()
{
  double bounds[6];
  m_PolyData->GetBounds(bounds);

  // a cloud of one point, or one that is flat in a direction, would give an axis of zero length
  for (int axis = 0; axis < 3; ++axis)
  {
    auto &lower = bounds[2 * axis];
    auto &upper = bounds[2 * axis + 1];
    if (upper - lower < 1e-6)
    {
      const auto center = 0.5 * (lower + upper);
      lower = center - 0.5;
      upper = center + 0.5;
    }
  }

  m_Axes->SetBounds(bounds);
}

void m2PointCloudWidget::UpdateCamera(bool resetPosition)
{
  auto *camera = m_Renderer->GetActiveCamera();

  if (m_Projection2D)
  {
    camera->ParallelProjectionOn();
    if (resetPosition)
    {
      double bounds[6];
      m_PolyData->GetBounds(bounds);
      const double center[3] = {0.5 * (bounds[0] + bounds[1]),
                                0.5 * (bounds[2] + bounds[3]),
                                0.5 * (bounds[4] + bounds[5])};
      camera->SetFocalPoint(center[0], center[1], 0.0);
      camera->SetPosition(center[0], center[1], 1.0);
      camera->SetViewUp(0.0, 1.0, 0.0);
    }
  }
  else
  {
    camera->ParallelProjectionOff();
    if (resetPosition)
    {
      camera->SetPosition(1.0, 1.0, 1.0);
      camera->SetFocalPoint(0.0, 0.0, 0.0);
      camera->SetViewUp(0.0, 0.0, 1.0);
    }
  }

  if (resetPosition)
    m_Renderer->ResetCamera();

  m_Renderer->ResetCameraClippingRange();
  RequestRender();
}

void m2PointCloudWidget::mousePressEvent(QMouseEvent *event)
{
  m_PressWasLeft = event->button() == Qt::LeftButton;
  m_PressPosition = event->pos();
  QVTKOpenGLNativeWidget::mousePressEvent(event);
}

void m2PointCloudWidget::mouseReleaseEvent(QMouseEvent *event)
{
  // the interactor is left to handle the event first, so that its idea of where the cursor is is
  // the one the picker uses; that also keeps the high resolution display scaling out of here
  QVTKOpenGLNativeWidget::mouseReleaseEvent(event);

  if (!m_PressWasLeft || event->button() != Qt::LeftButton)
    return;
  m_PressWasLeft = false;

  // a left button that travelled was the camera being moved, not a point being asked about
  if ((event->pos() - m_PressPosition).manhattanLength() > 3)
    return;

  auto *interactor = this->interactor();
  if (!interactor || m_PolyData->GetNumberOfPoints() == 0)
    return;

  const int *position = interactor->GetEventPosition();

  vtkNew<vtkPointPicker> picker;
  picker->SetTolerance(0.01);
  picker->Pick(position[0], position[1], 0.0, m_Renderer);

  Q_EMIT PointPicked(static_cast<int>(picker->GetPointId()));
}
