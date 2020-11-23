/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef m2Reconstruction3D_h
#define m2Reconstruction3D_h

#include "ui_m2Reconstruction3DControls.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFuture>
#include <QMimeData>
#include <QPoint>
#include <QPointF>
#include <QmitkAbstractView.h>
#include <berryISelectionListener.h>
#include <boost/filesystem.hpp>
#include <m2ImzMLMassSpecImage.h>
#include <m2MSImageBase.h>
#include <mitkImage.h>
#include <mitkPointSet.h>
#include <mitkTransformixMSDataObjectStack.h>
#include <qfuturewatcher.h>
#include <qlistwidget.h>
#include <qprocess.h>

/**
  \brief m2Reconstruction3D

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2Reconstruction3D : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;
  virtual void SetFocus() override {}

  struct DataTuple
  {
    mitk::Image::Pointer image, mask;
    // mitk::PointSet::Pointer points;
  };

  struct DataTupleWarpingResult
  {
    std::vector<std::string> m_transformations;
    std::vector<DataTuple> m_images;

    DataTupleWarpingResult(std::vector<std::string> &t, std::vector<DataTuple> &i) : m_transformations(t), m_images(i)
    {
    }

    const std::vector<std::string> &transformations() { return m_transformations; }
    const std::vector<DataTuple> &images() { return m_images; }
  };

  void WarpPoints(mitk::Image *ionImage, mitk::PointSet *inPoints, std::vector<std::string> transformations);

  std::shared_ptr<DataTupleWarpingResult> WarpImage(const DataTuple &fixed,
                                                    const DataTuple &moving,
                                                    bool useNormalization = false,
                                                    bool useInvertIntensities = false,
                                                    bool omitDeformable = false);
  static void ExportSlice(mitk::Image *input,
                          const boost::filesystem::path &directory,
                          const std::string &name,
                          bool useNormalization,
                          bool useInvertIntensities);

  Ui::m2Reconstruction3DControls m_Controls;
  QString GetTransformixPath() const;
  QString GetElastixPath() const;
  std::shared_ptr<QProcess> m_Process;

  QListWidget *m_list1, *m_list2;

  std::map<unsigned int, DataTuple> m_referenceMap;

public slots:
  void OnUpdateList();
  void OnCancelRegistration();
  void OnStartStacking();
};

#endif // m2Reconstruction3D_h
