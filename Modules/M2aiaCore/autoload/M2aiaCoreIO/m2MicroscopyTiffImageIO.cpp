/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <M2aiaCoreIOExports.h>
#include <itkTIFFImageIO.h>
#include <m2MicroscopyTiffImageIO.h>
#include <mitkAbstractFileIO.h>
#include <mitkIOMimeTypes.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkItkImageIO.h>

namespace m2
{
  MicroscopyTiffImageIO::MicroscopyTiffImageIO()
    : AbstractFileIO(mitk::Image::GetStaticNameOfClass(), MICROSCOPYTIFF_MIMETYPE(), "ITK TIFFImageIO (microscopy)")
  {
    AbstractFileWriter::SetRanking(0);
    AbstractFileReader::SetRanking(0);

    Options defaultOptions;

    // defaultOptions[OPTION_MERGE_POINTS()] = us::Any(true);
    defaultOptions["Import as volume"] = us::Any(true);
    defaultOptions["Slice thickness(µm)"] = us::Any(10.0f);
    defaultOptions["Split channels"] = us::Any(true);

    // defaultOptions["Pixel spacing x"] = us::Any(10.0f);
    // defaultOptions["Pixel spacing y"] = us::Any(10.0f);
    // defaultOptions["Slice thickness(µm)"] = us::Any(10.0f);

    this->SetDefaultReaderOptions(defaultOptions);

    this->RegisterService();
  }

  std::vector<mitk::BaseData::Pointer> MicroscopyTiffImageIO::DoRead()
  {
    // read the file
    auto io = itk::TIFFImageIO::New();
    mitk::ItkImageIO mitkItkImageIO(MICROSCOPYTIFF_MIMETYPE(), io, 0);
    mitkItkImageIO.mitk::AbstractFileReader::SetInput(this->GetInputLocation());
    auto resultVec = mitkItkImageIO.Read();

    // cast result and make shared pointer
    mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(resultVec.back().GetPointer());

    // MITK_INFO << image->GetPixelType().GetNumberOfComponents();
    // MITK_INFO << image->GetPixelType().GetComponentTypeAsString();
    // MITK_INFO << image->GetPixelType().GetPixelType();
    // MITK_INFO << image->GetPixelType().GetPixelTypeAsString();
    // MITK_INFO << image->GetGeometry()->GetSpacing();

    auto spacing = image->GetGeometry()->GetSpacing();
    spacing[0] = spacing[0] / 1000.0f;
    spacing[1] = spacing[1] / 1000.0f;
    spacing[2] = us::any_cast<float>(GetReaderOption("Slice thickness(µm)")) / 1000.0f;

    const auto *D = image->GetDimensions();
    const auto N = std::accumulate(D, D + image->GetDimension(), 1, std::multiplies<unsigned int>());
    auto imagePixelType = image->GetPixelType();

    if (imagePixelType.GetNumberOfComponents() >= 3 && image->GetDimension() == 2)
    {
      resultVec.pop_back();
      AccessFixedPixelTypeByItk(
        image,
        ([&](auto itkImage) {
          using RGBAPixelType = typename std::remove_pointer<decltype(itkImage)>::type::PixelType;
          using PixelType = typename RGBAPixelType::ComponentType;
          const auto newPixelType = mitk::MakePixelType<itk::Image<PixelType, 3>>(1);
          const std::array<unsigned int, 3> dims{D[0], D[1], 1};

          for (unsigned int i = 0; i < imagePixelType.GetNumberOfComponents(); ++i)
          {
            auto slice = mitk::Image::New();
            slice->Initialize(newPixelType, 3, dims.data());
            mitk::ImagePixelWriteAccessor<PixelType, 3> sAcc(slice);
            std::transform(itkImage->GetBufferPointer(),
                           itkImage->GetBufferPointer() + N,
                           sAcc.GetData(),
                           [&i](const RGBAPixelType &d) { return d.GetElement(i); });
            slice->SetSpacing(spacing);
            slice->GetSourceOutputName();
            resultVec.push_back(slice);
          }
        }),
        (itk::RGBPixel<char>)(itk::RGBPixel<unsigned char>)(itk::RGBPixel<unsigned short>)(itk::RGBPixel<short>)(itk::RGBPixel<float>)(itk::RGBAPixel<char>)(itk::RGBAPixel<unsigned char>)(itk::RGBAPixel<unsigned short>)(itk::RGBAPixel<short>)(itk::RGBAPixel<float>));
    }else if(imagePixelType.GetNumberOfComponents() == 1 && image->GetDimension() == 2){
        resultVec.pop_back();
        AccessByItk(image, ([&](auto itkImage) {
          using PixelType = typename std::remove_pointer<decltype(itkImage)>::type::PixelType;
          const auto newPixelType = mitk::MakePixelType<itk::Image<PixelType, 3>>(1);
          const std::array<unsigned int, 3> dims{D[0], D[1], 1};

          auto slice = mitk::Image::New();
          slice->Initialize(newPixelType, 3, dims.data());
          mitk::ImagePixelWriteAccessor<PixelType, 3> sAcc(slice);
          std::copy(itkImage->GetBufferPointer(), itkImage->GetBufferPointer() + N, sAcc.GetData());
          slice->SetSpacing(spacing);
          slice->GetSourceOutputName();
          resultVec.push_back(slice);
        }));
    }
    return resultVec;
  }

  mitk::DataStorage::SetOfObjects::Pointer MicroscopyTiffImageIO::Read(mitk::DataStorage &ds)
  {
    mitk::DataStorage::SetOfObjects::Pointer result = mitk::DataStorage::SetOfObjects::New();
    std::vector<mitk::BaseData::Pointer> data = this->mitk::AbstractFileReader::Read();
    unsigned int i = 0;
    for (auto iter = data.begin(); iter != data.end(); ++iter)
    {
      mitk::DataNode::Pointer node = mitk::DataNode::New();
      node->SetData(*iter);
      this->SetDefaultDataNodeProperties(node, this->GetInputLocation());
      node->SetName(node->GetName() + "_" + std::to_string(i));
      ++i;
      ds.Add(node);
      result->InsertElement(result->Size(), node);
    }
    return result;
  }

  void MicroscopyTiffImageIO::Write() {}

  mitk::AbstractFileIO::ConfidenceLevel MicroscopyTiffImageIO::GetReaderConfidenceLevel() const
  {
    return ConfidenceLevel::Supported;
  }

  mitk::AbstractFileIO::ConfidenceLevel MicroscopyTiffImageIO::GetWriterConfidenceLevel() const
  {
    return ConfidenceLevel::Unsupported;
  }

  MicroscopyTiffImageIO *MicroscopyTiffImageIO::IOClone() const { return new MicroscopyTiffImageIO(*this); }

} // namespace m2
