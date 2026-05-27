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
#include "Qm2ThumbnailWidget.h"
#include "ui_Qm2OpenSlideImageIOHelperDialog.h"
#include <QPushButton>
#include <QDialog>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QVector>
#include <string>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <m2OpenSlideImageIOHelperObject.h>
#include <mitkDataStorage.h>
#include <vector>

class QmitkSingleNodeSelectionWidget;

class Qm2OpenSlideImageIOHelperDialog : public QDialog
{
  Q_OBJECT

public:
  struct ImportedImage
  {
    std::string nameSuffix;
    mitk::Image::Pointer image;
  };

  enum class ImportSelection
  {
    Rgba,
    Vector,
    SeparatedRgb,
    SeparatedRgba,
    Channel0,
    Channel1,
    Channel2,
    Channel3,
    Luminance
  };

  explicit Qm2OpenSlideImageIOHelperDialog(QWidget *parent = nullptr) : QDialog(parent)
  {
    m_Controls.setupUi(this);
    this->setSizeGripEnabled(true);

    connect(m_Controls.labelImage,
            &Qm2ThumbnailWidget::selectionsChanged,
            this,
            [this]() {
              syncRegionNameInputs();
              UpdateStatusText();
              UpdateAcceptButtonState();
            });

    m_RegionNamesLayout = new QVBoxLayout(m_Controls.regionNamesWidget);
    m_RegionNamesLayout->setContentsMargins(0, 0, 0, 0);
    m_RegionNamesLayout->setSpacing(2);

    connect(m_Controls.spinThumbnailSize,
          qOverload<int>(&QSpinBox::valueChanged),
          this,
          [this](int) { UpdatePixmap(); });

    connect(m_Controls.chkMirrorX, &QCheckBox::toggled, this, [this](bool) {
      m_Controls.labelImage->clearAllSelections();
      UpdatePixmap();
    });

    connect(m_Controls.chkMirrorY, &QCheckBox::toggled, this, [this](bool) {
      m_Controls.labelImage->clearAllSelections();
      UpdatePixmap();
    });

    connect(m_Controls.thickness,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            [this](double value) { m_SliceThickness = value; });

    connect(m_Controls.imageSelectionList,
            &QListWidget::currentItemChanged,
            this,
            [this](QListWidgetItem *item, QListWidgetItem *) {
              if (item == nullptr)
              {
                m_SelectedLevel = -1;
                UpdateStatusText();
                return;
              }

              m_SelectedLevel = item->data(Qt::UserRole).toInt();
              m_SliceThickness = m_Controls.thickness->value();
              UpdateStatusText();
              UpdateAcceptButtonState();
            });

    for (auto *checkBox : {m_Controls.chkImportRGBA,
                           m_Controls.chkImportVector,
                           m_Controls.chkImportSeparatedRgb,
                           m_Controls.chkImportSeparatedRgba,
                           m_Controls.chkImportChannel0,
                           m_Controls.chkImportChannel1,
                           m_Controls.chkImportChannel2,
                           m_Controls.chkImportChannel3,
                           m_Controls.chkImportLuminance})
    {
      connect(checkBox, &QCheckBox::toggled, this, [this](bool) { UpdateAcceptButtonState(); });
    }

    UpdatePixmap();
    UpdateAcceptButtonState();

    m_SaveGeoJsonBtn = m_Controls.buttonBox->addButton(tr("Save GeoJSON"), QDialogButtonBox::ActionRole);
    m_SaveGeoJsonBtn->setEnabled(false);
    connect(m_SaveGeoJsonBtn, &QPushButton::clicked, this, [this]() { saveGeoJson(); });

    m_LoadGeoJsonBtn = m_Controls.buttonBox->addButton(tr("Load GeoJSON"), QDialogButtonBox::ActionRole);
    m_LoadGeoJsonBtn->setEnabled(false);
    connect(m_LoadGeoJsonBtn, &QPushButton::clicked, this, [this]() { loadGeoJson(); });
  }

  void SetOpenSlideImageIOHelperObject(m2::OpenSlideImageIOHelperObject *helper);
  void SetDataStorage(mitk::DataStorage *ds);
  void UpdateImageInformation();
  void UpdatePixmap();

  int exec() override
  {
    UpdateImageInformation();
    UpdateAcceptButtonState();
    return QDialog::exec();
  }

  int GetSelectedLevel() { return m_SelectedLevel; }
  double GetSliceThickness() { return m_SliceThickness; }
  std::vector<ImportedImage> GetData();
  mitk::Image::Pointer GetPreviewData();

  bool GetMirrorX() { return m_Controls.chkMirrorX->isChecked(); }
  bool GetMirrorY() { return m_Controls.chkMirrorY->isChecked(); }
  unsigned int GetMaxTextureSize();
  /** Returns the region name for the first (and only) selection, or empty string.
   *  For multi-region imports the name is already embedded in the nameSuffix of GetData(). */
  std::string GetRegionName()
  {
    return m_RegionNameEdits.size() == 1 ? m_RegionNameEdits[0]->text().trimmed().toStdString()
                                         : std::string{};
  }

protected:

private:
  using ItkRGBAImageType = itk::Image<itk::RGBAPixel<unsigned char>, 3>;

