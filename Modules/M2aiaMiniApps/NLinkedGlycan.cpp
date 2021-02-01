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
#include <m2NoiseEstimators.h>
#include <m2PeakDetection.h>
#include <mbilog.h>
#include <mitkCommandLineParser.h>
#include <mitkIOUtil.h>
#include <mitkImageCast.h>
#include <mitkTimer.h>
/*!
\brief Perform N-linked glycane analysis
*/
int main(int /*argc*/, char *argv[])
{
  std::string png1 = argv[1];
  std::string png2 = argv[2];
  std::string control = argv[3];
  std::string out_path = argv[4];
 

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
    I->SetBaselineCorrectionStrategy(m2::BaselineCorrectionType::TopHat);
    I->SetSmoothingStrategy(m2::SmoothingType::SavitzkyGolay);

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
    pngAll->SetBaselineCorrectionStrategy(m2::BaselineCorrectionType::TopHat);
    pngAll->SetSmoothingStrategy(m2::SmoothingType::SavitzkyGolay);
    pngAll->SetSmoothingHalfWindowSize(4);
    pngAll->SetBaseLinecorrectionHalfWindowSize(50);
    pngAll->InitializeImageAccess();
    pngAll->SetMassPickingTolerance(25);

	//mitk::IOUtil::Save(pngAll->GetNormalizationImage(), "D:/CombiResult_normalization.nrrd");    
    pngAll->SetExportMode(m2::ImzMLFormatType::ContinuousCentroid);
	mitk::IOUtil::Save(pngAll, out_path);
  }
  catch (std::exception &e)
  {
    MITK_WARN << e.what();
  }

  MITK_INFO << imagePNG1->MassAxis().front() << " " << imagePNG1->MassAxis().back();
  MITK_INFO << imagePNG2->MassAxis().front() << " " << imagePNG2->MassAxis().back();
  MITK_INFO << imageControl->MassAxis().front() << " " << imageControl->MassAxis().back();
}
