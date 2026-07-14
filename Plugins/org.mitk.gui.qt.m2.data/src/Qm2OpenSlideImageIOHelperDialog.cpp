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
#include "Qm2OpenSlideImageIOHelperDialog.h"

#include "GL/gl.h"
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QOffscreenSurface>
#include <QPixmap>
#include <QTransform>
#include <QVBoxLayout>
#include <QmitkNodeDescriptorManager.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <algorithm>
#include <cmath>
#include <itkImageDuplicator.h>
#include <itkImageFileReader.h>
#include <itkMetaDataObject.h>
#include <itkRGBToLuminanceImageFilter.h>
#include <itkRGBToVectorImageAdaptor.h>
#include <itkVectorImage.h>
#include <itkVectorImageToImageAdaptor.h>
#include <limits>
#include <mitkAnatomicalPlanes.h>
#include <mitkDataNode.h>
#include <mitkExtractSliceFilter.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImageStatisticsHolder.h>
#include <mitkItkImageIO.h>
#include <mitkNodePredicateDataType.h>
#include <qpushbutton.h>
#include <vtkLookupTable.h>
#include <vtkMitkLevelWindowFilter.h>

namespace
{
  constexpr unsigned int kMaxOpenSlideTileSize = 1u << 13;
}

QPixmap GetPixmapFromImageNode(const mitk::Image *image, int height)
{
  mitk::PlaneGeometry::Pointer planeGeometry = mitk::PlaneGeometry::New();
  int sliceNumber = image->GetDimension(2) / 2;
  planeGeometry->InitializeStandardPlane(image->GetGeometry(), mitk::AnatomicalPlane::Axial, sliceNumber);

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

void Qm2OpenSlideImageIOHelperDialog::SetDataStorage(mitk::DataStorage *ds)
{
  m_DataStorage = ds;
  const auto predicate = mitk::TNodePredicateDataType<mitk::Image>::New();
  for (auto *w : m_OriginNodeSelects)
  {
    w->SetDataStorage(ds);
    w->SetNodePredicate(predicate);
  }
}

void Qm2OpenSlideImageIOHelperDialog::SetOpenSlideImageIOHelperObject(m2::OpenSlideImageIOHelperObject *helper)
{
  m_Helper = helper;
}

unsigned int Qm2OpenSlideImageIOHelperDialog::GetMaxTextureSize()
{
  if (m_MaxTextureSize != 0)
    return m_MaxTextureSize;

  int value = 0;

  auto queryTextureSize = [&value]()
  {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
    return value > 0;
  };

  if (QOpenGLContext::currentContext() != nullptr)
  {
    queryTextureSize();
  }
  else if (auto *shareContext = QOpenGLContext::globalShareContext(); shareContext != nullptr)
  {
    QOffscreenSurface surface;
    surface.setFormat(shareContext->format());
    surface.create();
    if (surface.isValid() && shareContext->makeCurrent(&surface))
    {
      queryTextureSize();
      shareContext->doneCurrent();
    }
  }

  if (value <= 0)
    value = static_cast<int>(kMaxOpenSlideTileSize);

  m_MaxTextureSize = static_cast<unsigned int>(value);
  return m_MaxTextureSize;
}

std::vector<Qm2OpenSlideImageIOHelperDialog::ImportSelection>
  Qm2OpenSlideImageIOHelperDialog::GetSelectedImportSelections() const
{
  std::vector<ImportSelection> selections;

  if (m_Controls.chkImportRGBA->isChecked())
    selections.push_back(ImportSelection::Rgba);
  if (m_Controls.chkImportVector->isChecked())
    selections.push_back(ImportSelection::Vector);
  if (m_Controls.chkImportSeparatedRgb->isChecked())
    selections.push_back(ImportSelection::SeparatedRgb);
  if (m_Controls.chkImportSeparatedRgba->isChecked())
    selections.push_back(ImportSelection::SeparatedRgba);
  if (m_Controls.chkImportChannel0->isChecked())
    selections.push_back(ImportSelection::Channel0);
  if (m_Controls.chkImportChannel1->isChecked())
    selections.push_back(ImportSelection::Channel1);
  if (m_Controls.chkImportChannel2->isChecked())
    selections.push_back(ImportSelection::Channel2);
  if (m_Controls.chkImportChannel3->isChecked())
    selections.push_back(ImportSelection::Channel3);
  if (m_Controls.chkImportLuminance->isChecked())
    selections.push_back(ImportSelection::Luminance);

  return selections;
}

bool Qm2OpenSlideImageIOHelperDialog::HasSelectedImportSelections() const
{
  return !GetSelectedImportSelections().empty();
}

void Qm2OpenSlideImageIOHelperDialog::UpdateAcceptButtonState()
{
  if (auto *okButton = m_Controls.buttonBox->button(QDialogButtonBox::Ok))
    okButton->setEnabled(m_SelectedLevel >= 0 && HasSelectedImportSelections());
  if (m_SaveGeoJsonBtn)
    m_SaveGeoJsonBtn->setEnabled(m_SelectedLevel >= 0 && m_Controls.labelImage->hasAnySelection());
  if (m_LoadGeoJsonBtn)
    m_LoadGeoJsonBtn->setEnabled(m_SelectedLevel >= 0);
}

bool Qm2OpenSlideImageIOHelperDialog::TryGetSelectedRegion(ItkRGBAImageType::RegionType &region,
                                                           int selectionIndex) const
{
  if (selectionIndex < 0 || selectionIndex >= m_Controls.labelImage->selectionCount() || m_SelectedLevel < 0)
    return false;

  const auto levelIt = m_LevelDimensions.find(static_cast<unsigned int>(m_SelectedLevel));
  if (levelIt == m_LevelDimensions.end())
    return false;

  const QPixmap &thumbPixmap = m_Controls.labelImage->pixmap();
  if (thumbPixmap.isNull())
    return false;

  QRectF selectionRect = QRectF(m_Controls.labelImage->selectionAt(selectionIndex).rect);
  if (!selectionRect.isValid())
    return false;

  if (m_Controls.chkMirrorX->isChecked())
    selectionRect.moveLeft(thumbPixmap.width() - (selectionRect.left() + selectionRect.width()));

  if (m_Controls.chkMirrorY->isChecked())
    selectionRect.moveTop(thumbPixmap.height() - (selectionRect.top() + selectionRect.height()));

  const auto levelSize = levelIt->second;
  const double scaleX = static_cast<double>(levelSize.width()) / static_cast<double>(thumbPixmap.width());
  const double scaleY = static_cast<double>(levelSize.height()) / static_cast<double>(thumbPixmap.height());

  const int x0 = std::clamp(static_cast<int>(std::floor(selectionRect.left() * scaleX)), 0, levelSize.width() - 1);
  const int y0 = std::clamp(static_cast<int>(std::floor(selectionRect.top() * scaleY)), 0, levelSize.height() - 1);
  const int x1 = std::clamp(static_cast<int>(std::ceil(selectionRect.right() * scaleX)), x0 + 1, levelSize.width());
  const int y1 = std::clamp(static_cast<int>(std::ceil(selectionRect.bottom() * scaleY)), y0 + 1, levelSize.height());

  auto index = region.GetIndex();
  auto size = region.GetSize();
  index[0] = x0;
  index[1] = y0;
  index[2] = 0;
  size[0] = x1 - x0;
  size[1] = y1 - y0;
  size[2] = 1;
  region.SetIndex(index);
  region.SetSize(size);
  return true;
}

void Qm2OpenSlideImageIOHelperDialog::UpdateStatusText()
{
  if (m_SelectedLevel < 0)
  {
    m_Controls.warningLabel->clear();
    return;
  }

  const auto levelIt = m_LevelDimensions.find(static_cast<unsigned int>(m_SelectedLevel));
  if (levelIt == m_LevelDimensions.end())
  {
    m_Controls.warningLabel->clear();
    return;
  }

  const int maxTextureSize = static_cast<int>(GetMaxTextureSize());
  const int nSel = m_Controls.labelImage->selectionCount();

  QStringList lines;
  int worstWidth = levelIt->second.width();
  int worstHeight = levelIt->second.height();

  if (nSel == 0)
  {
    lines << QString("Selection: whole level (%1 x %2)").arg(worstWidth).arg(worstHeight);
  }
  else if (nSel == 1)
  {
    ItkRGBAImageType::RegionType region;
    region.SetSize({static_cast<itk::SizeValueType>(levelIt->second.width()),
                    static_cast<itk::SizeValueType>(levelIt->second.height()),
                    1});
    region.SetIndex({0, 0, 0});
    if (TryGetSelectedRegion(region, 0))
    {
      const auto index = region.GetIndex();
      const auto size = region.GetSize();
      worstWidth = static_cast<int>(size[0]);
      worstHeight = static_cast<int>(size[1]);
      lines << QString("Selection: x=%1 y=%2 width=%3 height=%4")
                 .arg(static_cast<int>(index[0]))
                 .arg(static_cast<int>(index[1]))
                 .arg(worstWidth)
                 .arg(worstHeight);
    }
  }
  else
  {
    worstWidth = 0;
    worstHeight = 0;
    for (int r = 0; r < nSel; ++r)
    {
      ItkRGBAImageType::RegionType region;
      region.SetSize({static_cast<itk::SizeValueType>(levelIt->second.width()),
                      static_cast<itk::SizeValueType>(levelIt->second.height()),
                      1});
      region.SetIndex({0, 0, 0});
      if (TryGetSelectedRegion(region, r))
      {
        worstWidth = std::max(worstWidth, static_cast<int>(region.GetSize()[0]));
        worstHeight = std::max(worstHeight, static_cast<int>(region.GetSize()[1]));
      }
    }
    lines << QString("%1 region(s) selected").arg(nSel);
  }

  m_TilesX = worstWidth / maxTextureSize + int((worstWidth % maxTextureSize) > 0);
  m_TilesY = worstHeight / maxTextureSize + int((worstHeight % maxTextureSize) > 0);

  QString warn;
  if (worstWidth > maxTextureSize)
    warn += "Division in x direction into " + QString::number(m_TilesX) + " parts";

  if (worstHeight > maxTextureSize)
  {
    if (!warn.isEmpty())
      warn += "  |  ";
    warn += "Division in y direction into " + QString::number(m_TilesY) + " parts";
  }

  if (!warn.isEmpty())
    lines << warn;

  m_Controls.warningLabel->setText(lines.join("  |  "));
}

void Qm2OpenSlideImageIOHelperDialog::loadGeoJson()
{
  const QFileInfo fi(QString::fromStdString(m_Helper->GetOpenSlideIO()->GetFileName()));
  const QString defaultPath = fi.path() + "/" + fi.completeBaseName() + ".json";

  const QString path = QFileDialog::getOpenFileName(
    this, tr("Load GeoJSON"), defaultPath, tr("GeoJSON files (*.json *.geojson);;All files (*)"));
  if (path.isEmpty())
    return;

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this, tr("Load GeoJSON"), tr("Could not open:\n") + path);
    return;
  }
  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  file.close();

  if (doc.isNull() || !doc.object().contains("features"))
  {
    QMessageBox::warning(this, tr("Load GeoJSON"), tr("Not a valid GeoJSON FeatureCollection."));
    return;
  }

  const QPixmap &pixmap = m_Controls.labelImage->pixmap();
  if (pixmap.isNull() || m_SelectedLevel < 0)
  {
    QMessageBox::warning(this, tr("Load GeoJSON"), tr("No image loaded. Select a level first."));
    return;
  }

  const auto levelIt = m_LevelDimensions.find(static_cast<unsigned int>(m_SelectedLevel));
  if (levelIt == m_LevelDimensions.end())
    return;

  const double scaleX = static_cast<double>(levelIt->second.width()) / static_cast<double>(pixmap.width());
  const double scaleY = static_cast<double>(levelIt->second.height()) / static_cast<double>(pixmap.height());
  const bool mirrorX = m_Controls.chkMirrorX->isChecked();
  const bool mirrorY = m_Controls.chkMirrorY->isChecked();

  QVector<QRect> rects;
  QVector<QString> names;

  for (const auto &featureVal : doc.object()["features"].toArray())
  {
    const QJsonObject geom = featureVal.toObject()["geometry"].toObject();
    if (geom["type"].toString() != "Polygon")
      continue;

    // Compute bounding box from the outer ring
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    for (const auto &ptVal : geom["coordinates"].toArray().first().toArray())
    {
      const auto pt = ptVal.toArray();
      minX = std::min(minX, pt[0].toDouble());
      minY = std::min(minY, pt[1].toDouble());
      maxX = std::max(maxX, pt[0].toDouble());
      maxY = std::max(maxY, pt[1].toDouble());
    }

    // Inverse of TryGetSelectedRegion: image coords → pixmap-local coords
    double pLeft, pRight, pTop, pBottom;
    if (mirrorX)
    {
      pLeft = pixmap.width() - maxX / scaleX;
      pRight = pixmap.width() - minX / scaleX;
    }
    else
    {
      pLeft = minX / scaleX;
      pRight = maxX / scaleX;
    }
    if (mirrorY)
    {
      pTop = pixmap.height() - maxY / scaleY;
      pBottom = pixmap.height() - minY / scaleY;
    }
    else
    {
      pTop = minY / scaleY;
      pBottom = maxY / scaleY;
    }

    rects.append(QRect(QPoint(static_cast<int>(pLeft), static_cast<int>(pTop)),
                       QPoint(static_cast<int>(pRight), static_cast<int>(pBottom)))
                   .normalized());

    const QJsonObject props = featureVal.toObject()["properties"].toObject();
    names.append(props["name"].toString());
  }

  // setSelections emits selectionsChanged() → syncRegionNameInputs() creates the QLineEdits
  m_Controls.labelImage->setSelections(rects);

  // Populate name inputs after sync
  for (int i = 0; i < names.size() && i < m_RegionNameEdits.size(); ++i)
    m_RegionNameEdits[i]->setText(names[i]);
}

