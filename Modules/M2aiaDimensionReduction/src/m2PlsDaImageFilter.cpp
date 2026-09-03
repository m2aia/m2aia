/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2PlsDaImageFilter.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace
{
  /** Steps of the inner loop that looks for one component; it converges in a handful of them. */
  constexpr unsigned int INNER_ITERATIONS = 100;
  constexpr float INNER_TOLERANCE = 1e-8f;
  constexpr float EPSILON = 1e-12f;
} // namespace

bool m2::PlsDaImageFilter::DetermineClasses()
{
  const auto &classes = m_Features.classes;

  if (classes.size() != static_cast<size_t>(m_Features.data.rows()))
  {
    MITK_ERROR << "PLS-DA needs one class per pixel; the mask did not provide them.";
    return false;
  }

  // the label value zero is the unlabeled background and is not a class; pixels carrying it were
  // already left out when the feature matrix was built
  std::set<unsigned int> distinct(classes.begin(), classes.end());
  distinct.erase(0);

  m_ClassValues.assign(distinct.begin(), distinct.end());

  if (m_ClassValues.size() < 2)
  {
    MITK_ERROR << "PLS-DA needs at least two classes, the mask provides " << m_ClassValues.size()
               << ". Select a mask whose labels mark the regions to tell apart.";
    return false;
  }

  return true;
}

void m2::PlsDaImageFilter::ComputeEmbedding()
{
  if (!DetermineClasses())
    return;

  const auto numberOfPixels = m_Features.data.rows();
  const auto numberOfPeaks = m_Features.data.cols();
  const auto numberOfClasses = static_cast<Eigen::Index>(m_ClassValues.size());

  // a component can neither exceed the number of peaks nor the number of pixels
  const auto components = static_cast<Eigen::Index>(
    std::min<Eigen::Index>(m_NumberOfComponents, std::min(numberOfPeaks, numberOfPixels - 1)));

  if (components < 1)
  {
    MITK_ERROR << "PLS-DA cannot compute a single component from this data.";
    return;
  }

  MITK_INFO << "PLS-DA on " << numberOfPixels << " pixels, " << numberOfPeaks << " peaks and " << numberOfClasses
            << " classes, " << components << " components";

  // one hot coding of the classes: one column per class, one in the column of the class of that
  // pixel and zero everywhere else
  std::map<unsigned int, Eigen::Index> classColumns;
  for (Eigen::Index column = 0; column < numberOfClasses; ++column)
    classColumns[m_ClassValues[column]] = column;

  Eigen::MatrixXf Y = Eigen::MatrixXf::Zero(numberOfPixels, numberOfClasses);
  for (Eigen::Index row = 0; row < numberOfPixels; ++row)
  {
    // a pixel whose class is not among the ones found stays all zero and therefore carries no
    // class information, rather than being counted towards the first one
    const auto column = classColumns.find(m_Features.classes[row]);
    if (column != classColumns.end())
      Y(row, column->second) = 1.0f;
  }

  // both blocks are centred; the peaks are additionally put on a common scale so that a weak peak
  // can separate the classes as well as a strong one
  Eigen::MatrixXf X = m_Features.data.rowwise() - m_Features.data.colwise().mean();
  if (m_AutoScale)
  {
    for (Eigen::Index column = 0; column < numberOfPeaks; ++column)
    {
      const float deviation =
        numberOfPixels > 1 ? std::sqrt(X.col(column).squaredNorm() / static_cast<float>(numberOfPixels - 1)) : 0.0f;
      if (deviation > EPSILON)
        X.col(column) /= deviation;
      else
        X.col(column).setZero();
    }
  }
  Y = Y.rowwise() - Y.colwise().mean();

  Eigen::MatrixXf scores(numberOfPixels, components);
  Eigen::MatrixXf weights(numberOfPeaks, components);
  m_ClassLoadings.resize(components, numberOfClasses);

  for (Eigen::Index component = 0; component < components && !IsCancelRequested(); ++component)
  {
    // start from the class column that still carries the most information
    Eigen::Index startColumn = 0;
    Y.colwise().squaredNorm().maxCoeff(&startColumn);
    Eigen::VectorXf u = Y.col(startColumn);

    Eigen::VectorXf w = Eigen::VectorXf::Zero(numberOfPeaks);
    Eigen::VectorXf t = Eigen::VectorXf::Zero(numberOfPixels);
    Eigen::VectorXf q = Eigen::VectorXf::Zero(numberOfClasses);

    for (unsigned int inner = 0; inner < INNER_ITERATIONS; ++inner)
    {
      // the direction in the peaks that lines up best with the current class direction
      w = X.transpose() * u;
      const float weightNorm = w.norm();
      if (weightNorm <= EPSILON)
        break;
      w /= weightNorm;

      t = X * w;
      const float scoreNorm = t.squaredNorm();
      if (scoreNorm <= EPSILON)
        break;

      q = Y.transpose() * t / scoreNorm;
      const float classNorm = q.squaredNorm();
      if (classNorm <= EPSILON)
        break;

      const Eigen::VectorXf updated = Y * q / classNorm;
      const float change = (updated - u).norm();
      u = updated;

      if (change < INNER_TOLERANCE)
        break;
    }

    const float scoreNorm = t.squaredNorm();
    if (scoreNorm <= EPSILON)
    {
      MITK_WARN << "PLS-DA stopped after " << component << " components; the data carries nothing further "
                   "that separates the classes.";
      scores.conservativeResize(Eigen::NoChange, component);
      weights.conservativeResize(Eigen::NoChange, component);
      m_ClassLoadings.conservativeResize(component, Eigen::NoChange);
      break;
    }

    scores.col(component) = t;
    weights.col(component) = w;
    m_ClassLoadings.row(component) = q.transpose();

    // what this component explains is removed, so the next one describes something else
    const Eigen::VectorXf peakLoading = X.transpose() * t / scoreNorm;
    X -= t * peakLoading.transpose();
    Y -= t * q.transpose();

    ReportProgress(static_cast<unsigned int>(component + 1), static_cast<unsigned int>(components));
  }

  if (scores.cols() == 0)
  {
    MITK_ERROR << "PLS-DA did not yield a single component.";
    return;
  }

  m_Embedding = scores;
  // rows are the components, columns the peaks, as for every other method
  m_Loadings = weights.transpose();

  MITK_INFO << "PLS-DA finished with " << m_Embedding.cols() << " components";
}
