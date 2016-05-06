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

#include <algorithm>
#include <sstream>
#include <memory>

#include "space.h"
#include "space/space_bit_hamming.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "method/perm_bin_vptree.h"
#include "utils.h"
#include "distcomp.h"

namespace similarity {

using std::unique_ptr;

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
PermBinVPTree<dist_t, RankCorrelDistFunc>::PermBinVPTree(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const AnyParams& AllParams) : 
      space_(space), data_(data),   // reference
      VPTreeSpace_(new SpaceBitHamming())
{
  AnyParamManager pmgr(AllParams);

  double        DbScanFrac   = 0.05;
  size_t        NumPivot     = 16;
  bin_threshold_ = 8;

  pmgr.GetParamOptional("dbScanFrac", DbScanFrac);
  pmgr.GetParamOptional("numPivot", NumPivot);
  pmgr.GetParamOptional("binThreshold", bin_threshold_);

  bin_perm_word_qty_ = (NumPivot + 31)/32;

  if (DbScanFrac < 0.0 || DbScanFrac > 1.0) {
    LOG(LIB_FATAL) << METH_PERM_BIN_VPTREE << " requires that dbScanFrac is in the range [0,1]";
  }

  AnyParams RemainParams;

  LOG(LIB_INFO) << "# pivots                  = " << NumPivot;
  LOG(LIB_INFO) << "# binarization threshold = "  << bin_threshold_;
  LOG(LIB_INFO) << "# binary entry size (words) = "  << bin_perm_word_qty_;
  LOG(LIB_INFO) << "db scan fraction = " << DbScanFrac;

  double AlphaLeft = 1.0, AlphaRight = 1.0;

  pmgr.GetParamOptional(ALPHA_LEFT_PARAM,  AlphaLeft);
  pmgr.GetParamOptional(ALPHA_RIGHT_PARAM, AlphaRight);

  RemainParams = pmgr.ExtractParametersExcept(
                        { "dbScanFrac",
                         "numPivot",
                         "binThreshold",

                         ALPHA_LEFT_PARAM, 
                         ALPHA_RIGHT_PARAM,
                        });

  // db_can_qty_ should always be > 0
  db_scan_qty_ = max(size_t(1), static_cast<size_t>(DbScanFrac * data.size())),
  GetPermutationPivot(data, space, NumPivot, &pivots_);
  BinPermData_.resize(data.size());

  for (size_t i = 0; i < data.size(); ++i) {
    Permutation TmpPerm;
    GetPermutation(pivots_, space_, data[i], &TmpPerm);
    vector<uint32_t>  binPivot;
    Binarize(TmpPerm, bin_threshold_, binPivot);
    CHECK(binPivot.size() == bin_perm_word_qty_);
    BinPermData_[i] = VPTreeSpace_->CreateObjFromVect(i, -1, binPivot);
  }

  ReportIntrinsicDimensionality("Set of permutations" , *VPTreeSpace_, BinPermData_);
  VPTreeIndex_ = new VPTree<int, PolynomialPruner<int>>(
                                          true,
                                          VPTreeSpace_,
                                          BinPermData_,
                                          RemainParams
                                    );
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
PermBinVPTree<dist_t, RankCorrelDistFunc>::~PermBinVPTree() {
  for (size_t i = 0; i < data_.size(); ++i) {
    delete BinPermData_[i];
  }
  delete VPTreeIndex_;
  delete VPTreeSpace_;
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
const std::string PermBinVPTree<dist_t, RankCorrelDistFunc>::ToString() const {
  std::stringstream str;
  str <<  "binarized permutation (vptree)";
  return str.str();
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermBinVPTree<dist_t, RankCorrelDistFunc>::Search(RangeQuery<dist_t>* query) {
  Permutation perm_q;
  GetPermutation(pivots_, query, &perm_q);

  vector<uint32_t>  binPivot;
  Binarize(perm_q, bin_threshold_, binPivot);
  CHECK(binPivot.size() == bin_perm_word_qty_);

  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, -1, binPivot));

  unique_ptr<KNNQuery<int>> VPTreeQuery(new KNNQuery<int>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get());

  unique_ptr<KNNQueue<int>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(data_[id]);
      ResQueue->Pop();
  }
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermBinVPTree<dist_t, RankCorrelDistFunc>::Search(KNNQuery<dist_t>* query) {
  Permutation perm_q;
  GetPermutation(pivots_, query, &perm_q);

  vector<uint32_t>  binPivot;
  Binarize(perm_q, bin_threshold_, binPivot);
  CHECK(binPivot.size() == bin_perm_word_qty_);

  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, -1, binPivot));

  unique_ptr<KNNQuery<int>> VPTreeQuery(new KNNQuery<int>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get());

  unique_ptr<KNNQueue<int>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(data_[id]);
      ResQueue->Pop();
  }
}

template class PermBinVPTree<float, SpearmanRho>;
template class PermBinVPTree<float, SpearmanRhoSIMD>;
template class PermBinVPTree<float, SpearmanFootrule>;
template class PermBinVPTree<double, SpearmanRho>;
template class PermBinVPTree<double, SpearmanRhoSIMD>;
template class PermBinVPTree<double, SpearmanFootrule>;
template class PermBinVPTree<int, SpearmanRho>;
template class PermBinVPTree<int, SpearmanRhoSIMD>;
template class PermBinVPTree<int, SpearmanFootrule>;

}  // namespace similarity

