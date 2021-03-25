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

#pragma once

#include <M2aiaCoreIOExports.h>
#include <m2ImzMLSpectrumImage.h>
#include <mitkAbstractFileIO.h>
#include <mitkIOMimeTypes.h>
#include <mitkImage.h>
#include <mitkItkImageIO.h>

namespace m2
{
  class M2AIACOREIO_EXPORT FSMImageIO : public mitk::AbstractFileIO
  {
  public:
    FSMImageIO();

    std::string FSM_MIMETYPE_NAME()
    {
      static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + ".image.fsm";
      return name;
    }

    mitk::CustomMimeType FSM_MIMETYPE()
    {
      mitk::CustomMimeType mimeType(FSM_MIMETYPE_NAME());
	  mimeType.AddExtension("fsm");    
	  mimeType.SetCategory("Images");
      mimeType.SetComment("FSM");
      return mimeType;
    }

    std::vector<mitk::BaseData::Pointer> DoRead() override;
    ConfidenceLevel GetReaderConfidenceLevel() const override;
    void Write() override;

    ConfidenceLevel GetWriterConfidenceLevel() const override;
    void LoadAssociatedData(mitk::BaseData *object);

  private:
    FSMImageIO *IOClone() const override;
  };
} // namespace m2
