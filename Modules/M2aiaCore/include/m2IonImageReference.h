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

#include "itkObjectFactory.h"
#include "mitkCommon.h"
#include <M2aiaCoreExports.h>
#include <array>
#include <string>

namespace m2
{
  class M2AIACORE_EXPORT IonImageReference : public itk::LightObject
  {
  private:
    IonImageReference() = default;
    IonImageReference(double mz, double tol, const std::string &name = "") : mz(mz), tol(tol), name(name) {}
    IonImageReference(const IonImageReference &other) : itk::LightObject()
    {
      this->mz = other.mz;
      this->tol = other.tol;
      this->name = other.name;
      this->color = other.color;
      this->scale = other.scale;
      this->intensity = other.intensity;
      this->isActive = other.isActive;
    }

  public:
    double mz = 0;
    double tol = 0;
    double intensity = 0;

    std::array<float, 4> color = {-1, -1, -1, -1};
    std::array<float, 2> scale = {-1, -1};
    std::string name = "";
    bool isActive = true;

    mitkClassMacroItkParent(m2::IonImageReference, itk::LightObject);
    itkFactorylessNewMacro(m2::IonImageReference);
    mitkNewMacro3Param(m2::IonImageReference, double, double, const std::string &);

    inline bool operator<(const IonImageReference &str) const
    {
      return mz < str.mz || (!(str.mz < mz) && tol < str.tol);
    }

    inline bool is_close(double a, double b, double tolerance = 1e-5) const
    {
      return std::abs(a - b) <= ((std::abs(a) < std::abs(b) ? std::abs(b) : std::abs(a)) * tolerance);
    }

    inline bool operator==(const IonImageReference &str) const
    {
      return is_close(mz, str.mz) && is_close(tol, str.tol);
    }

    inline bool operator==(const std::array<double, 2> &v) const { return is_close(mz, v[0]) && is_close(tol, v[1]); }

    struct Comp
    {
      inline bool operator()(const IonImageReference *lhs, const IonImageReference *rhs) const { return *lhs < *rhs; }
    };
  };

} // namespace m2
