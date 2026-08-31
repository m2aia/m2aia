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

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <ui_DataCompressionViewControls.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>


class QmitkDataCompressionView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  void CreateQtPartControl(QWidget* parent) override;

private slots:
  void OnStartPCA();
  void OnStartTSNE();
  void OnStartKMeans();
  void OnSaveDataCompressionResults();
  /** Repopulates the label value selection with the unique label values of the currently selected masks. */
  void OnMaskSelectionChanged();
  

private:
  /** The label value currently chosen in the label value selection, 0 if there is none. */
  mitk::MultiLabelSegmentation::LabelValueType GetSelectedMaskLabelValue() const;

  /** The selected mask that belongs to the given image node, null if there is none. */
  mitk::DataNode::ConstPointer GetMaskNode(const mitk::DataNode *imageNode);

  /** Mask restricting the given image to the pixels of the chosen label value.
      Falls back to the segmentation of the image if no mask is selected for it and
      returns null if the selected mask does not contain the chosen label value. */
  mitk::Image::Pointer GetMaskImage(const mitk::DataNode *imageNode);

  mitk::Image::Pointer ResampleVectorImage(mitk::Image::Pointer lowResImage, mitk::Image::Pointer referenceImage);
  void SetFocus() override;
  QWidget * m_Parent;

  Ui::DataCompressionViewControls m_Controls;
};

