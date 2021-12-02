/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <m2SubdivideImage2DFilter.h>
#include <mitkIOUtil.h>
#include <mitkImageCast.h>

#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>


void m2::SubdivideImage2DFilter::Update()
{
  GenerateData();
}

void m2::SubdivideImage2DFilter::SetInput( mitk::Image *input)
{
  m_Input = input;
}

mitk::Image::Pointer m2::SubdivideImage2DFilter::GetInput(){
  
  return m_Input;
}

std::vector<mitk::Image::Pointer> m2::SubdivideImage2DFilter::GetOutputs(){
  return m_Outputs;
}

void m2::SubdivideImage2DFilter::GenerateData()
{
  auto input = this->GetInput();
  m_InputWidth = input->GetDimensions()[0];
  m_InputHeight = input->GetDimensions()[1];

  m_HasResidualsX = bool(m_InputWidth % m_TileWidth);
  m_HasResidualsY = bool(m_InputHeight % m_TileHeight);

  m_NumberOfTilesInX = m_InputWidth / m_TileWidth + m_HasResidualsX;
  m_NumberOfTilesInY = m_InputHeight / m_TileHeight + m_HasResidualsY;
  m_NumberOfTiles = m_NumberOfTilesInX * m_NumberOfTilesInY;

  this->m_Outputs.resize(m_NumberOfTiles);

  unsigned residualTileWidth = m_InputWidth % m_TileWidth;
  unsigned residualTileHeight = m_InputHeight % m_TileHeight;

  const auto refPixelType = input->GetPixelType();
  const auto ps = refPixelType.GetSize();

  unsigned i = 0;
  for (unsigned y = 0; y < m_NumberOfTilesInY; ++y)
  {
    unsigned tileHeight = m_TileHeight;
    if (m_HasResidualsY && y == (m_NumberOfTilesInY - 1))
      tileHeight = residualTileHeight;

    for (unsigned x = 0; x < m_NumberOfTilesInX; ++x)
    {
      unsigned tileWidth = m_TileWidth;
      if (m_HasResidualsX && x == (m_NumberOfTilesInX - 1))
        tileWidth = residualTileWidth;

      auto output = m_Outputs[i] = mitk::Image::New();

      output->Initialize(refPixelType, 3, std::array<unsigned, 3>{tileWidth, tileHeight, 1}.data());
      auto s = input->GetGeometry()->GetSpacing();
      output->SetSpacing(s);
      auto o = input->GetGeometry()->GetOrigin();
      o[0] = o[0] + x * m_TileWidth * s[0];
      o[1] = o[1] + y * m_TileHeight * s[1];
      output->SetOrigin(o);
      

      MITK_INFO << "InFilter " << static_cast<mitk::Image*>(output.GetPointer())->GetGeometry()->GetOrigin();
      
      
      mitk::ImageWriteAccessor outAcc(output);
      mitk::ImageReadAccessor inAcc(input);

      char *outputStart = static_cast<char *>(outAcc.GetData());
      const char *inputStart = static_cast<const char *>(inAcc.GetData());

      for (unsigned row = 0; row < tileHeight; ++row)
      {
        const char *start =
          inputStart + (x * m_TileWidth * ps) + (y * m_TileHeight * m_InputWidth * ps) + (row * m_InputWidth * ps);
        const char *end = start + (tileWidth * ps);

        std::copy(start, end, outputStart);
        outputStart += tileWidth * ps;
      }

      ++i;
    }
  }
}
