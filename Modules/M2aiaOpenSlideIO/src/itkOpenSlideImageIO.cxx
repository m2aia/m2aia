/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include <cctype>
#include <algorithm>
#include <sstream>

#include "itkIOCommon.h"
#include "itkOpenSlideImageIO.h"
#include "itksys/SystemTools.hxx"
#include "itkMetaDataDictionary.h"
#include "itkMetaDataObject.h"

// OpenSlide
#include <openslide/openslide.h>

namespace itk
{

// OpenSlide wrapper class
// This is responsible for freeing the OpenSlide context on destruction
// It also allows for seamless access to various levels and associated images through one set of functions (as opposed to two)
class OpenSlideWrapper {
public:

  // GCD Algorithm from wikipedia pseudo code: 
  // https://en.wikipedia.org/wiki/Greatest_common_divisor#Binary_method
  // 
  // NOTE: While vnl_rational can do this too, vnl_rational is defined for long integers (32 bit integers on amd64).
  //       Slides can be extremely large. It would ideal to work with 64 bit integers.
  static uint64_t GCD(uint64_t ui64A, uint64_t ui64B) {
    if (ui64A == 0)
      return ui64B;

    if (ui64B == 0)
      return ui64A;

    if (ui64A == 1 || ui64B == 1)
      return 1;

    if (ui64A == ui64B)
      return ui64A;

    unsigned int uiExp = 0;
    while ((ui64A & 1) == 0 && (ui64B & 1) == 0) {
      ui64A >>= 1;
      ui64B >>= 1;
      ++uiExp;
    }

    while (ui64A != ui64B) {
      if ((ui64A & 1) == 0)
        ui64A >>= 1;
      else if ((ui64B & 1) == 0)
        ui64B >>= 1;
      else if (ui64A > ui64B)
        ui64A = (ui64A - ui64B) >> 1;
      else
        ui64B = (ui64B - ui64A) >> 1;
    }

    return ui64A << uiExp;
  }

  // Detects the vendor. Should return NULL if the file is not readable.
  static const char * DetectVendor(const char *p_cFileName) {
    return openslide_detect_vendor(p_cFileName);
  }

  // Weak check if the file can be read
  static bool CanReadFile(const char *p_cFileName) {
    return DetectVendor(p_cFileName) != NULL;
  }

  // Returns version of OpenSlide library
  static const char * GetVersion() {
    return openslide_get_version();
  }

  // Constructors
  OpenSlideWrapper() {
    m_Osr = NULL;
    m_Level = 0;
    m_ApproximateStreaming = false;
  }

  OpenSlideWrapper(const char *p_cFileName) {
    m_Osr = NULL;
    m_Level = 0;
    m_ApproximateStreaming = false;
    Open(p_cFileName);
  }

  // Destructor
  ~OpenSlideWrapper() {
    Close();
  }

  // Set whether streaming should be approximate or exact
  void SetApproximateStreaming(bool bApproximateStreaming) {
    m_ApproximateStreaming = bApproximateStreaming;
  }

  // Determine whether streaming is exact or approximate
  bool GetApproximateStreaming() const {
    return m_ApproximateStreaming;
  }

  // Tells the ImageIO if the wrapper is in a state where stream reading can occur.
  // While OpenSlide supports reading regions of level images, it does not for associated images.
  bool CanStreamRead() const {
    if (m_AssociatedImage.size() > 0)
      return false;

    // XXX: ITK streams along X. Shouldn't we check if the minimum spacing in Y is not the size of the whole image?
    return m_ApproximateStreaming || ComputeMaximumNumberOfStreamableRegions() > 1;
  }

  // Closes the currently opened file
  void Close() {
    if (m_Osr != NULL) {
      openslide_close(m_Osr);
      m_Osr = NULL;
    }
  }

  // Checks weather a slide file is currently opened
  bool IsOpened() const {
    return m_Osr != NULL;
  }

  // Opens a slide file
  bool Open(const char *p_cFileName) {
    Close();
    m_Osr = openslide_open(p_cFileName);
    return m_Osr != NULL;
  }

  // Get error string, NULL if there is no error
  const char * GetError() const {
    if (m_Osr == NULL)
      return "OpenSlideWrapper has no file open.";

    return openslide_get_error(m_Osr);
  }

