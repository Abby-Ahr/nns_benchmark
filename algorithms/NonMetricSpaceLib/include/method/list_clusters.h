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

#ifndef _LIST_OF_CLUSTERS_H_
#define _LIST_OF_CLUSTERS_H_

#include "index.h"
#include "lcstrategy.h"
#include "params.h"

#define METH_LIST_CLUSTERS          "list_clusters"

namespace similarity {

/* 
 * E. Chavez and G. Navarro.
 * A compact space decomposition for effective a metric indexing. 
 * Pattern Recognition Letters, 26(9):1363-1376, 2005
 */

template <typename dist_t>
class Space;

template <typename dist_t>
class ListClusters : public Index<dist_t> {
 public:
  ListClusters(const Space<dist_t>* space,
               const ObjectVector& data,
               const AnyParams& MethParams);
  ~ListClusters();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  virtual vector<string> GetQueryTimeParamNames() const;

  static const Object* SelectNextCenter(
      DistObjectPairVector<dist_t>& remaining,
      ListClustersStrategy strategy);

 private:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& );

  template <typename QueryType>
  void GenSearch(QueryType* query);

  class Cluster {
   public:
    Cluster(const Object* center);
    ~Cluster();

    void OptimizeBucket();
    void AddObject(const Object* object,
                   const dist_t dist);

    const Object* GetCenter();
    const dist_t GetCoveringRadius();
    const ObjectVector& GetBucket();

    template <typename QueryType>
    void Search(QueryType* query) const;

   private:
    const Object* center_;
    dist_t covering_radius_;
    char* CacheOptimizedBucket_;
    ObjectVector* bucket_;
    int MaxLeavesToVisit_;
  };

  std::vector<Cluster*> cluster_list_;

  ListClustersStrategy Strategy_; 
  bool                 UseBucketSize_;
  size_t               BucketSize_;
  dist_t               Radius_;
  int                  MaxLeavesToVisit_;
  bool                 ChunkBucket_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(ListClusters);
};

}   // namespace similarity

#endif     // _METRIC_GHTREE_H_

