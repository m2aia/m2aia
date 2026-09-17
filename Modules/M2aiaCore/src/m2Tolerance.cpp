/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2Tolerance.h>
#include <mitkIPreferences.h>

#include <algorithm>
#include <cctype>
#include <sstream>

std::string m2::ToString(ToleranceUnit unit)
{
  return unit == ToleranceUnit::PPM ? "ppm" : "Da";
}

std::string m2::ToString(const Tolerance &tolerance)
{
  std::ostringstream os;
  os << tolerance.GetValue() << " " << ToString(tolerance.GetUnit());
  return os.str();
}

m2::ToleranceUnit m2::ParseToleranceUnit(const std::string &unit)
{
  std::string lower(unit);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  if (lower == "ppm")
    return ToleranceUnit::PPM;
  if (lower == "da" || lower == "dalton")
    return ToleranceUnit::Dalton;
  mitkThrow() << "Unknown tolerance unit '" << unit << "'; use 'ppm' or 'Da'.";
}

m2::Tolerance m2::TolerancePreferences::Load(mitk::IPreferences *preferences, const Tolerance &defaultTolerance)
{
  if (!preferences)
    return defaultTolerance;

  const auto keys = preferences->Keys();
  const auto has = [&keys](const char *key) { return std::find(keys.begin(), keys.end(), key) != keys.end(); };

  if (has(LegacyKey))
  {
    if (!has(ValueKey))
    {
      const auto legacyValue = preferences->GetFloat(LegacyKey, 0);
      if (std::isfinite(legacyValue) && legacyValue >= 0)
        Store(preferences, Tolerance::PPM(10.0 * legacyValue));
    }
    preferences->Remove(LegacyKey);
  }

  try
  {
    return Tolerance(preferences->GetDouble(ValueKey, defaultTolerance.GetValue()),
                     ParseToleranceUnit(preferences->Get(UnitKey, ToString(defaultTolerance.GetUnit()))));
  }
  catch (const mitk::Exception &)
  {
    return defaultTolerance;
  }
}

void m2::TolerancePreferences::Store(mitk::IPreferences *preferences, const Tolerance &tolerance)
{
  if (!preferences)
    return;
  preferences->PutDouble(ValueKey, tolerance.GetValue());
  preferences->Put(UnitKey, ToString(tolerance.GetUnit()));
}
