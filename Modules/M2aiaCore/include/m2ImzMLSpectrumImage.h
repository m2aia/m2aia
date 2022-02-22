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
#include <m2SpectrumImageBase.h>
#include <signal/m2Baseline.h>
#include <signal/m2Smoothing.h>
#include <signal/m2Transformer.h>
#include <m2SpectrumImageProcessor.h>

namespace m2
{
 

  class M2AIACORE_EXPORT ImzMLSpectrumImage final : public SpectrumImageBase
  {
  public:
    mitkClassMacro(ImzMLSpectrumImage, SpectrumImageBase);
    itkNewMacro(Self);

    itkSetEnumMacro(ImageGeometryInitialized, bool);
    itkGetEnumMacro(ImageGeometryInitialized, bool);

    itkSetEnumMacro(ImageAccessInitialized, bool);
    itkGetEnumMacro(ImageAccessInitialized, bool);

    using BinaryDataOffsetType = unsigned long long;
    using BinaryDataLengthType = unsigned long;

    void GetImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;

    /**
     * @brief The BinarySpectrumMetaData structure holds meta data for a single spectrum.
     *
     */
    struct BinarySpectrumMetaData
    {
      BinaryDataOffsetType mzOffset;
      BinaryDataOffsetType intOffset;
      BinaryDataLengthType mzLength;
      BinaryDataLengthType intLength;
      // size_t id;
      itk::Index<3> index;
      struct
      {
        float x, y, z;
      } world;
      // In imzML-file defined normalization constant
      m2::NormImagePixelType normalizationFactor = 1.0;
      m2::NormImagePixelType inFileNormalizationFactor = 1.0;
      std::vector<m2::Peak> peaks;
    };
    using SpectrumVectorType = std::vector<BinarySpectrumMetaData>;

    /**
     * @brief The ImzMLImageSource structure represent the meta information of an imzML-file.
     */
    struct ImzMLImageSource
    {
      std::string m_ImzMLDataPath;
      std::string m_BinaryDataPath;
      std::string m_MaskDataPath;
      std::string m_PointsDataPath;

      // For each spectrum in the image exists a meta data object
      SpectrumVectorType m_Spectra;

      // Pixel data of each image source is placed with respect to this offset
      // Currentlz it is used by the Combine method
      itk::Offset<3> m_Offset = {0, 0, 0};

      // Transformations are applied if available using elastix transformix
      std::vector<std::string> m_Transformations;
    };
    using SourceListType = std::vector<ImzMLImageSource>;

    SourceListType &GetImzMLSpectrumImageSourceList() noexcept { return m_SourcesList; }
    const SourceListType &GetImzMLSpectrumImageSourceList() const noexcept { return m_SourcesList; }
    ImzMLImageSource &GetImzMLSpectrumImageSource(unsigned int i = 0);
    const ImzMLImageSource &GetImzMLSpectrumImageSource(unsigned int i = 0) const;

    void InitializeImageAccess() override;
    void InitializeGeometry() override;
    void InitializeProcessor() override;
    void GetSpectrum(unsigned int id,
                     std::vector<float> &xs,
                     std::vector<float> &ys,
                     unsigned int source = 0) const override;

    template <class OffsetType, class LengthType, class DataType>
    static void binaryDataToVector(std::ifstream &f, OffsetType offset, LengthType length, DataType *vec) noexcept
    {
      f.seekg(offset);
      f.read((char *)vec, length * sizeof(DataType));
    }

    inline void GetXValues(unsigned int id, std::vector<double> &xs, unsigned int source = 0)
    {
      m_Processor->GetXValues(id, xs, source);
    }

    inline void GetXValues(unsigned int id, std::vector<float> &xs, unsigned int source = 0)
    {
      m_Processor->GetXValues(id, xs, source);
    }

    inline void GetYValues(unsigned int id, std::vector<double> &ys, unsigned int source = 0)
    {
      m_Processor->GetYValues(id, ys, source);
    }

    inline void GetYValues(unsigned int id, std::vector<float> &ys, unsigned int source = 0)
    {
      m_Processor->GetYValues(id, ys, source);
    }


