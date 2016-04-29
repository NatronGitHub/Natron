
// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/*
 prosac.c
 
 version 1.3 (Sep. 22, 2011)
 
 Author: Frederic Devernay <Frederic.Devernay@inria.fr>
 
 Description: a sample implementation of the PROSAC sampling algorithm, derived from RANSAC.
 
 Reference:
 O. Chum and J. Matas.
 Matching with PROSAC - progressive sample consensus.
 Proc. of Conference on Computer Vision and Pattern Recognition (CVPR), volume 1, pages 220-226,
 Los Alamitos, California, USA, June 2005.
 ftp://cmp.felk.cvut.cz/pub/cmp/articles/matas/chum-prosac-cvpr05.pdf
 
 Note:
 In the above article, the test is step 2 of Algorithm 1 seem to be reversed, and the test in step 1
 is not consistent with the text. They were corrected in this implementation.
 
 History:
 version 1.0 (Apr. 14, 2009): initial version
 version 1.1 (Apr. 22, 2009): fix support computation, "The hypotheses are veriﬁed against all data"
 version 1.2 (Mar. 16, 2011): Add comments about beta, psi, and set eta0 to its original value (0.05 rather than 0.01)
 version 1.3 (Sep. 22, 2011): Check that k_n_star is never nore than T_N
 version 1.4 (Sep. 24, 2011): Don't stop until we have found at least the expected number of inliers (improvement over original PROSAC).
 version 1.5 (Oct. 10, 2011): Also stop if t > T_N (maximum number of iterations given the apriori proportion of outliers).
 version 1.6 (Oct. 10, 2011): Rewrite niter_RANSAC() and also use it to update k_n_star.
 */

#ifndef OPENMVG_ROBUST_ESTIMATION_PROSAC_H_
#define OPENMVG_ROBUST_ESTIMATION_PROSAC_H_

#include "openMVG/robust_estimation/rand_sampling.hpp"
#include "openMVG/robust_estimation/robust_estimator_ProsacKernelAdaptator.hpp"
#include <limits>
#include <numeric>
#include <cassert>
#include <algorithm>
#include <vector>

// Uncomment out to disable n_star optimization (i.e. draw the same # of samples as RANSAC)
//#define PROSAC_DISABLE_N_STAR_OPTIMIZATION

//#define PROSAC_DISABLE_LO_RANSAC


namespace openMVG {
namespace robust{
    
    
    /// Computation of the Maximum number of iterations for Ransac
    /// with the formula from [HZ] Section: "How many samples" p.119
inline
int niter_RANSAC(double p, // probability that at least one of the random samples picked up by RANSAC is free of outliers
                     double epsilon, // proportion of outliers
                     int s, // sample size
                     int Nmax = -1) // upper bound on the number of iterations (-1 means INT_MAX)
{
        // compute safely N = ceil(log(1. - p) / log(1. - exp(log(1.-epsilon) * s)))
    if (Nmax == -1) {
        Nmax = std::numeric_limits<int>::max();
    }
    assert(Nmax >= 1);
    if (epsilon <= 0.) {
        return 1;
    }
    // logarg = -(1-epsilon)^s
    double logarg = -std::pow(1.-epsilon, s); /* use -exp(s*log(1.-epsilon) if pow is not avail. */
    // logval = log1p(logarg)) = log(1-(1-epsilon)^s)
    double logval = std::log(1. + logarg); // C++/boost version: logval = boost::math::log1p(logarg)
    double N = std::log(1. - p) / logval;
    if (logval  < 0. && N < Nmax) {
        // for very big N, log1p(x) is more precise than log(1+x), so N values may differ
        assert(N > 1e8 || std::ceil(N) == std::ceil(std::log(1. - p) / std::log(1. - std::exp(std::log(1. - epsilon) * s))));
        return (int)std::ceil(N);
    }
    return Nmax;
}
    
