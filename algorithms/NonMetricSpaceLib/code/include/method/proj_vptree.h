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

#ifndef _PROJ_VPTREE_H_
#define _PROJ_VPTREE_H_

#include <vector>

#include "index.h"
#include "space/space_lp.h"
#include "space/space_vector.h"
#include "method/vptree.h"
#include "params.h"
#include "projection.h"
#include "searchoracle.h"

#define METH_PROJ_VPTREE     "proj_vptree"

namespace similarity {

/*
 * The VP-tree build over projections.
 */
template <typename dist_t>
class ProjectionVPTree : public Index<dist_t> {
 public:
  ProjectionVPTree(const Space<dist_t>* space,
                   const ObjectVector& data,
                   const AnyParams& MethPars);

  ~ProjectionVPTree();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  vector<string> GetQueryTimeParamNames() const { 
    vector<string> res = VPTreeIndex_->GetQueryTimeParamNames();
    res.push_back("dbScanFrac");
    res.push_back("knnAmp");
    return res;
  }
 private:
  void SetQueryTimeParamsInternal(AnyParamManager& pmgr);


  const Space<dist_t>*              space_;
  const ObjectVector&               data_;

  size_t                            K_;
  size_t                            knn_amp_;
  float					                    db_scan_frac_;

  size_t computeDbScan(size_t K) {
    if (knn_amp_) { return min(K * knn_amp_, data_.size()); }
    return static_cast<size_t>(db_scan_frac_ * data_.size());
  }

  unique_ptr<Projection<dist_t> >   projObj_;
  ObjectVector                      projData_;
  size_t                            projDim_;

  Object*                           ProjectOneVect(size_t targSpaceId,
                                                   const Query<dist_t>* pQuery,
                                                   const Object* pSrcObj) const;

  VPTree<float, PolynomialPruner<float>>*             VPTreeIndex_;
  unique_ptr<const VectorSpaceSimpleStorage<float>>   VPTreeSpace_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(ProjectionVPTree);
};

}  // namespace similarity

#endif     // _PROJ_VPTREE_H_

