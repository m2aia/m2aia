/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDataCompressionView.h"

#include <QMessageBox>
// #include <berryISelectionService.h>
// #include <berryIWorkbenchWindow.h>

// m2
#include <m2CoreCommon.h>
#include <m2IntervalVector.h>
#include <m2MultiSliceFilter.h>
#include <m2PcaImageFilter.h>
#include <m2SpectrumImage.h>
#include <m2TSNEImageFilter.h>

#include <m2KMeansImageFilter.h>
#include <signal/m2SignalCommon.h>

// mitk
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkNodePredicateNot.h>
#include <mitkLabelSetImage.h>
#include <mitkIOUtil.h>
#include <mitkProgressBar.h>

// std
#include <algorithm>

// itksys
#include <itksys/SystemTools.hxx>

// itk
#include <itkIdentityTransform.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>
#include <itkShrinkImageFilter.h>
#include <itkVectorImageToImageAdaptor.h>
#include <itkVectorImage.h>


// Don't forget to initialize the VIEW_ID.
const std::string QmitkDataCompressionView::VIEW_ID = "org.mitk.views.m2.datacompression";
using DisplayImageType = itk::Image<m2::DisplayImagePixelType, 3>;
using VectorImageAdaptorType = itk::VectorImageToImageAdaptor<m2::DisplayImagePixelType, 3>;
using VectorImageType = itk::VectorImage<m2::DisplayImagePixelType, 3>;

void QmitkDataCompressionView::CreateQtPartControl(QWidget *parent)
{
  m_Parent = parent;
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  auto NodePredicateIsCentroidSpectrum = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData()))
        return ((unsigned int)(intervals->GetType())) & ((unsigned int)(m2::SpectrumFormat::Centroid));
      return false;
    });

  auto NodePredicateIsActiveHelperNode = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) { return node->IsOn("helper object", nullptr, false); });
  auto NodePredicateNoActiveHelper = mitk::NodePredicateNot::New(NodePredicateIsActiveHelperNode);

  m_Controls.imageSelection->SetDataStorage(GetDataStorage());
  m_Controls.imageSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImage>::New(), NodePredicateNoActiveHelper));
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QString("Image selection"));
  m_Controls.imageSelection->SetPopUpTitel(QString("Image"));

  m_Controls.peakListSelection->SetDataStorage(GetDataStorage());
  m_Controls.peakListSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(NodePredicateIsCentroidSpectrum, NodePredicateNoActiveHelper));
  m_Controls.peakListSelection->SetSelectionIsOptional(true);
  m_Controls.peakListSelection->SetEmptyInfo(QString("PeakList selection"));
  m_Controls.peakListSelection->SetPopUpTitel(QString("PeakList"));

  m_Controls.maskSelection->SetDataStorage(GetDataStorage());
  m_Controls.maskSelection->SetNodePredicate(mitk::NodePredicateAnd::New(
    mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New(), NodePredicateNoActiveHelper));
  m_Controls.maskSelection->SetSelectionIsOptional(true);
  m_Controls.maskSelection->SetEmptyInfo(QString("Mask selection"));
  m_Controls.maskSelection->SetPopUpTitel(QString("Mask"));
  
  m_Controls.boxKMeansDistanceMetric->addItem("Euclidean", QVariant(to_underlying(m2::DistanceMetric::EUCLIDEAN)));
  m_Controls.boxKMeansDistanceMetric->addItem("Cosine", QVariant(to_underlying(m2::DistanceMetric::COSINE)));
  m_Controls.boxKMeansDistanceMetric->addItem("Correlation", QVariant(to_underlying(m2::DistanceMetric::CORRELATION)));
  m_Controls.boxKMeansDistanceMetric->setCurrentIndex(0);


  m_Controls.boxKMeansVariant->addItem("Standard", QVariant(to_underlying(m2::KMeansVariant::STANDARD)));
  m_Controls.boxKMeansVariant->addItem("Spatial", QVariant(to_underlying(m2::KMeansVariant::SPATIAL)));
  // m_Controls.boxKMeansVariant->addItem("Bisecting", QVariant(to_underlying(m2::KMeansVariant::BISECTING)));
  // m_Controls.boxKMeansVariant->addItem("Spectral-Spatial", QVariant(to_underlying(m2::KMeansVariant::SPECTRAL_SPATIAL)));
  m_Controls.boxKMeansVariant->setCurrentIndex(0);

  connect(m_Controls.maskSelection,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &QmitkDataCompressionView::OnMaskSelectionChanged);
  OnMaskSelectionChanged();

  connect(m_Controls.btnRunPCA, SIGNAL(clicked()), this, SLOT(OnStartPCA()));
  connect(m_Controls.btnRunKMeans, SIGNAL(clicked()), this, SLOT(OnStartKMeans()));
  connect(m_Controls.btnRunTSNE, SIGNAL(clicked()), this, SLOT(OnStartTSNE()));
  connect(m_Controls.btnSaveDataCompressionResults, SIGNAL(clicked()), this, SLOT(OnSaveDataCompressionResults()));
}