    // * Non-randomness: eq. (7) states that i-m (where i is the cardinal of the set of inliers for a wrong
    // model) follows the binomial distribution B(n,beta). http://en.wikipedia.org/wiki/Binomial_distribution
    // For n big enough, B(n,beta) ~ N(mu,sigma^2) (central limit theorem),
    // with mu = n*beta and sigma = sqrt(n*beta*(1-beta)).
    // psi, the probability that In_star out of n_star data points are by chance inliers to an arbitrary
    // incorrect model, is set to 0.05 (5%, as in the original paper), and you must change the Chi2 value if
    // you chose a different value for psi.
inline
int Prosac_Imin(int m, int n, double beta) {
    assert(beta > 0. && beta < 1.);
    const double mu = n * beta;
    const double sigma = std::sqrt(n*beta*(1-beta));
    // Imin(n) (equation (8) can then be obtained with the Chi-squared test with P=2*psi=0.10 (Chi2=2.706)
    return (int)std::ceil(m + mu + sigma * std::sqrt(2.706));
}

enum ProsacReturnCodeEnum
{
    eProsacReturnCodeFoundModel = 0,
    eProsacReturnCodeNotEnoughPoints,
    eProsacReturnCodeNoModelFound,
    eProsacReturnCodeMaxIterationsParamReached,
    eProsacReturnCodeMaxIterationsFromProportionParamReached,
    eProsacReturnCodeInliersIsMinSamples,
    
    
};

template<typename Kernel>
bool searchModel_minimalSamples(const Kernel &kernel,
                                typename Kernel::Model* bestModel,
                                InliersVec *bestInliers = 0)
{
    assert(kernel.NumSamples() == Kernel::MinimumSamples());
    
    InliersVec isInlier(kernel.NumSamples());
    int best_score = 0;
    bool bestModelFound = false;
    std::vector<typename Kernel::Model> possibleModels;
    kernel.ComputeModelFromMinimumSamples(&possibleModels);
    for (std::size_t i = 0; i < possibleModels.size(); ++i) {
        int model_score = kernel.ComputeInliersForModel(possibleModels[i], &isInlier);
        if (model_score > best_score) {
            best_score = model_score;
            *bestModel = possibleModels[i];
            bestModelFound = true;
        }
    }
    if (!bestModelFound) {
        return false;
    }
    if (bestInliers) {
        *bestInliers = isInlier;
    }
    kernel.Unnormalize(bestModel);
    return true;
}


template<typename Kernel>
ProsacReturnCodeEnum prosac(const Kernel &kernel,
                            typename Kernel::Model* bestModel,
                            InliersVec *bestInliers = 0)
{
    assert(bestModel);
    
    const int N = (int)std::min(kernel.NumSamples(), (std::size_t)RAND_MAX);
    
    // For us, the draw set is the same as the verification set
    const int N_draw = N;
    const int m = (int)Kernel::MinimumSamples();
    
    
    // Test if we have sufficient points for the kernel.
    if (N < m) {
        return eProsacReturnCodeNotEnoughPoints;
    } else if (N == m) {
        bool ok = searchModel_minimalSamples(kernel, bestModel, bestInliers);
        return ok ? eProsacReturnCodeFoundModel : eProsacReturnCodeNoModelFound;
    }
    
    InliersVec isInlier(N);
#ifndef PROSAC_DISABLE_LO_RANSAC
    InliersVec isInlierLO(N);
#endif
    
    /* NOTE: the PROSAC article sets T_N (the number of iterations before PROSAC becomes RANSAC) to 200000,
     but that means :
     - only 535 correspondences out of 1000 will be used after 2808 iterations (60% outliers)
     -      395                                                 588            (50%)
     -      170                                                 163            (40%)
     (the # of iterations is the # of RANSAC draws for a 0.99 confidence
     of finding the right model given the percentage of outliers)
     
     QUESTION: Is it more reasonable to set it to the maximum number of iterations we plan to
     do given the percentage of outlier?
     
     MY ANSWER: If you know that you won't draw more than XX samples (say 2808, because you only
     accept 60% outliers), then it's more reasonable to set N to that value, in order to give
     all correspondences a chance to be drawn (even if that chance is very small for the last ones).
     Anyway, PROSAC should find many inliers in the first rounds and stop right away.
     
     T_N=2808 gives:
     - only 961 correspondences out of 1000 will be used after 2808 iterations (60% outliers)
     -      595                                                 588            (50%)
     -      177                                                 163            (40%)
     
     */

    const int T_N = kernel.maxOutliersProportion >= 1. ?  std::numeric_limits<int>::max() : niter_RANSAC(kernel.probability, kernel.maxOutliersProportion, m, kernel.iMaxIter);
    const int t_max = kernel.iMaxIter > 0 ? kernel.iMaxIter : T_N;
    
    const double beta = kernel.getProsacBetaParam();
    assert(beta > 0. && beta < 1.);
    int n_star = N; // termination length (see sec. 2.2 Stopping criterion)
    int I_n_star = 0; // number of inliers found within the first n_star data points
    int I_N_best = 0; // best number of inliers found so far (store the model that goes with it)
    const int I_N_min = (1. - kernel.maxOutliersProportion) * N; // the minimum number of total inliers
    int t = 0; // iteration number
    int n = m; // we draw samples from the set U_n of the top n data points
    double T_n = T_N; // average number of samples {M_i}_{i=1}^{T_N} that contain samples from U_n only
    int T_n_prime = 1; // integer version of T_n, see eq. (4)
    
    for(int i = 0; i < m; ++i) {
        T_n *= (double)(n - i) / (N - i);
    }
    int k_n_star = T_N; // number of samples to draw to reach the maximality constraint

    bool bestModelFound = false;
    
    std::vector<std::size_t> sample(m);

    // Note: the condition (I_N_best < I_N_min) was not in the original paper, but it is reasonable:
    // we sholdn't stop if we haven't found the expected number of inliers
    while (((I_N_best < I_N_min) || t <= k_n_star) && t < T_N && t <= t_max) {
        int I_N; // total number of inliers for that sample
        
        // Choice of the hypothesis generation set
        t = t + 1;
        
        // from the paper, eq. (5) (not Algorithm1):
        // "The growth function is then deﬁned as
        //  g(t) = min {n : T′n ≥ t}"
        // Thus n should be incremented if t > T'n, not if t = T'n as written in the algorithm 1
        if ((t > T_n_prime) && (n < n_star)) {
            double T_nplus1 = (T_n * (n+1)) / (n+1-m);
            n = n+1;
            T_n_prime = T_n_prime + std::ceil(T_nplus1 - T_n);
            T_n = T_nplus1;
        }
        
        // Draw semi-random sample (note that the test condition from Algorithm1 in the paper is reversed):
        if (t > T_n_prime) {
            // during the finishing stage (n== n_star && t > T_n_prime), draw a standard RANSAC sample
            // The sample contains m points selected from U_n at random
            deal(n, m, sample);
        }  else {
            // The sample contains m-1 points selected from U_{n−1} at random and u_n
            deal(n - 1, m - 1, sample);
            sample[m - 1] = n - 1;
        }
        
        // INSERT Compute model parameters p_t from the sample M_t
        std::vector<typename Kernel::Model> possibleModels;
        kernel.ComputeModelFromMinimumSamples(sample, &possibleModels);
        
        for (std::size_t modelNb = 0; modelNb < possibleModels.size(); ++modelNb) {
            
            
            // Find support of the model with parameters p_t
            // From first paragraph of section 2: "The hypotheses are veriﬁed against all data"
            I_N = kernel.ComputeInliersForModel(possibleModels[modelNb], &isInlier);
            
            
            if (I_N > I_N_best) {
                int n_best; // best value found so far in terms of inliers ratio
                int I_n_best; // number of inliers for n_best
                int I_N_draw; // number of inliers withing the N_draw first data
                
                
                // INSERT (OPTIONAL): Test for degenerate model configuration (DEGENSAC)
                //                    (i.e. discard the sample if more than 1 model is consistent with the sample)
                // ftp://cmp.felk.cvut.cz/pub/cmp/articles/matas/chum-degen-cvpr05.pdf
                
                // Do local optimization, and recompute the support (LO-RANSAC)
                // http://cmp.felk.cvut.cz/~matas/papers/chum-dagm03.pdf
                // for the fundamental matrix, the normalized 8-points algorithm performs very well:
                // http://axiom.anu.edu.au/~hartley/Papers/fundamental/ICCV-final/fundamental.pdf
                
                
                // Store the best model
                *bestModel = possibleModels[modelNb];
                bestModelFound = true;
                
#ifndef PROSAC_DISABLE_LO_RANSAC
                int loransac_iter = 0;
                while (I_N > I_N_best) {
                    I_N_best = I_N;
                    
                    if (kernel.iMaxLOIter < 0 || loransac_iter < kernel.iMaxLOIter) {
                        
                        // Continue while LO-ransac finds a better support
                        typename Kernel::Model modelLO;
                        bool modelOptimized = kernel.OptimizeModel(*bestModel, isInlier, &modelLO);
                        
                        if (modelOptimized) {
                            // IN = findSupport(/* model, sample, */ N, isInlier);
                            int I_N_LO = kernel.ComputeInliersForModel(modelLO, &isInlierLO);
                            if (I_N_LO > I_N_best) {
                                isInlier = isInlierLO;
                                *bestModel = modelLO;
                                I_N = I_N_LO;
                            }
                        }
                        ++loransac_iter;
                    } // LO-RANSAC
                }
#else
                if (I_N > I_N_best) {
                    I_N_best = I_N;
                }
#endif
                
                if (bestInliers) {
                    *bestInliers = isInlier;
                }
                
                // Select new termination length n_star if possible, according to Sec. 2.2.
                // Note: the original paper seems to do it each time a new sample is drawn,
                // but this really makes sense only if the new sample is better than the previous ones.
                n_best = N;
                I_n_best = I_N_best;
                I_N_draw = std::accumulate(isInlier.begin(), isInlier.begin() + N_draw, 0);
#ifndef PROSAC_DISABLE_N_STAR_OPTIMIZATION
                int n_test; // test value for the termination length
                int I_n_test; // number of inliers for that test value
                double epsilon_n_best = (double)I_n_best/n_best;
                
                for (n_test = N, I_n_test = I_N_draw; n_test > m; n_test--) {
                    // Loop invariants:
                    // - I_n_test is the number of inliers for the n_test first correspondences
                    // - n_best is the value between n_test+1 and N that maximizes the ratio I_n_best/n_best
                    assert(n_test >= I_n_test);
                    
                    // * Non-randomness : In >= Imin(n*) (eq. (9))
                    // * Maximality: the number of samples that were drawn so far must be enough
                    // so that the probability of having missed a set of inliers is below eta=0.01.
                    // This is the classical RANSAC termination criterion (HZ 4.7.1.2, eq. (4.18)),
                    // except that it takes into account only the n first samples (not the total number of samples).
                    // kn_star = log(eta0)/log(1-(In_star/n_star)^m) (eq. (12))
                    // We have to minimize kn_star, e.g. maximize I_n_star/n_star
                    //printf("n_best=%d, I_n_best=%d, n_test=%d, I_n_test=%d\n",
                    //        n_best,    I_n_best,    n_test,    I_n_test);
                    // a straightforward implementation would use the following test:
                    //if (I_n_test > epsilon_n_best*n_test) {
                    // However, since In is binomial, and in the case of evenly distributed inliers,
                    // a better test would be to reduce n_star only if there's a significant improvement in
                    // epsilon. Thus we use a Chi-squared test (P=0.10), together with the normal approximation
                    // to the binomial (mu = epsilon_n_star*n_test, sigma=sqrt(n_test*epsilon_n_star*(1-epsilon_n_star)).
                    // There is a significant difference between the two tests (e.g. with the findSupport
                    // functions provided above).
                    // We do the cheap test first, and the expensive test only if the cheap one passes.
                    if (( I_n_test * n_best > I_n_best * n_test ) &&
                        ( I_n_test > epsilon_n_best * n_test + std::sqrt(n_test * epsilon_n_best * (1. - epsilon_n_best) * 2.706) )) {
                        if (I_n_test < Prosac_Imin(m,n_test,beta)) {
                            // equation 9 not satisfied: no need to test for smaller n_test values anyway
                            break; // jump out of the for(n_test) loop
                        }
                        n_best = n_test;
                        I_n_best = I_n_test;
                        epsilon_n_best = (double)I_n_best / n_best;
                    }
                    
                    // prepare for next loop iteration
                    I_n_test -= isInlier[n_test - 1];
                } // for(n_test ...
#endif // #ifndef PROSAC_DISABLE_N_STAR_OPTIMIZATION
                
                // is the best one we found even better than n_star?
                if ( I_n_best * n_star > I_n_star * n_best ) {
                    assert(n_best >= I_n_best);
                    // update all values
                    n_star = n_best;
                    I_n_star = I_n_best;
                    k_n_star = niter_RANSAC(1. - kernel.eta0, 1. - I_n_star / (double)n_star, m, T_N);
                }
            } // if (I_N > I_N_best)
        } //for (modelNb ...
    } // while(t <= k_n_star ...
    
    if (!bestModelFound) {
        return eProsacReturnCodeNoModelFound;
    }
    
    kernel.Unnormalize(bestModel);

    
    if (t == t_max) {
        return eProsacReturnCodeMaxIterationsParamReached ;
    }
    
    if (t == T_N) {
        return eProsacReturnCodeMaxIterationsFromProportionParamReached;
    }
    
    if (I_N_best == m) {
        return eProsacReturnCodeInliersIsMinSamples;
    }
    
    return eProsacReturnCodeFoundModel;
} // prosac
    
    
/*
Computes a model from N correspondences when we know the number of outliers is to be lower than 10%
This should be used on user input data where we known there is likely no outlier.
 
This function can be used when the model searched from the samples is likely not to be the correct model.
This will give an average result that fits all correspondences but that is not necessarily the correct model.

*/
template<typename Kernel>
bool searchModelLS(const Kernel &kernel,
                  typename Kernel::Model* bestModel)
{
    assert(bestModel);
    const int N = (int)kernel.NumSamples();
    const int m = (int)Kernel::MinimumSamples();
    
    // Test if we have sufficient points for the kernel.
    if (N < m) {
        return 0;
    } else if (N == m) {
        return searchModel_minimalSamples(kernel, bestModel, 0);
    }
    
    bool ok = kernel.ComputeModelFromAllSamples(bestModel);
    kernel.Unnormalize(bestModel);
    return ok;
    
} // searchModelWithMEstimator

/*
 Computes a model from N correspondences when we know the number of outliers is to be lower than 10%
 This should be used on user input data where we known there is likely no outlier
 
 @param maxNbIterations The number of iterations of the MEstimator
 @returns The number of successful iterations
 */
template<typename Kernel>
int searchModelWithMEstimator(const Kernel &kernel,
                              int maxNbIterations,
                              typename Kernel::Model* bestModel,
                              double *sigmaMAD_p = 0)
{
    assert(bestModel);
    const int N = (int)kernel.NumSamples();
    const int m = (int)Kernel::MinimumSamples();
    
    // Test if we have sufficient points for the kernel.
    if (N < m) {
        return 0;
    } else if (N == m) {
        bool ok = searchModel_minimalSamples(kernel, bestModel, 0);
        return ok ? 1 : 0;
    }

    // Compute a first model on all samples with least squares
    int hasModel = kernel.ComputeModelFromAllSamples(bestModel);
    if (!hasModel) {
        return 0;
    }

    InliersVec isInlier(N, true);
    
    int nbSuccessfulIterations = kernel.MEstimator(*bestModel, isInlier, maxNbIterations, bestModel, sigmaMAD_p);
    kernel.Unnormalize(bestModel);
    return nbSuccessfulIterations;

} // searchModelWithMEstimator
    

} // namespace robust
} // namespace openMVG
#endif // OPENMVG_ROBUST_ESTIMATION_PROSAC_H_
