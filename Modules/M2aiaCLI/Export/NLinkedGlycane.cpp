/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <itksys/SystemTools.hxx>
#include <m2ImzMLMassSpecImage.h>
#include <m2ImzMLXMLParser.h>
#include <m2NoiseEstimators.hpp>
#include <m2PeakDetection.hpp>
#include <mbilog.h>
#include <mitkCommandLineParser.h>
#include <mitkIOUtil.h>
#include <mitkImageCast.h>
#include <mitkTimer.h>
/*!
\brief Perform N-linked glycane analysis
*/
int main(int /*argc*/, char * /*argv*/[])
{
  //	mitkCommandLineParser parser;

  //	parser.setTitle("Gibbs Tracking");
  //	parser.setCategory("Fiber Tracking and Processing Methods");
  //	parser.setDescription("Perform global fiber tractography (Gibbs tractography)");
  //	parser.setContributor("MIC");

  //	parser.setArgumentPrefix("--", "-");
  //	parser.addArgument("", "i", mitkCommandLineParser::String, "Input:", "input image (tensor, ODF or SH-coefficient
  // image)", us::Any(), false, false, false, mitkCommandLineParser::Input); 	parser.addArgument("", "o",
  // mitkCommandLineParser::String, "Output:", "output tractogram", us::Any(), false, false, false,
  // mitkCommandLineParser::Output); 	parser.addArgument("parameters", "", mitkCommandLineParser::String, "Parameters:",
  //"parameter file (.gtp)", us::Any(), false, false, false, mitkCommandLineParser::Input); 	parser.addArgument("mask",
  //"", mitkCommandLineParser::String, "Mask:", "binary mask image", us::Any(), false, false, false,
  // mitkCommandLineParser::Input);

  //	std::map<std::string, us::Any> parsedArgs = parser.parseArguments(argc, argv);
  //	if (parsedArgs.size() == 0)
  //		return EXIT_FAILURE;

  //	std::string inFileName = us::any_cast<std::string>(parsedArgs["i"]);
  //	std::string paramFileName = us::any_cast<std::string>(parsedArgs["parameters"]);
  //	std::string outFileName = us::any_cast<std::string>(parsedArgs["o"]

  std::string png1 = "D:\\HSMannheim\\Data\\MFoell\\png1-no_normalization.imzML";
  std::string png2 = "D:\\HSMannheim\\Data\\MFoell\\png2-no_normalization.imzML";
  std::string control = "D:\\HSMannheim\\Data\\MFoell\\control-no_normalization.imzML";
  // std::string cal = "D:\\HSMannheim\\Data\\MFoell\\png1-no_normalization.imzML";

  auto dataVec = mitk::IOUtil::Load({png1, png2, control});
  m2::ImzMLMassSpecImage::Pointer imagePNG1, imagePNG2, imageControl;
  imagePNG1 = dynamic_cast<m2::ImzMLMassSpecImage *>(dataVec[0].GetPointer());
  imagePNG2 = dynamic_cast<m2::ImzMLMassSpecImage *>(dataVec[1].GetPointer());
  imageControl = dynamic_cast<m2::ImzMLMassSpecImage *>(dataVec[2].GetPointer());

  if (!(imageControl || imagePNG1 || imagePNG2))
  {
    MITK_ERROR << "Fail to load images.";
  }
  std::map<mitk::BaseData *, std::vector<m2::MassValue>> imagePeaks;
  std::map<mitk::BaseData *, std::vector<m2::MassValue>> pixelPeaks;
  for (auto I : {imagePNG1, imagePNG2, imageControl})
  {
    m2::ImzMLXMLParser::SlowReadMetaData(I);
    const auto &source = I->GetSpectraSource();
    auto filename = itksys::SystemTools::GetFilenameWithoutExtension(source._BinaryDataPath);
    I->UseBaseLineCorrection = true;
    I->UseSmoothing = true;

    I->SetSmoothingHalfWindowSize(10);
    I->SetBaseLinecorrectionHalfWindowSize(50);
    // I->SetUpperMZBound(3000);
    // I->SetLowerMZBound(900);

    I->InitializeImageAccess();

    // peak picking on overview spectrum
    auto &overviewSpectrum = I->MeanSpectrum();
    auto SNR = m2::Noise::mad(overviewSpectrum);
    std::vector<m2::MassValue> peaks;
    m2::Peaks::localMaxima(std::begin(overviewSpectrum),
                           std::end(overviewSpectrum),
                           std::begin(I->MassAxis()),
                           std::back_inserter(peaks),
                           10,
                           SNR * 5.5);

    std::vector<m2::MassValue> filteredPeaks;
    std::copy_if(std::begin(peaks), std::end(peaks), std::back_inserter(filteredPeaks), [](const auto &p) {
      return (p.mass > 900 && p.mass < 3000);
    });

    imagePeaks[I.GetPointer()] = m2::Peaks::monoisotopic(peaks, {3, 4, 5, 6, 7, 8, 9, 10}, 0.40);
    MITK_INFO << I->GetSpectraSource()._ImzMLDataPath << " monoisotopic peaks found "
              << imagePeaks[I.GetPointer()].size();
  }

  std::vector<m2::MassValue> unionPeaks, binPeaks;
  for (const auto &kv : imagePeaks)
    unionPeaks.insert(std::end(unionPeaks), std::begin(kv.second), std::end(kv.second));
  std::sort(std::begin(unionPeaks), std::end(unionPeaks));

  m2::Peaks::binPeaks(std::begin(unionPeaks), std::end(unionPeaks), std::back_inserter(binPeaks), 50 * 10e-6);

  MITK_INFO << "Size of common indieces " << binPeaks.size() << "\n";

  for (auto &I : {imagePNG1, imagePNG2, imageControl})
  {
    const auto &source = I->GetSpectraSource();
    auto filename = itksys::SystemTools::GetFilenameWithoutExtension(source._BinaryDataPath);
    for (const auto &p : binPeaks)
      I->GetPeaks().push_back(p);
    I->SetExportMode(m2::ImzMLFormatType::ContinuousCentroid);
    //mitk::IOUtil::Save(I, "D:\\" + filename + "_testresult.imzML");
  }

  try
  {
    auto pngAll = m2::ImzMLMassSpecImage::Combine(imagePNG1, imagePNG2, 'x');
	pngAll  = m2::ImzMLMassSpecImage::Combine(pngAll , imageControl, 'x');

    pngAll->SetNormalizationStrategy(m2::NormalizationStrategyType::TIC);
    pngAll->UseBaseLineCorrection = true;
    pngAll->UseSmoothing = true;
    pngAll->SetSmoothingHalfWindowSize(4);
    pngAll->SetBaseLinecorrectionHalfWindowSize(50);
    pngAll->InitializeImageAccess();
    pngAll->SetMassPickingTolerance(45);

	mitk::IOUtil::Save(pngAll->GetNormalizationImage(), "D:/CombiResult_normalization.nrrd");

//    auto filter = m2::ImageToTSNEImageFilter::New();
//    filter->SetMaskImage(pngAll->GetMaskImage());
//	filter->SetPerplexity(10);
//	filter->SetIterations(2000);
	

//	auto filter2 = m2::PcaEigenImageFilter::New();
//	filter2->SetMaskImage(pngAll->GetMaskImage());
	

    auto geom = pngAll->GetGeometry()->Clone();
    unsigned i = 0;
    std::list<mitk::Image::Pointer> list;

    for (const auto &p : binPeaks)
    {
      pngAll->GetPeaks().push_back(p);

      list.insert(list.end(), mitk::Image::New());
      list.back()->Initialize(mitk::MakeScalarPixelType<m2::IonImagePixelType>(), *geom);

      double mz = pngAll->MassAxis().at(p.massAxisIndex);
      pngAll->GrabIonImage(mz, 10 * 10e-6 * mz, nullptr, list.back());
//      filter->SetInput(i, list.back());
//	  filter2->SetInput(i, list.back());
	  ++i;
    }

//    filter->Update();
//	filter2->Update();
//	mitk::IOUtil::Save(filter->GetOutput(), "D:\\Combi_testresult_tsne.nrrd");
//    mitk::IOUtil::Save(filter2->GetOutput(), "D:\\Combi_testresult_pca.nrrd");
    
    pngAll->SetExportMode(m2::ImzMLFormatType::ContinuousCentroid);
	mitk::IOUtil::Save(pngAll, "D:\\Combi_testresult.imzML");
  }
  catch (std::exception &e)
  {
    MITK_WARN << e.what();
  }

  MITK_INFO << imagePNG1->MassAxis().front() << " " << imagePNG1->MassAxis().back();
  MITK_INFO << imagePNG2->MassAxis().front() << " " << imagePNG2->MassAxis().back();
  MITK_INFO << imageControl->MassAxis().front() << " " << imageControl->MassAxis().back();
}
