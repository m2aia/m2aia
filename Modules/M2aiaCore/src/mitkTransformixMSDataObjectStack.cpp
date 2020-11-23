/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <array>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <cstdlib>
#include <itkSignedMaurerDistanceMapImageFilter.h>
#include <m2ImzMLMassSpecImage.h>
#include <mitkExtractSliceFilter.h>
#include <mitkIOUtil.h>
#include <mitkITKImageImport.h>
#include <mitkImage2DToImage3DSliceFilter.h>
#include <mitkImage3DSliceToImage2DFilter.h>
#include <mitkImageAccessByItk.h>
#include <mitkTransformixMSDataObjectStack.h>

namespace m2
{
  mitk::Image::Pointer TransformixMSDataObjectStack::Transform(const mitk::Image *const ionImage,
                                                               const std::vector<std::string> &transformations,
                                                               std::function<void(std::string &)> adaptor) const
  {
    if (transformations.empty())
      return const_cast<mitk::Image *>(ionImage);

    mitk::Image::Pointer image;
    mitk::PointSet::Pointer pointset;

    const auto workdir = mitk::IOUtil::CreateTemporaryDirectory();
    auto workdirPath = boost::filesystem::path(workdir);
    workdirPath = workdirPath.lexically_normal();

    if (ionImage)
    {
      {
        auto filter = mitk::Image3DSliceToImage2DFilter::New();
        filter->SetInput(ionImage);
        filter->Update();
        mitk::IOUtil::Save(filter->GetOutput(), (workdirPath / "moving.nrrd").string());
      }

      for (std::string trafo : transformations)
      {
        // apply the transformation file changes e.g. pixel type output, interpolation mode etc..
        adaptor(trafo);

        auto p = workdirPath / ("Transform.txt");
        std::ofstream f(p.c_str());
        f << trafo;
        f.close();

        std::stringstream ss;
        std::vector<std::string> cmd{m_Transformix,
                                     "-in",
                                     (workdirPath / "moving.nrrd").string(),
                                     "-out",
                                     workdirPath.string(),
                                     "-tp",
                                     (workdirPath / ("Transform.txt")).string()};
        for (auto s : cmd)
          ss << s << " ";
        auto res = std::system(ss.str().c_str());
        if (res != 0)
          MITK_WARN << "Some error on system call of transformix";
        boost::filesystem::rename(workdirPath / "result.nrrd", workdirPath / "moving.nrrd");
      }

      auto res = mitk::IOUtil::Load((workdirPath / ("moving.nrrd")).string());
      image = dynamic_cast<mitk::Image *>(res[0].GetPointer());

      {
        auto filter = mitk::Image2DToImage3DSliceFilter::New();
        filter->SetInput(image);
        filter->Update();
        image = filter->GetOutput();
      }
    }

    boost::filesystem::remove_all(workdirPath);
    return image;
  }

  //	mitk::Image::Pointer TransformixMSDataObjectStack::TransformIonImage(unsigned i)
  //	{
  //		auto transformations = m_MSDataObjectTransformations[i].front();
  //		mitk::Image* ionImage = m_MSDataObjectReferences[i].front();
  //		if (transformations.empty()) return ionImage;

  //		const auto workdir = mitk::IOUtil::CreateTemporaryDirectory();
  //        auto workdirPath = boost::filesystem::path(workdir);
  //        workdirPath = workdirPath.lexically_normal();

  //        {
  //            auto filter = mitk::Image3DSliceToImage2DFilter::New();
  //            filter->SetInput(ionImage);
  //            filter->Update();
  //            mitk::IOUtil::Save(filter->GetOutput(), (workdirPath / "moving.nrrd").string());
  //        }

  //		for (auto & trafo : transformations) {
  //			ReplaceParameter(trafo, "ResultImagePixelType", "\"double\"");
  //            auto p = workdirPath / ("Transform.txt");
  //            std::ofstream f(p.c_str());
  //			f << trafo;
  //			f.close();

