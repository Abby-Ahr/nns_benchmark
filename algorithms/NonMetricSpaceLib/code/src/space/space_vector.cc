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

#include "object.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"
#include "space/space_vector.h"

namespace similarity {

template <typename dist_t>
void VectorSpace<dist_t>::ReadVec(std::string line, LabelType& label, std::vector<dist_t>& v) const
{
  v.clear();

  label = Object::extractLabel(line);

  std::stringstream str(line);

  str.exceptions(std::ios::badbit);

  dist_t val;

  ReplaceSomePunct(line); 

  try {
    while (str >> val) {
      v.push_back(val);
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what();
    LOG(LIB_FATAL) << "Failed to parse the line: '" << line << "'";
  }
}

template <typename dist_t>
void VectorSpace<dist_t>::WriteDataset(const ObjectVector& dataset,
                                       const char* outputfile) const {
  std::ofstream outFile(outputfile, ostream::out | ostream::trunc);

  if (!outFile) {
    LOG(LIB_FATAL) << "Cannot open: '" << outputfile << "' for writing!";
  }

  outFile.exceptions(std::ios::badbit | std::ios::failbit);

  for (const Object* obj: dataset) {
    CHECK(obj->datalength() > 0);
    const dist_t* x = reinterpret_cast<const dist_t*>(obj->data());
    const size_t length = obj->datalength() / sizeof(dist_t);

    if (obj->label()>=0) outFile << LABEL_PREFIX << obj->label() << " ";

    for (size_t i = 0; i < length; ++i) {
      outFile << x[i];
      if (i + 1 == length) outFile << std::endl; else outFile << "  ";
    }
    
  }
  outFile.close();
}


template <typename dist_t>
void VectorSpace<dist_t>::ReadDataset(
    ObjectVector& dataset,
    const ExperimentConfig<dist_t>* config,
    const char* FileName,
    const int MaxNumObjects) const {

  dataset.clear();
  dataset.reserve(MaxNumObjects);

  std::vector<dist_t>    temp;

  std::ifstream InFile(FileName);

  if (!InFile) {
      LOG(LIB_FATAL) << "Cannot open file: " << FileName;
  }

  InFile.exceptions(std::ios::badbit);

  try {

    std::string StrLine;

    int linenum = 0;
    int id = linenum;
    LabelType label = -1;

    int dim = 0;
    int actualDim = 0;

    while (getline(InFile, StrLine) && (!MaxNumObjects || linenum < MaxNumObjects)) {
      ReadVec(StrLine, label, temp);
      int currDim = static_cast<int>(temp.size());
      if (!dim) dim = currDim;
      else {
        if (dim != currDim) {
            LOG(LIB_FATAL) << "The # of vector elements (" << currDim << ")" <<
                      " doesn't match the # of elements in previous lines. (" << dim << " )" <<
                      "Found mismatch in line: " << (linenum + 1) << " file: " << FileName;
        }
      }

      actualDim = dim;

      if (config && config->GetDimension()) {
        if (config->GetDimension() > currDim) {
          LOG(LIB_FATAL) << "The # of vector elements (" << currDim << ")" <<
                      " is smaller than the requested # of dimensions. " <<
                      "Found mismatch in line: " << (linenum + 1) << " file: " << FileName;
        } else {
          actualDim = config->GetDimension();
        }
      }
      temp.resize(actualDim);
      id = linenum;
      ++linenum;
      dataset.push_back(CreateObjFromVect(id, label, temp));
    }
    LOG(LIB_INFO) << "Actual dimensionality: " << actualDim;
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what();
    LOG(LIB_FATAL) << "Failed to read/parse the file: '" << FileName << "'";
  }
}

template <typename dist_t>
Object* VectorSpace<dist_t>::CreateObjFromVect(IdType id, LabelType label, const std::vector<dist_t>& InpVect) const {
  return new Object(id, label, InpVect.size() * sizeof(dist_t), &InpVect[0]);
};

/* 
 * Note that we don't instantiate vector spaces for types other than float & double
 * The only exception is the VectorSpace<PivotIdType>
 */
template class VectorSpace<PivotIdType>;
template class VectorSpace<float>;
template class VectorSpace<double>;
}  // namespace similarity