  // Sets the level that is accessible with ReadRegion, GetDimensions, GetSpacing.
  // Clears any associated image context.
  void SetLevel(int32_t i32Level) {
    m_Level = i32Level;
    m_AssociatedImage.clear();
  }

  // Returns the currently selected level
  int32_t GetLevel() const {
    return m_Level;
  }

  // Sets the associated image that is accessible with ReadRegion, GetDimensions
  void SetAssociatedImageName(const std::string &strImageName) {
    m_AssociatedImage = strImageName;
    m_Level = 0;
  }

  // Returns the currently selected associated image
  const std::string & GetAssociatedImageName() const {
    return m_AssociatedImage;
  }

  // Given a downsample factor, uses OpenSlide to determine the best level to use.
  bool SetBestLevelForDownsample(double dDownsample) {
    if (m_Osr == NULL)
      return false;

    const int32_t i32Level = openslide_get_best_level_for_downsample(m_Osr, dDownsample);

    if (i32Level < 0)
      return false;

    SetLevel(i32Level);

    return true;
  }

  // Returns the number of levels in this file
  int32_t GetLevelCount() const {
    if (m_Osr == NULL)
      return -1;

    return openslide_get_level_count(m_Osr);
  }

  // Returns NULL for success
  // NOTE: When reading associated images, x, y, width and height are ignored.
  const char * ReadRegion(uint32_t *p_ui32Dest, int64_t i64X, int64_t i64Y, int64_t i64Width, int64_t i64Height) const {
    if (m_Osr == NULL)
      return "OpenSlideWrapper has no file open.";

    if (m_AssociatedImage.size() > 0) {
      openslide_read_associated_image(m_Osr, m_AssociatedImage.c_str(), p_ui32Dest);
    }
    else {
      const double dDownsampleFactor = openslide_get_level_downsample(m_Osr, m_Level);

      if (dDownsampleFactor <= 0.0)
        return "Could not get downsample factor.";

      // NOTE: API expects level 0 coordinates. So we upsample the coordinates.
      // XXX: This can subtly change the image compared to reading all at once.
      //      The handling of coordinates internally in OpenSlide is quite similar!
      i64X = (int64_t)(i64X * dDownsampleFactor);
      i64Y = (int64_t)(i64Y * dDownsampleFactor);

      openslide_read_region(m_Osr, p_ui32Dest, i64X, i64Y, m_Level, i64Width, i64Height);
    }

    return openslide_get_error(m_Osr);
  }

  // Computes the spacing depending on selected level
  // Default spacing is relative to 1 MPP if the function fails to detect spacing information (downsample factor is considered)
  bool GetSpacing(double &dSpacingX, double &dSpacingY) const {
    dSpacingX = dSpacingY = 1.0;

    if (m_Osr == NULL)
      return false;

    if (m_AssociatedImage.size() > 0)
      return false;

    const double dDownsample = openslide_get_level_downsample(m_Osr, m_Level);

    if (dDownsample <= 0.0)
      return false;

    if (!GetPropertyValue(OPENSLIDE_PROPERTY_NAME_MPP_X, dSpacingX) || !GetPropertyValue(OPENSLIDE_PROPERTY_NAME_MPP_Y, dSpacingY)) {
      dSpacingX = dSpacingY = dDownsample;
      return false;
    }

    dSpacingX *= dDownsample;
    dSpacingY *= dDownsample;

    return true;
  }

  // Returns the dimension of the level or associated image
  bool GetDimensions(int64_t &i64Width, int64_t &i64Height) const {
    i64Width = i64Height = 0;

    if (m_Osr == NULL)
      return false;

    if (m_AssociatedImage.size() > 0)
      openslide_get_associated_image_dimensions(m_Osr, m_AssociatedImage.c_str(), &i64Width, &i64Height);
    else
      openslide_get_level_dimensions(m_Osr, m_Level, &i64Width, &i64Height);

    return i64Width > 0 && i64Height > 0;
  }

  // Retrieves associated image names from the open slide and places them into a std::vector
  std::vector<std::string> GetAssociatedImageNames() const {
    if (m_Osr == NULL)
      return std::vector<std::string>();

    const char * const * p_cNames = openslide_get_associated_image_names(m_Osr);

    if (p_cNames == NULL)
      return std::vector<std::string>();
  
    std::vector<std::string> vNames;

    for (int i = 0; p_cNames[i] != NULL; ++i)
      vNames.push_back(p_cNames[i]);

    return vNames;
  }