void QmitkDataCompressionView::OnSaveDataCompressionResults()
{
  
  auto selectedNodes = m_Controls.imageSelection->GetSelectedNodesStdVector();
  for (auto node : selectedNodes)
  {
    std::string inputLocation;
    auto m2aiaDataPathProp = node->GetData()->GetProperty("m2aia.IO.path");
    auto dataPathProp = node->GetData()->GetProperty("path");
    if (m2aiaDataPathProp)
      inputLocation = m2aiaDataPathProp->GetValueAsString();
    else if (dataPathProp)      
      inputLocation = dataPathProp->GetValueAsString();
    
    
    if (auto child = this->GetDataStorage()->GetNamedDerivedNode((node->GetName() + ".PCA").c_str(), node)){
      auto pcaImage = dynamic_cast<mitk::Image *>(child->GetData());
      mitk::IOUtil::Save(pcaImage, 
      itksys::SystemTools::GetFilenamePath(inputLocation) + "/" +
      itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + ".PCA.nrrd");
    }

        
    if (auto child = this->GetDataStorage()->GetNamedDerivedNode((node->GetName() + ".tSNE").c_str(), node)){
      auto pcaImage = dynamic_cast<mitk::Image *>(child->GetData());
      mitk::IOUtil::Save(pcaImage, 
      itksys::SystemTools::GetFilenamePath(inputLocation) + "/" +
      itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + ".tSNE.nrrd");
    }
   
    auto predicate = mitk::NodePredicateFunction::New([] (const mitk::DataNode *node) -> bool
    {
      return node->GetName().find("KMeans_") != std::string::npos;
    });

    auto derivations = this->GetDataStorage()->GetDerivations(node, predicate, true);
    for(auto child : *derivations){
      auto pcaImage = dynamic_cast<mitk::Image *>(child->GetData());
      mitk::IOUtil::Save(pcaImage, 
      itksys::SystemTools::GetFilenamePath(inputLocation) + "/" +
      itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + "." + child->GetName() +".nrrd");
    }
  }
  }


void QmitkDataCompressionView::OnMaskSelectionChanged()
{
  using LabelValueType = mitk::MultiLabelSegmentation::LabelValueType;

  // keep the current choice if that label value is still available
  const auto previousData = m_Controls.boxMaskLabelValue->currentData();

  // count for each label value in how many of the selected masks it occurs
  std::map<LabelValueType, unsigned int> labelValueCounts;
  for (auto maskNode : m_Controls.maskSelection->GetSelectedNodesStdVector())
  {
    auto mask = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
    if (!mask)
      continue;

    // a label value is unique within a segmentation, but may be used by several of its groups
    const auto labelValues = mask->GetAllLabelValues();
    for (auto labelValue : std::set<LabelValueType>(labelValues.begin(), labelValues.end()))
      ++labelValueCounts[labelValue];
  }

  m_Controls.boxMaskLabelValue->clear();
  for (const auto &kv : labelValueCounts)
    m_Controls.boxMaskLabelValue->addItem(QString("%1 (%2)").arg(kv.first).arg(kv.second),
                                          QVariant::fromValue(kv.first));

  m_Controls.boxMaskLabelValue->setEnabled(!labelValueCounts.empty());
  if (labelValueCounts.empty())
    return;

  const auto previousIndex = m_Controls.boxMaskLabelValue->findData(previousData);
  m_Controls.boxMaskLabelValue->setCurrentIndex(previousIndex < 0 ? 0 : previousIndex);
}

