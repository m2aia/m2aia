/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <m2ImzMLMassSpecImage.h>
#include <mitkCommandLineParser.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <stdlib.h>

std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  auto argsMap = CommandlineParsing(argc, argv);

  const auto setDefaultValue = [&argsMap](const auto &s, const auto defaultValue, auto *v) {
    if (argsMap.count(s))
    {
      *v = us::any_cast<typename std::remove_pointer<decltype(v)>::type>(argsMap[s]);
    }
    else
    {
      argsMap[s] = defaultValue;
      *v = defaultValue;
    }
  };

  for (auto kv : argsMap)
  {
    MITK_INFO << kv.first << " " << kv.second.ToString();
  }
  auto image = mitk::IOUtil::Load(argsMap["image"].ToString()).front();
  if (auto imzMLImage = dynamic_cast<m2::ImzMLMassSpecImage *>(image.GetPointer()))
  {
    int bc_strategy, bc_hws, sm_strategy, sm_hws, int_type, mzs_type, imaging_strategy;
    float binning_tol, centroids_tol, mz, tol;

    setDefaultValue("baseline-correction", 0, &bc_strategy);
    setDefaultValue("baseline-correction-hws", 25, &bc_hws);
    setDefaultValue("smoothing", 0, &sm_strategy);
    setDefaultValue("smoothing-hws", 2, &sm_hws);

    setDefaultValue("intensity-type", 0, &int_type);
    setDefaultValue("mz-type-hws", 0, &mzs_type);
    setDefaultValue("imaging-strategy", 1, &imaging_strategy);

    setDefaultValue("binning-tol", 50, &binning_tol);
    setDefaultValue("centroids-tol", 50, &centroids_tol);
    setDefaultValue("smoothing-hws", 2, &sm_hws);

    setDefaultValue("mz", -1, &mz);
    setDefaultValue("tol", 0, &tol);

    imzMLImage->SetBaselineCorrectionStrategy(static_cast<m2::BaselineCorrectionType>(bc_strategy));
    imzMLImage->SetSmoothingHalfWindowSize(bc_hws);

    imzMLImage->SetSmoothingStrategy(static_cast<m2::SmoothingType>(sm_strategy));
    imzMLImage->SetBaseLinecorrectionHalfWindowSize(sm_hws);

    imzMLImage->SetBinningTolerance(binning_tol);
    imzMLImage->SetTolerance(centroids_tol);
    imzMLImage->InitializeImageAccess();

    imzMLImage->SetExportMode(m2::SpectrumFormatType::ContinuousProfile);
    imzMLImage->SetIntsOutputType(static_cast<m2::NumericType>(int_type));
    imzMLImage->SetMzsOutputType(static_cast<m2::NumericType>(mzs_type));

    if (mz > 0)
      imzMLImage->GenerateImageData(mz, 10e-6 * tol * mz, nullptr, imzMLImage);

    

    mitk::IOUtil::Save(imzMLImage, argsMap["output"].ToString());
  }
}

std::map<std::string, us::Any> CommandlineParsing(int argc, char *argv[])
{
  mitkCommandLineParser parser;
  parser.setArgumentPrefix("--", "-");
  // required params
  parser.addArgument("image",
                     "i",
                     mitkCommandLineParser::Image,
                     "Input imzML Image",
                     "Path to the input imzML",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("binning-tol",
                     "bt",
                     mitkCommandLineParser::Float,
                     "Read continuous centroid imzML and apply binning to peaks.",
                     "Collapsing tolerance of nearby peaks; in ppm",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("centroids-tol",
                     "p",
                     mitkCommandLineParser::Float,
                     "Centroids tolerance",
                     "If centroid data are written, resulting centroid intensity values are accumulated over the "
                     "centroid m/z +/- Centroids-tolerance",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("baseline-correction",
                     "b",
                     mitkCommandLineParser::Int,
                     "Baseline correction strategy",
                     "None = 0, TopHat = 1, Median = 2",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("baseline-correction-hws",
                     "b-hws",
                     mitkCommandLineParser::Int,
                     "Baseline-correction half window size",
                     "The parameter is used to create a convolution of 2*hws+1",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("smoothing",
                     "s",
                     mitkCommandLineParser::Int,
                     "Smoothing strategy",
                     "None = 0, SavitzkyGolay = 1, Gaussian = 2",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("smoothing-hws",
                     "s-hws",
                     mitkCommandLineParser::Int,
                     "Smoothing half window size",
                     "The parameter is used to create a convolution of 2*hws+1",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("mz-type",
                     "mt",
                     mitkCommandLineParser::Int,
                     "m/z value output data type",
                     "32 bit float = 1, 64 bit float = 1",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);

  parser.addArgument("intensity-type",
                     "it",
                     mitkCommandLineParser::Int,
                     "intensity value output data type",
                     "32 bit float = 1, 64 bit float = 1",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("imaging-strategy",
                     "s",
                     mitkCommandLineParser::Int,
                     "Accumulation function applied on the range m/z +/- tol",
                     "mean = 1, maximum = 2, median = 3,sum = 4",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("mz",
                     "mz",
                     mitkCommandLineParser::Float,
                     "Create an ion imaging at m/z position.",
                     "m/z value",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("tol",
                     "tol",
                     mitkCommandLineParser::Float,
                     "Create an ion imaging at m/z position with tolerance +/- tol.",
                     "tol value in Daltons",
                     us::Any(),
                     true,
                     false,
                     false,
                     mitkCommandLineParser::Input);
  parser.addArgument("output",
                     "o",
                     mitkCommandLineParser::File,
                     "Output Image",
                     "Path to the output image path",
                     us::Any(),
                     false,
                     false,
                     false,
                     mitkCommandLineParser::Output);

  // Miniapp Infos
  parser.setCategory("M2aia Tools");
  parser.setTitle("ImzMLConverter");
  parser.setDescription("Read a valid imzML file and write a new image.");
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
