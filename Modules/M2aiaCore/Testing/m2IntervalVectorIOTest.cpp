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
#include <m2IntervalVector.h>
#include <m2TestFixture.h>
#include <mitkIOMimeTypes.h>
#include <mitkIOUtil.h>
#include <mitkTestingMacros.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

/**
 * Centroid lists (m2::IntervalVector) written to disk must read back with exactly the m/z values
 * they were written with — every row, including the first and the last.
 *
 * Three writers handle centroid lists:
 * - TabularDataIO    .csv/.tsv/.txt/.dat  (comma separated "center,max,min,mean")
 * - SefFileIO        .sef/.centroids      (JSON, the default for .centroids)
 * - IntervalVectorIO .centroids           (comma separated, only when its mime type is requested)
 *
 * Both IOs claim .centroids, so their readers decide by content: JSON goes to SefFileIO.
 */
class m2IntervalVectorIOTestSuite : public m2::TestFixture
{
  CPPUNIT_TEST_SUITE(m2IntervalVectorIOTestSuite);
  MITK_TEST(CsvWriter_WritesExactMzForEveryRow);
  MITK_TEST(TsvWriter_WritesExactMzForEveryRow);
  MITK_TEST(Csv_RoundTripKeepsMzAndIntensity);
  MITK_TEST(IntervalVectorIOWriter_WritesExactMzForEveryRow);
  MITK_TEST(IntervalVectorIO_RoundTripKeepsMzAndIntensity);
  MITK_TEST(Centroids_RoundTripKeepsCentroidMz);
  MITK_TEST(Centroids_RoundTripKeepsBoundsNameAndColor);
  CPPUNIT_TEST_SUITE_END();

  std::filesystem::path m_Dir;

  // many significant digits, so that a truncated or shifted value is caught
  const std::vector<double> m_Mz{301.2170104980469, 755.50123456789012, 1464.894775390625};
  const std::vector<double> m_Intensity{12.5, 1000.25, 0.003};

  static std::string IntervalVectorIOMimeType()
  {
    return mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".csv.spectrum";
  }

  m2::IntervalVector::Pointer MakeCentroidList() const
  {
    auto list = m2::IntervalVector::New();
    list->SetType(m2::SpectrumFormat::Centroid);
    for (size_t i = 0; i < m_Mz.size(); ++i)
      list->GetIntervals().emplace_back(m_Mz[i], m_Intensity[i]);
    return list;
  }

  std::string FilePath(const std::string &name) const { return (m_Dir / name).string(); }

  // the first column of every data row, parsed the way the Docker containers read it
  static std::vector<double> ReadFirstColumn(const std::string &path)
  {
    std::ifstream file(path);
    std::string line;
    std::getline(file, line); // header
    std::vector<double> values;
    while (std::getline(file, line))
    {
      if (line.empty())
        continue;
      std::stringstream ss(line);
      std::string cell;
      std::getline(ss, cell, ',');
      values.push_back(std::stod(cell));
    }
    return values;
  }

  static m2::IntervalVector::Pointer LoadSingleList(const std::string &path)
  {
    const auto data = mitk::IOUtil::Load(path);
    CPPUNIT_ASSERT_EQUAL(size_t(1), data.size());
    auto list = dynamic_cast<m2::IntervalVector *>(data.front().GetPointer());
    CPPUNIT_ASSERT_MESSAGE("loaded data is not an m2::IntervalVector", list != nullptr);
    return list;
  }

  void AssertMzEqual(const std::vector<double> &actual) const
  {
    CPPUNIT_ASSERT_EQUAL(m_Mz.size(), actual.size());
    for (size_t i = 0; i < m_Mz.size(); ++i)
      CPPUNIT_ASSERT_EQUAL_MESSAGE("m/z of row " + std::to_string(i), m_Mz[i], actual[i]);
  }