namespace
{
  /** A mask can only be applied to an image if both cover the same pixel grid. */
  bool HasMatchingGrid(const mitk::Image *image, const mitk::MultiLabelSegmentation *mask)
  {
    const auto &maskDimensions = mask->GetDimensions();
    if (image->GetDimension() != maskDimensions.size())
      return false;

    for (unsigned int i = 0; i < image->GetDimension(); ++i)
      if (image->GetDimensions()[i] != maskDimensions[i])
        return false;

    return true;
  }
}

mitk::MultiLabelSegmentation::LabelValueType QmitkDataCompressionView::GetSelectedMaskLabelValue() const
{
  const auto data = m_Controls.boxMaskLabelValue->currentData();
  return data.isValid() ? data.value<mitk::MultiLabelSegmentation::LabelValueType>()
                        : mitk::MultiLabelSegmentation::UNLABELED_VALUE;
}

mitk::DataNode::ConstPointer QmitkDataCompressionView::GetMaskNode(const mitk::DataNode *imageNode)
{
  auto image = dynamic_cast<mitk::Image *>(imageNode->GetData());
  if (!image)
    return nullptr;

  const auto &derivations = this->GetDataStorage()->GetDerivations(imageNode)->CastToSTLConstContainer();

  mitk::DataNode::ConstPointer candidate;
  for (auto maskNode : m_Controls.maskSelection->GetSelectedNodesStdVector())
  {
    auto mask = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
    if (!mask || !HasMatchingGrid(image, mask))
      continue;

    // a mask below the image in the data hierarchy belongs to it unambiguously
    const auto isDerivation = std::any_of(derivations.begin(),
                                          derivations.end(),
                                          [&maskNode](const auto &node) { return node.GetPointer() == maskNode.GetPointer(); });
    if (isDerivation)
      return maskNode;

    if (candidate.IsNull())
      candidate = maskNode;
  }

  return candidate;
}

mitk::Image::Pointer QmitkDataCompressionView::GetMaskImage(const mitk::DataNode *imageNode)
{
  auto image = dynamic_cast<m2::SpectrumImage *>(imageNode->GetData());
  if (!image)
    return nullptr;

  auto maskNode = GetMaskNode(imageNode);
  if (maskNode.IsNull())
  {
    if (!m_Controls.maskSelection->GetSelectedNodesStdVector().empty())
      MITK_WARN << "None of the selected masks covers the grid of image [" << imageNode->GetName()
                << "]; its own segmentation is used instead.";
    return image->GetMultilabelSegmentation()->GetGroupImage(0);
  }

  auto mask = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
  const auto labelValue = GetSelectedMaskLabelValue();
  if (!mask->ExistLabel(labelValue))
  {
    MITK_WARN << "The mask [" << maskNode->GetName() << "] of image [" << imageNode->GetName()
              << "] does not contain the label value " << labelValue << "; the image is skipped.";
    return nullptr;
  }

  // only the pixels of the chosen label value are kept, all others are masked out
  mitk::Image::Pointer groupImage = mask->GetGroupImage(mask->GetGroupIndexOfLabel(labelValue));
  auto labelValueMask = groupImage->Clone();
  {
    mitk::ImagePixelWriteAccessor<mitk::MultiLabelSegmentation::LabelValueType, 3> acc(labelValueMask);
    auto data = acc.GetData();
    const auto dimensions = labelValueMask->GetDimensions();
    const auto n = dimensions[0] * dimensions[1] * dimensions[2];
    std::transform(data,
                   data + n,
                   data,
                   [labelValue](const auto value) -> mitk::MultiLabelSegmentation::LabelValueType
                   { return value == labelValue ? 1 : 0; });
  }

  return labelValueMask;
}

void QmitkDataCompressionView::SetFocus() {}