  // Forms an ITK MetaDataDictionary
  MetaDataDictionary GetMetaDataDictionary() const {
    if (m_Osr == NULL)
      return MetaDataDictionary();

    MetaDataDictionary clTags;

    const char * const * p_cNames = openslide_get_property_names(m_Osr);

    if (p_cNames != NULL) {
      std::string strValue;

      for (int i = 0; p_cNames[i] != NULL; ++i) {
        strValue.clear();

        if (GetPropertyValue(p_cNames[i], strValue))
          EncapsulateMetaData<std::string>(clTags, p_cNames[i], strValue);
      }
    }

    return clTags;
  }

  // Templated functions for accessing and casting property values
  template<typename ValueType>
  bool GetPropertyValue(const char *p_cKey, ValueType &value) const {
    if (m_Osr == NULL)
      return false;

    const char * const p_cValue = openslide_get_property_value(m_Osr, p_cKey);
    if (p_cValue == NULL)
      return false;

    std::stringstream valueStream;

    valueStream << p_cValue;
    valueStream >> value;

    return !valueStream.fail() && !valueStream.bad();
  }

  bool GetPropertyValue(const char *p_cKey, std::string &strValue) const {
    if (m_Osr == NULL)
      return false;

    const char * const p_cValue = openslide_get_property_value(m_Osr, p_cKey);
    if (p_cValue == NULL)
      return false;

    strValue = p_cValue;
    return true;
  }

  // Compute the minimum streamable region size
  // Explanation:
  // OpenSlide expects level 0 coordinates in openslide_read_region(). This is OK if we're reading level 0 images.
  // However, if we're reading level L images with L > 0, this becomes problematic. This can be seen with a little math.
  // First, note the equation relating level 0 and level L coordinates:
  //
  // x_0 = D * x_L
  //
  // Here D is the downsample factor and is a real number (openslide_get_level_downsample() returns double).
  // From OpenSlide's source code, OpenSlide downsamples x_0 to x_L with a floor operation:
  //
  // x_L = floor(x_0 / D)
  //
  // Likewise, OpenSlide upsamples x_0 with a floor operation
  //
  // x_0 = floor(x_L * D)
  //
  // In this implementation, ITK works with coordinates at the selected level. When L > 0, it becomes challenging
  // to pick coordinates x_0 that identify x_L exactly. We derive x_0 by upsampling as above. But several values of x_0 will map to x_L.
  // We need to pick x_L so that it is invariant to an upsample and subsequent downsample. In math we want x_L so that:
  //
  // x_L = Downsample(Upsample(x_L)) = floor(floor(x_L * D) / D)
  //
  // D is known to be a rational number A/B since it is computed by dividing the dimensions of level 0 and level L images.
  // If A/B is a reduced fraction, we would like to determine x_L so that it is divisible by B. When B divides x_L we have the identity:
  //
  // Upsample(x_L) = D * x_L
  //
  // A subsequent downsample gives:
  //
  // Downsample(D * x_L) = x_L
  //
  // Which is what we wanted. Consequently, if dimensions are coprime, the image cannot technically be streamed.
  // In that case the ImageIORegion would reflect entire level L image. 
  // This wrapper also supports approximate streaming (ignoring this issue).
  bool ComputeMinimumStreamableRegionSize(int64_t &i64Width, int64_t &i64Height) const {
    i64Width = i64Height = 0;

    if (m_Osr == NULL)
      return false;

    if (m_Level == 0 || m_ApproximateStreaming) { // Nothing to do
      i64Width = i64Height = 1;
      return true;
    }

    int64_t i64WidthLevel0 = 0, i64HeightLevel0 = 0;
    int64_t i64WidthLevelL = 0, i64HeightLevelL = 0;

    openslide_get_level0_dimensions(m_Osr, &i64WidthLevel0, &i64HeightLevel0);
    openslide_get_level_dimensions(m_Osr, m_Level, &i64WidthLevelL, &i64HeightLevelL);

    if (i64WidthLevel0 <= 0 || i64HeightLevel0 <= 0 || i64WidthLevelL <= 0 || i64HeightLevelL <= 0)
      return false;

    i64Width = i64WidthLevelL / (int64_t)GCD(i64WidthLevel0, i64WidthLevelL);
    i64Height = i64HeightLevelL / (int64_t)GCD(i64HeightLevel0, i64HeightLevelL);

    return true;
  }

