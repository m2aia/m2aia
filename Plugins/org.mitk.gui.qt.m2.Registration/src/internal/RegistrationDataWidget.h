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

#include <mitkDataStorage.h>

class RegistrationDataWidget : public QWidget
{
  Q_OBJECT
private:
  QWidget *m_Parent;
  mitk::DataStorage::Pointer m_DataStorage;
  std::shared_ptr<RegistrationData> m_RegistrationData;
  void UpdateRegistrationDataFromUI();

private slots:
  void OnLoadTransformations();
  void OnSaveTransformations();
  void OnApplyTransformations();
  void OnAddPointSet();

public:
  RegistrationDataWidget(QWidget *parent, mitk::DataStorage::Pointer storage);
  ~RegistrationDataWidget();
  std::shared_ptr<RegistrationData> GetRegistrationData();
  const RegistrationData * GetRegistrationData() const;
  
  bool HasImage() const;
  bool HasMask() const;
  bool HasPointSet() const;
  bool HasTransformations() const;
  void EnableButtons(bool);


  Ui_RegistrationDataWidgetControls m_Controls;
  void SetDataStorage(mitk::DataStorage::Pointer storage);
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
