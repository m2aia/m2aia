
//#include <Eigen/SVD>
#include <itkDataObject.h>
#include <itkImageRegionIterator.h>
#include <itkIndex.h>
#include <itkVariableLengthVector.h>
#include <itkVectorImage.h>
#include <m2PcaImageFilter.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <m2Timer.h>
#include <vnl/vnl_matrix.h>

void m2::PcaImageFilter::EqualizePcAxesEigen(MatrixXd *PcComponents)
{
  for (unsigned int col = 0; col < PcComponents->cols(); ++col)
  {
    if (PcComponents->coeffRef(0, col) < 0)
    {
      PcComponents->col(col) = PcComponents->col(col) * -1;
    }
  }
}

void m2::PcaImageFilter::initMatrix()
{
  this->GetValidIndices();

  const unsigned long numberOfrow = this->m_ValidIndices.size();
  const unsigned long numberOfcolumn = this->GetIndexedInputs().size();
  auto input = this->GetIndexedInputs();

  this->m_DataMatrix = MatrixXd(numberOfrow, numberOfcolumn);
  unsigned int column = 0;

  /*Fill matrix with image values one column includes values of one image*/
  for (auto it = input.begin(); it != input.end(); ++it, ++column)
  {
    mitk::Image::Pointer mitkImage = dynamic_cast<mitk::Image *>(it->GetPointer());
    AccessFixedDimensionByItk(mitkImage,
                              ([&column, this](auto input) {
                                unsigned int row = 0;

                                for (auto index : this->m_ValidIndices)
                                {
                                  this->m_DataMatrix(row, column) = input->GetPixel(index);
                                  ++row;
                                }
                              }),
                              3);
  }

  auto maxCoeffs = m_DataMatrix.colwise().maxCoeff();
  auto minCoeffs = m_DataMatrix.colwise().minCoeff();
  auto scalingFactors = maxCoeffs - minCoeffs;
  m_DataMatrix = ((m_DataMatrix.rowwise() - minCoeffs).array().rowwise() / scalingFactors.array()).matrix();

  auto means = m_DataMatrix.colwise().mean();
  m_DataMatrix = m_DataMatrix.rowwise() - means;
  auto stdDevs = m_DataMatrix.cwiseProduct(m_DataMatrix).colwise().sum() / (m_DataMatrix.rows()-1);
  
  for (unsigned int c = 0; c < m_DataMatrix.cols(); ++c)
	m_DataMatrix.col(c) = m_DataMatrix.col(c) / stdDevs[c];
}

void m2::PcaImageFilter ::GenerateData()
{
  auto timer = m2::Timer("Eigen Calculation");

  this->initMatrix();
  
  MatrixXd coMatrix = (m_DataMatrix.transpose() * m_DataMatrix);
  coMatrix = coMatrix / (m_DataMatrix.rows() - 1);

  Eigen::EigenSolver<MatrixXd> eigenSolver(coMatrix);
  auto result = this->m_DataMatrix * eigenSolver.eigenvectors().rightCols(m_NumberOfComponents).real();


  /*auto min = result.colwise().minCoeff();
  auto max = result.colwise().maxCoeff();
  result = ((result.rowwise() - min).array().rowwise() / ((max - min) * 255)).array();*/
  
  auto maxCoeffs = result.colwise().maxCoeff();
  auto minCoeffs = result.colwise().minCoeff();
  auto scalingFactors = maxCoeffs - minCoeffs;
  auto scaledResult = ((result.rowwise() - minCoeffs).array().rowwise() / scalingFactors.array() * 255).matrix();

  
  auto vectorImage = itk::VectorImage<PixelType, 3>::New();
  initializeItkVectorImage(vectorImage);
  
  unsigned int row = 0;
  for (auto index : m_ValidIndices)
  {
    itk::VariableLengthVector<PixelType> pixelValue;
    pixelValue.SetSize(m_NumberOfComponents);
    for (unsigned int i = 0; i < result.cols(); ++i)
    {
      pixelValue[i] = scaledResult(row, i);
    }
    vectorImage->SetPixel(index, pixelValue);
    ++row;
  }

  mitk::Image::Pointer output = this->GetOutput();
  mitk::CastToMitkImage(vectorImage, output);

  output->SetSpacing(this->GetInput()->GetGeometry()->GetSpacing());
  output->SetOrigin(this->GetInput()->GetGeometry()->GetOrigin());
}
