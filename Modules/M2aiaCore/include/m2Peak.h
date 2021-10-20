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
    double GetX() const { return GetXSum() / double(m_x.size()); }
    double GetXSum() const { return std::accumulate(m_x.data(), m_x.data() + m_x.size(), double(0)); }
    
    double GetY() const { return GetYSum() / double(m_y.size()); }
    double GetYSum() const { return std::accumulate(m_y.data(), m_y.data() + m_y.size(), double(0)); }
    double GetYMax() const { return *std::max_element(m_y.data(), m_y.data() + m_y.size()); }
    
    double GetFwhm() const
    {
      return std::accumulate(m_fwhm.data(), m_fwhm.data() + m_fwhm.size(), double(0)) / double(m_fwhm.size());
    }
    unsigned int GetCount() const { return m_x.size(); }
    unsigned int GetIndex() const
    {
      return std::round(std::accumulate(m_indexes.data(), m_indexes.data() + m_indexes.size(), double(0)) /
                        double(m_indexes.size()));
    }

    Peak InsertFwhm(const Peak &other){
      m_fwhm.insert(std::end(m_fwhm),std::begin(other.m_fwhm), std::end(other.m_fwhm));
      return *this;
    }

    Peak InsertFwhm(double fwhm){
      m_fwhm.push_back(fwhm);
      return *this;
    }

    Peak Insert(const Peak &other){
      m_x.insert(std::end(m_x),std::begin(other.m_x), std::end(other.m_x));
      m_y.insert(std::end(m_y),std::begin(other.m_y), std::end(other.m_y));
      m_indexes.insert(std::end(m_indexes),std::begin(other.m_indexes), std::end(other.m_indexes));
      return *this;
    }

    Peak Insert(unsigned int i, double x, double y){
      m_indexes.push_back(i);
      m_x.push_back(x);
      m_y.push_back(y);
      return *this;
    }

    Peak() = default;
    Peak(const Peak&) = default;
    Peak(unsigned int index, double x, double y, double fwhm = 0){
      m_indexes.push_back(index);
      m_x.push_back(x);
      m_y.push_back(y);
      if(fwhm != 0) m_fwhm.push_back(fwhm);
    }
    friend bool operator<(const Peak &lhs, const Peak &rhs) { return lhs.GetX() < rhs.GetX(); }
    friend bool operator==(const Peak &lhs, const Peak &rhs) { return lhs.GetX() == rhs.GetX(); }
    Peak operator+(const Peak & first) const
    {
        return Peak(*this).Insert(first);
    }

  protected:
    std::vector<double> m_x;
    std::vector<double> m_y;
    std::vector<double> m_fwhm;
    std::vector<unsigned int> m_indexes;
  };

} // namespace m2
