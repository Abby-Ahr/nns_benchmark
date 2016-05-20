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

#ifndef _FACTORY_LSH_
#define _FACTORY_LSH_

#include <method/lsh.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreateLSHCauchy(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
    unsigned  LSH_M = 20;
    unsigned  LSH_L = 50;
    unsigned  LSH_H = 1017881;
    float     LSH_W = 20;

    AnyParamManager pmgr(AllParams);

    pmgr.GetParamOptional("W",  LSH_W);
    pmgr.GetParamOptional("M",  LSH_M);
    pmgr.GetParamOptional("L",  LSH_L);
    pmgr.GetParamOptional("H",  LSH_H);

    int SpaceSelector = 1;
    if (SpaceType != "l1") LOG(LIB_FATAL) << "LSH (Cauchy) works only with L1";
    return new LSHCauchy<dist_t>(
                      space,
                      DataObjects,
                      SpaceSelector,
                      LSH_W,
                      LSH_M,
                      LSH_L,
                      LSH_H
                  );
}

template <typename dist_t>
Index<dist_t>* CreateLSHGaussian(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
    unsigned  LSH_M = 20;
    unsigned  LSH_L = 50;
    unsigned  LSH_H = 1017881;
    float     LSH_W = 20;

    AnyParamManager pmgr(AllParams);

    pmgr.GetParamOptional("W",  LSH_W);
    pmgr.GetParamOptional("M",  LSH_M);
    pmgr.GetParamOptional("L",  LSH_L);
    pmgr.GetParamOptional("H",  LSH_H);

    int SpaceSelector = 2;
    if (SpaceType != "l2") LOG(LIB_FATAL) << "LSH (Guassian) works only with L2";

    return new LSHGaussian<dist_t>(
                      space,
                      DataObjects,
                      SpaceSelector,
                      LSH_W,
                      LSH_M,
                      LSH_L,
                      LSH_H
                  );
}

template <typename dist_t>
Index<dist_t>* CreateLSHThreshold(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
    unsigned  LSH_M = 20;
    unsigned  LSH_L = 50;
    unsigned  LSH_H = 1017881;

    AnyParamManager pmgr(AllParams);

    pmgr.GetParamOptional("M",  LSH_M);
    pmgr.GetParamOptional("L",  LSH_L);
    pmgr.GetParamOptional("H",  LSH_H);

    int SpaceSelector = 1;
    if (SpaceType != "l1") LOG(LIB_FATAL) << "LSH (Threshold) works only with L1";
    return new LSHThreshold<dist_t>(
                      space,
                      DataObjects,
                      SpaceSelector,
                      0, // W is not used in this case.
                      LSH_M,
                      LSH_L,
                      LSH_H
                  );

}

/*
 * End of creating functions.
 */

}

#endif
