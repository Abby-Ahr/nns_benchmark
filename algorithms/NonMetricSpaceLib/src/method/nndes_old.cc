/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * This is a wrapper class for the Wei Dong implementation of https://code.google.com/p/nndes/,
 * which also contains some of the original code from Wei Dong's repository.  
 * Wei Dong implemented only the graph construction algorithm [1]. 
 * We implemented two search algorithms
 * i)  A greedy walk that starts from a random point and always proceeds to the closest neighbor [2].
 * ii) A priority queue based procedure where the queue may contain more distance not necessarily
 *     adjacent nodes [3]
 *
 * 1. Wei Dong, Charikar Moses, and Kai Li. 2011. 
 *    Efficient k-nearest neighbor graph construction for generic similarity measures. 
 *    In Proceedings of the 20th international conference on World wide web (WWW '11). 
 *    ACM, New York, NY, USA, 577-586. 
 *
 * 2. K. Hajebi, Y. Abbasi-Yadkori, H. Shahbazi, and H. Zhang. 
 *    Fast approximate nearest-neighbor search with k-nearest neighbor graph. 
 *    In IJCAI Proceedings-International Joint Conference on Artificial Intelligence, 
 *    volume 22, page 1312, 2011
 *
 * 3. The main publication is as follows, but the basic algorithm was also presented as SISAP'12: 
 *    Malkov, Y., Ponomarenko, A., Logvinov, A., & Krylov, V., 2014. 
 *    Approximate nearest neighbor algorithm based on navigable small world graphs. Information Systems, 45, 61-68. 
 * 
 * The Wei Dong's code (to construct the knn-graph) can be redistributed given 
 * that the license (see below) is retained in the source code.
 */
/*
Copyright (C) 2010,2011 Wei Dong <wdong@wdong.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iomanip>
#include <map>
#include <unordered_set>
#include <queue>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "space.h"
#include "utils.h"
#include "rangequery.h"
#include "knnquery.h"
#include "method/nndes_old.h"

namespace similarity {

template <typename dist_t>
NNDescentMethodOld<dist_t>::NNDescentMethodOld(
    bool  PrintProgress,
    const Space<dist_t>* space,
    const ObjectVector& data,
    const AnyParams& AllParams) : 
      space_(space), data_(data),   // reference
      NN_(20), // default value from Wei Dong's code
      searchNN_(20),
      controlQty_(0), // default value from Wei Dong's code
      iterationQty_(100), // default value from Wei Dong's code
      rho_(1.0), // default value from Wei Dong's code
      delta_(0.001), // default value from Wei Dong's code
      nndesOracle_(space, data),
      initSearchAttempts_(10),
      greedy_(false)
{
  AnyParamManager pmgr(AllParams);

  pmgr.GetParamOptional("NN", NN_);
  searchNN_ = NN_;
  //pmgr.GetParamOptional("controlQty", controlQty_);
  pmgr.GetParamOptional("iterationQty", iterationQty_);
  pmgr.GetParamOptional("rho", rho_); // Fast rho is 0.5
  pmgr.GetParamOptional("delta", delta_);
  pmgr.GetParamOptional("greedy", greedy_);

  SetQueryTimeParamsInternal(pmgr);

  LOG(LIB_INFO) <<  "NN           = " << NN_;
  //LOG(LIB_INFO) <<  "controlQty   = " << controlQty_;
  LOG(LIB_INFO) <<  "iterationQty = " << iterationQty_;
  LOG(LIB_INFO) <<  "rho          = " << rho_;
  LOG(LIB_INFO) <<  "delta        = " << delta_;

  LOG(LIB_INFO) <<  "(initial) initSearchAttempts= " << initSearchAttempts_;
  LOG(LIB_INFO) <<  "(initial) greedy       = " << greedy_;


  LOG(LIB_INFO) <<  "(initial) searchNN = " << searchNN_;

  LOG(LIB_INFO) << "Starting NN-Descent...";

  nndesObj_.reset(new NNDescent<SpaceOracle>(data_.size(), // N
                                             NN_, //K 
                                             rho_, //S, 
                                             nndesOracle_, GRAPH_BOTH));

    float total = float(data_.size()) * (data_.size() - 1) / 2;
    cout.precision(5);
    cout.setf(ios::fixed);
    for (int it = 0; it < iterationQty_; ++it) {
        int t = nndesObj_->iterate(PrintProgress);
        float rate = float(t) / (NN_ * data_.size());

// TODO @leo computation of recall needs to be re-written, can't use original Wei Dong's code
/*
        float recall = 0;
        if (controlQty_) {  // report accuracy
            const vector<KNN> &nn = nndes.getNN();
#pragma omp parallel for default(shared) reduction(+:recall) 
            for (int i = 0; i < control_knn.size(); ++i) {
                recall += nndes::recall(control_knn[i], nn[control_index[i]], K);
            }
            recall /= control;
        }
        cout << setw(2) << it << " update:" << rate << " recall:" << recall << " cost:" << float(nndes.getCost())/total  << endl;
*/
        LOG(LIB_INFO) << setw(2) << it << " update:" << rate << " cost:" << float(nndesObj_->getCost())/total;
        if (rate < delta_) break;
    }


  LOG(LIB_INFO) << "NN-Descent finished!";
}

