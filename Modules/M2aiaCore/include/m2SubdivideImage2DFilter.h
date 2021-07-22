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
#include <mitkImageToImageFilter.h>
#include <m2SpectrumImageBase.h>
#include <itkSmartPointer.h>

namespace m2
{

  /**
   * @brief This filter creates for a single input multiple outputs with user defined tile size
   * 
   */
  class M2AIACORE_EXPORT SubdivideImage2DFilter : public itk::Object
  {
  public:
    mitkClassMacroItkParent(SubdivideImage2DFilter, itk::Object);
    itkNewMacro(SubdivideImage2DFilter);
    
    // virtual void Modified();
    
    itkSetMacro(TileHeight, unsigned);
    itkGetMacro(TileHeight, unsigned);
    itkSetMacro(TileWidth, unsigned);
    itkGetMacro(TileWidth, unsigned);
    itkGetMacro(InputWidth, unsigned);
    itkGetMacro(InputHeight, unsigned);
    itkGetMacro(NumberOfTiles, unsigned);
    itkGetMacro(NumberOfTilesInX, unsigned);
    itkSetMacro(NumberOfTilesInX, unsigned);
    itkGetMacro(NumberOfTilesInY, unsigned);
    itkSetMacro(NumberOfTilesInY, unsigned);
    virtual mitk::Image::Pointer GetInput();
    virtual void SetInput(mitk::Image *);
    virtual void Update();
    virtual std::vector<mitk::Image::Pointer> GetOutputs();
    
  protected:
    unsigned m_TileWidth = 1 << 13;
    unsigned m_TileHeight = 1 << 13;
    unsigned m_NumberOfTiles = 1;
    unsigned m_NumberOfTilesInX = 1;
    unsigned m_NumberOfTilesInY = 1;
    unsigned m_InputWidth, m_InputHeight;

    bool m_HasResidualsX, m_HasResidualsY;
    
    mitk::Image::Pointer m_Input;
    std::vector<mitk::Image::Pointer> m_Outputs;
    virtual void GenerateData();
  };

} // namespace mitk
