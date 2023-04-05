/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2IntervalVector.h>

using namespace std;
namespace m2
{
  std::vector<double> IntervalVector::GetXMean() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.x.mean();});
    return data;
  }
  std::vector<double> IntervalVector::GetXSum() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.x.sum();});
    return data;
  }
  std::vector<double> IntervalVector::GetXMax() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.x.max();});
    return data;
  }
  std::vector<double> IntervalVector::GetXMin() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.x.min();});
    return data;
  }
  std::vector<unsigned int> IntervalVector::GetXCount() const {
    vector<unsigned int> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.x.count();});
    return data;
  }

  std::vector<double> IntervalVector::GetYMean() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.y.mean();});
    return data;
  }
  std::vector<double> IntervalVector::GetYSum() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.y.sum();});
    return data;
  }
  std::vector<double> IntervalVector::GetYMax() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.y.max();});
    return data;
  }
  std::vector<double> IntervalVector::GetYMin() const {
    vector<double> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.y.min();});
    return data;
  }
  std::vector<unsigned int> IntervalVector::GetYCount() const {
    vector<unsigned int> data;
    transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.y.count();});
    return data;
  }

  // std::vector<double> IntervalVector::GetIndexMean() const {
  //   vector<double> data;
  //   transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.index.mean();});
  //   return data;
  // }
  // std::vector<double> IntervalVector::GetIndexSum() const {
  //   vector<double> data;
  //   transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.index.sum();});
  //   return data;
  // }
  // std::vector<double> IntervalVector::GetIndexMax() const {
  //   vector<double> data;
  //   transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.index.max();});
  //   return data;
  // }
  // std::vector<double> IntervalVector::GetIndexMin() const {
  //   vector<double> data;
  //   transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.index.min();});
  //   return data;
  // }
  // std::vector<unsigned int> IntervalVector::GetIndexCount() const {
  //   vector<unsigned int> data;
  //   transform(m_Data.begin(),m_Data.end(),back_inserter(data), [](const Interval & I){return I.index.count();});
  //   return data;
  // }

} // namespace m2