void Qm2OpenSlideImageIOHelperDialog::saveGeoJson()
{
  const int nSel = m_Controls.labelImage->selectionCount();
  if (nSel == 0 || m_SelectedLevel < 0)
    return;

  const auto levelIt = m_LevelDimensions.find(static_cast<unsigned int>(m_SelectedLevel));
  if (levelIt == m_LevelDimensions.end())
    return;

  QJsonArray features;
  for (int r = 0; r < nSel; ++r)
  {
    ItkRGBAImageType::RegionType region;
    region.SetSize({static_cast<itk::SizeValueType>(levelIt->second.width()),
                    static_cast<itk::SizeValueType>(levelIt->second.height()),
                    1});
    region.SetIndex({0, 0, 0});
    if (!TryGetSelectedRegion(region, r))
      continue;

    const int x0 = static_cast<int>(region.GetIndex()[0]);
    const int y0 = static_cast<int>(region.GetIndex()[1]);
    const int x1 = x0 + static_cast<int>(region.GetSize()[0]);
    const int y1 = y0 + static_cast<int>(region.GetSize()[1]);

    // Closed ring: top-left → top-right → bottom-right → bottom-left → top-left
    QJsonArray ring;
    const std::array<std::pair<int, int>, 5> pts = {{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}, {x0, y0}}};
    for (const auto &[px, py] : pts)
      ring.append(QJsonArray{px, py});

    const std::string name = GetRegionName(r);
    QJsonObject props;
    props["name"] = name.empty() ? QString("region_%1").arg(r + 1) : QString::fromStdString(name);

    QJsonObject geometry;
    geometry["type"] = "Polygon";
    geometry["coordinates"] = QJsonArray{ring};

    QJsonObject feature;
    feature["type"] = "Feature";
    feature["properties"] = props;
    feature["geometry"] = geometry;
    features.append(feature);
  }

  QJsonObject root;
  root["type"] = "FeatureCollection";
  root["features"] = features;

  const QFileInfo fi(QString::fromStdString(m_Helper->GetOpenSlideIO()->GetFileName()));
  const QString jsonPath = fi.path() + "/" + fi.completeBaseName() + ".json";

  QFile file(jsonPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QMessageBox::warning(this, tr("Save GeoJSON"), tr("Could not write to:\n") + jsonPath);
    return;
  }
  file.write(QJsonDocument(root).toJson());
  file.close();
  QMessageBox::information(this, tr("Save GeoJSON"), tr("Saved:\n") + jsonPath);
}

