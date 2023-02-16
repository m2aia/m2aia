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
#include <boost/progress.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <itksys/SystemTools.hxx>
#include <m2ImzMLEngine.h>
#include <m2ImzMLImageIO.h>
#include <m2ImzMLParser.h>
#include <m2ImzMLTemplate.h>
#include <m2Timer.h>
#include <mitkIOUtil.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <signal/m2PeakDetection.h>
#include <signal/m2Pooling.h>
#include <itkMath.h>
#include <mitkLocaleSwitch.h>

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
    const char * category = "ImzMLImageIO::WriteContinuousProfile";
    MITK_INFO(category) << "ImzMLImageSource list size " << sourceList.size();

    // WHAT A MESS 
    std::vector<float> mzs;
    std::vector<float> ints;

    std::ifstream ibd(input->GetImzMLSpectrumImageSource().m_BinaryDataPath, std::ofstream::binary);
    std::ofstream b(GetIBDOutputPath(), std::ofstream::binary | std::ofstream::app);

    unsigned sourceId = 0;
    unsigned long long offset = 16;
    unsigned long long offsetDelta = 0;

    // Used for min/max limits on x axis
    std::pair<unsigned int, unsigned int> bounds;

    boost::progress_display show_progress(std::accumulate(std::begin(sourceList),std::end(sourceList),0, [](auto s, m2::ImzMLSpectrumImage::ImzMLImageSource source){
      return s + source.m_Spectra.size();}) + 1);

    // write mzs
    {
      auto &source = sourceList.front();
      // input->GetSpectrum(0, mzs, ints, sourceId); // get x axis
      // bounds = {0, mzs.size()};
      // image properties
      auto useLimits = input->GetExportSpectrumType().UseLimits;

      // Bounds of sub-spectrum
      if (useLimits)
      {
        auto xLimMin = input->GetExportSpectrumType().XLimMin;
        auto xLimMax = input->GetExportSpectrumType().XLimMax;
        bounds = m2::Signal::Subrange(mzs, xLimMin, xLimMax);
      }

      source.m_Spectra[0].mzOffset = 16;
      source.m_Spectra[0].mzLength = bounds.second;

      auto start = std::begin(mzs) + bounds.first;
      auto end = std::end(mzs) + bounds.first + bounds.second;
      switch (input->GetExportSpectrumType().XAxisType)
      {
        case m2::NumericType::Float:
          // input->GetProcessor()
          writeData<float>(start, end, b);
          offsetDelta = bounds.second * sizeof(float);
          break;
        case m2::NumericType::Double:
          writeData<double>(start, end, b);
          offsetDelta = bounds.second * sizeof(double);
          break;
      }
      offset += offsetDelta;
      ++show_progress;
    }

    MITK_INFO("ImzMLImageIO::WriteContinuousProfile") << "Write x axis done!";

    for (auto &source : sourceList)
    {
      for (size_t id = 0; id < source.m_Spectra.size(); ++id)
      {
        auto &s = source.m_Spectra[id];
        // update mz axis info
        s.mzLength = sourceList.front().m_Spectra[0].mzLength;
        s.mzOffset = sourceList.front().m_Spectra[0].mzOffset;

        // write ints
        {
          input->GetSpectrum(id, mzs, ints, sourceId);

          s.intOffset = offset;
          s.intLength = bounds.second;

          auto start = std::begin(ints) + bounds.first;
          auto end = std::end(ints) + bounds.first + bounds.second;

          switch (input->GetExportSpectrumType().YAxisType)
          {
            case m2::NumericType::Float:
              writeData<float>(start, end, b);
              offsetDelta = bounds.second * sizeof(float);
              break;
            case m2::NumericType::Double:
              writeData<double>(start, end, b);
              offsetDelta = bounds.second * sizeof(double);
              break;
          }

          offset += offsetDelta;
        }
        b.flush();
        ++show_progress;
      }
      ++sourceId;
    }
  }

  void ImzMLImageIO::SetIntervalVector(m2::IntervalVector::Pointer intervals){
    m_Intervals = intervals;
  }

  void ImzMLImageIO::WriteContinuousCentroid(m2::ImzMLSpectrumImage::SourceListType &sourceList) const
  {
    if (m_Intervals.IsNull() || m_Intervals->GetIntervals().empty())
      mitkThrow() << "No mass mask indices provided!";

    std::vector<float> mzs, mzsMasked;
    std::vector<float> ints, intsMasked;

    // Output file stream for ibd
    const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());  
    std::ofstream b(GetIBDOutputPath(), std::ofstream::binary | std::ofstream::app);

    unsigned sourceId = 0;
    unsigned long long offset = 16;
    unsigned long long offsetDelta = 0;
    b.seekp(offset);

    boost::progress_display show_progress(std::accumulate(std::begin(sourceList),std::end(sourceList),0, [](auto s, m2::ImzMLSpectrumImage::ImzMLImageSource source){
      return s + source.m_Spectra.size();}) + 1);
    // write mzs
    {
      auto &source = sourceList.front();
      // write mass axis
      // input->GetSpectrum(0, mzs, ints, sourceId);
      {
        using namespace std;
        const auto & xs = m_Intervals->GetXMean();
        copy(begin(xs), end(xs),back_inserter(mzsMasked));
      }

      // update source spectra meta data to its actual values
      source.m_Spectra[0].mzOffset = 16;
      source.m_Spectra[0].mzLength = mzsMasked.size();

      switch (input->GetExportSpectrumType().XAxisType)
      {
        case NumericType::Float:
          writeData<float>(std::begin(mzsMasked), std::end(mzsMasked), b);
          offsetDelta = source.m_Spectra[0].mzLength * sizeof(float);
          break;
        case NumericType::Double:
          writeData<double>(std::begin(mzsMasked), std::end(mzsMasked), b);
          offsetDelta = source.m_Spectra[0].mzLength * sizeof(double);
          break;
      }
      offset += offsetDelta;
      ++show_progress;
    }

    MITK_INFO("ImzMLImageIO") << "Write x axis done!";

    for (auto &source : sourceList)
    {
      for (size_t id = 0; id < source.m_Spectra.size(); ++id)
      {
        auto &s = source.m_Spectra[id];
        // update mz axis info
        s.mzLength = sourceList.front().m_Spectra[0].mzLength;
        s.mzOffset = sourceList.front().m_Spectra[0].mzOffset;

        // write ints
        {
          input->GetSpectrum(id, mzs, ints, sourceId);
          intsMasked.clear();

          for (const Interval & I : m_Intervals->GetIntervals())
          {
            
            double val = 0;
            const auto tol = input->ApplyTolerance(I.x.mean());
            const auto subRes = m2::Signal::Subrange(mzs, I.x.mean() - tol, I.x.mean() + tol);
            const auto s = std::next(std::begin(ints), subRes.first);
            const auto e = std::next(s, subRes.second);
            val = Signal::RangePooling<double>(s, e, input->GetRangePoolingStrategy());
          
            intsMasked.push_back(val);
          }

          s.intOffset = offset;
          s.intLength = intsMasked.size();

          switch (input->GetExportSpectrumType().YAxisType)
          {
            case m2::NumericType::Float:
              writeData<float>(std::begin(intsMasked), std::end(intsMasked), b);
              offsetDelta = s.intLength * sizeof(float);
              break;
            case m2::NumericType::Double:
              writeData<double>(std::begin(intsMasked), std::end(intsMasked), b);
              offsetDelta = s.intLength * sizeof(double);
              break;
          }

          offset += offsetDelta;
        }
        b.flush();
        ++show_progress;
      }
      ++sourceId;
    }
  }

  void ImzMLImageIO::WriteProcessedProfile(m2::ImzMLSpectrumImage::SourceListType & /*sourceList*/) const
  {
    mitkThrow() << "Not implemented";
  }

  void ImzMLImageIO::WriteProcessedCentroid(m2::ImzMLSpectrumImage::SourceListType & /*sourceList*/) const
  {
    // const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());

    // // Output file stream for ibd
    // std::ofstream b(GetIBDOutputPath(), std::ofstream::binary | std::ofstream::app);

    // // unsigned sourceId = 0;
    // unsigned long long offset = 16;
    // unsigned long long offsetDelta = 0;
    // b.seekp(offset);
    // // write mzs
    // {
    //   using namespace std;
    //   vector<float> xs, ys, mzs, ints;

    //   // std::vector<unsigned int> xs_index;
    //   unsigned int sourceId = 0;
    //   for (auto &source : sourceList)
    //   {
    //     MITK_INFO << "Write binary data ...";
    //     boost::progress_display show_progress(source.m_Spectra.size());

    //     for (unsigned int spectrumId = 0; spectrumId < source.m_Spectra.size(); ++spectrumId)
    //     {
    //       const auto &peaks = source.m_Spectra[spectrumId].peaks;
    //       xs.reserve(peaks.size());
    //       ys.reserve(peaks.size());
    //       // update source spectra meta data to its actual values
    //       source.m_Spectra[spectrumId].mzOffset = offset;
    //       source.m_Spectra[spectrumId].mzLength = peaks.size();
    //       xs.clear();
    //       ys.clear();

    //       input->GetSpectrum(spectrumId, mzs, ints, sourceId);

    //       for (const auto &p : peaks)
    //       {
    //         xs.push_back(p.GetX());
    //         if (input->GetTolerance() == 0)
    //         {
    //           ys.push_back(p.GetY());
    //         }
    //         else
    //         {
    //           const auto tol = input->ApplyTolerance(mzs[p.GetIndex()]);
    //           const auto subRes = m2::Signal::Subrange(mzs, mzs[p.GetIndex()] - tol, mzs[p.GetIndex()] + tol);
    //           const auto s = next(begin(ints), subRes.first);
    //           const auto e = next(s, subRes.second);

    //           ys.push_back(Signal::RangePooling<double>(s, e, input->GetRangePoolingStrategy()));
    //         }
    //       }

    //       switch (input->GetExportSpectrumType().XAxisType)
    //       {
    //         case m2::NumericType::Float:
    //           writeData<float>(begin(xs), end(xs), b);
    //           offsetDelta = xs.size() * sizeof(float);
    //           break;
    //         case m2::NumericType::Double:
    //           writeData<double>(begin(xs), end(xs), b);
    //           offsetDelta = xs.size() * sizeof(double);
    //           break;
    //       }
    //       offset += offsetDelta;
    //       source.m_Spectra[spectrumId].intOffset = offset;
    //       source.m_Spectra[spectrumId].intLength = peaks.size();

    //       switch (input->GetExportSpectrumType().YAxisType)
    //       {
    //         case m2::NumericType::Float:
    //           writeData<float>(begin(ys), end(ys), b);
    //           offsetDelta = ys.size() * sizeof(float);
    //           break;
    //         case m2::NumericType::Double:
    //           writeData<double>(begin(ys), end(ys), b);
    //           offsetDelta = ys.size() * sizeof(double);
    //           break;
    //       }
    //       offset += offsetDelta;
    //       b.flush();
    //       ++show_progress;
    //     }
    //     ++sourceId;
    //   }
    // } // namespace m2

    // MITK_INFO << "File-Size " << offset;
  }

  void ImzMLImageIO::Write()
  {
    mitk::LocaleSwitch localeSwitch("C");
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

      using m2::SpectrumFormat;

      // copy the whole source meta data container; spectra information is
      // updated based on the save mode.
      // mz and ints meta data is manipulated to write a correct imzML xml structure
      // copy of sources is discared after writing
      m2::ImzMLSpectrumImage::SourceListType sourceCopy(input->GetImzMLSpectrumImageSourceList());
      std::string sha1;
      switch (input->GetExportSpectrumType().Format)
      {
        case SpectrumFormat::ContinuousProfile:
          this->WriteContinuousProfile(sourceCopy);
          break;
        case SpectrumFormat::ProcessedCentroid:
          mitkThrow() << "ProcessedCentroid export type is not supported!";
          // this->WriteProcessedCentroid(sourceCopy);
          break;
        case SpectrumFormat::ContinuousCentroid:
          this->WriteContinuousCentroid(sourceCopy);
          break;
        case SpectrumFormat::ProcessedProfile:
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

      switch (input->GetExportSpectrumType().Format)
      {
        case SpectrumFormat::None:
          mitkThrow() << "SpectrumFormatType::None type is not supported!";
          break;
        case SpectrumFormat::ContinuousCentroid:
          context["spectrumtype"] = "centroid spectrum";
          context["mode"] = "continuous";
          break;
        case SpectrumFormat::ContinuousProfile:
          context["spectrumtype"] = "profile spectrum";
          context["mode"] = "continuous";
          break;
        case SpectrumFormat::ProcessedCentroid:
          context["spectrumtype"] = "centroid spectrum";
          context["mode"] = "processed";
          break;
        case SpectrumFormat::ProcessedProfile:
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

      switch (input->GetExportSpectrumType().XAxisType)
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

      switch (input->GetExportSpectrumType().YAxisType)
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

      auto xs = m2::MilliMeterToMicroMeter(input->GetGeometry()->GetSpacing()[0]);
      auto ys = m2::MilliMeterToMicroMeter(input->GetGeometry()->GetSpacing()[1]);
      auto zs = m2::MilliMeterToMicroMeter(input->GetGeometry()->GetSpacing()[2]);

      context["max dimension x"] =
        std::to_string(unsigned(input->GetDimensions()[0] * xs));
      context["max dimension y"] =
        std::to_string(unsigned(input->GetDimensions()[1] * ys));
      context["max dimension z"] =
        std::to_string(unsigned(input->GetDimensions()[2] * zs));

      context["pixel size x"] = std::to_string(xs);
      context["pixel size y"] = std::to_string(ys);
      context["pixel size z"] = std::to_string(zs);

      auto xo = m2::MilliMeterToMicroMeter(input->GetGeometry()->GetOrigin()[0]);
      auto yo = m2::MilliMeterToMicroMeter(input->GetGeometry()->GetOrigin()[1]);
      auto zo = m2::MilliMeterToMicroMeter(input->GetGeometry()->GetOrigin()[2]);

      context["absolute position offset x"] = std::to_string(xo);
      context["absolute position offset y"] = std::to_string(yo);
      context["absolute position offset z"] = std::to_string(zo);

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
    

    std::string mzGroupId, intGroupId;
    m2::ImzMLSpectrumImage::Pointer object = m2::ImzMLSpectrumImage::New();

    auto pathWithoutExtension = this->GetInputLocation();
    itksys::SystemTools::ReplaceString(pathWithoutExtension, ".imzML", "");

    if (!itksys::SystemTools::FileExists(pathWithoutExtension + ".ibd"))
      mitkThrow() << "No such file " << pathWithoutExtension;

    m2::ImzMLSpectrumImage::ImzMLImageSource source;
    source.m_ImzMLDataPath = this->GetInputLocation();
    source.m_BinaryDataPath = pathWithoutExtension + ".ibd";
    object->GetImzMLSpectrumImageSourceList().emplace_back(source);
    object->GetSpectrumType().Format = SpectrumFormat::None;
    

    {
      // m2::Timer t("Parsing imzML");
      m2::ImzMLParser::ReadImageMetaData(object);
      m2::ImzMLParser::ReadImageSpectrumMetaData(object);
    }

    auto dx = object->GetPropertyValue<double>("max dimension x");
    auto dy = object->GetPropertyValue<double>("max dimension y");

    auto sx = object->GetPropertyValue<unsigned>("max count of pixels x");
    auto sy = object->GetPropertyValue<unsigned>("max count of pixels y");

    auto px = object->GetPropertyValue<double>("pixel size x");
    auto py = object->GetPropertyValue<double>("pixel size y");

    px = m2::MilliMeterToMicroMeter(px);
    py = m2::MilliMeterToMicroMeter(py);

    MITK_INFO << "[imzML]: " + source.m_ImzMLDataPath +
               "\n\t[pixel size]: " + std::to_string(px)  + "x" + std::to_string(py) +
               "\n\t[image area]: " + std::to_string(sx) + "x" +std::to_string(sy) +
               "\n\t[image dims]: " + std::to_string(dx) + "x" +std::to_string(dy);

    // if(!itk::Math::AlmostEquals(sx*px, dx) ||
    //    !itk::Math::AlmostEquals(sy*py, dy)){
    //   mitkThrow() << "[ImzML Meta data error]\n" <<
    //                  "\n\t!! ImzML data should have consistent inputs. This error massage can be turned off in the properties." <<
    //                  "\n\t[pixel size]: " << std::to_string(px)  << "x" + std::to_string(py) <<
    //                  "\n\t[image area]: " << std::to_string(sx) << "x" + std::to_string(sy) <<
    //                  "\n\t[image dims]: " << std::to_string(dx) << "x" +std::to_string(dy);
    // }


    {
      // m2::Timer t("Initialize placeholder images and spectra");
      object->InitializeGeometry();
    }
    {
      // m2::Timer t("Load external data");
      EvaluateSpectrumFormatType(object);
      LoadAssociatedData(object);
    }

    return {object.GetPointer()};
  }

  void ImzMLImageIO::EvaluateSpectrumFormatType(m2::SpectrumImageBase *object)
  {
    if (object->GetProperty("continuous") && object->GetProperty("profile spectrum"))
      object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousProfile;
    else if (object->GetProperty("processed") && object->GetProperty("profile spectrum"))
      object->GetSpectrumType().Format = m2::SpectrumFormat::ProcessedProfile;
    else if (object->GetProperty("continuous") && object->GetProperty("centroid spectrum"))
      object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousCentroid;
    else if (object->GetProperty("processed") && object->GetProperty("centroid spectrum"))
      object->GetSpectrumType().Format = m2::SpectrumFormat::ProcessedCentroid;

    if (!object->GetProperty("processed") && !object->GetProperty("continuous"))
    {
      MITK_ERROR << "Set the continuos (IMS:1000030) or processed (IMS:1000031) property in the ImzML "
                    "fileContent element.";
      MITK_ERROR << "Fallback to continuos (MS:1000030)";

      object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousProfile;
    }

    if (!object->GetProperty("centroid spectrum") && !object->GetProperty("profile spectrum"))
    {
      MITK_ERROR << "Set the profile spectrum (MS:1000127) or centroid spectrum (MS:1000128) property in the ImzML "
                    "fileContent element.";
      MITK_ERROR << "Fallback to profile spectrum (MS:1000127)";
      object->GetSpectrumType().Format = m2::SpectrumFormat::ContinuousProfile;
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