  std::vector<ImportSelection> GetSelectedImportSelections() const;
  bool HasSelectedImportSelections() const;
  bool TryGetSelectedRegion(ItkRGBAImageType::RegionType &region, int selectionIndex) const;
  void UpdateAcceptButtonState();
  void UpdateStatusText();
  void syncRegionNameInputs();
  std::string GetRegionName(int index) const;
  void saveGeoJson();
  void loadGeoJson();

  mitk::Image::Pointer m_LowResImage = nullptr;
  m2::OpenSlideImageIOHelperObject::Pointer m_Helper;
  Ui::OpenSlideImageIOHelperDialog m_Controls;
  QVBoxLayout *m_RegionNamesLayout = nullptr;
  QVector<QLineEdit *> m_RegionNameEdits;
  QVector<QmitkSingleNodeSelectionWidget *> m_OriginNodeSelects;
  mitk::DataStorage *m_DataStorage = nullptr;
  QPushButton *m_SaveGeoJsonBtn = nullptr;
  QPushButton *m_LoadGeoJsonBtn = nullptr;
  int m_SelectedLevel = -1;
  int m_TilesX = 1;
  int m_TilesY = 1;
  unsigned int m_MaxTextureSize = 0;
  double m_SliceThickness = 1;
  std::map<unsigned int, QSize> m_LevelDimensions;

  /*This function casts a itk RGBA Image, to a itk varaible lenght vector image with 3 components. */
  static itk::VectorImage<unsigned char, 3>::Pointer ConvertRGBAToVectorImage(
    itk::Image<itk::RGBAPixel<unsigned char>, 3>::Pointer rgbaImage)
  {
    const auto size = rgbaImage->GetLargestPossibleRegion().GetSize();
	itk::VectorImage<unsigned char, 3>::RegionType region;
    region.SetSize(size);
    region.SetIndex({0,0,0});

    auto vectorImage = itk::VectorImage<unsigned char, 3>::New();
    vectorImage->SetRegions(region);
    vectorImage->SetOrigin(rgbaImage->GetOrigin());
    vectorImage->SetSpacing(rgbaImage->GetSpacing());
    vectorImage->SetDirection(rgbaImage->GetDirection());
    vectorImage->SetVectorLength(4);
    vectorImage->Allocate();

    itk::ImageRegionConstIterator<itk::Image<itk::RGBAPixel<unsigned char>, 3>> rgbaIterator(
      rgbaImage, rgbaImage->GetLargestPossibleRegion());

    itk::ImageRegionIterator<itk::VectorImage<unsigned char, 3>> vectorIterator(
      vectorImage, vectorImage->GetLargestPossibleRegion());

    rgbaIterator.GoToBegin();
    vectorIterator.GoToBegin();

    itk::VariableLengthVector<unsigned char> vec;
    vec.SetSize(4);
    while (!rgbaIterator.IsAtEnd())
    {
      memcpy(const_cast<unsigned char *>(vectorIterator.Get().GetDataPointer()), rgbaIterator.Get().GetDataPointer(), 4 * sizeof(unsigned char));
    
      ++vectorIterator;
      ++rgbaIterator;
    }

    return vectorImage;
  }

  /*This function casts a itk RGBA Image, to a itk varaible lenght vector image with 3 components. */
  static itk::Image<unsigned char, 3>::Pointer ConvertRGBAToImage(
    itk::Image<itk::RGBAPixel<unsigned char>, 3>::Pointer rgbaImage, unsigned int component)
  {
  
    const auto size = rgbaImage->GetLargestPossibleRegion().GetSize();

    itk::Image<unsigned char, 3>::RegionType region;
    region.SetSize(size);
    region.SetIndex({0,0,0});

    auto image = itk::Image<unsigned char, 3>::New();
    image->SetRegions(region);
    image->SetOrigin(rgbaImage->GetOrigin());
    image->SetSpacing(rgbaImage->GetSpacing());
    image->SetDirection(rgbaImage->GetDirection());
    image->Allocate();

    itk::ImageRegionConstIterator<itk::Image<itk::RGBAPixel<unsigned char>, 3>> rgbaIterator(
      rgbaImage, rgbaImage->GetLargestPossibleRegion());

    itk::ImageRegionIterator<itk::Image<unsigned char, 3>> imageIterator(image, image->GetLargestPossibleRegion());

    rgbaIterator.GoToBegin();
    imageIterator.GoToBegin();

    while (!rgbaIterator.IsAtEnd())
    {
      imageIterator.Set(rgbaIterator.Get().GetNthComponent(component));
      ++imageIterator;
      ++rgbaIterator;
    }

    return image;
  }

  static ItkRGBAImageType::Pointer ExtractBufferedPatch(ItkRGBAImageType *rgbaImage);
};