template <typename dist_t>
void NNDescentMethodOld<dist_t>::Search(RangeQuery<dist_t>* query) {
  throw runtime_error("Range search is not supported!");
}

template <typename dist_t>
void NNDescentMethodOld<dist_t>::Search(KNNQuery<dist_t>* query) {
  greedy_ ? SearchGreedy(query) : SearchSmallWorld(query);
}

template <typename dist_t>
void NNDescentMethodOld<dist_t>::SearchSmallWorld(KNNQuery<dist_t>* query) {
  const vector<KNN> &nn = nndesObj_->getNN();

  size_t k = query->GetK();
  set <EvaluatedNode>               resultSet;
  unordered_set <IdType>            visitedNodes;

  for (size_t i=0; i < initSearchAttempts_; i++) {
  /**
   * Search of most k-closest elements to the query.
   */
    IdType randPoint = RandomInt() % data_.size();

    priority_queue <dist_t>             closestDistQueue; //The set of all elements which distance was calculated
    priority_queue <EvaluatedNode>      candidateSet; //the set of elements which we can use to evaluate

    dist_t         d = query->DistanceObjLeft(data_[randPoint]);
    EvaluatedNode  ev(-d, randPoint);

    candidateSet.push(ev);
    closestDistQueue.push(d);
    visitedNodes.insert(randPoint);
    resultSet.insert(ev);

    while(!candidateSet.empty()){
      const EvaluatedNode& currEv = candidateSet.top();
      dist_t lowerBound = closestDistQueue.top();

      // Did we reach a local minimum?
      if ((-currEv.first) > lowerBound) {
        break;
      }

      IdType currEvId = currEv.second;

      // Can't access curEv anymore! The reference would become invalid
      candidateSet.pop();

      //calculate distance to each neighbor
      for (const KNNEntry& e: nn[currEvId]) {
        IdType currNew = e.key;
        if (currNew == KNNEntry::BAD) continue;

        if (visitedNodes.find(currNew) == visitedNodes.end()){
            d = query->DistanceObjLeft(data_[currNew]);
            EvaluatedNode  evE1(-d, currNew);

            visitedNodes.insert(currNew);
            closestDistQueue.push(d);
            if (closestDistQueue.size() > searchNN_) { 
              closestDistQueue.pop(); 
            }
            candidateSet.push(evE1);
            resultSet.insert(evE1);
          }
      }
    }
  }

  auto iter = resultSet.rbegin();

  while(k && iter != resultSet.rend()) {
    query->CheckAndAddToResult(-iter->first, data_[iter->second]);
    iter++;
    k--;
  }
}


template <typename dist_t>
void NNDescentMethodOld<dist_t>::SearchGreedy(KNNQuery<dist_t>* query) {
  const vector<KNN> &nn = nndesObj_->getNN();

  for (size_t i=0; i < initSearchAttempts_; i++) {
    IdType curr = RandomInt() % data_.size();

    dist_t currDist = query->DistanceObjLeft(data_[curr]);
    query->CheckAndAddToResult(currDist, data_[curr]);


    IdType currOld;

    do {
      currOld = curr;
      // Iterate over neighbors
      for (const KNNEntry&e: nn[currOld]) {
        IdType currNew = e.key;
        if (currNew != KNNEntry::BAD) {
          dist_t currDistNew = query->DistanceObjLeft(data_[currNew]);
          query->CheckAndAddToResult(currDistNew, data_[currNew]);
          if (currDistNew < currDist) {
            curr = currNew;
            currDist = currDistNew;
          }
        }
      }
    } while (currOld != curr);
  }
}

template <typename dist_t>
void 
NNDescentMethodOld<dist_t>::SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
  pmgr.GetParamOptional("initSearchAttempts", initSearchAttempts_);
  pmgr.GetParamOptional("searchNN", searchNN_);
  pmgr.GetParamOptional("greedy", greedy_);
}

template <typename dist_t>
vector<string>
NNDescentMethodOld<dist_t>::GetQueryTimeParamNames() const {
  vector<string> names;
  names.push_back("initSearchAttempts");
  names.push_back("searchNN");
  names.push_back("greedy");
  return names;
}

template class NNDescentMethodOld<float>;
template class NNDescentMethodOld<double>;
template class NNDescentMethodOld<int>;

}