    static m2::ImzMLSpectrumImage::Pointer Combine(const m2::ImzMLSpectrumImage *A,
                                                   const m2::ImzMLSpectrumImage *B,
                                                   const char stackAxis = 'x');

  private:
    using m2::SpectrumImageBase::InternalClone;
    template <class MassAxisType, class IntensityType>
    class ImzMLImageProcessor;

    SourceListType m_SourcesList;

    bool m_ImageAccessInitialized = false;
    bool m_ImageGeometryInitialized = false;

    ImzMLSpectrumImage();
    ~ImzMLSpectrumImage() override;

    template <class MassAxisType, class IntensityType>
    class Processor : public m2::ProcessorBase
    {
    private:
      friend class ImzMLSpectrumImage;
      m2::ImzMLSpectrumImage *p;

    public:
      explicit Processor(m2::ImzMLSpectrumImage *owner) : p(owner) {}
      void GetImagePrivate(double mz, double tol, const mitk::Image *mask, mitk::Image *image);
      // void GetSpectrumPrivate(unsigned int, std::vector<float> &, std::vector<float> &, unsigned int) override {}

      void InitializeImageAccess();
      void InitializeGeometry();
      void InitializeImageAccessContinuousProfile();

      /**
       * @brief Provides optimized access to centroid data.
       * No binning is applied. Normalization factors are provided.
       */
      void InitializeImageAccessContinuousCentroid();

      /**
       * @brief See InitializeImageAccessProcessedData()       
       */
      void InitializeImageAccessProcessedProfile();

      /**
       * @brief See InitializeImageAccessProcessedData()       
       */
      void InitializeImageAccessProcessedCentroid();

      /**
       * @brief Provides acces to processed centroid and processed profile spectra.
       * It includes binning for visualization purposes of the overview spectra
       * and invokes the calculation of normalization factors, but no further 
       * signal-processing is supported here.
       */
      void InitializeImageAccessProcessedData();

      
      static double GetNormalizationFactor(m2::NormalizationStrategyType strategy, MassAxisType * xFirst, MassAxisType * xLast, IntensityType * yFirst, IntensityType * yLast){
               using namespace std;
            switch (strategy)
            {
              case m2::NormalizationStrategyType::Median:
                return m2::Signal::Median(yFirst, yLast);
              case m2::NormalizationStrategyType::TIC:
                return m2::Signal::TotalIonCurrent(xFirst, xLast, yFirst);
              case m2::NormalizationStrategyType::Sum:
                return accumulate(yFirst, yLast, double(0.0));
              case m2::NormalizationStrategyType::Mean:
                return accumulate(yFirst, yLast, double(0.0)) / double(std::distance(yFirst, yLast));
              case m2::NormalizationStrategyType::Max:
                return *max_element(yFirst, yLast);
              case m2::NormalizationStrategyType::InFile:
              default:
                return 1;

            }
      }


      using XIteratorType = typename std::vector<MassAxisType>::iterator;
      using YIteratorType = typename std::vector<IntensityType>::iterator;
      m2::Signal::SmoothingFunctor<IntensityType> m_Smoother;
      m2::Signal::BaselineFunctor<IntensityType> m_BaselineSubstractor;
      m2::Signal::IntensityTransformationFunctor<IntensityType> m_Transformer;


      virtual void GetYValues(unsigned int id, std::vector<float> &yd, unsigned int source = 0)
      {
        GetYValues<float>(id, yd, source);
      }
      virtual void GetYValues(unsigned int id, std::vector<double> &yd, unsigned int source = 0)
      {
        GetYValues<double>(id, yd, source);
      }
      virtual void GetXValues(unsigned int id, std::vector<float> &yd, unsigned int source = 0)
      {
        GetXValues<float>(id, yd, source);
      }
      virtual void GetXValues(unsigned int id, std::vector<double> &yd, unsigned int source = 0)
      {
        GetXValues<double>(id, yd, source);
      }

    private:
      template <class OutputType>
      void GetYValues(unsigned int id, std::vector<OutputType> &yd, unsigned int source = 0);
      template <class OutputType>
      void GetXValues(unsigned int id, std::vector<OutputType> &xd, unsigned int source = 0);
    };

    std::unique_ptr<ProcessorBase> m_Processor;
  };

} // namespace m2
