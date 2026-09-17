/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#pragma once

#include <M2aiaCoreExports.h>
#include <m2CoreCommon.h>
#include <mitkExceptionMacro.h>

#include <cmath>
#include <string>

namespace mitk
{
  class IPreferences;
}

namespace m2
{
  enum class ToleranceUnit : unsigned int
  {
    Dalton = 0,
    PPM = 1
  };

  /// @brief A tolerance that carries its unit, so a value can never be read in the wrong unit.
  /// The tolerance is a half-width: the window around x is [x - HalfWidth(x), x + HalfWidth(x)].
  class Tolerance
  {
  public:
    Tolerance() = default; // 0 Da

    Tolerance(double value, ToleranceUnit unit) : m_Value(value), m_Unit(unit)
    {
      if (!std::isfinite(value) || value < 0)
        mitkThrow() << "A tolerance must be a finite, non-negative number (got " << value << ").";
      if (unit != ToleranceUnit::Dalton && unit != ToleranceUnit::PPM)
        mitkThrow() << "Unknown tolerance unit " << static_cast<unsigned int>(unit) << ".";
    }

    static Tolerance Dalton(double value) { return Tolerance(value, ToleranceUnit::Dalton); }
    static Tolerance PPM(double value) { return Tolerance(value, ToleranceUnit::PPM); }

    double GetValue() const { return m_Value; }
    ToleranceUnit GetUnit() const { return m_Unit; }

    /// @brief Absolute half-width (in units of the x axis) of the window around x.
    double HalfWidth(double x) const
    {
      if (m_Unit == ToleranceUnit::PPM)
        return m2::PartPerMillionToFactor(m_Value) * std::abs(x);
      return m_Value;
    }

    bool operator==(const Tolerance &other) const { return m_Value == other.m_Value && m_Unit == other.m_Unit; }
    bool operator!=(const Tolerance &other) const { return !(*this == other); }

  private:
    double m_Value = 0;
    ToleranceUnit m_Unit = ToleranceUnit::Dalton;
  };

  /// @brief "ppm" or "Da".
  M2AIACORE_EXPORT std::string ToString(ToleranceUnit unit);

  /// @brief e.g. "10 ppm" or "0.01 Da".
  M2AIACORE_EXPORT std::string ToString(const Tolerance &tolerance);

  /// @brief Accepts "ppm", "Da" or "Dalton" (case-insensitive); throws otherwise.
  M2AIACORE_EXPORT ToleranceUnit ParseToleranceUnit(const std::string &unit);

  namespace TolerancePreferences
  {
    constexpr const char *ValueKey = "m2aia.signal.Tolerance.Value";
    constexpr const char *UnitKey = "m2aia.signal.Tolerance.Unit";

    /// @brief Unitless key used before the tolerance stored its unit. The GUI always read it as ppm
    /// with a conversion factor of 1e-5 instead of 1e-6, so a stored value v meant 10 * v ppm.
    constexpr const char *LegacyKey = "m2aia.signal.Tolerance";

    inline Tolerance Default() { return Tolerance::PPM(75); }

    /// @brief Read the tolerance from the preferences. A legacy value is converted once so that the
    /// effective window stays the same, written to the new keys and removed.
    M2AIACORE_EXPORT Tolerance Load(mitk::IPreferences *preferences, const Tolerance &defaultTolerance = Default());

    M2AIACORE_EXPORT void Store(mitk::IPreferences *preferences, const Tolerance &tolerance);
  } // namespace TolerancePreferences
} // namespace m2