  // Compute absolute maximum number of streamable regions
  int64_t ComputeMaximumNumberOfStreamableRegions() const {
    int64_t i64RegionWidth = 0, i64RegionHeight = 0;

    if (!ComputeMinimumStreamableRegionSize(i64RegionWidth, i64RegionHeight))
      return -1;

    int64_t i64Width = 0, i64Height = 0;

    openslide_get_level_dimensions(m_Osr, m_Level, &i64Width, &i64Height);

    if (i64Width <= 0 || i64Height <= 0)
      return -1;

    // NOTE: The width and height are multiplies of region width and height
    return (i64Width / i64RegionWidth) * (i64Height / i64RegionHeight);
  }

  // After alignment, what's the maximum number of streamable regions of this size?
  int64_t ComputeMaximumNumberOfStreamableRegions(int64_t i64X, int64_t i64Y, int64_t i64Width, int64_t i64Height) const {
    if (m_Osr == NULL)
      return -1;

    int64_t i64ImageWidth = 0, i64ImageHeight = 0;

    openslide_get_level_dimensions(m_Osr, m_Level, &i64ImageWidth, &i64ImageHeight);

    if (i64ImageWidth <= 0 || i64ImageHeight <= 0)
      return -1;

    if (!AlignReadRegion(i64X, i64Y, i64Width, i64Height))
      return -1;

    return (i64ImageWidth + i64Width - 1)/i64Width * (i64ImageHeight + i64Height - 1)/i64Height;
  }

