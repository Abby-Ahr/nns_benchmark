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
#ifndef _MULT_INDEX_H_
#define _MULT_INDEX_H_

#include <vector>

#include "index.h"
#include "params.h"
#include "methodfactory.h"

#define METH_MULT_INDEX     "mult_index"

namespace similarity {

/*
 * This is a generic method that can create multiple copies of any method indices.
 * During search, results obtained from all the copies are aggregated.
 * This allows us to achieve higher recall when an approximate search method is used.
 * In other words, using multiple copies of the index created using the same method 
 * permits to trade efficiency for effectiveness. However, this makes sense only for the
 * methods where creation of the index is NON-deterministic, e.g., when
 * pivots are created randomly.
 * 
 */

template <typename dist_t> class Space;

template <typename dist_t>
class MultiIndex : public Index<dist_t> {
 public:
  MultiIndex(const string& SpaceType,
             const Space<dist_t>* space, 
             const ObjectVector& data,
             const AnyParams& params);
  ~MultiIndex();

  const std::string ToString() const;

  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  virtual vector<string> GetQueryTimeParamNames() const;
 protected:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& pmgr);

  std::vector<Index<dist_t>*> indices_;
  const Space<dist_t>*        space_;
  size_t                      IndexQty_;
  string                      MethodName_;
};

}   // namespace similarity

#endif