  void AssertSameCentroids(const m2::IntervalVector &list) const
  {
    CPPUNIT_ASSERT(list.GetType() == m2::SpectrumFormat::Centroid);
    const auto &intervals = list.GetIntervals();
    CPPUNIT_ASSERT_EQUAL(m_Mz.size(), intervals.size());
    for (size_t i = 0; i < m_Mz.size(); ++i)
    {
      CPPUNIT_ASSERT_EQUAL_MESSAGE("m/z of row " + std::to_string(i), m_Mz[i], intervals[i].x.mean());
      CPPUNIT_ASSERT_EQUAL_MESSAGE("intensity of row " + std::to_string(i), m_Intensity[i], intervals[i].y.mean());
    }
  }

public:
  void setUp() override { m_Dir = mitk::IOUtil::CreateTemporaryDirectory("m2IntervalVectorIOTest_XXXXXX"); }

  void tearDown() override { std::filesystem::remove_all(m_Dir); }

  void CsvWriter_WritesExactMzForEveryRow()
  {
    const auto path = FilePath("list.csv");
    mitk::IOUtil::Save(MakeCentroidList(), path);
    AssertMzEqual(ReadFirstColumn(path));
  }

  void TsvWriter_WritesExactMzForEveryRow()
  {
    const auto path = FilePath("list.tsv");
    mitk::IOUtil::Save(MakeCentroidList(), path);
    AssertMzEqual(ReadFirstColumn(path));
  }

  void Csv_RoundTripKeepsMzAndIntensity()
  {
    const auto path = FilePath("list.csv");
    mitk::IOUtil::Save(MakeCentroidList(), path);
    AssertSameCentroids(*LoadSingleList(path));
  }

  void IntervalVectorIOWriter_WritesExactMzForEveryRow()
  {
    const auto path = FilePath("list.centroids");
    mitk::IOUtil::Save(MakeCentroidList(), IntervalVectorIOMimeType(), path, false);
    AssertMzEqual(ReadFirstColumn(path));
  }

  // comma separated .centroids files, as older versions wrote them, still load
  void IntervalVectorIO_RoundTripKeepsMzAndIntensity()
  {
    const auto path = FilePath("list.centroids");
    mitk::IOUtil::Save(MakeCentroidList(), IntervalVectorIOMimeType(), path, false);
    AssertSameCentroids(*LoadSingleList(path));
  }

  // .centroids is what the Docker views hand to their containers
  void Centroids_RoundTripKeepsCentroidMz()
  {
    const auto path = FilePath("list.centroids");
    mitk::IOUtil::Save(MakeCentroidList(), path);

    const auto list = LoadSingleList(path);
    CPPUNIT_ASSERT(list->GetType() == m2::SpectrumFormat::Centroid);
    const auto &intervals = list->GetIntervals();
    CPPUNIT_ASSERT_EQUAL(m_Mz.size(), intervals.size());
    for (size_t i = 0; i < m_Mz.size(); ++i)
    {
      CPPUNIT_ASSERT_EQUAL_MESSAGE("lower of row " + std::to_string(i), m_Mz[i], intervals[i].x.min());
      CPPUNIT_ASSERT_EQUAL_MESSAGE("upper of row " + std::to_string(i), m_Mz[i], intervals[i].x.max());
    }
  }

  void Centroids_RoundTripKeepsBoundsNameAndColor()
  {
    auto list = m2::IntervalVector::New();
    list->GetIntervals().push_back(m2::Interval::FromBounds(301.2170104980469, 301.2245, "first", "#eee685"));
    list->GetIntervals().push_back(m2::Interval::FromBounds(755.4987, 755.50123456789012, "middle", "#1f77b4"));
    list->GetIntervals().push_back(m2::Interval::FromBounds(1464.8801, 1464.894775390625, "last", ""));

    const auto path = FilePath("bounds.centroids");
    mitk::IOUtil::Save(list, path);

    const auto loaded = LoadSingleList(path);
    const auto &expected = list->GetIntervals();
    const auto &actual = loaded->GetIntervals();
    CPPUNIT_ASSERT_EQUAL(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
      CPPUNIT_ASSERT_EQUAL(expected[i].x.min(), actual[i].x.min());
      CPPUNIT_ASSERT_EQUAL(expected[i].x.max(), actual[i].x.max());
      CPPUNIT_ASSERT_EQUAL(expected[i].description, actual[i].description);
      CPPUNIT_ASSERT_EQUAL(expected[i].color, actual[i].color);
    }
  }
};

MITK_TEST_SUITE_REGISTRATION(m2IntervalVectorIO)
