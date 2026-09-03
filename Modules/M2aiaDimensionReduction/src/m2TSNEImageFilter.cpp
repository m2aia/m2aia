/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2TSNEImageFilter.h>
#include <tsne/tsne.h>

#include <memory>

void m2::TSNEImageFilter::ComputeEmbedding()
{
  const auto &data = m_Features.data;
  const auto numberOfPixels = static_cast<int>(data.rows());
  const auto numberOfInputDimensions = static_cast<int>(data.cols());
  const auto numberOfOutputDimensions = static_cast<int>(m_NumberOfComponents);

  MITK_INFO << "t-SNE on " << numberOfPixels << " pixels of " << numberOfInputDimensions << " dimensions, "
            << "perplexity " << m_Perplexity << ", " << m_Iterations << " iterations";
  ReportProgress(0, 1);

  // the implementation expects the data points row by row
  std::vector<double> input(static_cast<size_t>(numberOfPixels) * numberOfInputDimensions);
  for (int row = 0; row < numberOfPixels; ++row)
    for (int column = 0; column < numberOfInputDimensions; ++column)
      input[static_cast<size_t>(row) * numberOfInputDimensions + column] = data(row, column);

  std::vector<double> output(static_cast<size_t>(numberOfPixels) * numberOfOutputDimensions, 0.0);

  auto tsne = std::make_unique<TSNE::TSNE>();
  tsne->run(input.data(),
            numberOfPixels,
            numberOfInputDimensions,
            output.data(),
            numberOfOutputDimensions,
            static_cast<double>(m_Perplexity),
            m_Theta,
            -1,
            false,
            static_cast<int>(m_Iterations),
            250,
            250);

  m_Embedding.resize(numberOfPixels, numberOfOutputDimensions);
  for (int row = 0; row < numberOfPixels; ++row)
    for (int column = 0; column < numberOfOutputDimensions; ++column)
      m_Embedding(row, column) = static_cast<float>(output[static_cast<size_t>(row) * numberOfOutputDimensions + column]);

  ReportProgress(1, 1);
}
