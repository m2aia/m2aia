/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <m2IntervalVectorIO.h>
#include <mitkIOUtil.h>
#include <mitkLocaleSwitch.h>
#include <itksys/SystemTools.hxx>

namespace m2
{
  IntervalVectorIO::IntervalVectorIO() : AbstractFileIO(m2::IntervalVector::GetStaticNameOfClass(), INTERVALVECTOR_MIMETYPE(), "Centroid Data (M²aia)")
  {
    AbstractFileWriter::SetRanking(10);
    AbstractFileReader::SetRanking(10);
    this->RegisterService();
  }

  mitk::IFileIO::ConfidenceLevel IntervalVectorIO::GetWriterConfidenceLevel() const
  {
    MITK_INFO << this->GetInput() <<  dynamic_cast<const m2::IntervalVector *>(this->GetInput());
    const auto *input = dynamic_cast<const m2::IntervalVector *>(this->GetInput());
    if (input)
      return Supported;
    else
      return Unsupported;
  }

  void IntervalVectorIO::Write()
  {
    mitk::LocaleSwitch localeSwitch("C");
    ValidateOutputLocation();

    const auto *input = static_cast<const m2::IntervalVector *>(this->GetInput());
    std::ofstream file(GetOutputLocation());
    
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << GetOutputLocation() << std::endl;
        return;
    }

    file << "center,max,min,mean\n";
    for(const m2::Interval & i: input->GetIntervals()){
      file << i.x.mean() << "," << i.y.max() << "," << i.y.min() << "," << i.y.mean() << "\n";
    }

    file.close();
  } // namespace mitk

  mitk::IFileIO::ConfidenceLevel IntervalVectorIO::GetReaderConfidenceLevel() const
  {
    if (AbstractFileIO::GetReaderConfidenceLevel() == Unsupported)
      return Unsupported;
    return Supported;
  }

  std::vector<mitk::BaseData::Pointer> IntervalVectorIO::DoRead()
  {  
    auto path = this->GetInputLocation();

    std::ifstream file(path);
    std::vector<std::vector<std::string>> columns;

    if (!file.is_open()) {
        std::cout << "Failed to open file: " << path << std::endl;
        return {};
    }

    std::string line;
    std::vector<std::string> header;

    if(std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            header.push_back(cell);
        }
    }

    columns.resize(header.size());

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        int i = 0;
        while (std::getline(ss, cell, ',')) {
            columns[i].push_back(cell);
            ++i;
        }
    }

    file.close();
    
    auto Strip = [](const std::string & str){
      size_t start = 0;
      size_t end = str.length();

      // Find the index of the first non-whitespace character
      while (start < end && std::isspace(str[start])) {
          ++start;
      }

      // Find the index of the last non-whitespace character
      while (end > start && std::isspace(str[end - 1])) {
          --end;
      }

      // Return the stripped substring
      return str.substr(start, end - start);
      
      };

    

    
    
    auto targetData = m2::IntervalVector::New();
    targetData->SetType(m2::SpectrumFormat::Centroid);
    targetData->SetInfo("Load from file: " + path);
    std::transform(std::begin(columns[0]), std::end(columns[0]), std::begin(columns[1]), std::back_inserter(targetData->GetIntervals()), [Strip](std::string xStr, std::string yStr){
      xStr = Strip(xStr);
      yStr = Strip(yStr);
      auto x = std::stod(xStr);
      auto y = std::stod(yStr);
      m2::Interval I(x,y);
      return I;
    });

    // check for labels. If labels exist split into multiple lists
    auto labelIt = std::find(std::begin(header), std::end(header), "label");
    if(labelIt != std::end(header)){
      auto colIdx = std::distance(std::begin(header), labelIt);
      
      auto uniqueLabels = columns[colIdx];
      auto eraseIt = std::unique(std::begin(uniqueLabels), std::end(uniqueLabels));
      uniqueLabels.erase(eraseIt, std::end(uniqueLabels));
      
      std::map<std::string, m2::IntervalVector::Pointer> labeldResults;
      auto intervalIt = std::begin(targetData->GetIntervals());
        
      for(std::string l : columns[colIdx]){
        if(labeldResults[l].IsNull()){
          labeldResults[l] = m2::IntervalVector::New();
        }
        labeldResults[l]->GetIntervals().push_back(*intervalIt);
        ++intervalIt;
      }

      std::vector<mitk::BaseData::Pointer> res;
      for(auto kv : labeldResults)
        res.push_back(kv.second);
      return res;
    }

    

    return {targetData};
  }


  IntervalVectorIO *IntervalVectorIO::IOClone() const { return new IntervalVectorIO(*this); }
} // namespace m2