  //            std::stringstream ss;
  //            std::vector<std::string> cmd{ m_Transformix, "-in", (workdirPath / "moving.nrrd").string(), "-out",
  //            workdirPath.string(), "-tp", (workdirPath / ("Transform.txt")).string() }; for (auto s : cmd) 	ss << s
  //            << " "; auto res = std::system(ss.str().c_str()); if(res != 0) MITK_WARN << "Some error on system call
  //            of transformix"; boost::filesystem::rename(workdirPath / "result.nrrd", workdirPath / "moving.nrrd");
  //		}

  //        auto res = mitk::IOUtil::Load((workdirPath / ("moving.nrrd")).string());
  //		mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(res[0].GetPointer());

  //		{
  //			auto filter = mitk::Image2DToImage3DSliceFilter::New();
  //			filter->SetInput(image);
  //			filter->Update();
  //			image = filter->GetOutput();
  //		}

  //        boost::filesystem::remove_all(workdirPath);
  //		return image;
  //	}

  void TransformixMSDataObjectStack::ReplaceParameter(std::string &paramFileString, std::string what, std::string by)
  {
    auto pos1 = paramFileString.find("(" + what);
    if (pos1 == std::string::npos)
      mitkThrow() << "Cancel operation, no entry found called " << what << " in transformix param file!";

    auto pos2 = paramFileString.find(')', pos1);
    if (pos2 == std::string::npos)
      mitkThrow() << "Cancel operation, no closing bracket found in transformix param file for " << what << "!";

    paramFileString.replace(pos1, pos2 - pos1 + 1, "(" + what + " " + by + ")\n");
  }

  TransformixMSDataObjectStack::TransformixMSDataObjectStack(std::string transformixPath)
    : m_Transformix(transformixPath)
  {
    SetPropertyValue<double>("min m/z", std::numeric_limits<double>::max());
    SetPropertyValue<double>("max m/z", std::numeric_limits<double>::min());
  }

  bool TransformixMSDataObjectStack::IsPathToTransformixValid() { return true; }

  void TransformixMSDataObjectStack::Resize(unsigned num)
  {
    m_StackSize = 0;
    m_MSDataObjectReferences.clear();
    m_MSDataObjectTransformations.clear();
    m_StackSize = num;
    m_MSDataObjectReferences.resize(m_StackSize);
    m_MSDataObjectTransformations.resize(m_StackSize);
  }

  void TransformixMSDataObjectStack::Insert(unsigned i,
                                            m2::MSImageBase::Pointer data,
                                            TransformixMSDataObjectStack::TransformationVector transformations)
  {
    /*

    unsigned a = 0;
    for (auto & t : transformations)
      this->SetPropertyValue<std::string>("ims.stack.transformation.t" + std::to_string(i) + "." + std::to_string(a++),
    t);

    if (auto imzml = dynamic_cast<m2::ImzMLMassSpecImage *>(data.GetPointer()))
      this->SetPropertyValue<std::string>("ims.stack.transformation.t" + std::to_string(i) + ".imzml",
    imzml->GetImzMLDataPath());

    */

    if (i < m_MSDataObjectReferences.size() && i < m_MSDataObjectTransformations.size())
    {
      m_MSDataObjectReferences[i].push_back(data);
      m_MSDataObjectTransformations[i].push_back(transformations);

      auto newMin = data->GetPropertyValue<double>("min m/z");
      auto newMax = data->GetPropertyValue<double>("max m/z");
      auto currentMin = GetPropertyValue<double>("min m/z");
      auto currentMax = GetPropertyValue<double>("max m/z");

      if (newMin < currentMin)
        SetPropertyValue<double>("min m/z", newMin);
      if (newMax > currentMax)
        SetPropertyValue<double>("max m/z", newMax);
    }
  }

