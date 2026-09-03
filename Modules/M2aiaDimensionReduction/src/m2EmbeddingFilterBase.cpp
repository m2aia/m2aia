/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2EmbeddingFilterBase.h>
#include <m2Timer.h>

void m2::EmbeddingFilterBase::Update()
{
  if (m_Features.IsEmpty())
  {
    MITK_WARN << GetMethodName() << " was started without a feature matrix.";
    return;
  }

  auto timer = m2::Timer(GetMethodName());
  m_Embedding.resize(0, 0);
  m_Loadings.resize(0, 0);
  m_Labels.clear();

  this->ComputeEmbedding();
}
