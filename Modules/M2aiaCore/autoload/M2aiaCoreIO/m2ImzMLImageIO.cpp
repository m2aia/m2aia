/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <Poco/SHA1Engine.h>
#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/progress.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <future>
#include <itksys/SystemTools.hxx>
#include <m2ImzMLEngine.h>
#include <m2ImzMLImageIO.h>
#include <m2ImzMLTemplate.h>
#include <m2ImzMLXMLParser.h>
#include <m2PeakDetection.h>
#include <m2Pooling.h>
#include <mitkIOMimeTypes.h>
#include <mitkIOUtil.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLocaleSwitch.h>
#include <mitkProgressBar.h>
#include <m2Timer.h>
#include <numeric>
#include <thread>

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
  ImzMLImageIO::ImzMLImageIO() : AbstractFileIO(mitk::Image::GetStaticNameOfClass(), IMZML_MIMETYPE(), "imzML Image")
  {
    AbstractFileWriter::SetRanking(10);
    AbstractFileReader::SetRanking(10);
    this->RegisterService();
  }

  mitk::IFileIO::ConfidenceLevel ImzMLImageIO::GetWriterConfidenceLevel() const
  {
    if (AbstractFileIO::GetWriterConfidenceLevel() == Unsupported)
      return Unsupported;
    const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());
    if (input)
      return Supported;
    else
      return Unsupported;
  }

  std::string ImzMLImageIO::GetIBDOutputPath() const
  {
    auto outFile = GetImzMLOutputPath();
    std::string ibdOutPath = itksys::SystemTools::GetParentDirectory(outFile) + "/" +
                             itksys::SystemTools::GetFilenameWithoutExtension(outFile) + ".ibd";
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

  void ImzMLImageIO::WriteContinuousProfile(m2::ImzMLSpectrumImage::SourceListType &sourceList) const
  {
    const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());

    MITK_INFO << "ImzMLImageSource list size " << sourceList.size();
    std::vector<double> mzs;
    std::vector<double> ints;

    // Output file stream for ibd
    std::ofstream b(GetIBDOutputPath(), std::ofstream::binary | std::ofstream::app);

    unsigned sourceId = 0;
    unsigned long long offset = 16;
    unsigned long long offsetDelta = 0;
    // b.seekp(offset);

    // write mzs
    {
      auto &source = sourceList.front();
      // write mass axis
      input->ReceivePositions(source.m_Spectra[0].id, mzs, sourceId);

      source.m_Spectra[0].mzOffset = 16;
      source.m_Spectra[0].mzLength = mzs.size();

      switch (input->GetXOutputType())
      {
        case m2::NumericType::Float:
          writeData<float>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source.m_Spectra[0].mzLength * sizeof(float);
          break;
        case m2::NumericType::Double:
          writeData<double>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source.m_Spectra[0].mzLength * sizeof(double);
          break;
      }
      offset += offsetDelta;
    }

    for (auto &source : sourceList)
    {
      for (auto &s : source.m_Spectra)
      {
        // update mz axis info
        s.mzLength = sourceList.front().m_Spectra[0].mzLength;
        s.mzOffset = sourceList.front().m_Spectra[0].mzOffset;

        // write ints
        {
          input->ReceiveIntensities(s.id, ints, sourceId);

          s.intOffset = offset;
          s.intLength = ints.size();

          switch (input->GetYOutputType())
          {
            case m2::NumericType::Float:
              writeData<float>(std::begin(ints), std::end(ints), b);
              offsetDelta = s.intLength * sizeof(float);
              break;
            case m2::NumericType::Double:
              writeData<double>(std::begin(ints), std::end(ints), b);
              offsetDelta = s.intLength * sizeof(double);
              break;
          }

          offset += offsetDelta;
        }
        b.flush();
      }
      ++sourceId;
    }
  }
  void ImzMLImageIO::WriteContinuousCentroid(m2::ImzMLSpectrumImage::SourceListType &sourceList) const
  {
    const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());
    auto massIndicesMask = input->GetPeaks();
    if (massIndicesMask.empty())
      mitkThrow() << "No mass mask indices provided!";

    std::vector<double> mzs, mzsMasked;
    std::vector<double> ints, intsMasked;

    // Output file stream for ibd
    std::ofstream b(GetIBDOutputPath(), std::ofstream::binary | std::ofstream::app);

    unsigned sourceId = 0;
    unsigned long long offset = 16;
    unsigned long long offsetDelta = 0;
    b.seekp(offset);
    // write mzs
    {
      auto &source = sourceList.front();
      // write mass axis
      input->ReceivePositions(source.m_Spectra[0].id, mzs, sourceId);
      for (auto i : massIndicesMask)
      {
        mzsMasked.push_back(mzs[i.massAxisIndex]);
      }
      mzs = mzsMasked;

      // update source spectra meta data to its actual values
      source.m_Spectra[0].mzOffset = 16;
      source.m_Spectra[0].mzLength = mzs.size();

      switch (input->GetXOutputType())
      {
        case m2::NumericType::Float:
          writeData<float>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source.m_Spectra[0].mzLength * sizeof(float);
          break;
        case m2::NumericType::Double:
          writeData<double>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source.m_Spectra[0].mzLength * sizeof(double);
          break;
      }
      offset += offsetDelta;
    }

    for (auto &source : sourceList)
    {
      for (auto &s : source.m_Spectra)
      {
        // update mz axis info
        s.mzLength = sourceList.front().m_Spectra[0].mzLength;
        s.mzOffset = sourceList.front().m_Spectra[0].mzOffset;

        // write ints
        {
          input->ReceiveSpectrum(s.id, mzs, ints, sourceId);

          for (auto p : massIndicesMask)
          {
            auto i = p.massAxisIndex;
            double val = 0;
            if (input->GetTolerance() == 0)
            {
              val = ints[i];
            }
            else
            {
              auto tol = input->GetTolerance() * 10e-6 * mzs[i];
              auto subRes = m2::Signal::Subrange(mzs, mzs[i] - tol, mzs[i] + tol);
              auto s = std::next(std::begin(ints), subRes.first);
              auto e = std::next(s, subRes.second);

              val = Signal::RangePooling<double>(s, e, input->GetRangePoolingStrategy());
            }
            intsMasked.push_back(val);
          }

          ints = std::move(intsMasked);
          intsMasked.clear();

          s.intOffset = offset;
          s.intLength = ints.size();

          switch (input->GetYOutputType())
          {
            case m2::NumericType::Float:
              writeData<float>(std::begin(ints), std::end(ints), b);
              offsetDelta = s.intLength * sizeof(float);
              break;
            case m2::NumericType::Double:
              writeData<double>(std::begin(ints), std::end(ints), b);
              offsetDelta = s.intLength * sizeof(double);
              break;
          }

          offset += offsetDelta;
        }
        b.flush();
      }
      ++sourceId;
    }
  }

  void ImzMLImageIO::WriteProcessedProfile(m2::ImzMLSpectrumImage::SourceListType & /*sourceList*/) const
  {
    mitkThrow() << "Not implemented";
  }

  void ImzMLImageIO::WriteProcessedCentroid(m2::ImzMLSpectrumImage::SourceListType &sourceList) const
  {
    const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());

    // Output file stream for ibd
    std::ofstream b(GetIBDOutputPath(), std::ofstream::binary | std::ofstream::app);

    // unsigned sourceId = 0;
    unsigned long long offset = 16;
    unsigned long long offsetDelta = 0;
    b.seekp(offset);
    // write mzs
    {
      using namespace std;
      vector<double> xs, ys, mzs, ints;

      // std::vector<unsigned int> xs_index;
      unsigned int sourceId = 0;
      for (auto &source : sourceList)
      {
        MITK_INFO << "Write binary data ...";
        boost::progress_display show_progress(source.m_Spectra.size());

        for (unsigned int spectrumId = 0; spectrumId < source.m_Spectra.size(); ++spectrumId)
        {
          const auto &peaks = source.m_Spectra[spectrumId].peaks;
          xs.reserve(peaks.size());
          ys.reserve(peaks.size());
          // update source spectra meta data to its actual values
          source.m_Spectra[spectrumId].mzOffset = offset;
          source.m_Spectra[spectrumId].mzLength = peaks.size();
          xs.clear();
          ys.clear();

          input->ReceiveSpectrum(spectrumId, mzs, ints, sourceId);

          for (const auto &p : peaks)
          {
            xs.push_back(p.mass);
            if (input->GetTolerance() == 0)
            {
              ys.push_back(p.intensity);
            }
            else
            {
              const auto tol = input->GetTolerance() * 10e-6 * mzs[p.massAxisIndex];
              const auto subRes = m2::Signal::Subrange(mzs, mzs[p.massAxisIndex] - tol, mzs[p.massAxisIndex] + tol);
              const auto s = next(begin(ints), subRes.first);
              const auto e = next(s, subRes.second);

              ys.push_back(Signal::RangePooling<double>(s, e, input->GetRangePoolingStrategy()));
            }
          }

          switch (input->GetXOutputType())
          {
            case m2::NumericType::Float:
              writeData<float>(begin(xs), end(xs), b);
              offsetDelta = xs.size() * sizeof(float);
              break;
            case m2::NumericType::Double:
              writeData<double>(begin(xs), end(xs), b);
              offsetDelta = xs.size() * sizeof(double);
              break;
          }
          offset += offsetDelta;
          source.m_Spectra[spectrumId].intOffset = offset;
          source.m_Spectra[spectrumId].intLength = peaks.size();

          switch (input->GetYOutputType())
          {
            case m2::NumericType::Float:
              writeData<float>(begin(ys), end(ys), b);
              offsetDelta = ys.size() * sizeof(float);
              break;
            case m2::NumericType::Double:
              writeData<double>(begin(ys), end(ys), b);
              offsetDelta = ys.size() * sizeof(double);
              break;
          }
          offset += offsetDelta;
          b.flush();
          ++show_progress;
        }
        ++sourceId;
      }
    } // namespace m2

    MITK_INFO << "File-Size " << offset;
  }

  void ImzMLImageIO::Write()
  {
    ValidateOutputLocation();

    const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());
    input->SaveModeOn();

    std::string uuidString;
    {
      // Write UUID string to ibd
      std::ofstream b(GetIBDOutputPath(), std::ofstream::binary);
      boost::uuids::basic_random_generator<boost::mt19937> gen;
      boost::uuids::uuid u = gen();
      uuidString = boost::uuids::to_string(u);
      b.seekp(0);
      b.write((char *)(u.data), u.static_size());
      b.flush();
      MITK_INFO << u.data;
    }

    try
    {
      MITK_INFO << "Start writing ImzML!";
      //// define working vectors
      std::vector<double> mzs;
      std::vector<double> ints;

      using m2::SpectrumFormatType;

      // copy the whole source meta data container; spectra information is
      // updated based on the save mode.
      // mz and ints meta data is manipuulated to write a correct imzML xml structure
      // copy of sources is discared after writing
      m2::ImzMLSpectrumImage::SourceListType sourceCopy(input->GetImzMLSpectrumImageSourceList());
      std::string sha1;
      switch (input->GetExportMode())
      {
        case SpectrumFormatType::ContinuousProfile:
          this->WriteContinuousProfile(sourceCopy);
          break;
        case SpectrumFormatType::ProcessedCentroid:
          this->WriteProcessedCentroid(sourceCopy);
          break;
        case SpectrumFormatType::ContinuousCentroid:
          this->WriteContinuousCentroid(sourceCopy);
          break;
        case SpectrumFormatType::ProcessedProfile:
          mitkThrow() << "ProcessedProfile export type is not supported!";
          break;
        default:
          break;
      }

      Poco::SHA1Engine generator;
      std::ifstream bf(GetIBDOutputPath(), std::fstream::binary);
      char *c = new char[2048];
      unsigned long long l = 0;
      while (bf)
      {
        bf.read(c, 2048);
        generator.update(c, bf.gcount());
        l += bf.gcount();
      }

      MITK_INFO << "bytes " << l;

      std::map<std::string, std::string> context;

      switch (input->GetExportMode())
      {
        case SpectrumFormatType::None:
          mitkThrow() << "SpectrumFormatType::None type is not supported!";
          break;
        case SpectrumFormatType::ContinuousCentroid:
          context["spectrumtype"] = "centroid spectrum";
          context["mode"] = "continuous";
          break;
        case SpectrumFormatType::ContinuousProfile:
          context["spectrumtype"] = "profile spectrum";
          context["mode"] = "continuous";
          break;
        case SpectrumFormatType::ProcessedCentroid:
          context["spectrumtype"] = "centroid spectrum";
          context["mode"] = "processed";
          break;
        case SpectrumFormatType::ProcessedProfile:
          context["spectrumtype"] = "profile spectrum";
          context["mode"] = "processed";
          break;
      }
      std::string sha1string = Poco::SHA1Engine::digestToHex(generator.digest());
      MITK_INFO << "[ibd SHA1] " << sha1string << "\n";
      MITK_INFO << "[uuid] " << uuidString << "\n";
      // context["mode"] = "continuous";
      context["uuid"] = uuidString;
      context["sha1sum"] = sha1string;
      unsigned mzBytes = 0;
      unsigned intBytes = 0;
      switch (input->GetXOutputType())
      {
        case m2::NumericType::Double:
          context["mz_data_type"] = "64-bit float";
          mzBytes = 8;
          break;
        case m2::NumericType::Float:
          context["mz_data_type"] = "32-bit float";
          mzBytes = 4;
          break;
      }

      switch (input->GetYOutputType())
      {
        case m2::NumericType::Double:
          context["int_data_type"] = "64-bit float";
          intBytes = 8;
          break;
        case m2::NumericType::Float:
          context["int_data_type"] = "32-bit float";
          intBytes = 4;
          break;
      }

      context["mz_data_type_code"] = m2::Template::TextToCodeMap[context["mz_data_type"]];
      context["int_data_type_code"] = m2::Template::TextToCodeMap[context["int_data_type"]];
      context["mz_compression"] = "no compression";
      context["int_compression"] = "no compression";

      context["mode_code"] = m2::Template::TextToCodeMap[context["mode"]];
      context["spectrumtype_code"] = m2::Template::TextToCodeMap[context["spectrumtype"]];

      context["size_x"] = std::to_string(input->GetDimensions()[0]);
      context["size_y"] = std::to_string(input->GetDimensions()[1]);
      context["size_z"] = std::to_string(input->GetDimensions()[2]);

      context["max dimension x"] =
        std::to_string(unsigned(input->GetDimensions()[0] * input->GetGeometry()->GetSpacing()[0] * 1e3));
      context["max dimension y"] =
        std::to_string(unsigned(input->GetDimensions()[1] * input->GetGeometry()->GetSpacing()[1] * 1e3));
      context["max dimension z"] =
        std::to_string(unsigned(input->GetDimensions()[2] * input->GetGeometry()->GetSpacing()[2] * 1e3));

      context["pixel size x"] = std::to_string(unsigned(input->GetGeometry()->GetSpacing()[0] * 1e3));
      context["pixel size y"] = std::to_string(unsigned(input->GetGeometry()->GetSpacing()[1] * 1e3));
      context["pixel size z"] = std::to_string(unsigned(input->GetGeometry()->GetSpacing()[2] * 1e3));

      context["origin x"] = std::to_string(input->GetGeometry()->GetOrigin()[0] * 1e3);
      context["origin y"] = std::to_string(input->GetGeometry()->GetOrigin()[1] * 1e3);
      context["origin z"] = std::to_string(input->GetGeometry()->GetOrigin()[2] * 1e3);

      context["run_id"] = std::to_string(0);
      const auto &sources = input->GetImzMLSpectrumImageSourceList();
      auto N = std::accumulate(
        std::begin(sources), std::end(sources), unsigned(0), [](auto s, auto v) { return s + v.m_Spectra.size(); });
      context["num_spectra"] = std::to_string(N);

      context["int_compression_code"] = m2::Template::TextToCodeMap[context["int_compression"]];
      context["mz_compression_code"] = m2::Template::TextToCodeMap[context["mz_compression"]];

      // Output file stream for imzML
      std::ofstream f(GetImzMLOutputPath(), std::ofstream::binary);

      std::string view = m2::Template::IMZML_TEMPLATE_START;
      f << m2::TemplateEngine::render(view, context);

      view = m2::Template::IMZML_SPECTRUM_TEMPLATE;
      unsigned long id = 0;
      for (const auto &source : sourceCopy)
      {
        MITK_INFO << "Write imzML data ...";
        boost::progress_display show_progress(source.m_Spectra.size());
        for (auto &s : source.m_Spectra)
        {
          auto x = s.index[0] + source.m_Offset[0] + 1; // start by 1
          auto y = s.index[1] + source.m_Offset[1] + 1; // start by 1
          auto z = s.index[2] + source.m_Offset[2] + 1; // start by 1

          context = {{"index", std::to_string(id++)},
                     {"x", std::to_string(x)},
                     {"y", std::to_string(y)},
                     {"z", std::to_string(z)},
                     {"mz_len", std::to_string(s.mzLength)},
                     {"mz_enc_len", std::to_string(s.mzLength * mzBytes)},
                     {"mz_offset", std::to_string(s.mzOffset)},
                     {"int_len", std::to_string(s.intLength)},
                     {"int_enc_len", std::to_string(s.intLength * intBytes)},
                     {"int_offset", std::to_string(s.intOffset)}};

          auto nonConst_input = const_cast<m2::ImzMLSpectrumImage *>(input);
          mitk::ImagePixelReadAccessor<m2::NormImagePixelType> nacc(nonConst_input->GetNormalizationImage());
          if (nacc.GetPixelByIndex(s.index + source.m_Offset) != 1)
          {
            context["tic"] = std::to_string(nacc.GetPixelByIndex(s.index + source.m_Offset));
          }
          f << m2::TemplateEngine::render(view, context);
          f.flush();
          ++show_progress;
        }
      }

      f << m2::Template::IMZML_TEMPLATE_END;
      f.close();
    }
    catch (std::exception &e)
    {
      input->SaveModeOff();
      MITK_ERROR << "Writing ImzML Faild! " + std::string(e.what());
    }

    input->SaveModeOff();
  } // namespace mitk

  mitk::IFileIO::ConfidenceLevel ImzMLImageIO::GetReaderConfidenceLevel() const
  {
    if (AbstractFileIO::GetReaderConfidenceLevel() == Unsupported)
      return Unsupported;
    return Supported;
  }

  std::vector<mitk::BaseData::Pointer> ImzMLImageIO::DoRead()
  {
    MITK_INFO << "Start reading imzML...";

    std::string mzGroupId, intGroupId;
    auto object = m2::ImzMLSpectrumImage::New();

    auto pathWithoutExtension = this->GetInputLocation();
    itksys::SystemTools::ReplaceString(pathWithoutExtension, ".imzML", "");

    if (!itksys::SystemTools::FileExists(pathWithoutExtension + ".ibd"))
      mitkThrow() << "No such file " << pathWithoutExtension;

    m2::ImzMLSpectrumImage::ImzMLImageSource source;
    source.m_ImzMLDataPath = this->GetInputLocation();
    source.m_BinaryDataPath = pathWithoutExtension + ".ibd";
    object->GetImzMLSpectrumImageSourceList().emplace_back(source);

    object->SetImportMode(SpectrumFormatType::None);
    {
      m2::Timer t("Parsing imzML");
      m2::ImzMLXMLParser::FastReadMetaData(object);
      m2::ImzMLXMLParser::SlowReadMetaData(object);
    }

    {
      m2::Timer t("Initialize placeholder images and spectra");
      object->InitializeGeometry();
    }
    {
       m2::Timer t("Load external data");
      EvaluateSpectrumFormatType(object);
      LoadAssociatedData(object);
    }


    return {object.GetPointer()};
  }

  void ImzMLImageIO::EvaluateSpectrumFormatType(m2::SpectrumImageBase *object)
  {
    if (object->GetProperty("continuous") && object->GetProperty("profile spectrum"))
      object->SetImportMode(m2::SpectrumFormatType::ContinuousProfile);
    else if (object->GetProperty("processed") && object->GetProperty("profile spectrum"))
      object->SetImportMode(m2::SpectrumFormatType::ProcessedProfile);
    else if (object->GetProperty("continuous") && object->GetProperty("centroid spectrum"))
      object->SetImportMode(m2::SpectrumFormatType::ContinuousCentroid);
    else if (object->GetProperty("processed") && object->GetProperty("centroid spectrum"))
      object->SetImportMode(m2::SpectrumFormatType::ProcessedCentroid);

    if (!object->GetProperty("processed") && !object->GetProperty("continuous"))
    {
      MITK_ERROR << "Set the continous (IMS:1000030) or processed (IMS:1000031) property in the ImzML "
                    "fileContent element.";
      MITK_ERROR << "Fallback to continous (MS:1000030)";
      object->SetImportMode(m2::SpectrumFormatType::ContinuousProfile);
    }

    if (!object->GetProperty("centroid spectrum") && !object->GetProperty("profile spectrum"))
    {
      MITK_ERROR << "Set the profile spectrum (MS:1000127) or centroid spectrum (MS:1000128) property in the ImzML "
                    "fileContent element.";
      MITK_ERROR << "Fallback to profile spectrum (MS:1000127)";
      object->SetImportMode(m2::SpectrumFormatType::ContinuousProfile);
    }
  }

  void ImzMLImageIO::LoadAssociatedData(m2::ImzMLSpectrumImage *object)
  {
    auto pathWithoutExtension = this->GetInputLocation();
    itksys::SystemTools::ReplaceString(pathWithoutExtension, ".imzML", "");

    auto source = object->GetImzMLSpectrumImageSource();

    if (itksys::SystemTools::FileExists(pathWithoutExtension + ".nrrd"))
    {
      source.m_MaskDataPath = pathWithoutExtension + ".nrrd";
      auto data = mitk::IOUtil::Load(source.m_MaskDataPath).at(0);
      object->GetImageArtifacts()["mask"] = dynamic_cast<mitk::Image *>(data.GetPointer());
      object->UseExternalMaskOn();
    }

    if (itksys::SystemTools::FileExists(pathWithoutExtension + ".mps"))
    {
      source.m_PointsDataPath = pathWithoutExtension + ".mps";
      auto data = mitk::IOUtil::Load(source.m_PointsDataPath).at(0);
      object->GetImageArtifacts()["references"] = dynamic_cast<mitk::PointSet *>(data.GetPointer());
    }
  }

  ImzMLImageIO *ImzMLImageIO::IOClone() const { return new ImzMLImageIO(*this); }
} // namespace m2