  // Align X, Y and region dimensions to be on the grid of points invariant to upsample/downsample
  bool AlignReadRegion(int64_t &i64X, int64_t &i64Y, int64_t &i64Width, int64_t &i64Height) const {
    if (m_Osr == NULL)
      return false;

    if (m_Level == 0 || m_ApproximateStreaming) // Nothing to do
      return true;

    int64_t i64MinWidth = 0, i64MinHeight = 0;

    if (!ComputeMinimumStreamableRegionSize(i64MinWidth, i64MinHeight))
      return false;

    int64_t i64XUpper = i64X + i64Width;
    int64_t i64YUpper = i64Y + i64Height;

    // Align X and Y to be on the grid with spacing MinWidth and MinHeight
    int64_t r = (i64X % i64MinWidth);
    i64X -= r;

    r = (i64Y % i64MinHeight);
    i64Y -= r;

    // Align the upper coordinates (remove remainder and add one full spacing)
    r = (i64XUpper % i64MinWidth);
    if (r != 0)
      i64XUpper += i64MinWidth - r;

    r = (i64YUpper % i64MinHeight);
    if (r != 0)
      i64YUpper += i64MinHeight - r;

    // Update width and height
    i64Width = i64XUpper - i64X;
    i64Height = i64YUpper - i64Y;

    return true;
  }

private:
  openslide_t *m_Osr;
  int32_t      m_Level;
  std::string  m_AssociatedImage;
  bool         m_ApproximateStreaming;
};

OpenSlideImageIO::OpenSlideImageIO()
{
  using PixelType = RGBAPixel<unsigned char>;
  PixelType clPixel;

  m_OpenSlideWrapper = NULL;
  m_OpenSlideWrapper = new OpenSlideWrapper();

  this->SetNumberOfDimensions(2); // OpenSlide is 2D.
  this->SetPixelTypeInfo(&clPixel);  

  m_Spacing[0] = 1.0;
  m_Spacing[1] = 1.0;

  m_Origin[0] = 0.0;
  m_Origin[1] = 0.0;

  m_Dimensions[0] = 0;
  m_Dimensions[1] = 0;

  // Trestle, Aperio, Ventana, and generic tiled tiff
  this->AddSupportedReadExtension(".tif");
  // Hamamatsu
  this->AddSupportedReadExtension(".vms");
  this->AddSupportedReadExtension(".vmu");
  this->AddSupportedReadExtension(".ndpi");
  // Aperio
  this->AddSupportedReadExtension(".svs");
  // MIRAX
  this->AddSupportedReadExtension(".mrxs");
  // Leica
  this->AddSupportedReadExtension(".scn");
  // Philips
  this->AddSupportedReadExtension(".tiff");
  // Ventana
  this->AddSupportedReadExtension(".bif");
  // Sakura
  this->AddSupportedReadExtension(".svslide");
}

OpenSlideImageIO::~OpenSlideImageIO()
{
  if (m_OpenSlideWrapper != NULL) {
    delete m_OpenSlideWrapper;
    m_OpenSlideWrapper = NULL;
  }
}

void OpenSlideImageIO::PrintSelf(std::ostream& os, Indent indent) const {
  Superclass::PrintSelf(os, indent);
  os << indent << "Level: " << GetLevel() << '\n';
  os << indent << "Associated Image: " << GetAssociatedImageName() << '\n';
}

bool OpenSlideImageIO::CanReadFile( const char* filename ) {
  std::string fname(filename);
  bool supportedExtension = false;
  ArrayOfExtensionsType::const_iterator extIt;

  for( extIt = this->GetSupportedReadExtensions().begin(); extIt != this->GetSupportedReadExtensions().end(); ++extIt) {
    if( fname.rfind( *extIt ) != std::string::npos ) {
      supportedExtension = true;
    }
  }
  if( !supportedExtension ) {
    return false;
  }

  return OpenSlideWrapper::CanReadFile(filename);
}

bool OpenSlideImageIO::CanStreamRead() {
  return m_OpenSlideWrapper != NULL && m_OpenSlideWrapper->CanStreamRead();
}

void OpenSlideImageIO::ReadImageInformation() {
  using PixelType = RGBAPixel<unsigned char>;
  PixelType clPixel;

  this->SetNumberOfDimensions(2);
  this->SetPixelTypeInfo(&clPixel);

  m_Dimensions[0] = 0;
  m_Dimensions[1] = 0;

  m_Spacing[0] = 1.0;
  m_Spacing[1] = 1.0;

  m_Origin[0] = 0.0;
  m_Origin[1] = 0.0;

  if (m_OpenSlideWrapper == NULL) {
    itkExceptionMacro( "Error OpenSlideImageIO could not open file: "
                       << this->GetFileName()
                       << std::endl
                       << "Reason: NULL OpenSlideWrapper pointer.");
  }

  if (!m_OpenSlideWrapper->Open(this->GetFileName())) {
    itkExceptionMacro( "Error OpenSlideImageIO could not open file: "
                       << this->GetFileName()
                       << std::endl
                       << "Reason: "
                       << itksys::SystemTools::GetLastSystemError() );
    // NOTE: OpenSlide needs to be opened to query API for errors. This is assumed to be related to a system error.
  }


  // This will fill in default values as needed (in case it fails)
  m_OpenSlideWrapper->GetSpacing(m_Spacing[0], m_Spacing[1]);

  {
    int64_t i64Width = 0, i64Height = 0;
    if (!m_OpenSlideWrapper->GetDimensions(i64Width, i64Height)) {
      std::string strError = "Unknown";
      const char * const p_cError = m_OpenSlideWrapper->GetError();

      if (p_cError != NULL) {
        strError = p_cError;
        m_OpenSlideWrapper->Close(); // Can only safely close this now
      }

      itkExceptionMacro( "Error OpenSlideImageIO could not read dimensions: "
                         << this->GetFileName()
                         << std::endl
                         << "Reason: " << strError );
    }

    // i64Width and i64Height are known to be positive
    if ((uint64_t)i64Width > std::numeric_limits<SizeValueType>::max() || (uint64_t)i64Height > std::numeric_limits<SizeValueType>::max()) {
      itkExceptionMacro( "Error OpenSlideImageIO image dimensions are too large for SizeValueType: "
                         << this->GetFileName()
                         << std::endl
                         << "Reason: " 
                         << i64Width << " > " << std::numeric_limits<SizeValueType>::max() 
                         << " or " << i64Height << " > " << std::numeric_limits<SizeValueType>::max() );
    }

    m_Dimensions[0] = (SizeValueType)i64Width;
    m_Dimensions[1] = (SizeValueType)i64Height;
  }

  this->SetMetaDataDictionary(m_OpenSlideWrapper->GetMetaDataDictionary());
}


void OpenSlideImageIO::Read( void * buffer)
{
  uint32_t * const p_u32Buffer = (uint32_t *)buffer;

  if (m_OpenSlideWrapper == NULL || !m_OpenSlideWrapper->IsOpened()) {
    itkExceptionMacro( "Error OpenSlideImageIO could not read region: "
                       << this->GetFileName()
                       << std::endl
                       << "Reason: OpenSlide context is not opened." );
  }

  const ImageIORegion clRegionToRead = this->GetIORegion();
  const ImageIORegion::SizeType clSize = clRegionToRead.GetSize();
  const ImageIORegion::IndexType clStart = clRegionToRead.GetIndex();

  if ( ((uint64_t)clSize[0])*((uint64_t)clSize[1]) > std::numeric_limits<ImageIORegion::SizeValueType>::max() ) {
    itkExceptionMacro( "Error OpenSlideImageIO could not read region: "
                       << this->GetFileName()
                       << std::endl
                       << "Reason: Requested region size in pixels overflows." );
  }

  const char *p_cError = m_OpenSlideWrapper->ReadRegion(p_u32Buffer, clStart[0], clStart[1], clSize[0], clSize[1]);

  if (p_cError != NULL) {
    std::string strError = p_cError; // Copy this since Close() may destroy the backing buffer
    m_OpenSlideWrapper->Close(); // Can only safely close this now
    itkExceptionMacro( "Error OpenSlideImageIO could not read region: "
                       << this->GetFileName()
                       << std::endl
                       << "Reason: " << strError );
  }

  // Re-order the bytes (ARGB -> RGBA)
  const int64_t i64TotalSize = clRegionToRead.GetNumberOfPixels();
  for (int64_t i = 0; i < i64TotalSize; ++i) {
    // XXX: Endianness?
    RGBAPixel<unsigned char> clPixel;
    clPixel.SetRed((p_u32Buffer[i] >> 16) & 0xff);
    clPixel.SetGreen((p_u32Buffer[i] >> 8) & 0xff);
    clPixel.SetBlue(p_u32Buffer[i] & 0xff);
    clPixel.SetAlpha((p_u32Buffer[i] >> 24) & 0xff);

    p_u32Buffer[i] = *reinterpret_cast<uint32_t *>(clPixel.GetDataPointer());
  }
}

bool OpenSlideImageIO::CanWriteFile( const char * /*name*/ )
{
  return false;
}

void
OpenSlideImageIO
::WriteImageInformation(void) {
  // add writing here
}


/**
 *
 */
void
OpenSlideImageIO::Write( const void* /*buffer*/) {
}

/** Given a requested region, determine what could be the region that we can
 * read from the file. This is called the streamable region, which will be
 * smaller than the LargestPossibleRegion and greater or equal to the
RequestedRegion */
ImageIORegion
OpenSlideImageIO::GenerateStreamableReadRegionFromRequestedRegion( const ImageIORegion & requested ) const {
  if (m_OpenSlideWrapper == NULL)
    return requested;

  ImageIORegion::SizeType clSize = requested.GetSize();
  ImageIORegion::IndexType clStart = requested.GetIndex();

  int64_t i64X = clStart[0];
  int64_t i64Y = clStart[1];

  int64_t i64Width = clSize[0];
  int64_t i64Height = clSize[1];

  if (!m_OpenSlideWrapper->AlignReadRegion(i64X, i64Y, i64Width, i64Height))
    return requested;

  clStart[0] = (SizeValueType)i64X;
  clStart[1] = (SizeValueType)i64Y;

  clSize[0] = (SizeValueType)i64Width;
  clSize[1] = (SizeValueType)i64Height;

  ImageIORegion clNewRegion(requested);

  clNewRegion.SetSize(clSize);
  clNewRegion.SetIndex(clStart);

  return clNewRegion;
}

/** Get underlying OpenSlide library version */
std::string OpenSlideImageIO::GetOpenSlideVersion() const {
  const char * const p_cVersion = OpenSlideWrapper::GetVersion();
  return p_cVersion != NULL ? std::string(p_cVersion) : std::string();
}

/** Detect the vendor of the current file. */
std::string OpenSlideImageIO::GetVendor() const {
  const char * const p_cVendor = OpenSlideWrapper::DetectVendor(this->GetFileName());
  return p_cVendor != NULL ? std::string(p_cVendor) : std::string();
}

/** Sets the level to read. Level 0 (default) is the highest resolution level.
 * This method overrides any previously selected associated image. 
 * Call ReadImageInformation() again after calling this function. */
void OpenSlideImageIO::SetLevel(int iLevel) {
  if (m_OpenSlideWrapper == NULL)
    return;

  m_OpenSlideWrapper->SetLevel(iLevel);
}

/** Returns the currently selected level. */
int OpenSlideImageIO::GetLevel() const {
  if (m_OpenSlideWrapper == NULL)
    return -1;

  return m_OpenSlideWrapper->GetLevel();
}

/** Returns the number of available levels. */
int OpenSlideImageIO::GetLevelCount() const {
  if (m_OpenSlideWrapper == NULL)
    return -1;

  return m_OpenSlideWrapper->GetLevelCount();
}

/** Sets the associated image to extract.
 * This method overrides any previously selected level.
 * Call ReadImageInformation() again after calling this function. */
void OpenSlideImageIO::SetAssociatedImageName(const std::string &strName) {
  if (m_OpenSlideWrapper == NULL)
    return;

  m_OpenSlideWrapper->SetAssociatedImageName(strName);
}

/** Returns the currently selected associated image name (empty string if none). */
std::string OpenSlideImageIO::GetAssociatedImageName() const {
  if (m_OpenSlideWrapper == NULL)
    return std::string();

  return m_OpenSlideWrapper->GetAssociatedImageName();
}

/** Sets the best level to read for the given downsample factor.
 * This method overrides any previously selected associated image. 
 * Call ReadImageInformation() again after calling this function. */
bool OpenSlideImageIO::SetLevelForDownsampleFactor(double dDownsampleFactor) {
  if (m_OpenSlideWrapper == NULL)
    return false;

  return m_OpenSlideWrapper->SetBestLevelForDownsample(dDownsampleFactor);
}

/** Returns all associated image names stored in the file. */
OpenSlideImageIO::AssociatedImageNameContainer OpenSlideImageIO::GetAssociatedImageNames() const {
  if (m_OpenSlideWrapper == NULL)
    return AssociatedImageNameContainer();

  return m_OpenSlideWrapper->GetAssociatedImageNames();
}

/** Returns the absolute maximum number of streamable regions (tiles). */
int64_t OpenSlideImageIO::ComputeMaximumNumberOfStreamableRegions() const {
  if (m_OpenSlideWrapper == NULL)
    return -1;

  return m_OpenSlideWrapper->ComputeMaximumNumberOfStreamableRegions();
}

/** Returns the maximum number of streamable regions similar (but >=) to the given region. */
int64_t OpenSlideImageIO::ComputeMaximumNumberOfStreamableRegions(const ImageIORegion &clRegion) const {
  if (m_OpenSlideWrapper == NULL)
    return -1;

  ImageIORegion::IndexType clStart = clRegion.GetIndex();
  ImageIORegion::SizeType clSize = clRegion.GetSize();

  if (clStart.size() != 2 || clSize.size() != 2)
    return -1;

  return m_OpenSlideWrapper->ComputeMaximumNumberOfStreamableRegions(clStart[0], clStart[1], clSize[0], clSize[1]);
}

/** Returns the minimum streamable region. */
ImageIORegion OpenSlideImageIO::GetMinimumStreamableRegion() const {
  if (m_OpenSlideWrapper == NULL)
    return ImageIORegion();

  ImageIORegion clRegion;
  ImageIORegion::SizeType clSize(2);
  ImageIORegion::IndexType clIndex(2, 0);

  int64_t i64Width = 0, i64Height = 0;
  if (!m_OpenSlideWrapper->ComputeMinimumStreamableRegionSize(i64Width, i64Height))
    return ImageIORegion();

  // XXX: Could overflow
  clSize[0] = (SizeValueType)i64Width;
  clSize[1] = (SizeValueType)i64Height;

  clRegion.SetIndex(clIndex);
  clRegion.SetSize(clSize);

  return clRegion;
}

/** Turn on/off approximate streaming. This only affects streaming level images other than level 0. */
void OpenSlideImageIO::SetApproximateStreaming(bool bApproximateStreaming) {
  if (m_OpenSlideWrapper != NULL)
    m_OpenSlideWrapper->SetApproximateStreaming(bApproximateStreaming);
}

/** Returns weather approximate streaming is enabled or not. */
bool OpenSlideImageIO::GetApproximateStreaming() const {
  return m_OpenSlideWrapper != NULL && m_OpenSlideWrapper->GetApproximateStreaming();
}

} // end namespace itk