void QmitkDataCompressionView::OnStartKMeans()
{

  auto data =m_Controls.boxKMeansDistanceMetric->itemData(m_Controls.boxKMeansDistanceMetric->currentIndex());
  auto metricType = data.value<m2::DistanceMetric>();
  
  auto variantData = m_Controls.boxKMeansVariant->itemData(m_Controls.boxKMeansVariant->currentIndex());
  auto variantType = variantData.value<m2::KMeansVariant>();
  
  MITK_INFO <<  to_underlying(metricType) << " " << to_underlying(variantType);


  m2::KMeansImageFilter::Pointer filter = m2::KMeansImageFilter::New();
  filter->SetNumberOfClusters(m_Controls.kmeans_clusters->value());
  filter->SetDistanceMetric(metricType);
  filter->SetKMeansVariant(variantType);
  filter->SetSpatialWeight(m_Controls.spatialWeight->value());

  // only the pixels of the chosen mask value contribute to the clusters
  std::vector<mitk::DataNode::ConstPointer> selectedNodes;
  unsigned int imageId = 0;
  for (auto imageNode : m_Controls.imageSelection->GetSelectedNodesStdVector())
  {
    auto image = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData());
    if (!image)
      continue;

    auto mask = GetMaskImage(imageNode);
    if (mask.IsNull())
      continue;

    filter->SetInput(image, imageId);
    filter->SetMaskImage(mask, imageId);
    ++imageId;
    selectedNodes.push_back(imageNode);
  }

  std::string vectorNodeNames = "";
  auto vectorNodes = m_Controls.peakListSelection->GetSelectedNodesStdVector();
  // for each selected peak list different clusters are cerated
  for (auto vectorNode : vectorNodes)
  {
    auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
    filter->SetIntervals(vector->GetIntervals());
    vectorNodeNames += vectorNode->GetName() + "_";
  }

  if (selectedNodes.empty() || vectorNodeNames.empty())
    return;

  vectorNodeNames.pop_back();
  filter->GenerateData();

  mitk::MultiLabelSegmentation::ConstLabelVectorType labelVector;
  for( int i = 0; i <= m_Controls.kmeans_clusters->value(); ++i){
    auto label = mitk::Label::New(i, "Cluster " + std::to_string(i));
    if(i == 0){
      label->SetColor(mitk::MakeColor(0,0,0));
    }else{
      label->SetColor(mitk::MakeColor(1.0 * rand() / RAND_MAX, 1.0 * rand() / RAND_MAX, 1.0 * rand() / RAND_MAX));
    }
    labelVector.emplace_back(label);
  }
  

  // the filter keys its outputs by the input id assigned above, so walk the
  // inputs in the same order to pair each image with its own cluster image
  imageId = 0;
  for(auto s : selectedNodes)
  { 
    const auto currentId = imageId++;
    if(auto specImage = dynamic_cast<m2::SpectrumImage* >(s->GetData())){
      auto clusterImage = filter->GetOutput(currentId);
      if(clusterImage.IsNull() || !clusterImage->IsInitialized()){
        MITK_WARN << "No KMeans result for image " << s->GetName() << "; skipping.";
        continue;
      }
      auto mlSeg = specImage->GetMultilabelSegmentation()->Clone();
      mlSeg->RemoveGroup(0);
      mlSeg->InsertGroup(0, clusterImage.GetPointer(), labelVector, "KMeans_" + std::to_string(m_Controls.kmeans_clusters->value()) + "_" + vectorNodeNames);
      auto outputNode = mitk::DataNode::New();
      outputNode->SetData(mlSeg);
      outputNode->SetName("KMeans_" + std::to_string(m_Controls.kmeans_clusters->value()) + "_" + vectorNodeNames);
      this->GetDataStorage()->Add(outputNode, const_cast<mitk::DataNode *>(s.GetPointer()));  
    }
  }
}

