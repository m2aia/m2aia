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
#include <mitkBaseData.h>
#include <mitkDataNode.h>
#include <m2CoreCommon.h>

namespace m2
{
  
  struct Accumulator{
    double mean() const {return m_sum/double(m_count);}
    double sum() const {return m_sum;}
    double min() const {return m_min;}
    double max() const {return m_max;}
    unsigned int count() const {return m_count;}

    double m_sum = 0;
    double m_min = std::numeric_limits<double>::max();
    double m_max = std::numeric_limits<double>::min();
    unsigned int m_count = 0;

    void operator()(double v){
      m_sum += v;
      m_min = std::min(m_min,v);
      m_max = std::max(m_max,v);
      ++m_count;
    }

    Accumulator& operator+=(const Accumulator& rhs){
      m_sum += rhs.m_sum;
      m_count += rhs.m_count;
      m_min = std::min(m_min,rhs.m_min);
      m_max = std::max(m_max,rhs.m_max);
      return *this;
    }

    Accumulator operator+(const Accumulator& rhs){
      Accumulator res;
      res.m_sum = rhs.m_sum + m_sum;
      res.m_count = rhs.m_sum + m_count;
      res.m_min = std::min(m_min,rhs.m_min);
      res.m_max = std::max(m_max,rhs.m_max);
      return res;
    }

  };


  struct Interval
  {
    Accumulator index;
    Accumulator x;
    Accumulator y;
    std::string description;

    Interval() = default;
    Interval(unsigned int index, double x, double y){
      this->index(index);
      this->x(x);
      this->y(y);
    }

    Interval& operator+=(const Interval& rhs){
      x += rhs.x;
      y += rhs.y;
      index += rhs.index;
      return *this;
    }
    friend bool operator<(const Interval &lhs, const Interval &rhs) { return lhs.x.mean() < rhs.x.mean(); }
    // friend bool operator==(const Interval &lhs, const Interval &rhs) { return lhs.x.mean() == rhs.x.mean(); }
  };

  enum class IntervalVectorType : unsigned int
    {
      None = 0,
      Profile = 1,
      Centroid = 2,
      Indicator = 4
    };

  class M2AIACORE_EXPORT IntervalVector : public mitk::BaseData
  {
  public:

    mitkClassMacro(IntervalVector, mitk::BaseData);
    itkCloneMacro(Self);
    itkFactorylessNewMacro(Self);

    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return false; }
    void SetRequestedRegion(const itk::DataObject *) override {}

    IntervalVectorType type;

    std::vector<double> GetXMean() const;
    std::vector<double> GetXSum() const;
    std::vector<double> GetXMax() const;
    std::vector<double> GetXMin() const;
    std::vector<unsigned int> GetXCount() const;

    std::vector<double> GetYMean() const;
    std::vector<double> GetYSum() const;
    std::vector<double> GetYMax() const;
    std::vector<double> GetYMin() const;
    std::vector<unsigned int > GetYCount() const;

    std::vector<double> GetIndexMean() const;
    std::vector<double> GetIndexSum() const;
    std::vector<double> GetIndexMax() const;
    std::vector<double> GetIndexMin() const;
    std::vector<unsigned int> GetIndexCount() const;

    std::vector<Interval> & GetIntervals(){return m_Data;}
    const std::vector<Interval> & GetIntervals() const {return m_Data;}

    private:
    std::vector<Interval> m_Data;

  };

} // namespace m2