void Qm2OpenSlideImageIOHelperDialog::syncRegionNameInputs()
{
  const int n = m_Controls.labelImage->selectionCount();

  // Remove widgets for selections that no longer exist (from the bottom)
  while (m_RegionNameEdits.size() > n)
  {
    const int last = m_RegionNameEdits.size() - 1;
    QLayoutItem *item = m_RegionNamesLayout->takeAt(last);
    if (item)
    {
      if (item->widget())
        item->widget()->deleteLater();
      delete item;
    }
    m_RegionNameEdits.removeLast();
    m_OriginNodeSelects.removeLast();
  }

  // Add widgets for new selections
  while (m_RegionNameEdits.size() < n)
  {
    const int idx = m_RegionNameEdits.size();
    const QColor color = m_Controls.labelImage->selectionAt(idx).color;

    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(4);

    auto *swatch = new QFrame;
    swatch->setFixedSize(14, 14);
    swatch->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(color.name()));

    auto *edit = new QLineEdit;
    edit->setPlaceholderText(QString("region_%1").arg(idx + 1));

    auto *nodeSelect = new QmitkSingleNodeSelectionWidget;
    nodeSelect->SetSelectionIsOptional(true);
    nodeSelect->SetEmptyInfo(QStringLiteral("(no reference)"));
    nodeSelect->SetPopUpTitel(QStringLiteral("Select reference image for center alignment"));
    nodeSelect->setToolTip(QStringLiteral("Select a reference image — the loaded region will be centered on it"));
    if (m_DataStorage)
    {
      nodeSelect->SetDataStorage(m_DataStorage);
      nodeSelect->SetNodePredicate(mitk::TNodePredicateDataType<mitk::Image>::New());
    }

    rowLayout->addWidget(swatch);
    rowLayout->addWidget(edit);
    rowLayout->addWidget(nodeSelect, 1);

    m_RegionNamesLayout->addWidget(row);
    m_RegionNameEdits.append(edit);
    m_OriginNodeSelects.append(nodeSelect);
  }
}

