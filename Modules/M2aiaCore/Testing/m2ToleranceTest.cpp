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
#include <m2TestFixture.h>
#include <m2Tolerance.h>
#include <mitkCoreServices.h>
#include <mitkException.h>
#include <mitkIOUtil.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkTestingMacros.h>

#include <algorithm>
#include <filesystem>
#include <limits>

class m2ToleranceTestSuite : public m2::TestFixture
{
  CPPUNIT_TEST_SUITE(m2ToleranceTestSuite);
  MITK_TEST(PartPerMillionIsOneMillionth);
  MITK_TEST(HalfWidthInDalton);
  MITK_TEST(HalfWidthInPPM);
  MITK_TEST(InvalidValuesThrow);
  MITK_TEST(UnitStringsRoundTrip);
  MITK_TEST(PreferencesDefaultWhenNothingStored);
  MITK_TEST(PreferencesStoreAndLoadKeepUnit);
  MITK_TEST(LegacyPreferenceIsMigratedToSameWindow);
  MITK_TEST(LegacyPreferenceIsIgnoredWhenNewKeysExist);
  CPPUNIT_TEST_SUITE_END();

  mitk::IPreferences *m_Preferences = nullptr;

  static bool HasKey(const mitk::IPreferences *preferences, const std::string &key)
  {
    const auto keys = preferences->Keys();
    return std::find(keys.begin(), keys.end(), key) != keys.end();
  }

public:
  void setUp() override
  {
    auto filename = mitk::IOUtil::CreateTemporaryFile("m2prefs_XXXXXX.xml");
    std::filesystem::remove(filename); // the storage needs a file name, not an existing file
    auto *service = mitk::CoreServices::GetPreferencesService();
    service->InitializeStorage(filename);
    m_Preferences = service->GetSystemPreferences();
  }

  void tearDown() override { mitk::CoreServices::GetPreferencesService()->UninitializeStorage(); }

  void PartPerMillionIsOneMillionth()
  {
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1e-6, m2::PartPerMillionToFactor(1), 1e-18);
  }

  void HalfWidthInDalton()
  {
    const auto tolerance = m2::Tolerance::Dalton(0.25);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.25, tolerance.HalfWidth(100), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.25, tolerance.HalfWidth(2000), 1e-12);
  }

  void HalfWidthInPPM()
  {
    // 10 ppm at m/z 1000 is +/- 0.01 Da
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.01, m2::Tolerance::PPM(10).HalfWidth(1000), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0038029, m2::Tolerance::PPM(5).HalfWidth(760.585), 1e-7);
    CPPUNIT_ASSERT(m2::Tolerance::PPM(10) != m2::Tolerance::Dalton(10));
    CPPUNIT_ASSERT(m2::Tolerance() == m2::Tolerance::Dalton(0));
  }

  void InvalidValuesThrow()
  {
    CPPUNIT_ASSERT_THROW(m2::Tolerance::PPM(-1), mitk::Exception);
    CPPUNIT_ASSERT_THROW(m2::Tolerance::Dalton(std::numeric_limits<double>::quiet_NaN()), mitk::Exception);
    CPPUNIT_ASSERT_THROW(m2::Tolerance(1, static_cast<m2::ToleranceUnit>(7)), mitk::Exception);
  }

  void UnitStringsRoundTrip()
  {
    CPPUNIT_ASSERT(m2::ParseToleranceUnit("ppm") == m2::ToleranceUnit::PPM);
    CPPUNIT_ASSERT(m2::ParseToleranceUnit("PPM") == m2::ToleranceUnit::PPM);
    CPPUNIT_ASSERT(m2::ParseToleranceUnit("Da") == m2::ToleranceUnit::Dalton);
    CPPUNIT_ASSERT(m2::ParseToleranceUnit("dalton") == m2::ToleranceUnit::Dalton);
    CPPUNIT_ASSERT_THROW(m2::ParseToleranceUnit("mDa"), mitk::Exception);
    for (auto unit : {m2::ToleranceUnit::PPM, m2::ToleranceUnit::Dalton})
      CPPUNIT_ASSERT(m2::ParseToleranceUnit(m2::ToString(unit)) == unit);
    CPPUNIT_ASSERT_EQUAL(std::string("10 ppm"), m2::ToString(m2::Tolerance::PPM(10)));
  }

  void PreferencesDefaultWhenNothingStored()
  {
    CPPUNIT_ASSERT(m2::TolerancePreferences::Load(m_Preferences) == m2::TolerancePreferences::Default());
    CPPUNIT_ASSERT(m2::TolerancePreferences::Load(m_Preferences, m2::Tolerance::Dalton(1)) == m2::Tolerance::Dalton(1));
    CPPUNIT_ASSERT(m2::TolerancePreferences::Load(nullptr) == m2::TolerancePreferences::Default());
  }

  void PreferencesStoreAndLoadKeepUnit()
  {
    m2::TolerancePreferences::Store(m_Preferences, m2::Tolerance::Dalton(0.05));
    CPPUNIT_ASSERT(m2::TolerancePreferences::Load(m_Preferences) == m2::Tolerance::Dalton(0.05));

    m2::TolerancePreferences::Store(m_Preferences, m2::Tolerance::PPM(12.5));
    CPPUNIT_ASSERT(m2::TolerancePreferences::Load(m_Preferences) == m2::Tolerance::PPM(12.5));
  }

  void LegacyPreferenceIsMigratedToSameWindow()
  {
    // Before the fix, a stored 75 was read as ppm with a factor of 1e-5, i.e. +/- 750 ppm.
    m_Preferences->PutFloat(m2::TolerancePreferences::LegacyKey, 75);
    const double oldHalfWidthAt1000 = 75 * 10e-6 * 1000;

    const auto migrated = m2::TolerancePreferences::Load(m_Preferences);
    CPPUNIT_ASSERT(migrated.GetUnit() == m2::ToleranceUnit::PPM);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(750, migrated.GetValue(), 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(oldHalfWidthAt1000, migrated.HalfWidth(1000), 1e-9);

    // migrated exactly once: the legacy key is gone and the new keys are stored
    CPPUNIT_ASSERT(!HasKey(m_Preferences, m2::TolerancePreferences::LegacyKey));
    CPPUNIT_ASSERT(HasKey(m_Preferences, m2::TolerancePreferences::ValueKey));
    CPPUNIT_ASSERT(HasKey(m_Preferences, m2::TolerancePreferences::UnitKey));
    CPPUNIT_ASSERT(m2::TolerancePreferences::Load(m_Preferences) == migrated);
  }

  void LegacyPreferenceIsIgnoredWhenNewKeysExist()
  {
    m2::TolerancePreferences::Store(m_Preferences, m2::Tolerance::Dalton(0.1));
    m_Preferences->PutFloat(m2::TolerancePreferences::LegacyKey, 75); // e.g. written by an older M2aia

    CPPUNIT_ASSERT(m2::TolerancePreferences::Load(m_Preferences) == m2::Tolerance::Dalton(0.1));
    CPPUNIT_ASSERT(!HasKey(m_Preferences, m2::TolerancePreferences::LegacyKey));
  }
};

MITK_TEST_SUITE_REGISTRATION(m2Tolerance)
