/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/
#define BOOST_TIMER_ENABLE_DEPRECATED
#include <Poco/SHA1Engine.h>
#include <boost/progress.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <itkMath.h>
#include <itksys/SystemTools.hxx>
#include <m2CoreCommon.h>
#include <m2ImzMLEngine.h>
#include <m2ImzMLImageIO.h>
#include <m2ImzMLParser.h>
#include <m2Timer.h>
#include <mitkCoreServices.h>
#include <mitkIOUtil.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLocaleSwitch.h>
#include <mitkImageCast.h>
#include <signal/m2PeakDetection.h>
#include <signal/m2Pooling.h>
#include <cstdio>
#include <iomanip>
#include <limits>
#include <sstream>

template <class ConversionType, class ItFirst, class ItLast, class OStreamType>
void writeData(ItFirst itFirst, ItLast itLast, OStreamType &os)
{
  std::for_each(itFirst,
                itLast,
                [&os](const auto &v)
                {
                  const ConversionType c(v);
                  os.write((char *)(&c), sizeof(ConversionType));
                });
}

namespace m2
{
  namespace
  {
    struct ImzMLAssociatedPaths
    {
      std::string legacyBase;
      std::string sidecarBase;
      std::string sidecarDir;
      std::string fileStem;
    };

    std::string EscapeXmlAttribute(const std::string &value)
    {
      std::string escaped;
      escaped.reserve(value.size());
      for (const auto c : value)
      {
        switch (c)
        {
          case '&': escaped += "&amp;"; break;
          case '<': escaped += "&lt;"; break;
          case '>': escaped += "&gt;"; break;
          case '"': escaped += "&quot;"; break;
          case '\'': escaped += "&apos;"; break;
          default: escaped.push_back(c); break;
        }
      }
      return escaped;
    }

    bool ExtractAttributeValue(const std::string &line, const std::string &attribute, std::string &value)
    {
      const std::string key = attribute + "=\"";
      const auto pos = line.find(key);
      if (pos == std::string::npos)
        return false;
      const auto start = pos + key.size();
      const auto end = line.find('"', start);
      if (end == std::string::npos)
        return false;
      value.assign(line.data() + start, end - start);
      return true;
    }

    std::string ReplaceAttributeValue(const std::string &line, const std::string &attribute, const std::string &newValue)
    {
      const std::string key = attribute + "=\"";
      const auto pos = line.find(key);
      if (pos == std::string::npos)
        return line;
      const auto start = pos + key.size();
      const auto end = line.find('"', start);
      if (end == std::string::npos)
        return line;
      std::string copy = line;
      copy.replace(start, end - start, newValue);
      return copy;
    }

    std::string FormatDouble(double value)
    {
      std::ostringstream oss;
      oss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << value;
      return oss.str();
    }

    // std::string GetPropertyAsString(const m2::ImzMLSpectrumImage *image, const std::string &propertyName, const std::string &fallback = "")
    // {
    //   if (auto property = image->GetProperty(propertyName.c_str()))
    //     return property->GetValueAsString();
    //   return fallback;
    // }