std::string Qm2OpenSlideImageIOHelperDialog::GetRegionName(int index) const
{
  if (index < 0 || index >= m_RegionNameEdits.size())
    return {};
  return m_RegionNameEdits[index]->text().trimmed().toStdString();
}

void Qm2OpenSlideImageIOHelperDialog::UpdatePixmap()
{
  if (m_LowResImage.IsNull())
  {
    m_Controls.labelImage->clearPixmap();
    return;
  }

  auto pixmap = GetPixmapFromImageNode(m_LowResImage, m_Controls.spinThumbnailSize->value());
  if (m_Controls.chkMirrorX->isChecked())
    pixmap = pixmap.transformed(QTransform().scale(-1, 1));
  if (m_Controls.chkMirrorY->isChecked())
    pixmap = pixmap.transformed(QTransform().scale(1, -1));

  m_Controls.labelImage->setPixmap(pixmap);
}

void Qm2OpenSlideImageIOHelperDialog::UpdateImageInformation()
{
  auto IO = m_Helper->GetOpenSlideIO();

  IO->SetLevel(IO->GetLevelCount() - 1);
  mitk::ItkImageIO reader(IO);
  reader.mitk::AbstractFileReader::SetInput(IO->GetFileName());
  auto dataList = reader.Read();
  m_LowResImage = dynamic_cast<mitk::Image *>(dataList.back().GetPointer());

  m_Controls.labelImage->clearPixmap();
  m_LevelDimensions.clear();

  UpdatePixmap();
  UpdateAcceptButtonState();

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
    m_LevelDimensions[kv.first] = QSize(std::stoi(inf.width), std::stoi(inf.height));
  }

  m_Controls.imageSelectionList->clear();
  for (const auto &kv : levelInfos)
  {
    auto item = new QListWidgetItem();
    item->setText(QString(("Level [" + kv.second.level + "] " + kv.second.width + " x " + kv.second.height).c_str()));
    item->setData(Qt::UserRole, (unsigned int)(kv.first));

    m_Controls.imageSelectionList->addItem(item);
  }

  // Preselect the first item
  m_Controls.imageSelectionList->setCurrentRow(0);
}

