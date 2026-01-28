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

#include <itkObject.h>
#include <M2aiaDimensionReductionExports.h>
#include <m2IntervalVector.h>
#include <mitkLabelSetImage.h>
#include <m2ImzMLSpectrumImage.h>


// Eigen
// #include <itkeigen/Eigen/Dense>

namespace m2 {

// Define distance metric types
enum class DistanceMetric {
    EUCLIDEAN,
    CORRELATION,
    COSINE
};

// Define clustering algorithm types
enum class KMeansVariant {
    STANDARD,         // Traditional k-means
    SPATIAL,          // Spatial k-means (incorporates pixel coordinates)
};

class M2AIADIMENSIONREDUCTION_EXPORT KMeansImageFilter : public itk::Object
{

private:
    // Input and output images
    std::map<int, mitk::MultiLabelSegmentation::Pointer> m_Outputs;
    std::map<int, mitk::Image::Pointer> m_Inputs;

    // stores for each input the valid indices (masked pixels are valid)
    std::map<int, std::vector<itk::Index<3>>> m_ValidIndicesMap;

    double m_ShrinkageFactor = 0.1;
    double m_SpatialWeight = 0.5;  // Weight for spatial component (0-1)
    DistanceMetric m_DistanceMetric = DistanceMetric::EUCLIDEAN;
    KMeansVariant m_KMeansVariant = KMeansVariant::STANDARD;

public:
    mitkClassMacroItkParent(KMeansImageFilter, itk::Object);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    typedef mitk::MultiLabelSegmentation OutputType;

    itkSetMacro(NumberOfClusters, unsigned int);
    itkGetMacro(NumberOfClusters, unsigned int);
    itkSetMacro(ShrinkageFactor, double);
    itkGetMacro(ShrinkageFactor, double);
    itkSetMacro(SpatialWeight, double);
    itkGetMacro(SpatialWeight, double);
    
    void SetDistanceMetric(DistanceMetric metric) { m_DistanceMetric = metric; }
    DistanceMetric GetDistanceMetric() const { return m_DistanceMetric; }
    
    void SetKMeansVariant(KMeansVariant variant) { m_KMeansVariant = variant; }
    KMeansVariant GetKMeansVariant() const { return m_KMeansVariant; }

    void GenerateData();
    
    void SetInput(m2::SpectrumImage::Pointer image, int idx = 0)
    {
        m_Inputs[idx] = image;
    }
    
    void SetIntervals(std::vector<m2::Interval> intervals){
        m_Intervals = intervals;
    }
    
    mitk::MultiLabelSegmentation::Pointer GetOutput(int idx)
    {
        if(m_Outputs.find(idx) == m_Outputs.end())
        {
            m_Outputs[idx] = mitk::MultiLabelSegmentation::New();
        }
        return dynamic_cast<mitk::MultiLabelSegmentation*>(m_Outputs[idx].GetPointer());
    }
 
private:

    void DoSpatialKMeans(const Eigen::MatrixXd& data, 
                        const std::vector<itk::Index<3>>& spatialCoordinates,
                        int k, 
                        std::vector<int>& clusterAssignments);
    
    double ComputeDistance(const Eigen::VectorXd& point1, const Eigen::VectorXd& point2) const;
    double ComputeSpatialDistance(const itk::Index<3>& coord1, const itk::Index<3>& coord2) const;
    
    std::vector<m2::Interval> m_Intervals;
    unsigned int m_NumberOfClusters = 0;
    std::vector<Eigen::VectorXd> m_Centroids;
};

} // end namespace m2