void QmitkDataCompressionView::OnStartPCA()
{
  for (auto imageNode : m_Controls.imageSelection->GetSelectedNodesStdVector())
  {
    for (auto vectorNode : m_Controls.peakListSelection->GetSelectedNodesStdVector())
    {
      auto image = dynamic_cast<const m2::SpectrumImage *>(imageNode->GetData());
      auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
      const auto &intervals = vector->GetIntervals();

      if (!image->GetImageAccessInitialized())
        continue;

      // only the pixels of the chosen mask value contribute to the components
      auto maskImage = GetMaskImage(imageNode);
      if (maskImage.IsNull())
        continue;

      auto filter = m2::PcaImageFilter::New();
      filter->SetMaskImage(maskImage);

      std::vector<mitk::Image::Pointer> temporaryImages;
      auto progressBar = mitk::ProgressBar::GetInstance();
      progressBar->AddStepsToDo(intervals.size() + 1);
      size_t inputIdx = 0;
      for (size_t row = 0; row < intervals.size(); ++row)
      {
        progressBar->Progress();
        temporaryImages.push_back(mitk::Image::New());
        temporaryImages.back()->Initialize(image);
        const auto mz = intervals.at(row).x.mean();
        image->GetImage(mz, image->ApplyTolerance(mz), maskImage, temporaryImages.back().GetPointer());
        filter->SetInput(inputIdx, temporaryImages.back());
        ++inputIdx;
      }

      if (temporaryImages.size() <= 2)
      {
        progressBar->Progress();
        QMessageBox::warning(nullptr,
                             "Select image,s first!",
                             "Select at least three peaks!",
                             QMessageBox::StandardButton::NoButton,
                             QMessageBox::StandardButton::Ok);
        continue;
      }

      filter->SetNumberOfComponents(m_Controls.pca_dims->value());
      filter->Update();
      progressBar->Progress();
      auto name = imageNode->GetName() + ".PCA";
      auto child = this->GetDataStorage()->GetNamedDerivedNode(name.c_str(), imageNode);
      if (!child){     
        auto outputNode = mitk::DataNode::New();
        mitk::Image::Pointer data = filter->GetOutput(0);
        outputNode->SetData(data);
        outputNode->SetName(name);
        this->GetDataStorage()->Add(outputNode, const_cast<mitk::DataNode *>(imageNode.GetPointer()));
      }else{
        child->SetData(filter->GetOutput(0));
      }
    }
  }
}

void QmitkDataCompressionView::OnStartTSNE()
{
  for (auto node : m_Controls.imageSelection->GetSelectedNodesStdVector())
  {
    if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
    {
      auto pcaName = node->GetName() + ".PCA";
      auto pcaChild = this->GetDataStorage()->GetNamedDerivedNode(pcaName.c_str(), node);
      if (!pcaChild)
        return;

      auto pcaImage = dynamic_cast<mitk::Image *>(pcaChild->GetData());
      const auto pcaComponents = pcaImage->GetPixelType().GetNumberOfComponents();

      auto filter = m2::TSNEImageFilter::New();
      filter->SetPerplexity(m_Controls.tsne_perplexity->value());
      filter->SetIterations(m_Controls.tnse_iters->value());
      filter->SetTheta(m_Controls.tsne_theta->value());

      using MaskImageType = itk::Image<mitk::MultiLabelSegmentation::LabelValueType, 3>;

      // only the pixels of the chosen mask value are embedded
      mitk::Image::Pointer maskImage = GetMaskImage(node);
      if (maskImage.IsNull())
        continue;

      if(m_Controls.tsne_shrink->value() > 1){
        MaskImageType::Pointer maskImageItk;
        mitk::CastToItkImage(maskImage, maskImageItk);
        auto caster = itk::ShrinkImageFilter<MaskImageType, MaskImageType>::New();
        caster->SetInput(maskImageItk);
        caster->SetShrinkFactor(0, m_Controls.tsne_shrink->value());
        caster->SetShrinkFactor(1, m_Controls.tsne_shrink->value());
        caster->SetShrinkFactor(2, 1);
        caster->Update();
        // write into a new image, casting into maskImage would modify the mask itself
        mitk::Image::Pointer shrunkMaskImage;
        mitk::CastToMitkImage(caster->GetOutput(), shrunkMaskImage);
        maskImage = shrunkMaskImage;
      }

      filter->SetMaskImage(maskImage);
      // const auto &peakList = image->GetPeaks();
      size_t index = 0;

      mitk::ImageReadAccessor racc(pcaImage);
      auto *inputData = static_cast<const typename DisplayImageType::PixelType *>(racc.GetData());

      std::vector<mitk::Image::Pointer> bufferedImages(pcaComponents);
      unsigned int n = pcaImage->GetDimensions()[0] * pcaImage->GetDimensions()[1] * pcaImage->GetDimensions()[2];
      for (auto &I : bufferedImages)
      {
        I = mitk::Image::New();
        I->Initialize(image);
        {
          mitk::ImageWriteAccessor acc(I);
          auto outCData = static_cast<typename DisplayImageType::PixelType *>(acc.GetData());
          for (unsigned int k = 0; k < n; ++k)
            *(outCData + k) = *(inputData + (k * pcaComponents) + index);
        }

        if(m_Controls.tsne_shrink->value() > 1){
          DisplayImageType::Pointer cImage;
          mitk::CastToItkImage(I, cImage);
          auto caster = itk::ShrinkImageFilter<DisplayImageType, DisplayImageType>::New();
          caster->SetInput(cImage);
          caster->SetShrinkFactor(0, m_Controls.tsne_shrink->value());
          caster->SetShrinkFactor(1, m_Controls.tsne_shrink->value());
          caster->SetShrinkFactor(2, 1);
          caster->Update();
          mitk::CastToMitkImage(caster->GetOutput(), I);
        }
        
        filter->SetInput(index, I);
        ++index;
      }
      filter->Update();


      
      auto data = m2::MultiSliceFilter::ConvertMitkVectorImageToRGB(ResampleVectorImage(filter->GetOutput(), image));
      auto name = node->GetName() + ".tSNE";
      auto child = this->GetDataStorage()->GetNamedDerivedNode(name.c_str(), node);
      if (!child){     
        auto outputNode = mitk::DataNode::New();
        outputNode->SetData(data);
        outputNode->SetName(node->GetName() + ".tSNE");
        this->GetDataStorage()->Add(outputNode, const_cast<mitk::DataNode *>(node.GetPointer()));
      }else{
        child->SetData(data);
      }

      // auto outputNode = mitk::DataNode::New();
      // outputNode->SetData(data);
      // outputNode->SetName("tSNE");
      // this->GetDataStorage()->Add(outputNode, const_cast<mitk::DataNode *>(node.GetPointer()));

    }
  }
}

