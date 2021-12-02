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
#include <vector>
#include <algorithm>
#include <numeric>

namespace m2
{
  class M2AIACORE_EXPORT Peak final
  {
  public:
    double GetX() const { return GetXSum() / double(m_count); }
    double GetXSum() const { return m_xSum; }
    
    double GetY() const { return GetYSum() / double(m_count); }
    double GetYSum() const { return m_ySum; }

    double GetYMax() const { return m_yMax; }
    
    double GetFwhm() const
    {
      return m_fwhmSum / double(m_count);
    }
    unsigned int GetCount() const { return m_count; }
    unsigned int GetIndex() const
    {
      return std::round( m_indexSum / double(m_count));
    }

    Peak InsertFwhm(const Peak &other){
      m_fwhmSum += other.m_fwhmSum;
      return *this;
    }

    Peak InsertFwhm(double fwhm){
      m_fwhmSum += fwhm;
      return *this;
    }

    Peak Insert(const Peak &other){
      m_xSum += other.m_xSum;
      m_ySum += other.m_ySum;
      m_indexSum += other.m_indexSum;
      m_count += other.m_count;
      return *this;
    }

    Peak Insert(unsigned int i, double x, double y){
      m_xSum += x;
      m_ySum += y;
      m_indexSum += i;
      ++m_count;
      return *this;
    }

    Peak() = default;
    Peak(const Peak&) = default;
    Peak(unsigned int index, double x, double y, double fwhm = 0):
    m_xSum(x), m_ySum(y), m_indexSum(index), m_count(1){
      if(fwhm != 0) m_fwhmSum += fwhm;
    }
    friend bool operator<(const Peak &lhs, const Peak &rhs) { return lhs.GetX() < rhs.GetX(); }
    friend bool operator==(const Peak &lhs, const Peak &rhs) { return lhs.GetX() == rhs.GetX(); }
    Peak operator+(const Peak & first) const
    {
        return Peak(*this).Insert(first);
    }

  protected:
    double m_xSum;
    double m_ySum;
    double m_yMax;
    unsigned int m_indexSum;
    unsigned int m_count;
    double m_fwhmSum;
  };

} // namespace m2