    bool PatchImzMLMetadata(const std::string &imzmlPath, const m2::ImzMLSpectrumImage *image)
    {
      std::ifstream in(imzmlPath);
      if (!in)
        return false;

      const std::string tempPath = imzmlPath + ".tmp";
      std::ofstream out(tempPath);
      if (!out)
      {
        in.close();
        return false;
      }

      bool hasOriginalDirectionMatrix = false;
      bool hasOriginalOffsetX = false;
      bool hasOriginalOffsetY = false;
      {
        std::ifstream check(imzmlPath);
        std::string checkLine;
        while (std::getline(check, checkLine))
        {
          if (!hasOriginalDirectionMatrix && checkLine.find("userParam") != std::string::npos && checkLine.find("name=\"original direction matrix\"") != std::string::npos)
          {
            hasOriginalDirectionMatrix = true;
          }
          else if (!hasOriginalOffsetX && checkLine.find("userParam") != std::string::npos && checkLine.find("name=\"original absolute position offset x\"") != std::string::npos)
          {
            hasOriginalOffsetX = true;
          }
          else if (!hasOriginalOffsetY && checkLine.find("userParam") != std::string::npos && checkLine.find("name=\"original absolute position offset y\"") != std::string::npos)
          {
            hasOriginalOffsetY = true;
          }
          if (hasOriginalDirectionMatrix && hasOriginalOffsetX && hasOriginalOffsetY)
            break;
        }
      }

      std::string newDirectionMatrix;
      if (auto geometry = image->GetGeometry())
      {
        const auto *vtkMatrix = geometry->GetVtkMatrix();
        if (vtkMatrix)
        {
          std::ostringstream dirStream;
          dirStream << std::setprecision(std::numeric_limits<double>::digits10 + 1);
          for (unsigned int row = 0; row < 3; ++row)
          {
            for (unsigned int col = 0; col < 3; ++col)
            {
              if (row != 0 || col != 0)
                dirStream << ' ';
              dirStream << vtkMatrix->GetElement(row, col);
            }
          }
          newDirectionMatrix = dirStream.str();
        }
        else
        {
          newDirectionMatrix = "1 0 0 0 1 0 0 0 1";
        }
      }
      else
      {
        newDirectionMatrix = "1 0 0 0 1 0 0 0 1";
      }
      const std::string newOffsetX = FormatDouble(m2::MilliMeterToMicroMeter(image->GetGeometry()->GetOrigin()[0]));
      const std::string newOffsetY = FormatDouble(m2::MilliMeterToMicroMeter(image->GetGeometry()->GetOrigin()[1]));

      bool patchedDirectionMatrix = false;
      bool patchedOffsetX = false;
      bool patchedOffsetY = false;
      std::string line;

      while (std::getline(in, line))
      {
        std::string outputLine = line;

        if (!patchedDirectionMatrix && line.find("userParam") != std::string::npos && line.find("name=\"direction matrix\"") != std::string::npos)
        {
          std::string originalValue;
          ExtractAttributeValue(line, "value", originalValue);
          outputLine = ReplaceAttributeValue(line, "value", newDirectionMatrix);
          out << outputLine << "\n";
          if (!hasOriginalDirectionMatrix)
          {
            out << "      <userParam name=\"original direction matrix\" value=\"" << EscapeXmlAttribute(originalValue) << "\" type=\"xsd:string\"/>\n";
          }
          patchedDirectionMatrix = true;
          continue;
        }

        if (!patchedOffsetX && line.find("cvParam") != std::string::npos && line.find("IMS:1000053") != std::string::npos && line.find("name=\"absolute position offset x\"") != std::string::npos)
        {
          std::string originalValue;
          ExtractAttributeValue(line, "value", originalValue);
          outputLine = ReplaceAttributeValue(line, "value", newOffsetX);
          MITK_INFO << "[PatchImzMLMetadata] offset x: file=" << originalValue << " µm  ->  new=" << newOffsetX << " µm"
                    << "  (property was " << image->GetPropertyValue<double>("[IMS:1000053] absolute position offset x") << " mm)";
          out << outputLine << "\n";
          if (!hasOriginalOffsetX)
          {
            out << "      <userParam name=\"original absolute position offset x\" value=\"" << EscapeXmlAttribute(originalValue) << "\" type=\"xsd:string\"/>\n";
          }
          patchedOffsetX = true;
          continue;
        }

        if (!patchedOffsetY && line.find("cvParam") != std::string::npos && line.find("IMS:1000054") != std::string::npos && line.find("name=\"absolute position offset y\"") != std::string::npos)
        {
          std::string originalValue;
          ExtractAttributeValue(line, "value", originalValue);
          outputLine = ReplaceAttributeValue(line, "value", newOffsetY);
          MITK_INFO << "[PatchImzMLMetadata] offset y: file=" << originalValue << " µm  ->  new=" << newOffsetY << " µm"
                    << "  (property was " << image->GetPropertyValue<double>("[IMS:1000054] absolute position offset y") << " mm)";
          out << outputLine << "\n";
          if (!hasOriginalOffsetY)
          {
            out << "      <userParam name=\"original absolute position offset y\" value=\"" << EscapeXmlAttribute(originalValue) << "\" type=\"xsd:string\"/>\n";
          }
          patchedOffsetY = true;
          continue;
        }

        out << outputLine << "\n";
      }

      in.close();
      out.close();

      if (patchedDirectionMatrix || patchedOffsetX || patchedOffsetY)
      {
        if (std::rename(tempPath.c_str(), imzmlPath.c_str()) != 0)
        {
          itksys::SystemTools::CopyFileAlways(tempPath, imzmlPath);
          itksys::SystemTools::RemoveFile(tempPath);
        }
        return true;
      }

      itksys::SystemTools::RemoveFile(tempPath);
      return false;
    }