mitk::Image::Pointer QmitkDataCompressionView::ResampleVectorImage(mitk::Image::Pointer vectorImage,
                                                                   mitk::Image::Pointer referenceImage)
{
  const unsigned int components = vectorImage->GetPixelType().GetNumberOfComponents();

  VectorImageType::Pointer vectorImageItk;
  mitk::CastToItkImage(vectorImage, vectorImageItk);

  DisplayImageType::Pointer referenceImageItk;
  mitk::CastToItkImage(referenceImage, referenceImageItk);

  auto resampledVectorImageItk = VectorImageType::New();
  resampledVectorImageItk->SetOrigin(referenceImageItk->GetOrigin());
  resampledVectorImageItk->SetDirection(referenceImageItk->GetDirection());
  resampledVectorImageItk->SetSpacing(referenceImageItk->GetSpacing());
  resampledVectorImageItk->SetRegions(referenceImageItk->GetLargestPossibleRegion());
  resampledVectorImageItk->SetNumberOfComponentsPerPixel(components);
  resampledVectorImageItk->Allocate();
  itk::VariableLengthVector<m2::DisplayImagePixelType> v(components);
  v.Fill(0);
  resampledVectorImageItk->FillBuffer(v);

  auto inAdaptor = VectorImageAdaptorType::New();
  auto outAdaptor = VectorImageAdaptorType::New();
  using LinearInterpolatorType = itk::LinearInterpolateImageFunction<VectorImageAdaptorType>;
  using TransformType = itk::IdentityTransform<double, 3>;

  for (unsigned int i = 0; i < components; ++i)
  {
    inAdaptor->SetExtractComponentIndex(i);
    inAdaptor->SetImage(vectorImageItk);
    inAdaptor->SetOrigin(vectorImageItk->GetOrigin());
    inAdaptor->SetDirection(vectorImageItk->GetDirection());
    inAdaptor->SetSpacing(vectorImageItk->GetSpacing());

    outAdaptor->SetExtractComponentIndex(i);
    outAdaptor->SetImage(resampledVectorImageItk);

    auto resampler = itk::ResampleImageFilter<VectorImageAdaptorType, DisplayImageType>::New();
    resampler->SetInput(inAdaptor);
    resampler->SetOutputParametersFromImage(referenceImageItk);
    resampler->SetInterpolator(LinearInterpolatorType::New());
    resampler->SetTransform(TransformType::New());
    resampler->Update();

    itk::ImageAlgorithm::Copy<DisplayImageType, VectorImageAdaptorType>(
      resampler->GetOutput(),
      outAdaptor,
      resampler->GetOutput()->GetLargestPossibleRegion(),
      outAdaptor->GetLargestPossibleRegion());
  }

  mitk::Image::Pointer outImage;
  mitk::CastToMitkImage(resampledVectorImageItk, outImage);

  return outImage;
}