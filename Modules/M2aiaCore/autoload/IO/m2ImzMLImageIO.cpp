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
#include <m2NoiseEstimators.hpp>
#include <m2PeakDetection.hpp>
#include <mitkIOMimeTypes.h>
#include <mitkIOUtil.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLocaleSwitch.h>
#include <mitkProgressBar.h>
#include <mitkTimer.h>
#include <numeric>
#include <thread>

template <class ConversionType, class ItFirst, class ItLast, class OStreamType>
void writeData(ItFirst itFirst, ItLast itLast, OStreamType &os)
{
  std::for_each(itFirst, itLast, [&os](const auto &v) {
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
    const auto *input = static_cast<const m2::ImzMLMassSpecImage *>(this->GetInput());
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

  void ImzMLImageIO::WriteContinuousProfile(m2::ImzMLMassSpecImage::SourceListType &sourceList) const
  {
    const auto *input = static_cast<const m2::ImzMLMassSpecImage *>(this->GetInput());

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
      input->GrabMass(source._Spectra[0].id, mzs, sourceId);

      source._Spectra[0].mzOffset = 16;
      source._Spectra[0].mzLength = mzs.size();

      switch (input->GetMzsOutputType())
      {
        case m2::NumericType::Float:
          writeData<float>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source._Spectra[0].mzLength * sizeof(float);
          break;
        case m2::NumericType::Double:
          writeData<double>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source._Spectra[0].mzLength * sizeof(double);
          break;
      }
      offset += offsetDelta;
    }

    for (auto &source : sourceList)
    {
      for (auto &s : source._Spectra)
      {
        // update mz axis info
        s.mzLength = sourceList.front()._Spectra[0].mzLength;
        s.mzOffset = sourceList.front()._Spectra[0].mzOffset;

        // write ints
        {
          input->GrabIntensity(s.id, ints, sourceId);

          s.intOffset = offset;
          s.intLength = ints.size();

          switch (input->GetIntsOutputType())
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
  void ImzMLImageIO::WriteContinuousCentroid(m2::ImzMLMassSpecImage::SourceListType &sourceList) const
  {
    const auto *input = static_cast<const m2::ImzMLMassSpecImage *>(this->GetInput());
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
      input->GrabMass(source._Spectra[0].id, mzs, sourceId);
      for (auto i : massIndicesMask)
      {
        mzsMasked.push_back(mzs[i.massAxisIndex]);
      }
      mzs = mzsMasked;

      // update source spectra meta data to its actual values
      source._Spectra[0].mzOffset = 16;
      source._Spectra[0].mzLength = mzs.size();

      switch (input->GetMzsOutputType())
      {
        case m2::NumericType::Float:
          writeData<float>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source._Spectra[0].mzLength * sizeof(float);
          break;
        case m2::NumericType::Double:
          writeData<double>(std::begin(mzs), std::end(mzs), b);
          offsetDelta = source._Spectra[0].mzLength * sizeof(double);
          break;
      }
      offset += offsetDelta;
    }

    for (auto &source : sourceList)
    {
      for (auto &s : source._Spectra)
      {
        // update mz axis info
        s.mzLength = sourceList.front()._Spectra[0].mzLength;
        s.mzOffset = sourceList.front()._Spectra[0].mzOffset;

        // write ints
        {
          input->GrabSpectrum(s.id, mzs, ints, sourceId);

          for (auto p : massIndicesMask)
          {
            auto i = p.massAxisIndex;
            double val = 0;
            if (input->GetMassPickingTolerance() == 0)
            {
              val = ints[i];
            }
            else
            {
              auto tol = input->GetMassPickingTolerance() * 10e-6 * mzs[i];
              auto subRes = m2::Peaks::Subrange(mzs, mzs[i] - tol, mzs[i] + tol);
              auto s = std::next(std::begin(ints), subRes.first);
              auto e = std::next(s, subRes.second);
              switch (input->GetIonImageGrabStrategy())
              {
                case m2::IonImageGrabStrategyType::None:
                  break;
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
            }
            intsMasked.push_back(val);
          }

          ints = std::move(intsMasked);
          intsMasked.clear();

          s.intOffset = offset;
          s.intLength = ints.size();

          switch (input->GetIntsOutputType())
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
  void ImzMLImageIO::WriteProcessedProfile(m2::ImzMLMassSpecImage::SourceListType & /*sourceList*/) const
  {
    mitkThrow() << "Not implemented";
  }
  void ImzMLImageIO::WriteProcessedCentroid(m2::ImzMLMassSpecImage::SourceListType & /*sourceList*/) const {}


  void ImzMLImageIO::Write()
  {
    ValidateOutputLocation();

    const auto *input = static_cast<const m2::ImzMLMassSpecImage *>(this->GetInput());
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

      using m2::ImzMLFormatType;

      // copy the whole source meta data container; spectra information is
      // updated based on the save mode.
      // mz and ints meta data is manipuulated to write a correct imzML xml structure
      // copy of sources is discared after writing
      m2::ImzMLMassSpecImage::SourceListType sourceCopy(input->GetSourceList());
      std::string sha1;
      switch (input->GetExportMode())
      {
        case ImzMLFormatType::ContinuousProfile:
          this->WriteContinuousProfile(sourceCopy);
          break;

        case ImzMLFormatType::ProcessedMonoisotopicCentroid:
        case ImzMLFormatType::ProcessedCentroid:
          this->WriteProcessedCentroid(sourceCopy);
          break;

        case ImzMLFormatType::ContinuousMonoisotopicCentroid:
        case ImzMLFormatType::ContinuousCentroid:
          this->WriteContinuousCentroid(sourceCopy);
          break;
        case ImzMLFormatType::ProcessedProfile:
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
        case ImzMLFormatType::NotSet:
          break;
        case ImzMLFormatType::ContinuousMonoisotopicCentroid:
          break;
        case ImzMLFormatType::ContinuousCentroid:
          context["spectrumtype"] = "centroid spectrum";
          context["mode"] = "continuous";
          break;
        case ImzMLFormatType::ContinuousProfile:
          context["spectrumtype"] = "profile spectrum";
          context["mode"] = "continuous";
          break;
        case ImzMLFormatType::ProcessedCentroid:
          context["spectrumtype"] = "centroid spectrum";
          context["mode"] = "processed";
          break;
        case ImzMLFormatType::ProcessedProfile:
          context["spectrumtype"] = "profile spectrum";
          context["mode"] = "processed";
          break;
        case ImzMLFormatType::ProcessedMonoisotopicCentroid:
          context["spectrumtype"] = "centroid spectrum";
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
      switch (input->GetMzsOutputType())
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

      switch (input->GetIntsOutputType())
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
      const auto &sources = input->GetSourceList();
      auto N = std::accumulate(
        std::begin(sources), std::end(sources), unsigned(0), [](auto s, auto v) { return s + v._Spectra.size(); });
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
        for (auto &s : source._Spectra)
        {
          auto x = s.index[0] + source._offset[0] + 1; // start by 1
          auto y = s.index[1] + source._offset[1] + 1; // start by 1
          auto z = s.index[2] + source._offset[2] + 1; // start by 1

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

          auto nonConst_input = const_cast<m2::ImzMLMassSpecImage *>(input);
          mitk::ImagePixelReadAccessor<m2::NormImagePixelType> nacc(nonConst_input->GetNormalizationImage());
          if (nacc.GetPixelByIndex(s.index + source._offset) != 1)
          {
            context["tic"] = std::to_string(nacc.GetPixelByIndex(s.index+ source._offset));
          }
          f << m2::TemplateEngine::render(view, context);
          f.flush();
          //++show_progress;
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
    std::string mzGroupId, intGroupId;
    auto object = m2::ImzMLMassSpecImage::New();

    auto p(this->GetInputLocation());

    if (!itksys::SystemTools::FileExists(p))
    {
      MITK_ERROR << "No such file " << p;
      return {nullptr};
    }

    auto pIbd(this->GetInputLocation());
    itksys::SystemTools::ReplaceString(pIbd, ".imzML", ".ibd");
    
    if (!itksys::SystemTools::FileExists(pIbd))
    {
      mitkThrow() << "No such file " << pIbd;
    }

    {
      mitk::Timer t("Initialize ImzML");
	  m2::ImzMLMassSpecImage::Source source;
	  source._BinaryDataPath = pIbd;
	  source._ImzMLDataPath = GetInputLocation();
	  source.ImportMode = m2::ImzMLFormatType::NotSet;

      object->GetSourceList().emplace_back(source);

      m2::ImzMLXMLParser::FastReadMetaData(object);
      m2::ImzMLXMLParser::SlowReadMetaData(object);
      object->InitializeGeometry();
      
    }

    // ----------------- FILE VALIDATAION ---------------

    /*	auto tohex = [](unsigned char *bytes, int bytesLength)
      {
        const char lookup[] = "0123456789abcdef";
        std::string s;
        s.reserve(256);
        for (int i = 0; i < bytesLength; i++) {
          s += lookup[bytes[i] >> 4];
          s += lookup[bytes[i] & 0xF];
        }
        return s;
      };*/

    //{ //uuid check
    //	std::string uuid;
    //	auto prop = object->GetProperty("universally unique identifier");
    //	if (prop) {
    //		auto strProp = dynamic_cast<mitk::GenericProperty<std::string> *>(prop.GetPointer());
    //		uuid = strProp->GetValue();
    //	}

    //	std::ifstream f(binaryPath, std::ifstream::binary);
    //	f.seekg(0);
    //	char uuid_ibd[16];
    //	f.read(uuid_ibd, 16);
    //
    //	auto s1 = tohex((unsigned char*)uuid_ibd, 16);
    //	auto s2 = uuid;

    //	MITK_INFO << s1 << " : " << s2;

    //	//if (.compare(uuid) != 0) {
    //		//mitkThrow() << "uuid of ibd and imzml not equal";
    //	//}
    //}
    //{ // ibd sha1 check
    //	boost::uuids::detail::sha1 sha1_generator;
    //	unsigned int sha1_ibd[5];

    //	std::ifstream f(binaryPath, std::ifstream::binary);
    //	const size_t blocksize = 65536;
    //	auto block = std::make_unique<char[]>(blocksize);
    //
    //	size_t size = std::ifstream(binaryPath, std::ifstream::ate | std::ifstream::binary).tellg();
    //	while (true) {
    //		f.read(block.get(), blocksize);
    //		if (f.eof()) {
    //			auto residuals = size % blocksize;
    //			sha1_generator.process_bytes(block.get(), residuals);
    //			break;
    //		}
    //		else {
    //			sha1_generator.process_bytes(block.get(), blocksize);
    //		}
    //	}
    //
    //	unsigned int digest[5];
    //	sha1_generator.get_digest(digest);
    //	for (int i = 0; i < 5; ++i)
    //	{
    //		const char* tmp = reinterpret_cast<char*>(digest);
    //		char* tmp_hash = reinterpret_cast<char*>(sha1_ibd);
    //		tmp_hash[i * 4] = tmp[i * 4 + 3];
    //		tmp_hash[i * 4 + 1] = tmp[i * 4 + 2];
    //		tmp_hash[i * 4 + 2] = tmp[i * 4 + 1];
    //		tmp_hash[i * 4 + 3] = tmp[i * 4];
    //	}

    //
    //}

    return {object.GetPointer()};
  }

  ImzMLImageIO *ImzMLImageIO::IOClone() const { return new ImzMLImageIO(*this); }
} // namespace mitk