  void TransformixMSDataObjectStack::InitializeImages(unsigned refIndex, double zSpacing)
  {
    if (m_MSDataObjectReferences[refIndex].front().IsNull())
      mitkThrow() << "Reference MS data object is not set!";
    if (zSpacing <= 0)
      mitkThrow() << "z-spacing must be a non zero positive number!";

    auto outputDouble = [this](std::string &trafo) { ReplaceParameter(trafo, "ResultImagePixelType", "\"double\""); };

    auto outputNearestNeighborMask = [this](std::string &trafo) {
      ReplaceParameter(trafo, "ResultImagePixelType", "\"unsigned short\"");
      ReplaceParameter(trafo, "ResampleInterpolator", "\"FinalNearestNeighborInterpolator\"");
    };

    auto outputNearestNeighborIndex = [this](std::string &trafo) {
      ReplaceParameter(trafo, "ResultImagePixelType", "\"unsigned int\"");
      ReplaceParameter(trafo, "ResampleInterpolator", "\"FinalNearestNeighborInterpolator\"");
    };

    //    auto outputLinearInterpolationLandmark = [this](std::string &trafo) {
    //      ReplaceParameter(trafo, "ResultImagePixelType", "\"float\"");
    //      ReplaceParameter(trafo, "ResampleInterpolator", "\"FinalLinearInterpolator\"");
    //    };

    MITK_INFO << "Initialize Stack object";
    TransformationVector transformations = m_MSDataObjectTransformations[refIndex].front();
    auto refImage = Transform(m_MSDataObjectReferences[refIndex].front(), transformations, outputDouble);

    // auto transformations = m_MSDataObjectTransformations[refIndex].front();
    // auto refIndexImage =
    // TransformImageUsingNNInterpolation(m_MSDataObjectReferences[refIndex].front()->GetIndexImage(), transformations);
    // auto reMaskImage = TransformImageUsingNNInterpolation(m_MSDataObjectReferences[refIndex].front()->GetMaskImage(),
    // transformations);

    unsigned int dims[3];
    std::memcpy(dims, refImage->GetDimensions(), 3 * sizeof(unsigned int));
    dims[2] = m_MSDataObjectReferences.size();

    auto spacing = refImage->GetGeometry()->GetSpacing();
    spacing[2] = zSpacing;

    {
      this->Initialize(mitk::MakeScalarPixelType<m2::IonImagePixelType>(), 3, dims);
      this->SetSpacing(spacing);
      this->SetOrigin(refImage->GetGeometry()->GetOrigin());
    }

    {
      GetImageArtifacts()["index"] = mitk::Image::New();
      GetIndexImage()->Initialize(mitk::MakeScalarPixelType<m2::IndexImagePixelType>(), 3, dims);
      GetIndexImage()->SetSpacing(spacing);
      GetIndexImage()->SetOrigin(refImage->GetGeometry()->GetOrigin());
    }

    {
      GetImageArtifacts()["mask"] = mitk::Image::New();
      GetMaskImage()->Initialize(mitk::MakeScalarPixelType<m2::MaskImagePixelType>(), 3, dims);
      GetMaskImage()->SetSpacing(spacing);
      GetMaskImage()->SetOrigin(refImage->GetGeometry()->GetOrigin());
    }

    {
      GetImageArtifacts()["distance"] = mitk::Image::New();
      mitk::Image::Pointer distanceImage = dynamic_cast<mitk::Image *>(GetImageArtifacts()["distance"].GetPointer());
      distanceImage->Initialize(mitk::MakeScalarPixelType<float>(), 3, dims);
      distanceImage->SetSpacing(spacing);
      distanceImage->SetOrigin(refImage->GetGeometry()->GetOrigin());
    }

    //if (auto lmImage = dynamic_cast<mitk::Image *>(GetImageArtifacts()["landmarks"].GetPointer()))
    //{
    //  lmImage->Initialize(mitk::MakeScalarPixelType<m2::IndexImagePixelType>(), 3, dims);
    //  lmImage->SetSpacing(spacing);
    //  lmImage->SetOrigin(refImage->GetGeometry()->GetOrigin());
    //}

    for (unsigned i = 0; i < m_MSDataObjectReferences.size(); ++i)
    {
      transformations = m_MSDataObjectTransformations[i].front();
      auto msImage = m_MSDataObjectReferences[i].front();
      auto warped = Transform(msImage, transformations, outputDouble);

      auto warpedIndex = Transform(msImage->GetIndexImage(), transformations, outputNearestNeighborIndex);
      auto warpedMask = Transform(msImage->GetMaskImage(), transformations, outputNearestNeighborMask);

      CopyWarpedImageToStack(warped, this, i);
      CopyWarpedImageToStack(warpedIndex, GetIndexImage(), i);
      CopyWarpedImageToStack(warpedMask, GetMaskImage(), i);

     /* if (msImage->GetImageArtifacts().find("landmarks") != msImage->GetImageArtifacts().end())
      {
        auto lmImageSource = dynamic_cast<mitk::Image *>(msImage->GetImageArtifacts()["landmarks"].GetPointer());
        auto lmImageTarget = dynamic_cast<mitk::Image *>(GetImageArtifacts()["landmarks"].GetPointer());

        auto warpedLmImage = Transform(lmImageSource, transformations, outputNearestNeighborIndex);
        CopyWarpedImageToStack(warpedLmImage, lmImageTarget, i);
      }*/

      mitk::Image::Pointer distanceImage;
      AccessByItk(warpedMask, ([this, i, &distanceImage](auto itkImage) {
                    using InputImageType = typename std::remove_pointer<decltype(itkImage)>::type;
                    auto filter =
                      itk::SignedMaurerDistanceMapImageFilter<InputImageType,
                                                              itk::Image<float, InputImageType::ImageDimension>>::New();
                    filter->SetInput(itkImage);
                    filter->Update();
                    mitk::CastToMitkImage(filter->GetOutput(), distanceImage);
                  }));
      auto distanceTarget = dynamic_cast<mitk::Image *>(GetImageArtifacts()["distance"].GetPointer());
      CopyWarpedImageToStack(distanceImage, distanceTarget, i);
    }

    MITK_INFO << "Stacking complete!";
  }

