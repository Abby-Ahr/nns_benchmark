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
#ifndef SEARCH_ORACLE_HPP
#define SEARCH_ORACLE_HPP

#include <string>
#include <cmath>
#include <vector>
#include <sstream>

#include "object.h"
#include "space.h"
#include "pow.h"
#include "params.h"
#include "experimentconf.h"
#include "logging.h"

#define EXP_LEFT_PARAM              "expLeft"
#define EXP_RIGHT_PARAM             "expRight"
#define ALPHA_LEFT_PARAM            "alphaLeft"
#define ALPHA_RIGHT_PARAM           "alphaRight"
#define MIN_EXP_PARAM               "minExp"
#define MAX_EXP_PARAM               "maxExp"
#define DESIRED_RECALL_PARAM        "desiredRecall"
#define TUNE_K_PARAM                "tuneK"
#define TUNE_R_PARAM                "tuneR"
#define TUNE_QTY_PARAM              "tuneQty"

#define MAX_CACHE_GS_QTY_PARAM      "maxCacheGSQty"
#define MAX_ITER_PARAM              "maxIter"
#define MAX_REC_DEPTH_PARAM         "maxRecDepth"
#define STEP_N_PARAM                "stepN"
#define ADD_RESTART_QTY_PARAM       "addRestartQty"
#define FULL_FACTOR_PARAM           "fullFactor"

namespace similarity {

const size_t MIN_EXP_DEFAULT          =  1;
const size_t MAX_EXP_DEFAULT          =  1;

const size_t MAX_CACHE_GS_QTY_DEFAULT =  1000;
const size_t MAX_ITER_DEFAULT         =  10;
const size_t MAX_REC_DEPTH_DEFAULT    =  6;
const size_t STEP_N_DEFAULT           =  2;
const size_t ADD_RESTART_QTY_DEFAULT  =  4;
const double FULL_FACTOR_DEFAULT      = 8.0;

using std::string; 
using std::vector; 
using std::stringstream;

enum OptimMetric  {IMPR_DIST_COMP,
                   IMPR_EFFICIENCY,
                   IMPR_INVALID};

#define OPTIM_METRIC_PARAMETER  "metric"

#define OPTIM_IMPR_DIST_COMP  "dist"
#define OPTIM_IMPR_EFFICIENCY "time"
#define OPTIM_METRIC_DEFAULT   OPTIM_IMPR_DIST_COMP 

inline OptimMetric getOptimMetric(const string& s) {
  string s1 = s;
  ToLower(s1);
  if (s1 == OPTIM_IMPR_DIST_COMP) return IMPR_DIST_COMP;
  if (s1 == OPTIM_IMPR_EFFICIENCY) return IMPR_EFFICIENCY;
  return IMPR_INVALID;
}

inline string getOptimMetricName(OptimMetric metr) {
  if (IMPR_DIST_COMP == metr) return "improvement in dist. comp";
  if (IMPR_EFFICIENCY == metr) return "improvement in efficiency";
  throw runtime_error("Bug: Invalid optimization metric name");
}

/*
 * Basic pruning oracles are built on the idea that you can relax the pruning criterion
 * in a kd-tree or a vp-tree.
 *
 * First, this idea was proposed by P.N. Yianilos in 1999:
 * Peter N. Yianilos, Locally lifting the curse of dimensionality for nearest neighbor search.
 *
 * It was later generalized to metric spaces. The introduced technique was called
 * "stretching of the triangle inequality". Stretching was governed by a single coefficient alpha,
 * so that the classic metric-space VP-tree pruning rule:
 *
 * MaxDist <= | M  - d(q, pivot) |
 *
 * was replaced by:
 *
 * MaxDist <= alpha | M  - d(q, pivot) |
 *
 * Here, M is a median distance from data points to the pivot and MaxDist
 * is the minimum distance from an object to query encountered during the search
 * (prior to encountering the current pivot/node), which plays a role of the 
 * query radius.
 *
 * Stretching of the triangle inequality was described in:
 *
 * Probabilistic proximity search: 
 * Fighting the curse of dimensionality in metric spaces
 * E Chavez, G Navarro 
 *
 * Boytsov and Bilegsaikhan showed that a more generic pruning is needed if we want to
 * search in generic metric spaces, where the distance is not symmetric. This more generic
 * pruning method can also be more efficient in metric spaces than the originally proposed 
 * stretching rule.
 *
 * More specifically, two potentially different stretching coefficients alphaLeft and alphaRight
 * are used for the left and the right partition, respectively. 
 *
 * The results were published in:
 * Boytsov, Leonid, and Bilegsaikhan Naidan. 
 * "Learning to prune in metric and non-metric spaces." Advances in Neural Information Processing Systems. 2013.
 *
 * A further extension relies on a polynomial approximation of the pruning rule.
 * In the left subtree we prune if:
 * MaxDist <= alphaLeft | M  - d(q, pivot) |^expLeft
 * In the right subtree we prune if:
 * MaxDist <= alphaRight | M  - d(q, pivot) |^expRight
 */

enum VPTreeVisitDecision { kVisitLeft = 1, kVisitRight = 2, kVisitBoth = 3 };

template <typename dist_t>
class PolynomialPruner {
public:
  static std::string GetName() { return "polynomial pruner"; }
  PolynomialPruner(const Space<dist_t>* space, const ObjectVector& data, bool bPrintProgres) : 
      space_(space), data_(data), printProgress_(bPrintProgres), alpha_left_(1), exp_left_(1), alpha_right_(1), exp_right_(1) {}
  void SetParams(AnyParamManager& pmgr);

