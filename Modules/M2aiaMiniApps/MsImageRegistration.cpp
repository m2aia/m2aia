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
#include <itksys/SystemTools.hxx>
#include <m2ImzMLSpectrumImage.h>
#include <m2Process.hpp>
#include <mapDeploymentDLLAccess.h>
#include <mapDeploymentDLLDirectoryBrowser.h>
#include <mapDeploymentDLLHandle.h>
#include <mapDeploymentDLLInfo.h>
#include <mapRegistration.h>
#include <mitkCommandLineParser.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImage3DSliceToImage2DFilter.h>
#include <mitkImageMappingHelper.h>
#include <mitkMAPAlgorithmHelper.h>
#include <mitkMAPRegistrationWrapper.h>
#include <mutex>
#include <numeric>
#include <stdlib.h>


std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  auto argsMap = CommandlineParsing(argc, argv);

  auto ifs = std::ifstream(argsMap["parameterfile"].ToString());
  std::string params(std::istreambuf_iterator<char>{ifs}, {}); // read whole file

  auto GetIonImage = [&params](auto sImage) -> mitk::Image::Pointer {
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

    auto filter = mitk::Image3DSliceToImage2DFilter::New();
    filter->SetInput(i1);
    filter->Update();

    return filter->GetOutput();
  };

  mitk::Image::Pointer f =
    dynamic_cast<mitk::Image *>(mitk::IOUtil::Load(argsMap["fixed"].ToString()).front().GetPointer());
  mitk::Image::Pointer m =
    dynamic_cast<mitk::Image *>(mitk::IOUtil::Load(argsMap["moving"].ToString()).front().GetPointer());

  if (f && m)
  {
    /*auto f = GetIonImage(fixed);
    auto m = GetIonImage(moving);
    auto s = f->GetGeometry()->GetSpacing();
    s[0] = 1.0;
    s[1] = 1.0;
    s[2] = 1.0;
    f->GetGeometry()->SetSpacing(s);

    s = m->GetGeometry()->GetSpacing();
    s[0] = 1.0;
    s[1] = 1.0;
    s[2] = 1.0;
    m->GetGeometry()->SetSpacing(s);

    mitk::IOUtil::Save(f, argsMap["output"].ToString() + "/tfixed.nrrd");
    mitk::IOUtil::Save(m, argsMap["output"].ToString() + "/tmoving.nrrd");*/
    MITK_INFO << itksys::SystemTools::GetCurrentWorkingDirectory();
    const auto searchPath = itksys::SystemTools::GetParentDirectory(argv[0]);
    auto browser = map::deployment::DLLDirectoryBrowser::New();
    for (std::string path : {searchPath})
    {
      browser->addDLLSearchLocation(path);
    }

    browser->update();
    auto list = browser->getLibraryInfos();
    auto algo = std::find_if(std::begin(list), std::end(list), [](decltype(list)::value_type v) {
      return v->getAlgorithmUID().getName().find("MultiModal") != std::string::npos &&
             v->getAlgorithmUID().getName().find("rigid") != std::string::npos;
    });

    MITK_INFO << (*algo)->getAlgorithmUID();

    ::map::deployment::DLLHandle::Pointer tempDLLHandle =
      ::map::deployment::openDeploymentDLL((*algo)->getLibraryFilePath());
    ::map::algorithm::RegistrationAlgorithmBase::Pointer tempAlgorithm =
      ::map::deployment::getRegistrationAlgorithm(tempDLLHandle);

    mitk::MAPAlgorithmHelper helper(tempAlgorithm);
    mitk::MAPAlgorithmHelper::CheckError::Type err;
    if (helper.CheckData(m, f, err))
    {
      MITK_INFO << "Valid data! Start registration ...";
      helper.SetData(m, f);
      auto res = helper.GetMITKRegistrationWrapper();

      auto resImage = mitk::ImageMappingHelper::map(m, res, false, 0, f->GetGeometry());
      mitk::IOUtil::Save(resImage, argsMap["output"].ToString() + "warped.nrrd");
      mitk::IOUtil::Save(res, argsMap["output"].ToString() + "warped.mapr");
    }
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