  // void TransformixMSDataObjectStack::GrabSpectrum(unsigned int /*index*/,
  //                                                std::vector<double> & /*mzs*/,
  //                                                std::vector<double> & /*ints*/) const
  //{
  //}

  void TransformixMSDataObjectStack::TransformixMSDataObjectStack::CopyWarpedImageToStack(mitk::Image *warped,
                                                                                          mitk::Image *stack,
                                                                                          unsigned i) const
  {
    auto N = warped->GetDimensions()[0] * warped->GetDimensions()[1];
    if (N != (stack->GetDimensions()[0] * stack->GetDimensions()[1]))
      mitkThrow() << "Slice dimensions are not equal!";
    if (i >= stack->GetDimensions()[2])
      mitkThrow() << "Stack index is invalid!";

    AccessTwoImagesFixedDimensionByItk(warped,
                                       stack,
                                       ([i, N](auto itkWarped, auto itkStacked) {
                                         auto *warpedP = itkWarped->GetPixelContainer()->GetBufferPointer();
                                         auto *stackP = itkStacked->GetPixelContainer()->GetBufferPointer();
                                         std::copy(warpedP, warpedP + N, stackP + i * N);
                                       }),
                                       3);
  }

  void TransformixMSDataObjectStack::GrabIonImage(double mz,
                                                  double tol,
                                                  const mitk::Image * /*mask*/,
                                                  mitk::Image *img) const
  {
    for (unsigned i = 0; i < m_MSDataObjectReferences.size(); ++i)
    {
      auto current = m_MSDataObjectReferences[i].front();
      auto transformations = m_MSDataObjectTransformations[i].front();
      current->GrabIonImage(mz, tol, current->GetMaskImage(), current);

      MITK_INFO << "Grab ok";

      auto warped = Transform(current, transformations, [this](auto trafo) {
        ReplaceParameter(trafo, "ResultImagePixelType", "\"double\"");
      });

      CopyWarpedImageToStack(warped, img, i);

      // auto warpedIndexImage = TransformImageUsingNNInterpolation(i);
      // CopyWarpedImageToStack<IndexImagePixelType>(warpedIndexImage, GetIndexImage(), i);
    }
  }
} // namespace m2