  vector<string> GetParams() const {
    vector<string> res = {ALPHA_LEFT_PARAM, EXP_LEFT_PARAM, ALPHA_RIGHT_PARAM, EXP_RIGHT_PARAM, 

                          MIN_EXP_PARAM, MAX_EXP_PARAM, DESIRED_RECALL_PARAM, TUNE_K_PARAM, TUNE_R_PARAM, TUNE_QTY_PARAM,

                          MAX_CACHE_GS_QTY_PARAM, MAX_ITER_PARAM, MAX_REC_DEPTH_PARAM, STEP_N_PARAM, ADD_RESTART_QTY_PARAM, FULL_FACTOR_PARAM};
    return res;
  }
  
  void LogParams() {
    LOG(LIB_INFO) << ALPHA_LEFT_PARAM << " = "   << alpha_left_ << " " << EXP_LEFT_PARAM << " = " << exp_left_;
    LOG(LIB_INFO) << ALPHA_RIGHT_PARAM << " = " << alpha_right_ << " " << EXP_RIGHT_PARAM << " = " << exp_right_;
  }

  inline VPTreeVisitDecision Classify(dist_t distQueryPivot, dist_t MaxDist, dist_t MedianDist) const {
   
    /*
     * If the median is in both subtrees (e.g., this is often the case of a discrete metric)
     * and the distance to the pivot is MedianDist, we need to visit both subtrees.
     * Hence, we check for the strict inequality!!! Even if MaxDist == 0, 
     * for the case of dist == MedianDist, 0 < 0 may be false. 
     * Thus, we visit both subtrees. 
     */
    if (distQueryPivot <= MedianDist) {
      double diff = double(MedianDist - distQueryPivot);
      double expDiff = EfficientPow(diff, exp_left_);
      //LOG(LIB_INFO) << " ### " << diff << " -> " << expDiff;
      if (double(MaxDist) < alpha_left_ * expDiff) return (kVisitLeft);
    }
    if (distQueryPivot >= MedianDist) {
      double diff = double(distQueryPivot - MedianDist);  
      double expDiff = EfficientPow(diff, exp_right_);
      //LOG(LIB_INFO) << " ### " << diff << " -> " << expDiff;
      if (double(MaxDist) < alpha_right_* expDiff) return (kVisitRight);
    }

    return (kVisitBoth);
  }
  string Dump() { 
    stringstream str;

    str << ALPHA_LEFT_PARAM << ": " << alpha_left_ << " ExponentLeft: " << exp_left_ << 
           ALPHA_RIGHT_PARAM << ": " << alpha_right_ << " ExponentRight: " << exp_right_ ;
    return str.str();
  }
private:
  const Space<dist_t>*  space_;
  const ObjectVector    data_;
  bool                  printProgress_;

  double    alpha_left_;
  unsigned  exp_left_;
  double    alpha_right_;
  unsigned  exp_right_;
};


template <typename dist_t>
class TriangIneq {
public:
  static std::string GetName() { return "triangle inequality"; }
  TriangIneq(double alpha_left, double alpha_right) : alpha_left_(alpha_left), alpha_right_(alpha_right){}

  inline VPTreeVisitDecision Classify(dist_t dist, dist_t MaxDist, dist_t MedianDist) const {
	
    /*
     * If the median is in both subtrees (e.g., this is often the case of a discrete metric)
     * and the distance to the pivot is MedianDist, we need to visit both subtrees.
     * Hence, we check for the strict inequality!!! Even if MaxDist == 0, 
     * for the case of dist == MedianDist, 0 < 0 may be false. 
     * Thus, we visit both subtrees. 
     */
    if (double(MaxDist) < alpha_left_ * (double(MedianDist) - dist)) return (kVisitLeft);
    if (double(MaxDist) < alpha_right_* (dist - double(MedianDist))) return (kVisitRight);

    return (kVisitBoth);
  }
  string Dump() { 
    stringstream str;

    str << ALPHA_LEFT_PARAM << ": " << alpha_left_ << ALPHA_RIGHT_PARAM << ": " << alpha_right_;
    return str.str();
  }
private:
  double alpha_left_;
  double alpha_right_;
};

template <typename dist_t>
class TriangIneqCreator {
public:
  TriangIneqCreator(double alpha_left, double alpha_right) : alpha_left_(alpha_left), alpha_right_(alpha_right){
    LOG(LIB_INFO) << ALPHA_LEFT_PARAM << " (left stretch coeff)= "   << alpha_left;
    LOG(LIB_INFO) << ALPHA_RIGHT_PARAM << " (right stretch coeff)= " << alpha_right;
  }
  TriangIneq<dist_t>* Create(unsigned level, const Object* /*pivot_*/, const DistObjectPairVector<dist_t>& /*dists*/) const {
    return new TriangIneq<dist_t>(alpha_left_, alpha_right_);
  }
private:
  double alpha_left_;
  double alpha_right_;
};

template <typename dist_t>
class SamplingOracle {
public:
    SamplingOracle(const typename similarity::Space<dist_t>* space,
                   const ObjectVector& AllVectors,
                   const Object* pivot,
                   const DistObjectPairVector<dist_t>& dists,
                   bool   DoRandSample,
                   size_t MaxK,
                   float  QuantileStepPivot,
                   float  QuantileStepPseudoQuery,
                   size_t NumOfPseudoQueriesInQuantile,
                   float  DistLearnThreshold
                   );
    static std::string GetName() { return "sampling"; }

