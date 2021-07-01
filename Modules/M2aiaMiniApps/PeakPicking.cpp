/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <algorithm>
#include <boost/progress.hpp>
#include <m2ImzMLSpectrumImage.h>
#include <m2PeakDetection.h>
#include <m2Process.hpp>
#include <mitkCommandLineParser.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mutex>
#include <numeric>
#include <stdlib.h>

std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  auto argsMap = CommandlineParsing(argc, argv);

  auto ifs = std::ifstream(argsMap["parameterfile"].ToString());
  std::string params(std::istreambuf_iterator<char>{ifs}, {}); // read whole file

  auto image = mitk::IOUtil::Load(argsMap["input"].ToString()).front();

  for (auto kv : argsMap)
  {
    MITK_INFO << kv.first << " " << kv.second.ToString();
  }
  if (auto sImage = dynamic_cast<m2::SpectrumImageBase *>(image.GetPointer()))
  {
    if (sImage->GetImportMode() != m2::SpectrumFormatType::ContinuousProfile)
    {
      MITK_ERROR << "Only imzML files in continuous profile mode are accepted!";
      return 1;
    }
    /*int bc_strategy, bc_hws, sm_strategy, sm_hws, int_type, mzs_type;
    float binning_tol, centroids_tol, mz, tol;*/
    using namespace std::string_literals;
    std::map<std::string, std::string> pMap;
    const auto bsc_s = m2::Find(params, "baseline-correction", "None"s, pMap);
    const auto bsc_hw = m2::Find(params, "baseline-correction-hw", int(50), pMap);
    const auto sm_s = m2::Find(params, "smoothing", "None"s, pMap);
    const auto sm_hw = m2::Find(params, "smoothing-hw", int(2), pMap);
    const auto norm = m2::Find(params, "normalization", "None"s, pMap);
    const auto pool = m2::Find(params, "pooling", "Maximum"s, pMap);
    const auto tol = m2::Find(params, "tolerance", double(0), pMap);
    const auto threads = m2::Find(params, "threads", int(10), pMap);
    const auto binning_tol = m2::Find(params, "binning-tolerance", double(0), pMap);
    const auto SNR = m2::Find(params, "SNR", double(1.5), pMap);
    const auto peakpicking_hw = m2::Find(params, "peakpicking-hw", int(5), pMap);
    const auto monoisotopick = m2::Find(params, "monoisotopic", bool(false), pMap);
    const auto y_output_type = m2::Find(params, "y-type", "Float"s, pMap);
    const auto x_output_type = m2::Find(params, "x-type", "Float"s, pMap);

    sImage->SetBaselineCorrectionStrategy(static_cast<m2::BaselineCorrectionType>(m2::SIGNAL_MAPPINGS.at(bsc_s)));
    sImage->SetBaseLineCorrectionHalfWindowSize(bsc_hw);

    sImage->SetSmoothingStrategy(static_cast<m2::SmoothingType>(m2::SIGNAL_MAPPINGS.at(sm_s)));
    sImage->SetSmoothingHalfWindowSize(sm_hw);

    sImage->SetNormalizationStrategy(static_cast<m2::NormalizationStrategyType>(m2::SIGNAL_MAPPINGS.at(norm)));
    sImage->SetRangePoolingStrategy(static_cast<m2::RangePoolingStrategyType>(m2::SIGNAL_MAPPINGS.at(pool)));

    sImage->InitializeImageAccess();
    sImage->SetTolerance(tol);
    sImage->SetBinningTolerance(binning_tol);

    sImage->SetExportMode(m2::SpectrumFormatType::ProcessedCentroid);
    sImage->SetYOutputType(static_cast<m2::NumericType>(m2::CORE_MAPPINGS.at(y_output_type)));
    sImage->SetXOutputType(static_cast<m2::NumericType>(m2::CORE_MAPPINGS.at(x_output_type)));

    if (auto imzMLImage = dynamic_cast<m2::ImzMLSpectrumImage *>(sImage))
    {
      const auto &xValues = imzMLImage->GetXAxis();
      int sourceId = 0;

      for (auto &source : imzMLImage->GetImzMLSpectrumImageSourceList())
      {
        MITK_INFO << "Start peak picking for " << source.m_Spectra.size() << " spectra...";
        boost::progress_display show_progress(source.m_Spectra.size());

        m2::Process::Map(source.m_Spectra.size(), threads, [&](const auto /*tId*/, const auto s, const auto e) {
          std::vector<double> yValues(xValues.size());
          for (unsigned int spectrumId = s; spectrumId < e; ++spectrumId)
          {
            imzMLImage->ReceiveIntensities(spectrumId, yValues, sourceId);
            source.m_Spectra[spectrumId].peaks =
              m2::Signal::PickPeaks(xValues, yValues, SNR, peakpicking_hw, binning_tol, monoisotopick);
            ++show_progress;
          }
        });
        ++sourceId;

        float sumPeaks = 0;
        for (const auto &p : source.m_Spectra)
          sumPeaks += p.peaks.size();

		MITK_INFO << "Average number of peaks " << sumPeaks / double(source.m_Spectra.size());
      }
    }

    mitk::IOUtil::Save(sImage, argsMap["output"].ToString());
  }
}

std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[])
{
  mitkCommandLineParser parser;
  parser.setArgumentPrefix("--", "-");
  // required params
  parser.addArgument("input",
                     "i",
                     mitkCommandLineParser::Image,
                     "Input imzML Image",
                     "Path to the input imzML",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument(
    "parameterfile",
    "p",
    mitkCommandLineParser::File,
    "peak picking parameter file",
    "Template parameter files can be found here https://github.com/jtfcordes/m2aia/tree/master/Templates",
    us::Any(),
    false,
    false,
    false,
    mitkCommandLineParser::Input);
  parser.addArgument("output",
                     "o",
                     mitkCommandLineParser::Image,
                     "Output Image",
                     "Path to the output image path",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Output);

  // Miniapp Infos
  parser.setCategory("M2aia Tools");
  parser.setTitle("PeakPicking");
  parser.setDescription("Read a imzML file and a peak picked version.");
  parser.setContributor("Jonas Cordes");
  auto parsedArgs = parser.parseArguments(argc, argv);

  if (parsedArgs.size() == 0)
  {
    exit(EXIT_FAILURE);
  }
  if (parsedArgs.count("help") || parsedArgs.count("h"))
  {
    exit(EXIT_SUCCESS);
  }

  return parsedArgs;
}
