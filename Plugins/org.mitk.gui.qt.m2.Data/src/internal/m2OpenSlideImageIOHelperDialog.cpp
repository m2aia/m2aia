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

#include "GL/gl.h"
#include <QMessageBox>
#include <QPixmap>
#include <QmitkNodeDescriptorManager.h>
#include <itkImageDuplicator.h>
#include <itkImageFileReader.h>
#include <itkMetaDataObject.h>
#include <itkRGBToLuminanceImageFilter.h>
#include <itkRGBToVectorImageAdaptor.h>
#include <itkVectorImage.h>
#include <itkVectorImageToImageAdaptor.h>
#include <mitkDataNode.h>
#include <mitkExtractSliceFilter.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImageStatisticsHolder.h>
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
  lookupTable->SetRange(image->GetStatistics()->GetScalarValue2ndMin(), image->GetStatistics()->GetScalarValue2ndMax());
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

  connect(m_Controls.imageSelectionList,
          &QListWidget::currentItemChanged,
          [this, levelInfos](QListWidgetItem *item, auto /*prev*/)
          {
            this->m_SelectedLevel = item->data(Qt::UserRole).toUInt();
            this->m_SliceThickness = m_Controls.thickness->value();

            int value;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value); // Returns 1 value
            unsigned int maxGL = value;
            maxGL = 1 << 13;

            const unsigned int tileSize = maxGL;
            const unsigned int width = std::stoi(levelInfos.at(this->m_SelectedLevel).width);
            const unsigned int height = std::stoi(levelInfos.at(this->m_SelectedLevel).height);
            QString warn = "";

            this->m_TilesX = width / tileSize + int((width % tileSize) > 0);
            this->m_TilesY = height / tileSize + int((height % tileSize) > 0);

            if (width > maxGL)
            {
              warn += "Devision in x direction into " + QString::number(m_TilesX) + " parts";
            }
            if (height > maxGL)
            {
              if (!warn.isEmpty())
                warn += "\n";
              warn += "Devision in y direction into " + QString::number(m_TilesY) + " parts";
            }

            m_Controls.warningLabel->setText(warn);
          });
}

mitk::Image::Pointer m2OpenSlideImageIOHelperDialog::GetPreviewData()
{
  auto IO = m_Helper->GetOpenSlideIO();
  if (this->GetSelectedLevel() == IO->GetLevelCount() - 1)
    return nullptr;
  const auto prevLevel = this->GetSelectedLevel();
  // read the image data
  IO->SetLevel(IO->GetLevelCount() - 1);
  mitk::ItkImageIO reader(IO);
  reader.mitk::AbstractFileReader::SetInput(IO->GetFileName());
  auto dataList = reader.Read();
  mitk::Image::Pointer lowResImage = dynamic_cast<mitk::Image *>(dataList.back().GetPointer());

  IO->SetLevel(prevLevel);
  return lowResImage;
}

std::vector<mitk::Image::Pointer> m2OpenSlideImageIOHelperDialog::GetData()
{
  auto IO = m_Helper->GetOpenSlideIO();

  // read the image data
  IO->SetLevel(this->GetSelectedLevel());

  using ItkRGBAImageType = itk::Image<itk::RGBAPixel<unsigned char>, 3>;

  auto reader = itk::ImageFileReader<ItkRGBAImageType>::New();
  reader->SetImageIO(IO);
  reader->SetFileName(IO->GetFileName());
  reader->Update();
  ItkRGBAImageType::Pointer itkImage = reader->GetOutput();
  auto s = itkImage->GetSpacing();
  s[0] *= 1e-3;
  s[1] *= 1e-3;
  s[2] = this->GetSliceThickness();
  itkImage->SetSpacing(s);

  if (m_Controls.comboBox->currentIndex() == 0) // RGBA
  {
    mitk::Image::Pointer image;
    mitk::CastToMitkImage(itkImage, image);
    return {image};
  }
  else if (m_Controls.comboBox->currentIndex() == 1) // vector
  {
    auto vectorImage = ConvertRGBAToVectorImage(itkImage);
    mitk::Image::Pointer image;
    mitk::CastToMitkImage(vectorImage, image);
    return {image};
  }
  else if (m_Controls.comboBox->currentIndex() == 2) // split images RGB
  {
    std::vector<mitk::Image::Pointer> result;
    for (unsigned int c : {0, 1, 2})
    {
      auto componentImage = ConvertRGBAToImage(itkImage, c);
      mitk::Image::Pointer image;
      mitk::CastToMitkImage(componentImage, image);
      result.push_back(image);
    }
    return result;
  }
  else if (m_Controls.comboBox->currentIndex() == 3) // split images RGBA
  {
    std::vector<mitk::Image::Pointer> result;
    for (unsigned int c : {0, 1, 2, 3})
    {
      auto componentImage = ConvertRGBAToImage(itkImage, c);
      mitk::Image::Pointer image;
      mitk::CastToMitkImage(componentImage, image);
      result.push_back(image);
    }
    return result;
  }
  else if (m_Controls.comboBox->currentIndex() == 4) // Channel 0
  {
    auto componentImage = ConvertRGBAToImage(itkImage, 0);
    mitk::Image::Pointer image;
    mitk::CastToMitkImage(componentImage, image);
    return {image};
  }
  else if (m_Controls.comboBox->currentIndex() == 5) // Channel 1
  {
    auto componentImage = ConvertRGBAToImage(itkImage, 1);
    mitk::Image::Pointer image;
    mitk::CastToMitkImage(componentImage, image);
    return {image};
  }
  else if (m_Controls.comboBox->currentIndex() == 6) // Channel 2
  {
    auto componentImage = ConvertRGBAToImage(itkImage, 2);
    mitk::Image::Pointer image;
    mitk::CastToMitkImage(componentImage, image);
    return {image};
  }
  else if (m_Controls.comboBox->currentIndex() == 7) // Channel 3
  {
    auto componentImage = ConvertRGBAToImage(itkImage, 3);
    mitk::Image::Pointer image;
    mitk::CastToMitkImage(componentImage, image);
    return {image};
  }
  else
  {
    using FilterType = itk::RGBToLuminanceImageFilter<ItkRGBAImageType, itk::Image<unsigned char, 3>>;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput(itkImage);
    filter->Update();

    mitk::Image::Pointer image;
    mitk::CastToMitkImage(filter->GetOutput(), image);
    return {image};
  }
  return {};
}
