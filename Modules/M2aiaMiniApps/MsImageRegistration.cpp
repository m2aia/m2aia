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

  auto image = mitk::IOUtil::Load(argsMap["fixed"].ToString()).front();

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
    const auto bsc_s = m2::Find(params, "baseline-correction", "None"s);
    const auto bsc_hw = m2::Find(params, "baseline-correction-hw", int(50));
    const auto sm_s = m2::Find(params, "smoothing", "None"s);
    const auto sm_hw = m2::Find(params, "smoothing-hw", int(2));
    const auto norm = m2::Find(params, "normalization", "None"s);
    const auto pool = m2::Find(params, "pooling", "Maximum"s);
    const auto tol = m2::Find(params, "tolerance", double(0));
    const auto threads = m2::Find(params, "threads", int(10));
    const auto binning_tol = m2::Find(params, "binning-tolerance", double(0));
    const auto SNR = m2::Find(params, "SNR", double(1.5));
    const auto peakpicking_hw = m2::Find(params, "peakpicking-hw", int(5));
    const auto monoisotopick = m2::Find(params, "monoisotopic", bool(false));
    const auto y_output_type = m2::Find(params, "y-type", "Float"s);
    const auto x_output_type = m2::Find(params, "x-type", "Float"s);
    const auto x_center = m2::Find(params, "x-center", double(0));
    const auto x_tol = m2::Find(params, "x-tol", double(100));
    const auto x_tol_ppm = m2::Find(params, "x-tol-ppm", bool(true));

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
    auto i1 = sImage->Clone();
    sImage->UpdateImage(x_center, x_tol, nullptr, i1);

   
    mitk::IOUtil::Save(i1, argsMap["output"].ToString()+"/test.nrrd");
  }
}

std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[])
{
  mitkCommandLineParser parser;
  parser.setArgumentPrefix("--", "-");
  // required params
  parser.addArgument("fixed",
                     "f",
                     mitkCommandLineParser::Image,
                     "Fixed input MS image",
                     "Path to the fixed input image",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("moving",
                     "m",
                     mitkCommandLineParser::Image,
                     "Moving input MS image",
                     "Path to the moving input image",
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
  parser.setTitle("MsImageRegistration");
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
