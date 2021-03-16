/*===================================================================

BSD 3-Clause License

Copyright (c) 2017, Guillaume Lemaitre
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===================================================================*/

#include <functional>
#include <itkOpenSlideImageIO.h>
#include <m2FSMImageIO.h>
#include <m2FsmIRSpecImage.h>
#include <map>
#include <mitkImageCast.h>

namespace m2
{
  FSMImageIO::FSMImageIO() : AbstractFileIO(mitk::Image::GetStaticNameOfClass(), FSM_MIMETYPE(), "FSM Image")
  {
    AbstractFileWriter::SetRanking(10);
    AbstractFileReader::SetRanking(10);
    this->RegisterService();
  }

  mitk::IFileIO::ConfidenceLevel FSMImageIO::GetWriterConfidenceLevel() const
  {
    if (AbstractFileIO::GetWriterConfidenceLevel() == Unsupported)
      return Unsupported;
    const auto *input = static_cast<const m2::ImzMLSpectrumImage *>(this->GetInput());
    if (input)
      return Supported;
    else
      return Unsupported;
  }

  void FSMImageIO::Write() { ValidateOutputLocation(); }

  mitk::IFileIO::ConfidenceLevel FSMImageIO::GetReaderConfidenceLevel() const
  {
    if (AbstractFileIO::GetReaderConfidenceLevel() == Unsupported)
      return Unsupported;
    return Supported;
  }

