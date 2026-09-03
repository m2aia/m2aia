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
#include <mitkBaseData.h>
#include <mitkDataNode.h>
#include <mitkProperties.h>

#include <map>
#include <string>
#include <vector>

namespace m2
{

  struct Accumulator
  {

    double mean() const { return m_sum / double(m_count); }
    double sum() const { return m_sum; }
    double min() const { return m_min; }
    double max() const { return m_max; }
    unsigned int count() const { return m_count; }

    double m_sum = 0;
    double m_min = std::numeric_limits<double>::max();
    double m_max = 0;
    unsigned int m_count = 0;

    void add(double v)
    {
      m_sum += v;
      m_min = std::min(m_min, v);
      m_max = std::max(m_max, v);
      ++m_count;
    }

    // Accumulator &operator+=(const double &s)
    // {
    //   m_sum += rhs.m_sum;
    //   m_count += rhs.m_count;
    //   m_min = std::min(m_min, rhs.m_min);
    //   m_max = std::max(m_max, rhs.m_max);
    //   return *this;
    // }

    Accumulator &operator+=(const Accumulator &rhs)
    {
      m_sum += rhs.m_sum;
      m_count += rhs.m_count;
      m_min = std::min(m_min, rhs.m_min);
      m_max = std::max(m_max, rhs.m_max);
      return *this;
    }

    // Accumulator operator+(const Accumulator &rhs)
    // {
    //   Accumulator res;
    //   res.m_sum +=  m_sum;
    //   res.m_count = rhs.m_count + m_count;
    //   res.m_min = std::min(m_min, rhs.m_min);
    //   res.m_max = std::max(m_max, rhs.m_max);
    //   return res;
    // }
  };

  struct Interval
  {
    // Accumulator index;
    Accumulator x;
    Accumulator y;
    std::string description;

    /// Hex colour from, e.g. "#eee685". Empty string if not set.
    std::string color;


    /**
     * This tag can be used to indicate it's source. Can be used during processing and is not guaranteed to be unique.
     * Default:0
     */
    unsigned int sourceId = 0;

    /**
     * Named values attached to this centroid by an analysis: the loading of a component, a score,
     * a p-value. Several analyses can describe the same centroid side by side, which is what the
     * feature list view shows as one column per name. The names are registered by the
     * m2::IntervalVector holding this interval, see m2::IntervalVector::SetFeature.
     */
    std::map<std::string, double> features;

    bool HasFeature(const std::string &name) const { return features.find(name) != features.end(); }

    double GetFeature(const std::string &name, double fallback = 0.0) const
    {
      const auto it = features.find(name);
      return it == features.end() ? fallback : it->second;
    }

    void SetFeature(const std::string &name, double value) { features[name] = value; }

    Interval(unsigned int source = 0) : sourceId(source) {}

    Interval(double x, double y, unsigned int source = 0) : Interval(source)
    {
      // this->index(index);
      this->x.add(x);
      this->y.add(y);
    }

    /**
     * @brief Construct an Interval from explicit lower/upper m/z bounds as stored in .sef files.
     *
     * After construction:
     *   x.min()  == lower,  x.max()  == upper,
     *   x.mean() == (lower+upper)/2,  x.count() == 2
     *
     * @param lower        m/z lower bound
     * @param upper        m/z upper bound
     * @param name         annotation name (stored in description)
     * @param col          hex colour string, e.g. "#eee685"
     */
    static Interval FromBounds(double lower,
                                double upper,
                                const std::string &name     = {},
                                const std::string &col      = {})
    {
      Interval I;
      I.x.add(lower);
      I.x.add(upper);
      I.description   = name;
      I.color         = col;
      return I;
    }

    Interval &operator+=(const Interval &rhs)
    {
      x += rhs.x;
      y += rhs.y;
      return *this;
    }
    friend bool operator<(const Interval &lhs, const Interval &rhs) { return lhs.x.mean() < rhs.x.mean(); }
    // friend bool operator==(const Interval &lhs, const Interval &rhs) { return lhs.x.mean() == rhs.x.mean(); }

