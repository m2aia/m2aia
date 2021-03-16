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

namespace m2
{
  class M2AIACORE_EXPORT ImzMLSpectrumImage final : public SpectrumImageBase
  {
  public:
    mitkClassMacro(ImzMLSpectrumImage, SpectrumImageBase);
    itkNewMacro(Self);

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

    using SpectrumVectorType = std::vector<BinarySpectrumMetaData>;

    /**
     * @brief The Source structure represent the meta information of an imzML-file.
     */
    struct Source
    {
      std::string _ImzMLDataPath;
      std::string _BinaryDataPath;
      std::string _MaskDataPath;
      std::string _PointsDataPath;
      SpectrumVectorType _Spectra;
      itk::Offset<3> _offset = {0, 0, 0};
    };
    using SourceListType = std::vector<Source>;

    SourceListType &GetSpectrumImageSourceList() noexcept { return m_SourcesList; }
    const SourceListType &GetSpectrumImageSourceList() const noexcept { return m_SourcesList; }
    Source &GetSpectrumImageSource(unsigned int i = 0);
    const Source &GetSpectrumImageSource(unsigned int i = 0) const;

    void InitializeImageAccess() override;
    void InitializeGeometry() override;

    bool m_PreventMaskImageInitialization = false;
    bool m_PreventNormalizationImageInitialization = false;

    template <class T>
    static void binaryDataToVector(std::ifstream &f,
                                   const unsigned long long int &offset,
                                   const unsigned long int &length,
                                   std::vector<T> &vec) noexcept;

    static m2::ImzMLSpectrumImage::Pointer Combine(const m2::ImzMLSpectrumImage *A,
                                                   const m2::ImzMLSpectrumImage *B,
                                                   const char stackAxis = 'x');
	
    

  private:
    using m2::SpectrumImageBase::InternalClone;
    template <class MassAxisType, class IntensityType>
    class ImzMLProcessor;
    void InitializeProcessor();

    SourceListType m_SourcesList;

    bool m_ImageAccessInitialized = false;
    bool m_ImageGeometryInitialized = false;

    ImzMLSpectrumImage();
    ~ImzMLSpectrumImage() override;
  };

  template <class MassAxisType, class IntensityType>
  class ImzMLSpectrumImage::ImzMLProcessor : public ImzMLSpectrumImage::ProcessorBase
  {
  private:
    friend class ImzMLSpectrumImage;
    m2::ImzMLSpectrumImage *p;

  public:
    explicit ImzMLProcessor(m2::ImzMLSpectrumImage *owner) : p(owner) {}
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

} // namespace m2