    inline VPTreeVisitDecision Classify(dist_t dist, dist_t MaxDist, dist_t MedianDist) {
        if (NotEnoughData_ || dist == MedianDist) return kVisitBoth;

        if (dist < QuantilePivotDists[0]) return kVisitBoth;

        auto it = std::lower_bound(QuantilePivotDists.begin(), QuantilePivotDists.end(), dist);

        if (QuantilePivotDists.end() == it) return kVisitBoth;

        size_t quant = it - QuantilePivotDists.begin();
      
        if (quant >= QuantileMaxPseudoQueryDists.size()) return kVisitBoth;

        dist_t MaxQueryR = QuantileMaxPseudoQueryDists[quant];

        if (MaxQueryR <= MaxDist) return kVisitBoth;

        CHECK(dist != MedianDist); // should return kVisitBoth before reaching this point
        return dist < MedianDist ? kVisitLeft : kVisitRight;
    }
    string Dump() { 
      stringstream str1, str2;

      for (unsigned i = 0; i < QuantileMaxPseudoQueryDists.size(); ++i) {
        if (i) {
          str1 << ",";
          str2 << ",";
        }
        str1 << QuantilePivotDists[i];
        str2 << QuantileMaxPseudoQueryDists[i];
      }

      return str1.str() + "\n" + str2.str() + "\n";
    }

private:
  const  unsigned MinQuantIndQty = 4;  
  bool   NotEnoughData_; // If true, the classifier returns kVisitBoth

  std::vector<dist_t>  QuantilePivotDists;
  std::vector<dist_t>  QuantileMaxPseudoQueryDists;

  
};

template <typename dist_t>
class SamplingOracleCreator {
public:
    SamplingOracle<dist_t>* Create(unsigned level, const Object* pivot, const DistObjectPairVector<dist_t>& dists) const {
      try {
        return new SamplingOracle<dist_t>(space_,
                                          AllVectors_,
                                          pivot,
                                          dists,
                                          DoRandSample_,
                                          MaxK_,
                                          QuantileStepPivot_,
                                          QuantileStepPseudoQuery_,
                                          NumOfPseudoQueriesInQuantile_,
                                          DistLearnThreshold_);
      } catch (const std::exception& e) {
        LOG(LIB_FATAL) << "Exception while creating sampling oracle: " << e.what();
      } catch (...) {
        LOG(LIB_FATAL) << "Unknown exception while creating sampling oracle";
      } 
      return NULL;
    }
    SamplingOracleCreator(const typename similarity::Space<dist_t>* space,
                   const ObjectVector& AllVectors,
                   bool   DoRandSample,
                   size_t MaxK,
                   float  QuantileStepPivotDists,
                   float  QuantileStepPseudoQuery,
                   size_t NumOfPseudoQueriesInQuantile,
                   float  FractToDetectFuncVal
                   );
private:
  const  typename similarity::Space<dist_t>* space_;
  const  ObjectVector& AllVectors_;
  bool   DoRandSample_; // If true, we don't compute K-neighborhood exactly, MaxK points are sampled randomly.
  size_t MaxK_;
  float  QuantileStepPivot_;       // Quantiles for the distances to a pivot
  float  QuantileStepPseudoQuery_;      // Quantiles for the distances to a pseudo-query
  size_t NumOfPseudoQueriesInQuantile_; /* The number of pseudo-queries, 
                                          which are selected in each distance quantile.
                                        */
  float  DistLearnThreshold_; /* A fraction of observed kVisitBoth-type points we wnat to encounter
                                  before declaring that some radius r is the maximum radius for which
                                  all results are within the same ball as the query point.
                                  The smaller is FractToDetectFuncVal, the close our sampling-based 
                                  procedure to the exact searching. That is, the highest recall
                                  would be for FractToDetectFuncVal == 0.
                                */
};

}

#endif