    ImzMLAssociatedPaths BuildAssociatedPaths(const std::string &imzmlPath)
    {
      const std::string parentDir = itksys::SystemTools::GetParentDirectory(imzmlPath);
      const std::string stem = itksys::SystemTools::GetFilenameWithoutExtension(imzmlPath);
      const std::string legacyBase = parentDir + "/" + stem;
      const std::string sidecarDir = legacyBase;
      const std::string sidecarBase = sidecarDir + "/" + stem;
      return {legacyBase, sidecarBase, sidecarDir, stem};
    }

    std::string PreferSidecarThenLegacy(const std::string &sidecarPath, const std::string &legacyPath)
    {
      if (itksys::SystemTools::FileExists(sidecarPath))
        return sidecarPath;
      return legacyPath;
    }
  }

  ImzMLImageIO::ImzMLImageIO() : AbstractFileIO(mitk::Image::GetStaticNameOfClass(), IMZML_MIMETYPE(), "imzML Image")
  {
    AbstractFileWriter::SetRanking(1);
    AbstractFileReader::SetRanking(10);
    this->RegisterService();
  }

  mitk::IFileIO::ConfidenceLevel ImzMLImageIO::GetWriterConfidenceLevel() const
  {
    

    auto input1 = dynamic_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());
    auto input2 = dynamic_cast<const m2::SpectrumImageStack *>(this->GetInput());
    if (input1 || input2)
    {
      return Supported;
    }
    else
      return Unsupported;
  }

  std::string ImzMLImageIO::GetNamedOutputPath(std::string parentPathWithExtension, std::string newExtension) const
  {

    const auto path = itksys::SystemTools::GetParentDirectory(parentPathWithExtension);
    const std::string ibdOutPath = path + "/" + itksys::SystemTools::GetFilenameWithoutExtension(parentPathWithExtension) + newExtension;

    return itksys::SystemTools::ConvertToOutputPath(ibdOutPath);
  }

  std::string ImzMLImageIO::GetImzMLOutputPath() const
  {
    auto outFile = this->GetOutputLocation();
    return itksys::SystemTools::ConvertToOutputPath(outFile);
  }

  /*if (any(input->GetExportMode() & ImzMLFormatType::ContinuousCentroid))
  {
    std::vector<double> is;
    auto mzIt = std::cbegin(mzs); // mzs contains already masked mzs
    for (const auto &i : maskIndices)
    {
      double mz = *mzIt++;
      if (input->GetMassPickingTolerance() != 0.0)
      {
        auto iLower = std::lower_bound(mzs.begin(), mzs.end(), mz - input->GetMassPickingTolerance() * 10e-6);
        auto iUpper = std::upper_bound(mzs.begin(), mzs.end(), mz + input->GetMassPickingTolerance() * 10e-6);
        auto s = std::next(std::begin(ints), std::distance(mzs.begin(), iLower));
        auto e = std::next(s, std::distance(iLower, iUpper));
        double val = 0;
        switch (input->GetIonImageGrabStrategy())
        {
        case m2::IonImageGrabStrategyType::Sum:
          val = std::accumulate(s, e, double(0));
          break;
        case m2::IonImageGrabStrategyType::Mean:
          val = std::accumulate(s, e, double(0)) / double(std::distance(s, e));
          break;
        case m2::IonImageGrabStrategyType::Maximum:
          val = *std::max_element(s, e);
          break;
        case m2::IonImageGrabStrategyType::Median:
        {
          const unsigned int _N = std::distance(s, e);
          double median = 0;
          if ((_N % 2) == 0)
          {
            std::nth_element(s, s + _N * 0.5, e);
            std::nth_element(s, s + _N * 0.5 + 1, e);
            median = 0.5 * (*std::next(s, _N * 0.5) + *std::next(s, _N * 0.5 + 1));
          }
          else
          {
            std::nth_element(s, s + ((_N + 1) * 0.5), e);
            median = 0.5 * (*std::next(s, _N * 0.5) + *std::next(s, _N * 0.5 + 1));
          }
          val = median;
          break;
        }
        }
        is.push_back(val);
      }
      else
      {
        is.push_back(ints[i + input->GetConstantOffset()]);
      }
    }
    ints = std::move(is);
  }
  */


  void ImzMLImageIO::Write()
  {
    mitk::LocaleSwitch localeSwitch("C");
    ValidateOutputLocation();

    if (const auto input = dynamic_cast<const m2::ImzMLSpectrumImage *>(this->GetInput()))
    {
      const std::string srcImzML = input->GetImzMLDataPath();
      if (!itksys::SystemTools::FileExists(srcImzML))
      {
        mitkThrow() << "[ImzMLImageIO::Write] Source imzML file does not exist: " << srcImzML;
        }

      const std::string dstImzML = GetImzMLOutputPath();
      
      // Copy .imzML
      MITK_INFO << "[ImzMLImageIO::Write] Copy: " << srcImzML << " -> " << dstImzML;
      itksys::SystemTools::CopyFileAlways(srcImzML, dstImzML);
      PatchImzMLMetadata(dstImzML, input);

      const std::string srcIbd = input->GetBinaryDataPath();
      if (itksys::SystemTools::FileExists(srcIbd))
      {
        const std::string dstIbd = GetNamedOutputPath(dstImzML, ".ibd"); 
        MITK_INFO << "[ImzMLImageIO::Write] Copy: " << srcIbd << " -> " << dstIbd;
        itksys::SystemTools::CopyFileAlways(srcIbd, dstIbd);
        }

      auto srcBase = itksys::SystemTools::GetFilenameWithoutExtension(srcImzML);
      auto dstBase = itksys::SystemTools::GetFilenameWithoutExtension(dstImzML);

      // Copy associated files if they exist at the source
      auto copyIfExists = [&](const std::string &suffix)
      {
        const std::string src = srcBase + suffix;
        if (itksys::SystemTools::FileExists(src))
        {
          const std::string dst = dstBase + suffix;
          MITK_INFO << "[ImzMLImageIO::Write] Copy: " << src << " -> " << dst;
          itksys::SystemTools::CopyFileAlways(src, dst);
        }
      };

      copyIfExists(".mask.nrrd");
      copyIfExists(".shift.nrrd");
      copyIfExists(".mps");
      for (auto type : m2::NormalizationStrategyTypeList)
      {
        const auto typeName = m2::NormalizationStrategyTypeNames[to_underlying(type)];
        copyIfExists("." + typeName + ".nrrd");
      }
    }

  } // namespace mitk

  mitk::IFileIO::ConfidenceLevel ImzMLImageIO::GetReaderConfidenceLevel() const
  {
    if (AbstractFileIO::GetReaderConfidenceLevel() == Unsupported)
      return Unsupported;
    return Supported;
  }


  std::vector<mitk::BaseData::Pointer> ImzMLImageIO::DoRead()
  {
    std::string mzGroupId, intGroupId;
    m2::ImzMLSpectrumImage::Pointer object = m2::ImzMLSpectrumImage::New();
    bool verboseOutput = false;
    if (auto *preferencesService = mitk::CoreServices::GetPreferencesService())
      if (auto *preferences = preferencesService->GetSystemPreferences())
        verboseOutput = preferences->GetBool("m2aia.spectrumimage.verbose_output", false);
    object->SetVerboseOutput(verboseOutput);

    auto filename = itksys::SystemTools::GetFilenameWithoutExtension(GetInputLocation());
    auto parentDir = itksys::SystemTools::GetParentDirectory(GetInputLocation());


    auto pathWithoutExtension = parentDir + "/" + filename;
    std::string ibdPath = pathWithoutExtension + ".ibd";
    std::string ibdPathUpper = pathWithoutExtension + ".IBD";
    
    if(!itksys::SystemTools::FileExists(ibdPath)){
      if (itksys::SystemTools::FileExists(ibdPathUpper))
        ibdPath = ibdPathUpper;
      else
      {
        MITK_WARN << "No binary file (.ibd/.IBD) found for: " << pathWithoutExtension;
      }
    }

    // m2::ImzMLSpectrumImage::ImzMLImageSource source;
    object->SetImzMLDataPath(this->GetInputLocation());
    object->SetBinaryDataPath(ibdPath);
    // object->GetImzMLSpectrumImageSourceList().emplace_back(source);
    object->GetSpectrumType().Format = SpectrumFormat::None;
    object->GetSpectrumType().XAxisLabel = "m/z";
    object->GetSpectrumType().YAxisLabel = "Intensity";

    {
      m2::ImzMLParser::ReadImageMetaData(object);
      m2::ImzMLParser::ReadImageSpectrumMetaData(object);
    }
    {
      object->InitializeGeometry();
    }
    {
      EvaluateSpectrumFormatType(object);
      LoadAssociatedData(object);
    }

    object->SetProperty("binary", mitk::BoolProperty::New(false));

    return {object.GetPointer()};
  }

  void ImzMLImageIO::EvaluateSpectrumFormatType(m2::SpectrumImage *object)
  {
    auto imzMLData = dynamic_cast<m2::ImzMLSpectrumImage *>(object);
    auto spectra = imzMLData->GetSpectra();

    bool hasTheSameMZAxisOffset = true;
    const auto mzAxisOffset = spectra[0].mzOffset;

    for (auto s : spectra)
    {
      if (s.mzOffset != mzAxisOffset)
      {
        hasTheSameMZAxisOffset = false;
      }
    }

    // check if the format type is set in the imzML fileContent element
    std::string formatType = object->GetPropertyValue<std::string>("m2aia.imzml.format_type");

    if (formatType.empty())
    {
      MITK_ERROR << "No format type found in the ImzML fileContent element. [IMS:1000030, IMS:1000031]";
      if (hasTheSameMZAxisOffset)
      {
        MITK_WARN << "All spectra have the same mz offset: " << hasTheSameMZAxisOffset;
      }
      formatType = "continuous";
      MITK_WARN << "Format type set to: '" << formatType << "'";

      object->SetPropertyValue<std::string>("m2aia.imzml.format_type", formatType);
    }

    // check if the spectrum type is set in the imzML fileContent element
    std::string spectrumType = object->GetPropertyValue<std::string>("m2aia.imzml.spectrum_type");

    if (spectrumType.empty())
    {
      MITK_ERROR << "No spectrum type found in the ImzML fileContent element. [MS:1000127, MS:1000128]";
      MITK_WARN << "Could not load the dataset due to invalid imzML file! Make sure to properly set the format type "
                   "and spectrum type in the imzML fileContent element.";
      MITK_WARN << "Fallback to centroid spectrum (MS:1000128)";
    }

    // MITK_WARN << "formatType: " << formatTypeProp << formatType;
    // MITK_WARN << "spectrumType: " << spectrumTypeProp << spectrumType;
    bool isProcessedSpectrum = formatType == "processed";
    bool isContinuousSpectrum = formatType == "continuous";
    bool isProfileSpectrum = spectrumType == "profile spectrum";
    bool isCentroidSpectrum = spectrumType == "centroid spectrum";

    if (!isProcessedSpectrum && !isContinuousSpectrum)
    {
      MITK_ERROR << "Set the continuos IMS:1000030) or processed (IMS:1000031) property in the ImzML "
                    "fileContent element.";
      MITK_ERROR << "Fallback to continuos (IMS:1000030)";

      object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousProfile;
    }

    if (!isCentroidSpectrum && !isProfileSpectrum)
    {
      MITK_ERROR << "Set the profile spectrum (MS:1000127) or centroid spectrum (MS:1000128) property in the ImzML "
                    "fileContent element.";
      if (isProcessedSpectrum)
      {
        MITK_ERROR << "Fallback to processed (IMS:1000031) centroid spectrum (MS:1000127)";
        object->GetSpectrumType().Format = m2::SpectrumFormat::ProcessedCentroid;
        object->SetPropertyValue("[MS:1000127] centroid spectrum", std::string());
        object->SetPropertyValue("m2aia.imzml.spectrum_type", "centroid spectrum"s);
      }

      if (isContinuousSpectrum)
      {
        MITK_ERROR << "Fallback to continuos (IMS:1000030) profile spectrum (MS:1000128)";
        object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousProfile;
        object->SetPropertyValue("[MS:1000128] profile spectrum", std::string());
        object->SetPropertyValue("m2aia.imzml.spectrum_type", "profile spectrum"s);
      }
    }

    if (object->GetProperty("[MS:1000128] profile spectrum") && object->GetProperty("[IMS:1000031] processed"))
    {
      MITK_WARN << "Processed profile spectrum is not fully supported! Check the ImzML file.";
    }

    if (isContinuousSpectrum && isProfileSpectrum)
      object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousProfile;
    else if (isProcessedSpectrum && isProfileSpectrum)
      object->GetSpectrumType().Format = m2::SpectrumFormat::ProcessedProfile;
    else if (isContinuousSpectrum && isCentroidSpectrum)
      object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousCentroid;
    else if (isProcessedSpectrum && isCentroidSpectrum)
      object->GetSpectrumType().Format = m2::SpectrumFormat::ProcessedCentroid;
  }

  void ImzMLImageIO::LoadAssociatedData(m2::ImzMLSpectrumImage *object)
  {
    const auto paths = BuildAssociatedPaths(GetInputLocation());
    const auto resolveAssociatedPath = [&](const std::string &suffix) {
      return PreferSidecarThenLegacy(paths.sidecarBase + suffix, paths.legacyBase + suffix);
    };

    auto maskPath = resolveAssociatedPath(".mask.nrrd");
    if (itksys::SystemTools::FileExists(maskPath))
    {
      auto maskData = mitk::IOUtil::Load(maskPath).at(0);
      
      // Try to cast to MultiLabelSegmentation first
      if (auto lsImage = dynamic_cast<mitk::MultiLabelSegmentation *>(maskData.GetPointer()))
      {
        auto groupImage = lsImage->GetGroupImage(0);
        if (ValidateChildImage(object, groupImage))
        {
          // Store the MultiLabelSegmentation directly
          object->SetMultilabelSegmentation(lsImage);
        }
      }
      // Otherwise, convert regular Image to MultiLabelSegmentation
      else if (auto maskImage = dynamic_cast<mitk::Image *>(maskData.GetPointer()))
      {
        if (ValidateChildImage(object, maskImage))
        {
          auto newLsImage = mitk::MultiLabelSegmentation::New();
          newLsImage->InitializeByLabeledImage(maskImage);
          // Store the MultiLabelSegmentation
          object->SetMultilabelSegmentation(newLsImage);
        }
      }
    }

    auto shiftImagePath = resolveAssociatedPath(".shift.nrrd");
    if (itksys::SystemTools::FileExists(shiftImagePath))
    {
      auto data = mitk::IOUtil::Load(shiftImagePath).at(0);
      object->SetShiftImage(dynamic_cast<mitk::Image *>(data.GetPointer()));
      data->GetGeometry()->SetOrigin(object->GetGeometry()->GetOrigin());
      data->GetGeometry()->SetSpacing(object->GetGeometry()->GetSpacing());
      data->GetGeometry()->SetIndexToWorldTransformByVtkMatrixWithoutChangingSpacing(object->GetGeometry()->GetVtkMatrix());
    }

    // for (auto type : m2::NormalizationStrategyTypeList)
    // {
    //   if (type == m2::NormalizationStrategyType::None)
    //     continue;

    //   auto normalizationImage = object->GetNormalizationImage(type);
    //   if (normalizationImage.IsNull())
    //     continue;

    //   const auto numPixels = std::accumulate(
    //     object->GetDimensions(),
    //     object->GetDimensions() + object->GetDimension(),
    //     1u,
    //     std::multiplies<unsigned int>());
    //   const auto numPixelsLoaded = std::accumulate(
    //     normalizationImage->GetDimensions(),
    //     normalizationImage->GetDimensions() + normalizationImage->GetDimension(),
    //     1u,
    //     std::multiplies<unsigned int>());

    //   const auto expectedComponentType = mitk::MakeScalarPixelType<m2::NormImagePixelType>().GetComponentType();
    //   const auto loadedComponentType = normalizationImage->GetPixelType().GetComponentType();

    //   if (numPixels != numPixelsLoaded || normalizationImage->GetDimension() != 3 || loadedComponentType != expectedComponentType)
    //   {
    //     MITK_WARN << "Normalization image does not match the expected float 3D layout for type "
    //               << m2::to_string(type) << ". Using lazy regeneration instead.";
    //     object->SetNormalizationImage(nullptr, type);
    //   }
    // }

    // auto normPath = pathWithoutExtension + ".norm.nrrd";
    // if (itksys::SystemTools::FileExists(normPath))
    // {
    //   auto data = mitk::IOUtil::Load(normPath).at(0);

    //   if (mitk::Equal(*object->GetGeometry(), *data->GetGeometry()))
    //   {
    //     auto type = m2::NormalizationStrategyType::External;
    //     object->SetNormalizationImage(dynamic_cast<mitk::Image *>(data.GetPointer()), type);
    //     object->SetNormalizationImageStatus(type, true);
    //   }
    //   else
    //   {
    //     MITK_ERROR << "External normalization image geometry is not equal to the loaded data";
    //   }
    // }

    auto pointsPath = resolveAssociatedPath(".mps");
    if (itksys::SystemTools::FileExists(pointsPath))
    {
      auto data = mitk::IOUtil::Load(pointsPath).at(0);
      object->SetPoints(dynamic_cast<mitk::PointSet *>(data.GetPointer()));
    }
  }

  ImzMLImageIO *ImzMLImageIO::IOClone() const
  {
    return new ImzMLImageIO(*this);
  }
} // namespace m2

#undef BOOST_TIMER_ENABLE_DEPRECATED