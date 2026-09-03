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

#include <algorithm>

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

bool m2::IntervalVector::SetFeature(const std::string &name, const std::vector<double> &values)
{
  if (name.empty())
  {
    MITK_WARN << "A feature needs a name; nothing was attached.";
    return false;
  }

  if (values.size() != m_Data.size())
  {
    MITK_WARN << "The feature [" << name << "] carries " << values.size() << " values but there are " << m_Data.size()
              << " intervals; nothing was attached.";
    return false;
  }

  for (size_t i = 0; i < m_Data.size(); ++i)
    m_Data[i].SetFeature(name, values[i]);

  // the name is registered once, so that attaching a feature again keeps its place in the order
  if (std::find(m_FeatureNames.begin(), m_FeatureNames.end(), name) == m_FeatureNames.end())
    m_FeatureNames.push_back(name);

  this->Modified();

  return true;
}

std::vector<double> m2::IntervalVector::GetFeature(const std::string &name, double fallback) const
{
  if (!this->HasFeature(name))
    return {};

  std::vector<double> values;
  values.reserve(m_Data.size());
  for (const auto &interval : m_Data)
    values.push_back(interval.GetFeature(name, fallback));

  return values;
}

bool m2::IntervalVector::HasFeature(const std::string &name) const
{
  return std::find(m_FeatureNames.begin(), m_FeatureNames.end(), name) != m_FeatureNames.end();
}

void m2::IntervalVector::RemoveFeature(const std::string &name)
{
  const auto it = std::find(m_FeatureNames.begin(), m_FeatureNames.end(), name);
  if (it == m_FeatureNames.end())
    return;

  m_FeatureNames.erase(it);
  for (auto &interval : m_Data)
    interval.features.erase(name);

  this->Modified();
}

void m2::IntervalVector::ClearFeatures()
{
  if (m_FeatureNames.empty())
    return;

  m_FeatureNames.clear();
  for (auto &interval : m_Data)
    interval.features.clear();

  this->Modified();
}
