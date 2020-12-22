
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
#include <mitkTimer.h>
#include <vnl/vnl_matrix.h>

//void m2::PcaImageFilter::EqualizePcAxesEigen(MatrixXd *PcComponents)
//{
//  for (unsigned int col = 0; col < PcComponents->cols(); ++col)
//  {
//    if (PcComponents->coeffRef(0, col) < 0)
//    {
//      PcComponents->col(col) = PcComponents->col(col) * -1;
//    }
//  }
//}

void m2::PcaImageFilter::initMatrix()
{
  //this->GetValidIndices();

  //const unsigned long numberOfrow = this->m_ValidIndices.size();
  //const unsigned long numberOfcolumn = this->GetIndexedInputs().size();
  //auto input = this->GetIndexedInputs();

  //this->m_DataMatrix = MatrixXd(numberOfrow, numberOfcolumn);
  //unsigned int column = 0;

  //// performance increase when iterating over indices and get the values from the images, instead of iterating over the
  //// images and iterating over indices?
  ///*Fill matrix with image values
  //one column includes values of one image*/
  //for (auto it = input.begin(); it != input.end(); ++it, ++column)
  //{
  //  mitk::Image::Pointer mitkImage = dynamic_cast<mitk::Image *>(it->GetPointer());

  //  AccessFixedDimensionByItk(mitkImage,
  //                            ([&column, this](auto input) {
  //                              unsigned int row = 0;
  //                            
  //                              for (auto index : this->m_ValidIndices)
  //                              {
  //                               // auto v = input->GetPixel(index);
  //                                
  //                                this->m_DataMatrix(row, column) = input->GetPixel(index);
  //                                ++row;
  //                              }
  //                            }),
  //                            3);

  //  Eigen::VectorXd v(m_DataMatrix.col(column).size());
  //  this->m_DataMatrix.col(column) -= v.setConstant(this->m_DataMatrix.col(column).mean());
  //}
}

void m2::PcaImageFilter ::GenerateData()
{
  //auto timer = mitk::Timer("Eigen Calculation");

  //this->initMatrix();
  //MatrixXd coMatrix = this->m_DataMatrix.transpose() * this->m_DataMatrix / (this->m_ValidIndices.size() - 1);

  //Eigen::EigenSolver<MatrixXd> eigenSolver(coMatrix);
  //Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> eigenVectorMatrix = eigenSolver.eigenvectors().real();
  //auto eigenValues = eigenSolver.eigenvalues().real();
  //MatrixXd nEigenVectors(eigenVectorMatrix.rows(), m_NumberOfComponents);
  //std::vector<int> EigenValuesIndex;
  //EigenValuesIndex.resize(eigenValues.rows());
  //std::vector<double> nEigenValues;

  ///*Get the n largest eigenValues and save the corresponding index*/
  //for (unsigned int i = 0; i < eigenValues.rows(); ++i)
  //{
  //  nEigenValues.push_back(eigenValues(i, 0));
  //  EigenValuesIndex.at(i) = i;
  //}

  ///*This is struct is used to get the indices of the eigenvectors with the biggest eigenvalue*/
  //struct Comp
  //{
  //  Comp(const std::vector<double> &v) : _v(v) {}
  //  bool operator()(int a, int b) { return _v[a] > _v[b]; }
  //  const std::vector<double> &_v;
  //};

  //std::sort(EigenValuesIndex.begin(), EigenValuesIndex.end(), Comp(nEigenValues));

  //for (unsigned int i = 0; i < m_NumberOfComponents; ++i)
  //{
  //  nEigenVectors.col(i) = eigenVectorMatrix.col(EigenValuesIndex.at(i));
  //}

  //MatrixXd result = this->m_DataMatrix * nEigenVectors;
  //EqualizePcAxesEigen(&result);

  //itk::VectorImage<PixelType, 3>::Pointer vectorImage = itk::VectorImage<PixelType, 3>::New();
  //initializeItkVectorImage(vectorImage);

  //std::vector<double> dividers;
  //std::vector<PixelType> minValues;

  //for (unsigned int i = 0; i < result.cols(); ++i)
  //{
  //  auto max = result.col(i).maxCoeff();
  //  auto min = result.col(i).minCoeff();

  //  dividers.push_back(max - min);
  //  minValues.push_back(min);
  //}

  //int row = 0;
  //for (auto index : m_ValidIndices)
  //{
  //  itk::VariableLengthVector<PixelType> pixelValue;
  //  pixelValue.SetSize(m_NumberOfComponents);
  //  for (unsigned int i = 0; i < result.cols(); ++i)
  //  {
  //    pixelValue[i] = ((result(row, i) - minValues.at(i)) / dividers.at(i) * 255);
  //  }
  //  vectorImage->SetPixel(index, pixelValue);
  //  ++row;
  //}

  //mitk::Image::Pointer output = this->GetOutput();
  //mitk::CastToMitkImage(vectorImage, output);

  //mitk::Image::Pointer mitkPCAImage = this->GetOutput();

  //output->SetSpacing(this->GetInput()->GetGeometry()->GetSpacing());
  //output->SetOrigin(this->GetInput()->GetGeometry()->GetOrigin());
}