  std::vector<mitk::BaseData::Pointer> FSMImageIO::DoRead()
  {
    std::ifstream inFile(this->GetInputLocation(), std::ios_base::binary);

    unsigned long long n_bytes = 0, start_byte = 0;
    std::vector<char> s;

    const auto read = [&inFile](const auto &s, const auto &n, auto &v) {
      v.resize(n);
      inFile.seekg(s);
      inFile.read(v.data(), n);
      return n;
    };
    n_bytes = 4;
    start_byte += read(start_byte, n_bytes, s);
    MITK_INFO << std::string(s.data(), n_bytes);

    n_bytes = 40;
    start_byte += read(start_byte, n_bytes, s);
    MITK_INFO << std::string(s.data(), n_bytes);
    const auto Print = [](const auto &names, const auto &data) {
      for (int i = 0; i < names.size(); ++i)
        MITK_INFO << names[i] << " " << data[i];
    };
    std::array<double, 10> meta;
    std::array<std::string, 10> meta_names = {
      "x_delta", "y_delta", "z_delta", "z_start", "z_end", "z_4d_start", "z_4d_end", "x_init", "y_init", "z_init"};

    std::array<int32_t, 3> dimensions;
    std::array<std::string, 3> dimension_names = {"n_x", "n_y", "n_z"};
    std::array<int16_t, 8> residuals;
    std::array<std::string, 8> residual_names = {
      "", "text1", "", "text2", "resolution", "text3", "transmission", "text4"};

    std::array<std::string, 19> information;
    std::array<std::string, 19> information_names{"analyst",
                                                  "date",
                                                  "image_name",
                                                  "instrument_model",
                                                  "instrument_serial_number",
                                                  "instrument_software_version",
                                                  "accumulations",
                                                  "detector",
                                                  "source",
                                                  "beam_splitter",
                                                  "apodization",
                                                  "spectrum_type",
                                                  "beam_type",
                                                  "phase_correction",
                                                  "ir_accessory",
                                                  "igram_type",
                                                  "scan_direction",
                                                  "background_scans",
                                                  "ir_laser_wave_number_unit"};

    std::vector<std::vector<float>> spectraDataVectors;

    auto fsize = inFile.tellg();
    inFile.seekg(0, std::ios::end);
    fsize = inFile.tellg() - fsize;

    while (!inFile.eof())
    {
      // read block info
      n_bytes = 6;
      start_byte += read(start_byte, n_bytes, s);
      uint16_t block_id = *((uint16_t *)s.data());
      int32_t block_size = *((int32_t *)(s.data() + sizeof(uint16_t)));
      n_bytes = block_size;

      if (!inFile)
        break;

      start_byte += read(start_byte, n_bytes, s);
      std::vector<float> data;
      bool eof = inFile.eof();

      switch (block_id)
      {
        case 5100:
        {
          const char *d = s.data();
          int16_t name_size = *((short *)(d));
          d += 2;
          std::string name;
          name.resize(name_size + 2);
          memcpy((char *)(name.data()), d, name_size);
          d += name_size;

          memcpy(meta.data(), d, sizeof(double) * 10);
          d += sizeof(double) * 10;

          memcpy(dimensions.data(), d, sizeof(int32_t) * 3);
          d += sizeof(int32_t) * 3;

          for (unsigned char i = 0; i < 4; ++i)
          {
            residuals[i * 2] = (*(int16_t *)(d));
            d += 2;
            residuals[i * 2 + 1] = (*(uint8_t *)(d));
            d += 1;
          }

          Print(meta_names, meta);
          Print(dimension_names, dimensions);
          Print(residual_names, residuals);
        }
        break;
        case 5104:
        {
          unsigned int pos = 0;
          std::vector<std::string> text;
          std::vector<int> text_mapping = {0, 2, 4, 5, 6, 7, 9, 11, 12, 13, 15, 16, 17, 20, 26, 28, 29, 32, 67};
          while (pos + 2 < s.size())
          {
            auto tag = std::string(s.data() + pos, 2);
            if (tag.compare("#u") == 0)
            {
              pos += 2;
              auto text_size = (*(int16_t *)(s.data() + pos));
              pos += 2;
              text.push_back(std::string(s.data() + pos, text_size));
              pos += text_size;
              pos += 6;
            }
            else if (tag.compare("$u") == 0)
            {
              pos += 2;
              text.push_back(std::to_string(*(int16_t *)(s.data() + pos)));
              pos += 2;
              pos += 6;
            }
            else if (tag.compare(",u") == 0)
            {
              pos += 2;
              text.push_back(std::to_string(*(int16_t *)(s.data() + pos)));
              pos += 2;
            }
            else
            {
              pos += 1;
            }
          }

          // copy information
          for (int i = 0; i < text_mapping.size(); ++i)
            information[i] = text[text_mapping[i]];

          Print(information_names, information);
        }
        break;
        case 5105:
          data.resize(s.size() / sizeof(float));
          memcpy(data.data(), s.data(), s.size());
          spectraDataVectors.push_back(data);
          break;
        default:
          break;
      }
    }

    auto fsmImage = m2::FsmIRSpecImage::New();
    fsmImage->SetPropertyValue<unsigned>("dim_x", dimensions[0]); // n_x
    fsmImage->SetPropertyValue<unsigned>("dim_y", dimensions[1]); // n_z
    fsmImage->SetPropertyValue<unsigned>("dim_z", 1);

    fsmImage->SetPropertyValue<double>("origin x", meta[7]); // x_init
    fsmImage->SetPropertyValue<double>("origin y", meta[8]);
    fsmImage->SetPropertyValue<double>("origin z", 0);

    fsmImage->SetPropertyValue<double>("spacing_x", meta[0] * 1e-3); // x_delta
    fsmImage->SetPropertyValue<double>("spacing_y", meta[1] * 1e-3); // y_delta
    fsmImage->SetPropertyValue<double>("spacing_z", 0.01);

    fsmImage->SetPropertyValue<double>("wavelength_delta", meta[2]); // z_delta
    fsmImage->SetPropertyValue<double>("wavelength_start", meta[3]); // z_start
    fsmImage->SetPropertyValue<double>("wavelength_end", meta[4]);   // z_end

    auto start = meta[3];
    auto end = meta[4];

    bool inverse = start > end;

    if (inverse)
      std::swap(start, end);

    auto delta = std::abs(meta[2]);

    auto &wavelengths = fsmImage->GetXAxis();
    for (; start <= end; start += delta)
    {
      wavelengths.push_back(start);
    }

    fsmImage->InitializeGeometry();


    itk::Image<m2::DisplayImagePixelType, 3>::Pointer ionImage;
    mitk::CastToItkImage(fsmImage, ionImage);

    auto sit = spectraDataVectors.begin();
    itk::ImageRegionIteratorWithIndex<itk::Image<m2::DisplayImagePixelType, 3>> it(
      ionImage, ionImage->GetLargestPossibleRegion());
    auto &source_list = fsmImage->GetSpectrumImageSourceList();
    m2::FsmIRSpecImage::Source source;

    unsigned long i = 0;
    while (!it.IsAtEnd())
    {
      m2::FsmIRSpecImage::SpectrumData spectrum;

      if (inverse)
      {
        spectrum.data;
        std::copy(std::rbegin(*sit), std::rend(*sit), std::back_inserter(spectrum.data));
      }
      else
      {
        spectrum.data = std::move(*sit);
      }

      spectrum.id = i;
      spectrum.index = it.GetIndex();
      source._Spectra.push_back(spectrum);

      ++i;
      ++sit;
      ++it;
    }
    source_list.emplace_back(source);
    fsmImage->InitializeImageAccess();
    return {fsmImage.GetPointer()};
  } // namespace m2

  FSMImageIO *FSMImageIO::IOClone() const { return new FSMImageIO(*this); }
} // namespace m2
