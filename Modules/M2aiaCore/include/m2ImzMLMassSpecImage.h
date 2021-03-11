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
#include <algorithm>
#include <array>
#include <m2SpectrumImageBase.h>
#include <mitkDataNode.h>
#include <mitkITKImageImport.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkStringProperty.h>
#include <mitkVectorProperty.h>
#include <numeric>
#include <string>
#include <thread>

namespace m2
{
  class M2AIACORE_EXPORT ImzMLMassSpecImage final : public SpectrumImageBase
  {
  public:
    mitkClassMacro(ImzMLMassSpecImage, SpectrumImageBase);
    itkNewMacro(Self);

    itkSetEnumMacro(ExportMode, SpectrumFormatType);
    itkGetEnumMacro(ExportMode, SpectrumFormatType);

    itkSetEnumMacro(MzsOutputType, NumericType);
    itkGetEnumMacro(MzsOutputType, NumericType);

    itkSetEnumMacro(IntsOutputType, NumericType);
    itkGetEnumMacro(IntsOutputType, NumericType);

	itkSetEnumMacro(MzsInputType, NumericType);
    itkGetEnumMacro(MzsInputType, NumericType);

    itkSetEnumMacro(IntsInputType, NumericType);
    itkGetEnumMacro(IntsInputType, NumericType);

	itkSetEnumMacro(PreventMaskImageInitialization, bool);
	itkGetEnumMacro(PreventMaskImageInitialization, bool);
	itkBooleanMacro(PreventMaskImageInitialization);
	
	itkSetEnumMacro(PreventNormalizationImageInitialization, bool);
	itkGetEnumMacro(PreventNormalizationImageInitialization, bool);
	itkBooleanMacro(PreventNormalizationImageInitialization);

	itkSetEnumMacro(ImageGeometryInitialized, bool);
	itkGetEnumMacro(ImageGeometryInitialized, bool);
	
	itkSetEnumMacro(ImageAccessInitialized, bool);
	itkGetEnumMacro(ImageAccessInitialized, bool);
	



    using BinaryDataOffsetType = unsigned long long;
    using BinaryDataLengthType = unsigned long;

	void GenerateImageData(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const override;

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
      uint32_t id;
      itk::Index<3> index;
      struct
      {
        float x, y, z;
      } world;
      // In imzML-file defined normalization constant
      m2::NormImagePixelType normalize = -1.0;
    };

  /**
   * @brief BinaryTransformationMetaData structure holds meta information for a image 
   * transformation parameter collection. M2aia stores those collections in the imzML
   *  binary file, additionally and at the end of the spectral information. Within 
   *  XML of the imzML-file the tags 
   */
	struct BinaryTransformationMetaData
	{
		BinaryDataOffsetType offset;
		BinaryDataLengthType length;
	};

  using SpectraVectorType = std::vector<BinarySpectrumMetaData>;
	using TransformationsVectorType = std::vector<BinaryTransformationMetaData>;
/**
 * @brief The Source structure represent the meta information of an imzML-file.
 * 
 */
    struct Source
    {
      std::string _ImzMLDataPath;
      std::string _BinaryDataPath;
	  std::string _MaskDataPath;
	  std::string _PointsDataPath;

      /**
       * @brief _Spectra
       * Each single spectrum is represented as a BinarySpectrumMetaData object.
       */
      SpectraVectorType _Spectra;
      /**
       * @brief _Transformations
       * Is a vector of N BinaryTransformationMetaData objects.
       * 2D MSI: T transformations are applied sequentially (T=N).
       * 3D MSI with Z slices: - S transformations are applied sequentially slice-wise (N=S*Z+T). 
       *                       - T leftover transformations are applied to the whole volume.
       * 
       */
	    TransformationsVectorType _Transformations;
      itk::Offset<3> _offset = {0, 0, 0};
      m2::SpectrumFormatType ImportMode;
    };
    using SourceListType = std::vector<Source>;

    inline SourceListType &GetSourceList() noexcept { return m_SourcesList; }
    inline const SourceListType &GetSourceList() const noexcept { return m_SourcesList; }
    inline Source &GetSpectraSource(unsigned int i = 0)
    {
      if (i >= m_SourcesList.size())
        mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
      return m_SourcesList[i];
    }

    inline const Source &GetSpectraSource(unsigned int i = 0) const
    {
      if (i >= m_SourcesList.size())
        mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
      return m_SourcesList[i];
    }

    void InitializeImageAccess() override;
    void InitializeGeometry() override;

    bool UseLog10 = false;
	bool UsePercentage = false;
    
	bool m_PreventMaskImageInitialization = false;
	bool m_PreventNormalizationImageInitialization = false;

    template <class T>
    static inline void binaryDataToVector(std::ifstream &f,
                                          const unsigned long long int &offset,
                                          const unsigned long int &length,
                                          std::vector<T> &vec) noexcept;

    static m2::ImzMLMassSpecImage::Pointer Combine(const m2::ImzMLMassSpecImage *A,
                                                   const m2::ImzMLMassSpecImage *B,
                                                   const char stackAxis = 'x');

  private:
    m2::NumericType m_IntsOutputType = m2::NumericType::Float;
    m2::NumericType m_MzsOutputType = m2::NumericType::Float;
    m2::NumericType m_IntsInputType;
    m2::NumericType m_MzsInputType;
    m2::SpectrumFormatType m_ExportMode = m2::SpectrumFormatType::ContinuousProfile;

    using m2::SpectrumImageBase::InternalClone;
    template <class MassAxisType, class IntensityType>
    class ImzMLProcessor;
    void InitializeProcessor();

    SourceListType m_SourcesList;

    bool m_ImageAccessInitialized = false;
	bool m_ImageGeometryInitialized = false;

    ImzMLMassSpecImage();
    ~ImzMLMassSpecImage() override;
  };

  template <class MassAxisType, class IntensityType>
  class ImzMLMassSpecImage::ImzMLProcessor : public ImzMLMassSpecImage::ProcessorBase
  {
  private:
    friend class ImzMLMassSpecImage;
    m2::ImzMLMassSpecImage *p;

  public:
    explicit ImzMLProcessor(m2::ImzMLMassSpecImage *owner) : p(owner) {}
    void GrabIonImagePrivate(double mz, double tol, const mitk::Image *mask, mitk::Image *image) const override;
    void GrabIntensityPrivate(unsigned long int index,
                              std::vector<double> &ints,
                              unsigned int sourceIndex = 0) const override;
    void GrabMassPrivate(unsigned long int index,
                         std::vector<double> &mzs,
                         unsigned int sourceIndex = 0) const override;
    void InitializeImageAccess() override;
    void InitializeGeometry() override;
  };

  template <class T>
  void ImzMLMassSpecImage::binaryDataToVector(std::ifstream &f,
                                                     const unsigned long long int &offset,
                                                     const unsigned long int &length,
                                                     std::vector<T> &vec) noexcept
  {
    if (!f)
    {
      MITK_ERROR << "Binary ImzML Data object not accessible!";
    }

    vec.resize(length);
    f.seekg(offset);
    f.read((char *)vec.data(), length * sizeof(T));
  }

} // namespace m2