Qm2OpenSlideImageIOHelperDialog::ItkRGBAImageType::Pointer Qm2OpenSlideImageIOHelperDialog::ExtractBufferedPatch(
  ItkRGBAImageType *rgbaImage)
{
  const auto bufferedRegion = rgbaImage->GetBufferedRegion();
  const auto inputSpacing = rgbaImage->GetSpacing();
  const auto inputOrigin = rgbaImage->GetOrigin();
  const auto inputDirection = rgbaImage->GetDirection();
  const auto inputIndex = bufferedRegion.GetIndex();

  ItkRGBAImageType::PointType patchOrigin = inputOrigin;
  for (unsigned int row = 0; row < ItkRGBAImageType::ImageDimension; ++row)
  {
    for (unsigned int column = 0; column < ItkRGBAImageType::ImageDimension; ++column)
      patchOrigin[row] += inputDirection[row][column] * inputSpacing[column] * static_cast<double>(inputIndex[column]);
  }

  ItkRGBAImageType::RegionType outputRegion;
  outputRegion.SetIndex({0, 0, 0});
  outputRegion.SetSize(bufferedRegion.GetSize());

  auto patchImage = ItkRGBAImageType::New();
  patchImage->SetRegions(outputRegion);
  patchImage->SetOrigin(patchOrigin);
  patchImage->SetSpacing(inputSpacing);
  patchImage->SetDirection(inputDirection);
  patchImage->Allocate();

  itk::ImageRegionConstIterator<ItkRGBAImageType> inputIterator(rgbaImage, bufferedRegion);
  itk::ImageRegionIterator<ItkRGBAImageType> outputIterator(patchImage, outputRegion);

  for (inputIterator.GoToBegin(), outputIterator.GoToBegin(); !inputIterator.IsAtEnd();
       ++inputIterator, ++outputIterator)
    outputIterator.Set(inputIterator.Get());

  return patchImage;
}

