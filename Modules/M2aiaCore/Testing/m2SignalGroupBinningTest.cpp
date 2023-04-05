/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <cppunit/TestAssert.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2TestingConfig.h>
#include <m2Timer.h>
#include <mitkIOUtil.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>
#include <signal/m2Binning.h>
#include <signal/m2SignalCommon.h>

class m2SignalGroupBinningTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2SignalGroupBinningTestSuite);
  MITK_TEST(RunGroupBinning_Strict_Tol0_002);
  MITK_TEST(RunGroupBinning_Strict_Tol0_100);
  MITK_TEST(RunGroupBinning_Strict_ProcessedCentroidData);

  CPPUNIT_TEST_SUITE_END();

public:
  void RunGroupBinning_Strict_ProcessedCentroidData()
  {
    using namespace std;

    auto data = mitk::IOUtil::Load(GetTestDataFilePath("processed_centroids.imzML", M2AIA_DATA_DIR));
    if (auto image = dynamic_cast<m2::ImzMLSpectrumImage *>(data[0].GetPointer()))
    {
      image->InitializeImageAccess();
      auto N = image->GetXAxis().size() * image->GetNumberOfValidPixels();
      vector<float> xs, ys, xsAll, ysAll;
      vector<int> ssAll;
      xsAll.reserve(N);
      ysAll.reserve(N);
      ssAll.reserve(N);

      auto xsAllIt = back_inserter(xsAll);
      auto ysAllIt = back_inserter(ysAll);
      auto ssAllIt = back_inserter(ssAll);

      for (unsigned int i = 0; i < image->GetNumberOfValidPixels(); ++i)
      {
        image->GetSpectrum(i, xs, ys);
        copy(begin(xs), end(xs), xsAllIt);
        copy(begin(ys), end(ys), ysAllIt);
        fill_n(ssAllIt, xs.size(), i);
      }

      auto runBinning = [&](){
        vector<float> xsSorted, ysSorted;
        vector<int> ssSorted;
        
        auto indices = m2::argsort(xsAll);
        xsSorted = m2::argsortApply(xsAll, indices);
        ysSorted = m2::argsortApply(ysAll, indices);
        ssSorted = m2::argsortApply(ssAll, indices);

        using dIt = decltype(cbegin(xsAll));
        using iIt = decltype(cbegin(ssAll));
        auto F = m2::Signal::grouperStrict<dIt, dIt, dIt, iIt>;
        return m2::Signal::groupBinning(xsSorted, ysSorted, ssSorted, F, 0.002);
      };

      // m2::Timer t("Run binning", 1, runBinning);
      
      decltype(runBinning()) res;
      {
        m2::Timer t("Run binning");
        res = runBinning();
      }

      auto bins = get<0>(res);
      auto source = get<1>(res);
      auto M = image->GetNumberOfValidPixels()*0.01;
      auto V = std::accumulate(begin(source), end(source), 0, [&M](auto a, auto b){ return a+ ((b.size() > M)  ? 1: 0);});
      MITK_INFO << "Intervals found: " << bins.size();
      MITK_INFO << "Filtered 1%: " << V;
      
    }
  }

  void RunGroupBinning_Strict_Tol0_002()
  {
    //                        0 1 2 3 4    5  6  7  8
    std::vector<double> xs = {100, 200, 300, 400, 500, 100.2, 200.2, 300.2, 395};
    std::vector<double> ys = {1, 2, 3, 4, 5, 1, 2, 3, 4};
    std::vector<int> ss = {0, 0, 0, 0, 0, 1, 1, 1, 1};
    auto indices = m2::argsort(xs);

    auto xsSorted = m2::argsortApply(xs, indices);
    auto ysSorted = m2::argsortApply(ys, indices);
    auto ssSorted = m2::argsortApply(ss, indices);

    using namespace std;
    using dIt = std::vector<double>::const_iterator;
    using iIt = std::vector<int>::const_iterator;
    auto F = m2::Signal::grouperStrict<dIt, dIt, dIt, iIt>;

    auto v = m2::Signal::groupBinning(xsSorted, ysSorted, ssSorted, F, 0.002);
    auto xsNew = std::get<0>(v);
    CPPUNIT_ASSERT_EQUAL(6, (int)xsNew.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(100.1, xsNew[0].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(200.1, xsNew[1].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(300.1, xsNew[2].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(395.0, xsNew[3].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(400.0, xsNew[4].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(500.0, xsNew[5].x.mean(), itk::Math::eps);

    auto ssNew = std::get<1>(v);
    CPPUNIT_ASSERT_EQUAL(6, (int)ssNew.size());
    CPPUNIT_ASSERT_EQUAL(0, ssNew[0][0]);
    CPPUNIT_ASSERT_EQUAL(1, ssNew[0][1]);
    CPPUNIT_ASSERT_EQUAL(2, (int)ssNew[0].size());

    CPPUNIT_ASSERT_EQUAL(0, ssNew[1][0]);
    CPPUNIT_ASSERT_EQUAL(1, ssNew[1][1]);
    CPPUNIT_ASSERT_EQUAL(2, (int)ssNew[1].size());

    CPPUNIT_ASSERT_EQUAL(0, ssNew[2][0]);
    CPPUNIT_ASSERT_EQUAL(1, ssNew[2][1]);
    CPPUNIT_ASSERT_EQUAL(2, (int)ssNew[2].size());

    CPPUNIT_ASSERT_EQUAL(1, (int)ssNew[3].size());
    CPPUNIT_ASSERT_EQUAL(1, (int)ssNew[4].size());
    CPPUNIT_ASSERT_EQUAL(1, (int)ssNew[5].size());
  }

  void RunGroupBinning_Strict_Tol0_100()
  {
    //                        0 1 2 3 4    5  6  7  8
    std::vector<double> xs = {100, 200, 300, 400, 500, 100.2, 200.2, 300.2, 395};
    std::vector<double> ys = {1, 2, 3, 4, 5, 1, 2, 3, 4};
    std::vector<int> ss = {0, 0, 0, 0, 0, 1, 1, 1, 1};

    auto indices = m2::argsort(xs);

    auto xsSorted = m2::argsortApply(xs, indices);
    auto ysSorted = m2::argsortApply(ys, indices);
    auto ssSorted = m2::argsortApply(ss, indices);

    for(auto x : xsSorted)
        std::cout << x << " ";
    std::cout << std::endl;

    for(auto x : ysSorted)
        std::cout << x << " ";
    std::cout << std::endl;

    for(auto x : ssSorted)
        std::cout << x << " ";
    std::cout << std::endl;

    using namespace std;
    using dIt = std::vector<double>::const_iterator;
    using iIt = std::vector<int>::const_iterator;
    auto F = m2::Signal::grouperStrict<dIt, dIt, dIt, iIt>;

    auto v = m2::Signal::groupBinning(xsSorted, ysSorted, ssSorted, F, 0.1);
    auto xsNew = std::get<0>(v);
    CPPUNIT_ASSERT_EQUAL(5, (int)xsNew.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(100.1, xsNew[0].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(200.1, xsNew[1].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(300.1, xsNew[2].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(397.5, xsNew[3].x.mean(), itk::Math::eps);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(500.0, xsNew[4].x.mean(), itk::Math::eps);

    auto ssNew = std::get<1>(v);
        for(auto ss : ssNew){
      for(auto s : ss){
        std::cout << s << " ";
      }
      std::cout << std::endl;
    }
    CPPUNIT_ASSERT_EQUAL(5, (int)ssNew.size());
    CPPUNIT_ASSERT_EQUAL(0, ssNew[0][0]);
    CPPUNIT_ASSERT_EQUAL(1, ssNew[0][1]);
    CPPUNIT_ASSERT_EQUAL(2, (int)ssNew[0].size());

    CPPUNIT_ASSERT_EQUAL(0, ssNew[1][0]);
    CPPUNIT_ASSERT_EQUAL(1, ssNew[1][1]);
    CPPUNIT_ASSERT_EQUAL(2, (int)ssNew[1].size());

    CPPUNIT_ASSERT_EQUAL(0, ssNew[2][0]);
    CPPUNIT_ASSERT_EQUAL(1, ssNew[2][1]);
    CPPUNIT_ASSERT_EQUAL(2, (int)ssNew[2].size());

    CPPUNIT_ASSERT_EQUAL(1, ssNew[3][0]);
    CPPUNIT_ASSERT_EQUAL(0, ssNew[3][1]);
    CPPUNIT_ASSERT_EQUAL(2, (int)ssNew[3].size());

    CPPUNIT_ASSERT_EQUAL(1, (int)ssNew[4].size());
  }
};

MITK_TEST_SUITE_REGISTRATION(m2SignalGroupBinning)