    std::string ToString() const
    {
      std::stringstream ss;
      ss << "m/z " << x.mean() << " +/- " << y.mean() << " Da";
      return ss.str();
    }
  };
  
  
  /**
   * DataNode properties:
   * - spectrum.plot.color: color of plot lines in spectrum view's
   * - spectrum.marker.color: color of markers in spectrum view's selected area
   * - spectrum.marker.size: size of markers in spectrum view's selected area
   * 
   * BaseData properties:
   * - m2aia.image.pixel.count: number of reference pixels for this spectrum container
   * - m2aia.helper.spectrum.xaxis.count: number of values on the x axis (i.e. number of m/z values or centroids)
   */
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

    std::vector<double> GetXMean() const;
    std::vector<double> GetXSum() const;
    std::vector<double> GetXMax() const;
    std::vector<double> GetXMin() const;
    std::vector<unsigned int> GetXCount() const;

    std::vector<double> GetYMean() const;
    std::vector<double> GetYSum() const;
    std::vector<double> GetYMax() const;
    std::vector<double> GetYMin() const;
    std::vector<unsigned int> GetYCount() const;

    // std::vector<double> GetIndexMean() const;
    // std::vector<double> GetIndexSum() const;
    // std::vector<double> GetIndexMax() const;
    // std::vector<double> GetIndexMin() const;
    // std::vector<unsigned int> GetIndexCount() const;

    std::vector<Interval> &GetIntervals() { return m_Data; }
    const std::vector<Interval> &GetIntervals() const { return m_Data; }

    /**
     * @brief The names of the features carried by the intervals, in the order they were attached.
     *
     * A feature is one value per centroid produced by some analysis. Keeping the names here rather
     * than deriving them from the intervals keeps the order stable and lets a feature exist even
     * when some intervals do not carry a value for it.
     */
    const std::vector<std::string> &GetFeatureNames() const { return m_FeatureNames; }

    /**
     * @brief Attaches one value per interval under the given name, replacing a feature of the same
     * name. The number of values has to match the number of intervals; nothing is changed and
     * false is returned otherwise.
     */
    bool SetFeature(const std::string &name, const std::vector<double> &values);

    /**
     * @brief The values of the named feature, one per interval. Intervals that do not carry the
     * feature get the fallback. Empty if the feature does not exist.
     */
    std::vector<double> GetFeature(const std::string &name, double fallback = 0.0) const;

    bool HasFeature(const std::string &name) const;

    /** @brief Removes the feature from the vector and from every interval. */
    void RemoveFeature(const std::string &name);

    /** @brief Removes every feature. */
    void ClearFeatures();
    void SetType(SpectrumFormat type)
    {
      m_Type = type;
      this->SetProperty("m2aia.spectrum.type", mitk::StringProperty::New(m2::to_string(type)));
    }
    void SetInfo(std::string info)
    {
      m_Info = info;
      this->SetProperty("m2aia.spectrum.info", mitk::StringProperty::New(info));
    }

    /**
     * Get the type of an interval vector.
     * See enum definition: m2::SpectrumFormat
     */
    SpectrumFormat GetType() const { return m_Type; }

    /**
     * Custom data info
     * e.g. if it is an overview spectrum:
     * - overview.mean
     * - overview.max
     * - overview.single
     * - overview.centroids
     * ToDo: write as string property of the data
     */
    std::string GetInfo() const { return m_Info; }


    /**
     * @brief Set the number of source pixels for this spectrum container.
    */
    void SetNumberOfSourcePixels(unsigned int n)
    {
      m_NumberOfSourcePixels = n;
      this->SetProperty("m2aia.image.pixel.count", mitk::IntProperty::New(n));
    }

    /**
     * @brief Get the number of source pixels for this spectrum container.
    */
    unsigned int GetNumberOfSourcePixels() const { return m_NumberOfSourcePixels; }

    /**
     * @brief Check whether object contains data (at
     * least at one point in time), e.g., a set of points
     * may be empty
    */
    
    //## \warning Returns IsInitialized()==false by default for
    //## compatibility reasons. Override in sub-classes that
    //## support distinction between empty/non-empty state.
    virtual bool IsEmpty() const{
      return m_Data.empty();
    }

  private:
    std::vector<Interval> m_Data;
    std::vector<std::string> m_FeatureNames;
    std::string m_Info = "Not Set!";
    SpectrumFormat m_Type = SpectrumFormat::Centroid;

    unsigned int m_NumberOfSourcePixels = 0;
  };

} // namespace m2