mitk::Image::Pointer Qm2OpenSlideImageIOHelperDialog::GetPreviewData()
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

std::vector<Qm2OpenSlideImageIOHelperDialog::ImportedImage> Qm2OpenSlideImageIOHelperDialog::GetData()
{
  auto IO = m_Helper->GetOpenSlideIO();
  IO->SetLevel(this->GetSelectedLevel());

  const auto importSelections = GetSelectedImportSelections();
  const int nRegions = m_Controls.labelImage->selectionCount();

  std::vector<ImportedImage> result;

  // Loads one region (regionIndex < 0 = whole image) and appends to result.
  // Each type suffix is prefixed with regionPrefix (e.g. "tumor_" for multi-region).
  const auto loadRegion = [&](int regionIndex)
  {
    // For level L > 0, CanStreamRead() may return false when the level dimensions are
    // coprime with the level-0 dimensions, causing ITK to ignore SetRequestedRegion()
    // and load the full level.  Enabling approximate streaming makes CanStreamRead()
    // always return true and bypasses the strict grid-alignment requirement so the
    // requested sub-region is passed directly to openslide_read_region().
    if (regionIndex >= 0 && IO->GetLevel() != 0)
      IO->SetApproximateStreaming(true);

    auto reader = itk::ImageFileReader<ItkRGBAImageType>::New();
    reader->SetImageIO(IO);
    reader->SetFileName(IO->GetFileName());
    reader->UpdateOutputInformation();

    auto requestedRegion = reader->GetOutput()->GetLargestPossibleRegion();
    if (regionIndex >= 0 && TryGetSelectedRegion(requestedRegion, regionIndex))
      reader->GetOutput()->SetRequestedRegion(requestedRegion);

    reader->Update();
    ItkRGBAImageType::Pointer itkImage = reader->GetOutput();

    if (itkImage->GetBufferedRegion().GetIndex() != itkImage->GetLargestPossibleRegion().GetIndex() ||
        itkImage->GetBufferedRegion().GetSize() != itkImage->GetLargestPossibleRegion().GetSize())
      itkImage = ExtractBufferedPatch(itkImage);

    auto s = itkImage->GetSpacing();
    s[0] *= 1e-3;
    s[1] *= 1e-3;
    s[2] = this->GetSliceThickness();
    itkImage->SetSpacing(s);

    auto origin = itkImage->GetOrigin();
    origin[0] *= 1e-3;
    origin[1] *= 1e-3;
    itkImage->SetOrigin(origin);

    // Center-align to reference image if one is selected for this region.
    // The loaded region's bounding-box center is shifted to coincide with
    // the reference image's bounding-box center.
    if (regionIndex >= 0 && regionIndex < static_cast<int>(m_OriginNodeSelects.size()))
    {
      if (auto refNode = m_OriginNodeSelects[regionIndex]->GetSelectedNode())
      {
        if (auto refImg = dynamic_cast<mitk::Image *>(refNode->GetData()))
        {
          const auto refCenter = refImg->GetGeometry()->GetCenter();

          const auto sz = itkImage->GetLargestPossibleRegion().GetSize();
          const auto sp = itkImage->GetSpacing();

          // Reset direction to identity so downstream geometry is clean.
          ItkRGBAImageType::DirectionType identityDir;
          identityDir.SetIdentity();
          itkImage->SetDirection(identityDir);

          // SubdivideImage2DFilter mirrors by negating M[d][d] without adjusting
          // the origin.  This shifts the effective world-space center by -sp*sz in
          // the mirrored dimension.  Pre-compensate: use +sz/2 (instead of -sz/2)
          // as the half-extent offset in dimensions that will be mirrored, so that
          // after negation the center still lands on refCenter.
          const bool willMirrorX = GetMirrorX();
          const bool willMirrorY = GetMirrorY();

          ItkRGBAImageType::PointType newOrigin;
          for (unsigned int d = 0; d < 2; ++d) // XY only — keep Z at 0
          {
            const bool willMirror = (d == 0) ? willMirrorX : willMirrorY;
            newOrigin[d] = refCenter[d] + (willMirror ? +1.0 : -1.0) * sp[d] * static_cast<double>(sz[d]) * 0.5;
          }
          newOrigin[2] = 0.0;

          itkImage->SetOrigin(newOrigin);
        }
      }
    }
    if (auto refNode = m_OriginNodeSelects[regionIndex]->GetSelectedNode())
    {
      for (const auto sel : importSelections)
      {
        if (sel == ImportSelection::Rgba)
        {
          mitk::Image::Pointer image;
          mitk::CastToMitkImage(itkImage, image);
          result.push_back({refNode->GetName() + ".HE_rgba", image});
        }
        else if (sel == ImportSelection::Vector)
        {
          auto vectorImage = ConvertRGBAToVectorImage(itkImage);
          mitk::Image::Pointer image;
          mitk::CastToMitkImage(vectorImage, image);
          result.push_back({refNode->GetName() + ".HE_vector", image});
        }
        else if (sel == ImportSelection::SeparatedRgb)
        {
          for (unsigned int c : {0u, 1u, 2u})
          {
            auto componentImage = ConvertRGBAToImage(itkImage, c);
            mitk::Image::Pointer image;
            mitk::CastToMitkImage(componentImage, image);
            result.push_back({refNode->GetName() + ".HE_rgb_" + std::to_string(c), image});
          }
        }
        else if (sel == ImportSelection::SeparatedRgba)
        {
          for (unsigned int c : {0u, 1u, 2u, 3u})
          {
            auto componentImage = ConvertRGBAToImage(itkImage, c);
            mitk::Image::Pointer image;
            mitk::CastToMitkImage(componentImage, image);
            result.push_back({refNode->GetName() + ".HE_rgba_" + std::to_string(c), image});
          }
        }
        else if (sel == ImportSelection::Channel0)
        {
          auto componentImage = ConvertRGBAToImage(itkImage, 0);
          mitk::Image::Pointer image;
          mitk::CastToMitkImage(componentImage, image);
          result.push_back({refNode->GetName() + ".HE_channel0", image});
        }
        else if (sel == ImportSelection::Channel1)
        {
          auto componentImage = ConvertRGBAToImage(itkImage, 1);
          mitk::Image::Pointer image;
          mitk::CastToMitkImage(componentImage, image);
          result.push_back({refNode->GetName() + ".HE_channel1", image});
        }
        else if (sel == ImportSelection::Channel2)
        {
          auto componentImage = ConvertRGBAToImage(itkImage, 2);
          mitk::Image::Pointer image;
          mitk::CastToMitkImage(componentImage, image);
          result.push_back({refNode->GetName() + ".HE_channel2", image});
        }
        else if (sel == ImportSelection::Channel3)
        {
          auto componentImage = ConvertRGBAToImage(itkImage, 3);
          mitk::Image::Pointer image;
          mitk::CastToMitkImage(componentImage, image);
          result.push_back({refNode->GetName() + ".HE_channel3", image});
        }
        else
        {
          using FilterType = itk::RGBToLuminanceImageFilter<ItkRGBAImageType, itk::Image<unsigned char, 3>>;
          FilterType::Pointer filter = FilterType::New();
          filter->SetInput(itkImage);
          filter->Update();
          mitk::Image::Pointer image;
          mitk::CastToMitkImage(filter->GetOutput(), image);
          result.push_back({refNode->GetName() + ".HE_lum", image});
        }
      }
    }
  };

  if (nRegions <= 1)
  {
    // 0 regions: whole image; 1 region: single selection.
    // Use empty prefix so that the caller's GetRegionName() appends the name externally
    // (preserving backward-compatible naming for the single-region case).
    loadRegion(nRegions == 1 ? 0 : -1);
  }
  else
  {
    // Multiple regions: embed region name in the suffix prefix.
    for (int r = 0; r < nRegions; ++r)
    {
      loadRegion(r);
    }
  }

  return result;
}
