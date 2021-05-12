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
#pragma once

#include "itkImageIOBase.h"
#include "M2aiaOpenSlideIOExports.h"

namespace itk
{

	// Forward declare a wrapper class that is responsible for openslide_t (among other things)
	class OpenSlideWrapper;

	/** \class OpenSlideImageIO
	 *
	 * \brief OpenSlide is a C library that provides a simple interface to read whole-slide
	 * images (also known as virtual slides). The following formats can be read:
	 *
	 * - Trestle (.tif),
	 * - Hamamatsu (.vms, .vmu, .ndpi)
	 * - Aperio (.svs, .tif)
	 * - MIRAX (.mrxs)
	 * - Leica (.scn)
	 * - Sakura (.svslide)
	 * - Ventana (.bif, .tif)
	 * - Philips (.tiff)
	 * - Generic tiled TIFF (.tif)
	 *
	 *  \warning Streaming level images other than level 0 may not give pixel-by-pixel
	 *  identical images as reading the image in all at once.
	 *
	 *  \ingroup IOFilters
	 *
	 *  \ingroup IOOpenSlide
	 */
	class M2AIAOPENSLIDEIO_EXPORT OpenSlideImageIO : public ImageIOBase
	{
	public:


		/** Standard class type alias. */
		using Self = OpenSlideImageIO;
		using Superclass = ImageIOBase;
		using Pointer = SmartPointer<Self>;
		using AssociatedImageNameContainer = std::vector<std::string>;

		/** Method for creation through the object factory. */
		itkFactorylessNewMacro(Self);

		/** Run-time type information (and related methods). */
		itkTypeMacro(OpenSlideImageIO, ImageIOBase);

		using ArrayOfExtensionsType = Superclass::ArrayOfExtensionsType;

		/*-------- This part of the interfaces deals with reading data. ----- */

		 /** Determine the file type. Returns true if this ImageIO can read the
		  * file specified. */
		virtual bool CanReadFile(const char*);

		/** Determine if the ImageIO can stream reading from the
			current settings. Default is false. If this is queried after
			the header of the file has been read then it will indicate if
			that file can be streamed */
		virtual bool CanStreamRead();

		/** Set the spacing and dimension information for the set filename. */
		virtual void ReadImageInformation();

		/** Reads the data from disk into the memory buffer provided. */
		virtual void Read(void* buffer);

		/*-------- This part of the interfaces deals with writing data. ----- */

		/** Determine the file type. Returns true if this ImageIO can write the
		 * file specified. */
		virtual bool CanWriteFile(const char*);

		/** Set the spacing and dimension information for the set filename. */
		virtual void WriteImageInformation();

		/** Writes the data to disk from the memory buffer provided. Make sure
		 * that the IORegions has been set properly. */
		virtual void Write(const void* buffer);

		/** Method for supporting streaming.  Given a requested region, determine what
		 * could be the region that we can read from the file. This is called the
		 * streamable region, which will be smaller than the LargestPossibleRegion and
		 * greater or equal to the RequestedRegion */
		virtual ImageIORegion
			GenerateStreamableReadRegionFromRequestedRegion(const ImageIORegion & requested) const;

		/** Get underlying OpenSlide library version */
		virtual std::string GetOpenSlideVersion() const;

		/** Detect the vendor of the current file. */
		virtual std::string GetVendor() const;

		/** Sets the level to read. Level 0 (default) is the highest resolution level.
		 * This method overrides any previously selected associated image.
		 * Call ReadImageInformation() again after calling this function. */
		virtual void SetLevel(int iLevel);

		/** Returns the currently selected level. */
		virtual int GetLevel() const;

		/** Returns the number of available levels. */
		virtual int GetLevelCount() const;

		/** Sets the associated image to extract.
		 * This method overrides any previously selected level.
		 * Call ReadImageInformation() again after calling this function. */
		virtual void SetAssociatedImageName(const std::string &strName);

		/** Returns the currently selected associated image name (empty string if none). */
		virtual std::string GetAssociatedImageName() const;

		/** Sets the best level to read for the given downsample factor.
		 * This method overrides any previously selected associated image.
		 * Call ReadImageInformation() again after calling this function. */
		virtual bool SetLevelForDownsampleFactor(double dDownsampleFactor);

		/** Returns all associated image names stored in the file. */
		virtual AssociatedImageNameContainer GetAssociatedImageNames() const;

		/** Returns the absolute maximum number of streamable regions (tiles). */
		virtual int64_t ComputeMaximumNumberOfStreamableRegions() const;

		/** Returns the maximum number of streamable regions similar (but >=) to the given region. */
		virtual int64_t ComputeMaximumNumberOfStreamableRegions(const ImageIORegion &clRegion) const;

		/** Returns the minimum streamable region. */
		virtual ImageIORegion GetMinimumStreamableRegion() const;

		/** Turn on/off approximate streaming. This only affects streaming level images other than level 0.
		  * This gives the ImageIO more flexibility with the read regions allowing for more streaming divisons
		  * at the cost of image accuracy. This flexibility is already available for level 0 whether this is
		  * enabled or not (and does not affect accuracy).
		  * Use this if streaming results in excessive memory usage when streaming level images other than
		  * level 0.
		  */
		virtual void SetApproximateStreaming(bool bApproximateStreaming);

		/** Returns whether approximate streaming is enabled or not. */
		virtual bool GetApproximateStreaming() const;

	protected:
		OpenSlideImageIO();
		~OpenSlideImageIO();
		virtual void PrintSelf(std::ostream& os, Indent indent) const;

	private:
		ITK_DISALLOW_COPY_AND_ASSIGN(OpenSlideImageIO);
		OpenSlideWrapper *m_OpenSlideWrapper; // Opaque pointer to a wrapper that manages openslide_t
	};

} // end namespace itk


