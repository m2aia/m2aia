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
#include "RegistrationData.h"
#include "ui_RegistrationDataWidgetControls.h"
#include <QString>
#include <QWidget>

namespace mitk
{
  class DataStorage;
}

class RegistrationDataWidget : public QWidget
{
  Q_OBJECT
private:
  Ui_RegistrationEntityWidgetControls m_Controls;
  QWidget *m_Parent;
  mitk::DataStorage *m_DataStorage;
  std::shared_ptr<RegistrationData> m_RegistrationData;

  void UpdateRegistrationDataFromUI();

  // std::vector<std::string> m_Transformations;
  // mitk::DataNode::Pointer m_ResultNode;
  // std::vector<mitk::DataNode::Pointer> m_ResultAttachments;
  // std::vector<QmitkSingleNodeSelectionWidget *> m_Attachments;

private slots:
  void OnLoadTransformations();
  void OnSaveTransformations();
  void OnApplyTransformations();
  void OnAddPointSet();

public:
  RegistrationDataWidget(QWidget *parent, mitk::DataStorage *storage);
  ~RegistrationDataWidget();
  std::shared_ptr<RegistrationData> GetRegistrationData();
  const RegistrationData * GetRegistrationData() const;
  
  bool HasImage() const;
  bool HasMask() const;
  bool HasPointSet() const;
  bool HasTransformations() const;
  void EnableButtons(bool);

  mitk::DataNode::Pointer GetImageNode() const;
  mitk::DataNode::Pointer GetMaskNode() const;
  mitk::DataNode::Pointer GetPointSetNode() const;
  std::vector<std::string> GetTransformations() const;
  void SetTransformations(const std::vector<std::string> & data);

  /**
   * @brief Returns the selected mitk::Image or null;
   *
   * @return mitk::Image::Pointer
   */
  mitk::Image::Pointer GetImage() const;

  /**
   * @brief Returns the selected mitk::Image mask or null;
   *
   * @return mitk::Image::Pointer
   */
  mitk::Image::Pointer GetMask() const;

  /**
   * @brief Returns the selected mitk::PointSet or null;
   *
   * @return mitk::Image::Pointer
   */
  mitk::PointSet::Pointer GetPointSet() const;

  int GetIndex() const;

signals:
  void RemoveSelf();
};
