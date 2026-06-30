/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2SefFileIO.h"

#include <mitkLog.h>
#include <nlohmann/json.hpp>

#include <fstream>

namespace m2
{

  SefFileIO::SefFileIO()
    : AbstractFileIO(m2::IntervalVector::GetStaticNameOfClass(), SEF_MIMETYPE(), "SEF Annotation List (M²aia)")
  {
    AbstractFileReader::SetRanking(10);
    AbstractFileWriter::SetRanking(10);
    this->RegisterService();
  }

  // ---------------------------------------------------------------------------
  mitk::IFileIO::ConfidenceLevel SefFileIO::GetReaderConfidenceLevel() const
  {
    if (AbstractFileIO::GetReaderConfidenceLevel() == Unsupported)
      return Unsupported;
    return Supported;
  }

  // ---------------------------------------------------------------------------
  std::vector<mitk::BaseData::Pointer> SefFileIO::DoRead()
  {
    const std::string path = this->GetInputLocation();

    std::ifstream in(path);
    if (!in.is_open())
    {
      MITK_ERROR << "[m2::SefFileIO] Cannot open file: " << path;
      return {};
    }

    nlohmann::json j;
    try
    {
      in >> j;
    }
    catch (const std::exception &e)
    {
      MITK_ERROR << "[m2::SefFileIO] JSON parse error in '" << path << "': " << e.what();
      return {};
    }

    if (!j.contains("peaklist") || !j["peaklist"].contains("intervals"))
    {
      MITK_ERROR << "[m2::SefFileIO] Missing 'peaklist.intervals' key in: " << path;
      return {};
    }

    auto vec = m2::IntervalVector::New();
    vec->SetType(m2::SpectrumFormat::Centroid);
    vec->SetInfo("SEF annotation list: " + path);

    for (const auto &entry : j["peaklist"]["intervals"])
    {
      if (!entry.contains("lower") || !entry.contains("upper"))
      {
        MITK_INFO << "[m2::SefFileIO] Skipping interval without 'lower'/'upper' in: " << path;
        continue;
      }

      const double lower = entry.at("lower").get<double>();
      const double upper = entry.at("upper").get<double>();
      const std::string name  = entry.value("name",          std::string{});
      const std::string color = entry.value("color",         std::string{});
      

      vec->GetIntervals().push_back(
        m2::Interval::FromBounds(lower, upper, name, color));
      
      MITK_INFO << lower << " - " << upper << " : " << name << " (" << color << ")";
    }

    // Resolve overlapping neighbouring intervals: subtract half the overlap from each side
    // {
    //   auto &intervals = vec->GetIntervals();
    //   std::sort(intervals.begin(), intervals.end()); // sorts by x.mean()
    //   for (std::size_t i = 0; i + 1 < intervals.size(); ++i)
    //   {
    //     const double rightEdge = intervals[i].x.max();
    //     const double leftEdge  = intervals[i + 1].x.min();
    //     if (rightEdge > leftEdge)
    //     {
    //       const double iLower  = intervals[i].x.min();
    //       const double i1Upper = intervals[i + 1].x.max();

    //       // Degenerate case: lower of i equals upper of i+1 — no meaningful midpoint
    //       if (iLower == i1Upper)
    //       {
    //         MITK_INFO << "[m2::SefFileIO] Degenerate overlap between '"
    //                   << intervals[i].description << "' and '" << intervals[i + 1].description
    //                   << "': skipping adjustment";
    //         continue;
    //       }

    //       const double half = (rightEdge - leftEdge) * 0.5;
    //       const double newUpper = rightEdge - half;
    //       const double newLower = leftEdge  + half;

    //       // Guard: correction must not invert either interval
    //       if (newUpper <= iLower || newLower >= i1Upper)
    //       {
    //         MITK_INFO << "[m2::SefFileIO] Half-overlap correction would invert interval '"
    //                   << intervals[i].description << "' or '" << intervals[i + 1].description
    //                   << "': skipping adjustment";
    //         continue;
    //       }

    //       MITK_INFO << "[m2::SefFileIO] Overlap (" << (rightEdge - leftEdge) << ") between '"
    //                 << intervals[i].description << "' and '" << intervals[i + 1].description
    //                 << "': adjusting bounds by half-overlap=" << half;
    //       intervals[i] = m2::Interval::FromBounds(
    //         intervals[i].x.min(), newUpper,
    //         intervals[i].description, intervals[i].color,
    //         intervals[i].mobilityLower, intervals[i].mobilityUpper);
    //       intervals[i + 1] = m2::Interval::FromBounds(
    //         newLower, intervals[i + 1].x.max(),
    //         intervals[i + 1].description, intervals[i + 1].color,
    //         intervals[i + 1].mobilityLower, intervals[i + 1].mobilityUpper);
    //     }
    //   }
    // }

    MITK_INFO << "[m2::SefFileIO] Loaded " << vec->GetIntervals().size()
              << " interval(s) from: " << path;

    return {vec};
  }

  // ---------------------------------------------------------------------------
  mitk::IFileIO::ConfidenceLevel SefFileIO::GetWriterConfidenceLevel() const
  {
    const auto *input = dynamic_cast<const m2::IntervalVector *>(this->GetInput());
    return input ? Supported : Unsupported;
  }

  void SefFileIO::Write()
  {
    const auto *input = dynamic_cast<const m2::IntervalVector *>(this->GetInput());
    if (!input)
    {
      MITK_ERROR << "[m2::SefFileIO] Write: input is not an m2::IntervalVector";
      return;
    }

    const auto &intervals = input->GetIntervals();

    nlohmann::json jIntervals = nlohmann::json::array();
    for (const auto &iv : intervals)
    {
      nlohmann::json entry;
      entry["lower"] = iv.x.count() > 0 ? iv.x.min() : 0.0;
      entry["upper"] = iv.x.count() > 0 ? iv.x.max() : 0.0;
      entry["name"]  = iv.description;
      entry["color"] = iv.color;
      jIntervals.push_back(std::move(entry));
    }

    nlohmann::json j;
    j["version"] = "2";
    j["peaklist"]["metaInformation"]["numberOfIntervals"] = static_cast<int>(intervals.size());
    j["peaklist"]["intervals"] = std::move(jIntervals);

    const std::string path = this->GetOutputLocation();
    std::ofstream out(path);
    if (!out.is_open())
    {
      MITK_ERROR << "[m2::SefFileIO] Cannot open file for writing: " << path;
      return;
    }

    out << j.dump(2) << "\n";
    MITK_INFO << "[m2::SefFileIO] Wrote " << intervals.size() << " interval(s) to: " << path;
  }

  // ---------------------------------------------------------------------------
  SefFileIO *SefFileIO::IOClone() const { return new SefFileIO(*this); }

} // namespace m2
