/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <bitset>

#include "space/space_bit_hamming.h"
#include "permutation_utils.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

int SpaceBitHamming::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj1->datalength() == obj2->datalength());
  const uint32_t* x = reinterpret_cast<const uint32_t*>(obj1->data());
  const uint32_t* y = reinterpret_cast<const uint32_t*>(obj2->data());
  const size_t length = obj1->datalength() / sizeof(uint32_t);

  return BitHamming(x, y, length);
}

void SpaceBitHamming::ReadVec(std::string line, LabelType& label, std::vector<uint32_t>& binVect) const
{
  binVect.clear();

  label = Object::extractLabel(line);

  std::stringstream str(line);

  str.exceptions(std::ios::badbit);

  unsigned val;

  ReplaceSomePunct(line); 

  vector<PivotIdType> v;

  try {
    while (str >> val) {
      if (val != 0 && val != 1) {
        LOG(LIB_FATAL) << "Only zeros and ones are allowed, line: '" << line << "'";
      }
      v.push_back(val);
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what() << std::endl;
    LOG(LIB_FATAL) << "Failed to parse the line: '" << line << "'" << std::endl;
  }
  Binarize(v, 1, binVect);
/*
  for (int i = 0; i < binVect.size(); ++i)
    cout << bitset<32>(binVect[i]);
  cout << endl;
*/
}


void SpaceBitHamming::ReadDataset(
    ObjectVector& dataset,
    const ExperimentConfig<int>* config,
    const char* FileName,
    const int MaxNumObjects) const {

  dataset.clear();
  dataset.reserve(MaxNumObjects);

  std::vector<uint32_t>    temp;

  std::ifstream InFile(FileName);
  InFile.exceptions(std::ios::badbit);

  try {

    std::string StrLine;

    int linenum = 0;
    int id = linenum;
    LabelType label = -1;

    int wordQty = 0;

    while (getline(InFile, StrLine) && (!MaxNumObjects || linenum < MaxNumObjects)) {
      ReadVec(StrLine, label, temp);
      int currWordQty = static_cast<int>(temp.size());
      if (!wordQty) wordQty = currWordQty;
      else {
        if (wordQty != currWordQty) {
            LOG(LIB_FATAL) << "The # of vector elements (" << currWordQty << ")" <<
                      " doesn't match the # of elements in previous lines. (" << wordQty << " )" <<
                      "Found mismatch in line: " << (linenum + 1) << " file: " << FileName;
        }
      }

      id = linenum;
      ++linenum;
      dataset.push_back(CreateObjFromVect(id, label, temp));
    }
    LOG(LIB_INFO) << "Number of words per vector : " << wordQty;
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what() << std::endl;
    LOG(LIB_FATAL) << "Failed to read/parse the file: '" << FileName << "'" << std::endl;
  }
}

Object* SpaceBitHamming::CreateObjFromVect(IdType id, LabelType label, const std::vector<uint32_t>& InpVect) const {
  return new Object(id, label, InpVect.size() * sizeof(uint32_t), &InpVect[0]);
};

}  // namespace similarity
