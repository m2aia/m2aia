/*===================================================================

Mass Spectrometry Imaging applications for interactive
analysis in MITK (M2aia)

Copyright (c) Jonas Cordes, Hochschule Mannheim.
Division of Medical Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include "m2OpenSlideImageIOHelperDialog.h"

#include <QPixmap>
#include <QmitkNodeDescriptorManager.h>
#include <itkMetaDataObject.h>
#include <mitkDataNode.h>
#include <mitkExtractSliceFilter.h>
#include <mitkImage.h>
#include <mitkItkImageIO.h>
#include <qpushbutton.h>
#include <vtkLookupTable.h>
#include <vtkMitkLevelWindowFilter.h>

QPixmap GetPixmapFromImageNode(const mitk::Image *image, int height)
{
  mitk::PlaneGeometry::Pointer planeGeometry = mitk::PlaneGeometry::New();
  int sliceNumber = image->GetDimension(2) / 2;
  planeGeometry->InitializeStandardPlane(image->GetGeometry(), mitk::PlaneGeometry::Axial, sliceNumber);

  mitk::ExtractSliceFilter::Pointer extractSliceFilter = mitk::ExtractSliceFilter::New();
  extractSliceFilter->SetInput(image);
  extractSliceFilter->SetInterpolationMode(mitk::ExtractSliceFilter::RESLICE_CUBIC);
  extractSliceFilter->SetResliceTransformByGeometry(image->GetGeometry());
  extractSliceFilter->SetWorldGeometry(planeGeometry);
  extractSliceFilter->SetOutputDimensionality(2);
  extractSliceFilter->SetVtkOutputRequest(true);
  extractSliceFilter->Update();

  vtkImageData *imageData = extractSliceFilter->GetVtkOutput();

  vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New();
  lookupTable->SetRange(image->GetScalarValue2ndMin(), image->GetScalarValue2ndMax());
  lookupTable->SetSaturationRange(0.0, 0.0);
  lookupTable->SetValueRange(0.0, 1.0);
  lookupTable->SetHueRange(0.0, 0.0);
  lookupTable->SetRampToLinear();

  vtkSmartPointer<vtkMitkLevelWindowFilter> levelWindowFilter = vtkSmartPointer<vtkMitkLevelWindowFilter>::New();
  levelWindowFilter->SetLookupTable(lookupTable);
  levelWindowFilter->SetInputData(imageData);
  levelWindowFilter->SetMinOpacity(0.0);
  levelWindowFilter->SetMaxOpacity(1.0);
  int dims[3];
  imageData->GetDimensions(dims);
  double clippingBounds[] = {0.0, static_cast<double>(dims[0]), 0.0, static_cast<double>(dims[1])};
  levelWindowFilter->SetClippingBounds(clippingBounds);
  levelWindowFilter->Update();
  imageData = levelWindowFilter->GetOutput();

  QImage thumbnailImage(
    reinterpret_cast<const unsigned char *>(imageData->GetScalarPointer()), dims[0], dims[1], QImage::Format_ARGB32);

  if (dims[0] > dims[1])
  {
    thumbnailImage = thumbnailImage.scaledToWidth(height, Qt::SmoothTransformation).rgbSwapped();
  }
  else
  {
    thumbnailImage = thumbnailImage.scaledToHeight(height, Qt::SmoothTransformation).rgbSwapped();
  }

  return QPixmap::fromImage(thumbnailImage);
}

void m2OpenSlideImageIOHelperDialog::SetOpenSlideImageIOHelperObject(m2::OpenSlideImageIOHelperObject *helper)
{
  m_Helper = helper;
}

void m2OpenSlideImageIOHelperDialog::UpdateImageInformation()
{
  auto IO = m_Helper->GetOpenSlideIO();

  IO->SetLevel(IO->GetLevelCount() - 1);
  mitk::ItkImageIO reader(IO);
  reader.mitk::AbstractFileReader::SetInput(IO->GetFileName());
  auto dataList = reader.Read();
  mitk::Image::Pointer lowResImage = dynamic_cast<mitk::Image *>(dataList.back().GetPointer());
  auto pixmap = GetPixmapFromImageNode(lowResImage, 256);
  m_Controls.labelImage->setPixmap(pixmap);
  m_Controls.metaDataList->clear();
  auto metadata = IO->GetMetaDataDictionary();

  struct LevelInfo
  {
    std::string width;
    std::string height;
    std::string level;
  };
  std::map<unsigned int, LevelInfo> levelInfos;

  for (auto &key : metadata.GetKeys())
  {
    auto valueObject = metadata.Get(key);
    if (const auto value_object = dynamic_cast<const itk::MetaDataObject<std::string> *>(valueObject))
    {
      const std::string value_string = value_object->GetMetaDataObjectValue();
      m_Controls.metaDataList->addItem(QString(key.c_str()) + ": " + QString(value_string.c_str()));
    }
  }

  for (const auto &kv : m_Helper->GetLevelsMap())
  {
    LevelInfo inf;
    inf.level = std::to_string(kv.first);
    for (const auto &kv2 : kv.second)
    {
      if (kv2.first.compare("width") == 0)
        inf.width = kv2.second;

      if (kv2.first.compare("height") == 0)
        inf.height = kv2.second;
    }
    levelInfos[kv.first] = inf;
  }

  m_Controls.imageSelectionList->clear();
  for (const auto &kv : levelInfos)
  {
    auto item = new QListWidgetItem();
    item->setText(QString(("Level [" + kv.second.level + "] " + kv.second.width + " x " + kv.second.height).c_str()));
    item->setData(Qt::UserRole, (unsigned int)(kv.first));
    m_Controls.imageSelectionList->addItem(item);
  }

  connect(m_Controls.imageSelectionList, &QListWidget::currentItemChanged, [this](QListWidgetItem *item, auto prev) {
    MITK_INFO << "Selected level " << item->data(Qt::UserRole).toUInt();
    this->m_SelectedLevel = item->data(Qt::UserRole).toUInt();
  });
}
