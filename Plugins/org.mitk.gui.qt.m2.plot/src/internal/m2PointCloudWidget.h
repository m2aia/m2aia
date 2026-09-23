/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef m2PointCloudWidget_h
#define m2PointCloudWidget_h

#include <QColor>
#include <QPoint>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>

#include <vector>

class vtkActor;
class vtkCubeAxesActor;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkScalarBarActor;
class vtkScalarsToColors;

/**
  \brief An interactive scatter plot of a point cloud, in two or three dimensions.

  The widget only draws what it is given: coordinates, one value per point and a colour table
  mapping those values. What the points stand for - the pixels of an embedding, the centroids of a
  peak list - is decided by the view that fills it.

  In three dimensions the camera orbits the cloud; in two dimensions it looks straight down the
  third axis with a parallel projection and rotation switched off, so the plot behaves like an
  ordinary scatter plot. Both use the same actors, so switching between them keeps the colours,
  the scalar bar and the axes.

  A click that is not a drag picks the point under the cursor and reports its position in the
  arrays that were handed in, which is what lets the view name it.
*/
class m2PointCloudWidget : public QVTKOpenGLNativeWidget
{
  Q_OBJECT

public:
  explicit m2PointCloudWidget(QWidget *parent = nullptr);
  ~m2PointCloudWidget() override;

  /** The coordinates of the cloud, one entry per point. A z of a different length than x is
      treated as a plane at z = 0, which is what a two dimensional cloud is drawn as. */
  void SetPoints(const std::vector<float> &x, const std::vector<float> &y, const std::vector<float> &z);

  /** Colours every point by its own value. The table is what turns a value into a colour and is
      also what the scalar bar shows, so a categorical table gives a legend of swatches and a
      continuous one a colour ramp. Fewer values than points switches the colouring off. */
  void SetScalars(const std::vector<float> &values, vtkScalarsToColors *table, const QString &title);

  /** Gives every point the colour it is handed, three values per point in the order of
      SetPoints. Used for a colour that is not a value on a scale - the RGB image a three
      component reduction is rendered as, say. A legend table may still be given: the colours are
      then the ones the caller computed, while the bar keeps saying what they stood for before
      anything was done to them. Fewer values than points switches the colouring off. */
  void SetColors(const std::vector<unsigned char> &rgb,
                 vtkScalarsToColors *legend = nullptr,
                 const QString &legendTitle = QString());

  /** Draws every point in the same colour and hides the scalar bar. */
  void SetUniformColor(const QColor &color);

  void SetAxisTitles(const QString &x, const QString &y, const QString &z);

  void SetPointSize(int size);

  /** True looks down the z axis with a parallel projection and no rotation. */
  void SetProjection2D(bool enabled);
  bool GetProjection2D() const { return m_Projection2D; }

  /** Frames the whole cloud again. */
  void ResetCamera();

  /** Removes every point; the widget then shows an empty scene. */
  void Clear();

  std::size_t GetNumberOfPoints() const;

Q_SIGNALS:
  /** The point under the cursor of a click that was not a drag, by its position in the arrays
      passed to SetPoints. -1 when the click hit no point. */
  void PointPicked(int index);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  /** Renders the scene again after it was changed.

      A plain repaint would not: QVTKOpenGLNativeWidget only asks VTK for a new rendering in
      paintGL right after the context was created or the widget was resized, and otherwise blits
      the frame it already holds. Anything that changes the scene therefore has to go through the
      render window itself. */
  void RequestRender();

  /** Puts the camera where the current projection wants it and refreshes the axes. */
  void UpdateCamera(bool resetPosition);
  void UpdateAxes();

  vtkSmartPointer<vtkRenderer> m_Renderer;
  vtkSmartPointer<vtkPolyData> m_PolyData;
  vtkSmartPointer<vtkPolyDataMapper> m_Mapper;
  vtkSmartPointer<vtkActor> m_Actor;
  vtkSmartPointer<vtkCubeAxesActor> m_Axes;
  vtkSmartPointer<vtkScalarBarActor> m_ScalarBar;

  bool m_Projection2D = false;
  /** Where the left button went down, to tell a click from a drag of the camera. */
  QPoint m_PressPosition;
  bool m_PressWasLeft = false;
};

#endif // m2PointCloudWidget_h
