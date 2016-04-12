
// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_ROBUST_ESTIMATOR_PROSAC_KERNEL_ADAPTATOR_H_
#define OPENMVG_ROBUST_ESTIMATOR_PROSAC_KERNEL_ADAPTATOR_H_

#include <vector>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "openMVG/multiview/solver_homography_kernel.hpp"
#include "openMVG/multiview/solver_fundamental_kernel.hpp"
#include <Eigen/Geometry>
#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/SVD>

namespace openMVG {
    namespace robust{

        template<typename _Matrix_Type_, typename _Inverse_Matrix_Type>
        _Inverse_Matrix_Type pseudoInverse( const _Matrix_Type_& a, bool omit = true)
        {
            
            int n = a.cols();
            int m = a.rows();
        
            _Inverse_Matrix_Type inverse(n,m);
            
            Eigen::JacobiSVD< _Matrix_Type_ > svd(a ,Eigen::ComputeThinU | Eigen::ComputeThinV);

            if (svd.rank() > 0) {
                
                
                const typename Eigen::JacobiSVD< _Matrix_Type_ >::SingularValuesType &singularValues = svd.singularValues();
                
                Vec reciprocalS(singularValues.rows());
                if (omit) {
                    double tol = std::max(m,n) * singularValues(0) * std::numeric_limits<double>::epsilon();
                    for (int i = singularValues.rows() - 1; i >= 0; i--) {
                        reciprocalS(i) = std::abs(singularValues(i)) < tol ? 0.0 : 1.0 / singularValues(i);
                    }
                } else {
                    for (int i = singularValues.rows() - 1; i >= 0; i--) {
                        reciprocalS(i) = singularValues(i) == 0. ? 0.0 : 1.0 / singularValues(i);
                    }
                }
                
                const typename Eigen::JacobiSVD< _Matrix_Type_ >::MatrixVType& V = svd.matrixV();
                const typename Eigen::JacobiSVD< _Matrix_Type_ >::MatrixUType& U = svd.matrixU();
                
                int min = std::min(n, m);
                for (int i = n-1; i >= 0; i--) {
                    for (int j = m-1; j >= 0; j--) {
                        for (int k = min-1; k >= 0; k--) {
                            inverse(i,j) += V(i,k) * reciprocalS(k) * U(j,k);
                        }
                    }
                }
            }
            return inverse;
        }
        
        struct Mat3ModelNormalizer {
            
            static void Normalize(Mat* normalizedPoints, Mat3* t, int w, int h)
            {
                // Normalize point in image coordinates to [-.5, .5]
                openMVG::NormalizePoints(*normalizedPoints, normalizedPoints, t, w, h);
            }
            
            static void Unnormalize(Mat3* model, const Mat3& t1, const Mat3& t2) {
                // Unnormalize model from the computed conditioning.
                *model = t2.inverse() * (*model) * t1;
            }
        };
        
        
        
        /// Compute the distance threshold for inliers from the codimension of the model and
        /// the standard deviation of the data points (mostly surf sigma for us)
        /// \param iCodimension The codimension of the model, i.e. the dimension of
        ///                     the error measurements, see [HZ] p.118 (4.7.1):
        ///                     1 line in image, fundamental matrix, plane in space
        ///                     2 homography, camera matrix, line in space
        ///                     3 trifocal tensor
        /// \param sigma The variance of the method used to measure the data points
        ///              (for a discussion on SIFT and SURF covariance, see
        ///              http://www.bmva.org/bmvc/2009/Papers/Paper276/Paper276.pdf )
        /// \warning This values assume a Chi-Square distribution with alpha = 0.95 :
        ///          This means that an inlier will only be incorrectly rejected 5% of the time
        template <int CODIMENSION>
        inline double InlierThreshold(double sigma) {
            switch(CODIMENSION) {
                case 1:
                    // return sqrt(3.84 * sigma * sigma);
                    return 1.9596 * sigma;
                case 2:
                    // return sqrt(5.99 * sigma * sigma);
                    return 2.44745 * sigma;
                case 3:
                    //return sqrt(7.81 * sigma * sigma);
                    return 2.79465 * sigma;
                default:
                    assert(false && "Bad codimension value");
                    return 0;
            }
        }
        
        struct FundamentalSolver {
            
            typedef Mat3 Model;
            
            static std::size_t MinimumSamples() { return openMVG::fundamental::kernel::SevenPointSolver::MINIMUM_SAMPLES; }
            
            static std::size_t MaximumModels() { return openMVG::fundamental::kernel::SevenPointSolver::MAX_MODELS; }
            
            enum CoDimensionEnum { CODIMENSION = 1 };
            
            /** 3D rigid transformation estimation (7 dof) with z=1
             * Compute a Scale Rotation and Translation rigid transformation.
             * This transformation provide a distortion-free transformation
             * using the following formulation Xb = S * R * Xa + t.
             * "Least-squares estimation of transformation parameters between two point patterns",
             * Shinji Umeyama, PAMI 1991, DOI: 10.1109/34.88573
             */
            static void Solve(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                return openMVG::fundamental::kernel::SevenPointSolver::Solve(x,y,Hs);
            }
            
            /// Beta is the probability that a match is declared inlier by mistake,
            ///  i.e. the ratio of the "inlier surface" by the "total surface".
            /// For the computation of the 2D Similarity:
            ///     The "inlier surface" is a disc of radius InlierThreshold.
            ///     The "total surface" is the surface of the image = width * height
            /// Beta (its ratio) is then
            ///  M_PI*InlierThreshold^2/(width*height)
            static double Beta(double inlierThreshold,double width, double height)
            {
                return std::min(1., inlierThreshold * (width + height) / width / height);
            }
            
            static double Error(const Model &model, const Vec2 &x1, const Vec2 &x2) {
                return openMVG::fundamental::kernel::SampsonError::Error(model, x1, x2);
            }
            
            static void Normalize(Mat* x1, Mat* x2, Mat3* t1, Mat3* t2, int w1, int h1, int w2, int h2)
            {
                Mat3ModelNormalizer::Normalize(x1, t1, w1, h1);
                Mat3ModelNormalizer::Normalize(x2, t2, w2, h2);
            }
            
            static void Unnormalize(Model* model, const Mat3& t1, const Mat3& t2) {
                Mat3ModelNormalizer::Unnormalize(model, t1, t2);
            }

        };

        
        struct Homography2DSolver {
            
            typedef Mat3 Model;
            
            static std::size_t MinimumSamples() { return 4; }
            
            static std::size_t MaximumModels() { return 1; }
            
            enum CoDimensionEnum { CODIMENSION = 2 };
            
            /**
             * Computes the homography that transforms x to y with the Direct Linear
             * Transform (DLT).
             *
             * \param x  A 2xN matrix of column vectors.
             * \param y  A 2xN matrix of column vectors.
             * \param Hs A vector into which the computed homography is stored.
             *
             * The estimated homography should approximately hold the condition y = H x.
             */
            static void Solve(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                return openMVG::homography::kernel::FourPointSolver::Solve(x , y, Hs);
            }
            
            /// Beta is the probability that a match is declared inlier by mistake,
            ///  i.e. the ratio of the "inlier surface" by the "total surface".
            /// For the computation of the 2D Homography:
            ///     The "inlier surface" is a disc of radius InlierThreshold.
            ///     The "total surface" is the surface of the image = width * height
            /// Beta (its ratio) is then
            ///  M_PI*InlierThreshold^2/(width*height)
            static double Beta(double inlierThreshold, double width, double height)
            {
                return std::min(1., M_PI * inlierThreshold * inlierThreshold / (width * height));
            }
            
            
            /** Sampson's distance for Homographies.
             * Ref: The Geometric Error for Homographies, Ondra Chum, Tomás Pajdla, Peter Sturm,
             * Computer Vision and Image Understanding, Volume 97, Number 1, page 86-102 - January 2005
             *
             * Let (x1,y1),(x2,y2) be a noisy estimate of a match related by a homography H:
             * [x2,y2,1] ~= H [x1,y1,1].
             *
             * According to the above article above (Sec. 4), the Sampson distance for homographies is
             * (X_s - X) = pseudoinverse(J).t
             * where:
             * X := [x1,y1,x2,y2]
             * t := [x1*h1+y1*h2+h3-x1*x2*h7-y1*x2*h8-x2*h9,x1*h4+y1*h5+h6-x1*y2*h7-y1*y2*h8-y2*h9]
             * J := Jacobian(t,X)
             *
             * the formula for the pseudo-inverse of a matrix with linearly independent rows is:
             * pseudoinverse(J) = trans(J)inverse(J.trans(J))
             */
            static inline void Homography2DSampsonDistanceSigned(const Mat3& model, const Vec2 &xPoints, const Vec2& yPoints, Vec4 *dX)
            {
     
                double x1 = xPoints(0);
                double y1 = xPoints(1);
                double x2 = yPoints(0);
                double y2 = yPoints(1);
                double h1 = model(0,0), h2 = model(0,1), h3 = model(0,2);
                double h4 = model(1,0), h5 = model(1,1), h6 = model(1,2);
                double h7 = model(2,0), h8 = model(2,1), h9 = model(2,2);
                
                Vec2 t(x1 * h1 + y1 * h2 + h3 - x1 * x2 * h7- y1 * x2 * h8 - x2 * h9,
                       x1 *h4 + y1 * h5 + h6 - x1 * y2 * h7 - y1 * y2 * h8 - y2 * h9);
                
                double j1 = h1-h7*x2, j2 = h2-h8*x2, j3 = -h7*x1-h8*y1-h9, j4 = h4-h7*y2, j5 =h5-h8*y2;
                
                typedef Eigen::Matrix<double, 2, 4> Mat24;
                typedef Eigen::Matrix<double, 4, 2> Mat42;
                
                Mat24 J;
                J(0,0) = j1; J(0,1) = j2; J(0,2) = j3; J(0,3) = 0;
                J(1,0) = j4; J(1,1) = j5; J(1,2) = 0; J(1,3) = j3;
                
                // J
                *dX = J.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(t);
                
                /*Mat2 JJt = J * J.transpose();
                double det = JJt.determinant();
                if (det != 0.) {
                    Mat2 invJJt = JJt.inverse();
                    *dX = J.transpose() * Vec2(invJJt * t);
                } else {
                    // if the closed-form formula for the Moore-Penrose pseudo-inverse doesn't work, use SVD
                    *dX = pseudoInverse<Mat24,Mat42>(J) * t;
                }*/

            }
            
            static double Homography2DSampsonDistance(const Mat3& model, const Vec2 &xPoints, const Vec2& yPoints)
            {
                
                Vec4 dX;
                Homography2DSampsonDistanceSigned(model, xPoints, yPoints,&dX);
                return dX.norm() / std::sqrt(2.); // divide by sqrt(2) to be consistent with image distance in each image
            }
            
            static double Error(const Model &model, const Vec2 &x1, const Vec2 &x2) {
                return Homography2DSampsonDistance(model, x1, x2);
            }
            
            static void Normalize(Mat* x1, Mat* x2, Mat3* t1, Mat3* t2, int w1, int h1, int w2, int h2)
            {
                Mat3ModelNormalizer::Normalize(x1, t1, w1, h1);
                Mat3ModelNormalizer::Normalize(x2, t2, w2, h2);
            }
            
            static void Unnormalize(Model* model, const Mat3& t1, const Mat3& t2) {
                Mat3ModelNormalizer::Unnormalize(model, t1, t2);
            }
        };
        
        
        // A 2-D similarity is a rotation R, followed by a scale factor c and a translation t:
        // x' = cRx + t
        // R = [ cos(alpha) -sin(alpha) ]
        //     [ sin(alpha)  cos(alpha) ]
        // We use 4 parameters: S = (tx, ty, c.sin(alpha), c.cos(alpha))
        // c can be recovered as c = sqrt(S(2)^2+S(3)^2)
        // and alpha = atan2(S(2),S(3))
        struct Similarity2DSolver {
            
            typedef Vec4 Model;
            
            static std::size_t MinimumSamples() { return 2; }
            
            static std::size_t MaximumModels() { return 1; }
            
            enum CoDimensionEnum { CODIMENSION = 2 };
            
            static void rtsFromVec4(const Vec4& S, double* tx, double* ty, double* scale, double* rot)
            {
                *scale = std::sqrt(S(2) * S(2) + S(3) * S(3));
                *rot = std::atan2(S(2),S(3));
                *tx = S(0);
                *ty = S(1);
            }
            
            
            
            /* Compute 2D similarity by linear least-squares
             Ref: "Least-Squares Estimation of Transformation Parameters Between Two Point Patterns",
             Shinji Umeyama, IEEE PAMI 13(9), April 1991 (eqs. 40 to 43)
             */
            static int Similarity2DFromNPoints(const Mat &x1,
                                        const Mat &x2,
                                        const Vec& weights,
                                        Vec4 *S)
            {
                assert(x1.rows() == 2 && x2.rows() == 2 && x1.cols() == x2.cols() && weights.rows() == x1.cols());

                const int n = x1.cols() ;
                
                if (n < 2) {
                    return 0; // there must be at least 2 matches
                }
                
                double sumW = 0;
                for (int i = 0; i < weights.rows();++i) {
                    sumW += weights[i];
                }
                if (sumW <= 0) {
                    return 0;
                }
                Vec2 mu1 = (x1 * weights) / sumW;
                Vec2 mu2 = (x2 * weights) / sumW;
                double var1 = 0.;
                //double var2 = 0; // var2 is only used to compute the value of the minimum (Umeyama, eq. (33))
                double covar = 0;
                int nbInliers = 0;
                for (int i = 0; i < n; ++i) {
                    if (weights(i) > 0) {
                        Vec2 dx1 = x1.col(i) - mu1;
                        Vec2 dx2 = x2.col(i) - mu2;
                        var1 += weights(i) * dx1.dot(dx1); //inner_prod(dx1, dx1);
                        //var2 += weights(i)*inner_prod(dx2, dx2);
                        // covar can be updated using outer prod:
                        // (outer_prod (v1, v2)) [i] [j] = v1 [i] * v2 [j]
                        covar += weights(i) * (dx2(1) * dx1(0) - dx2(0) * dx1(1));
                        ++nbInliers;
                    }
                }
                if (var1 == 0) {
                    return 0; // there must be at least 2 distinct points
                }
                if (nbInliers < 2) {
                    return 0; // there must be at least 2 matches
                }
                var1 /= sumW;
                //var2 /= sumW;
                covar /= sumW;
                // TODO: 2x2 SVD algorithm (T=theta, P=phi s=sigma) "direct two-angle method" :
                //[[cos(T) sin(T)][-sin(T) cos(T)]]^T [[a b][c d]] [[cos(P) sin(P)][-sin(P) cos(P)]] = [[s_1 0][0 s_2]]
                //with P+T = atan((c+b)/(d-a)), P-T = atan((c-b)/(d+a)) [be careful with zero denominators]
                //
                /* -*- maple -*- code
                 with(LinearAlgebra):
                 U := <cos(theta),sin(theta) | -sin(theta), cos(theta)>;
                 V := <cos(phi),sin(phi) | -sin(phi), cos(phi)>;
                 Sigma := <sigma[1],0 | 0, sigma[2]>;
                 M:= < a, b | c, d >;
                 phi := (arctan((c+b)/(d-a))+arctan((c-b)/(d+a)))/2;
                 theta := (arctan((c+b)/(d-a))-arctan((c-b)/(d+a)))/2;
                 S := Transpose(U).M.V;
                 */
                
                Mat2 covarMat;
                covarMat(0,0) = covarMat(0,1) = covarMat(1,0) = covarMat(1,1) = covar;

                Eigen::JacobiSVD<Mat2> covsvd(covarMat, Eigen::ComputeFullU | Eigen::ComputeFullV);

                if (covsvd.rank() < 1) {
                    return 0; // probably no two points are distinct
                }
                Mat2 U = covsvd.matrixU();
                Mat2 VT = covsvd.matrixV().transpose();
                Vec2 D = covsvd.singularValues();
                Mat2 R; // actually, we only need one line or column of R
                double c;
                // R = U S V^T
                // c = 1/var1 tr(DS)
                // t = mu2 - c R mu1
                // where S = (1, ..., 1, 1)  if det(U)det(V) = 1
                //       S = (1, ..., 1, -1) if det(U)det(V) = -1
                if (U.determinant() * VT.determinant() > 0) {
                    R = U * VT;
                    c = (D(0) + D(1))/var1;
                } else {
                    // multiply U by S
                    U.col(1) *= -1;
                    R = U * VT;
                    c = (D(0) - D(1))/var1;
                }
                Vec2 t = mu2 - c * R * mu1;
                
                // S = (tx, ty, c.sin(alpha), c.cos(alpha))
                (*S)(0) =  t(0);
                (*S)(1) =  t(1);
                (*S)(2) =  c*R(1,0);
                (*S)(3) =  c*R(0,0);
                
                return 1;
            }
           
            static void Solve(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                Vec4 model;
                Vec weights(x.cols());
                for (int i = 0; i < x.cols(); ++i) {
                    weights(i) = 1.;
                }
                if (Similarity2DFromNPoints(x, y, weights, &model)) {
                    Hs->push_back(model);
                }
            }
            
            /// Beta is the probability that a match is declared inlier by mistake,
            ///  i.e. the ratio of the "inlier surface" by the "total surface".
            /// For the computation of the 2D Similarity:
            ///     The "inlier surface" is a disc of radius InlierThreshold.
            ///     The "total surface" is the surface of the image = width * height
            /// Beta (its ratio) is then
            ///  M_PI*InlierThreshold^2/(width*height)
            static double Beta(double inlierThreshold,double width, double height)
            {
                return std::min(1., M_PI * inlierThreshold * inlierThreshold / (width * height));
            }
            
            /** Sampson's distance for Similarity converted to geometric distance.
             */
            static double Error(const Vec4 &model, const Vec2 &x1, const Vec2 &x2) {
                const double tx = model(0);
                const double ty = model(1);
                const double csina = model(2);
                const double ccosa = model(3);
                const double c = std::sqrt(csina*csina + ccosa*ccosa);
                const double fac = 1./(1.+c);
                const double dx = (x1(0) * ccosa- x1(1) * csina) + tx - x2(0);
                const double dy = (x1(0) * csina + x1(1) * ccosa) + ty - x2(1);
                return std::sqrt(dx * dx + dy * dy) * fac; // be consistent with image distance in each image
            }
            
            static void Normalize(Mat* x1, Mat* x2, Mat3* t1, Mat3* t2, int w1, int h1, int w2, int h2)
            {
                // Use same scale for both axis because this is a similarity
                double s1 = (double)std::max(w1,h1);
                double s2 = (double)std::max(w2,h2);
                for (int i = 0; i < x1->cols(); ++i) {
                    (*x1)(0,i) /= s1;
                    (*x1)(1,i) /= s1;
                    
                    (*x2)(0,i) /= s2;
                    (*x2)(1,i) /= s2;
                }
                // we just store the scale factor in the first member of the matrice
                (*t1)(0,0) = s1;
                (*t2)(0,0) = s2;
            }
            
            
            
            static void Unnormalize(Model* model, const Mat3& t1, const Mat3& t2) {
                
                double s1 = t1(0,0);
                double s2 = t2(0,0);
                (*model)(0) *= s1;
                (*model)(1) *= s2;
                (*model)(2) *= s1 / s2;
                (*model)(2) *= s1 / s2;
            }
           
        };
        
        
        struct Translation2DSolver {
            
            typedef Vec2 Model;
            
            static std::size_t MinimumSamples() { return 1; }
            
            static std::size_t MaximumModels() { return 1; }
            
            enum CoDimensionEnum { CODIMENSION = 2 };
        
            static void Solve(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                Vec2 model;
                model(0) = y(0) - x(0);
                model(1) = y(1) - x(1);
                Hs->push_back(model);
            }
            
            /// Beta is the probability that a match is declared inlier by mistake,
            ///  i.e. the ratio of the "inlier surface" by the "total surface".
            /// For the computation of the 2D Translation:
            ///     The "inlier surface" is a disc of radius InlierThreshold.
            ///     The "total surface" is the surface of the image = width * height
            /// Beta (its ratio) is then
            ///  M_PI*InlierThreshold^2/(width*height)
            static double Beta(double inlierThreshold, double width, double height)
            {
                return std::min(1., M_PI * inlierThreshold * inlierThreshold / (width * height));
            }
            
            static double Error(const Model &model, const Vec2 &x1, const Vec2 &x2) {
                Vec2 tmp = 0.5 * (x1 + model - x2); // be consistent with image distance in each image
                return tmp.norm();
            }
            
            static void Normalize(Mat* x1, Mat* x2, Mat3* t1, Mat3* t2, int w1, int h1, int w2, int h2)
            {
                double s1 = (double)std::max(w1,h1);
                double s2 = (double)std::max(w2,h2);
                for (int i = 0; i < x1->cols(); ++i) {
                    (*x1)(0,i) /= s1;
                    (*x1)(1,i) /= s1;
                    
                    (*x2)(0,i) /= s2;
                    (*x2)(1,i) /= s2;

                }
                
                // we just store the scale factor in the first member of the matrice
                (*t1)(0,0) = s1;
                (*t2)(0,0) = s2;
            }
            
            
            
            static void Unnormalize(Model* model, const Mat3& t1, const Mat3& t2) {
                
                double s1 = t1(0,0);
                double s2 = t2(0,0);
                (*model)(0) *= s1;
                (*model)(1) *= s2;
            }
        };
        
        
        // For each point in the original points, whether it is an inlier or not.
        // This vector should always have the same size as the original data.
        typedef std::vector<bool> InliersVec;
        
        ///
        /**
         * @brief A Kernel to be passed to the prosac() function as template parameter. Note that
         * This kernel adapter is working for translation, similarity, homothety, affine, homography, fundamental matrix estimation.
         * The x1 and x2 set of points must have the same number of points
         **/
        template <typename SolverArg>
        class ProsacKernelAdaptor
        {
        public:
            
            
            typedef SolverArg Solver;
            typedef typename Solver::Model Model;
            
            ProsacKernelAdaptor(const Mat &x1, int w1, int h1,
                                const Mat &x2, int w2, int h2,
                                double sigma = 1. // Estimated noise on points position in pixels, this will be normalized to w1,h1 size
                                )
            : _x1(x1)
            , _x2(x2)
            , _N1(3, 3)
            , _N2(3, 3)
            , beta(0.01)
            , inlierThreshold(0.)
            , maxOutliersProportion(0.8)
            , iMaxIter(-1)
            , iMaxLOIter(-1)
            , eta0(0.05)
            , probability(0.99)
            {
                assert(2 == x1.rows());
                assert(x1.rows() == x2.rows());
                assert(x1.cols() == x2.cols());
                
                Solver::Normalize(&_x1, &_x2, &_N1, &_N2, w1, h1, w2, h2);
                
                assert(w1 != 0 && h1 != 0 && w2 != 0 && h2 != 0);
                double normalizedSigma = sigma / (double)w1 * h1;
                
                inlierThreshold = InlierThreshold<Solver::CODIMENSION>(normalizedSigma);
                beta = Solver::Beta(inlierThreshold, w1, h1);
            }

            static std::size_t MinimumSamples() { return Solver::MinimumSamples(); }
            
            static std::size_t MaximumModels() { return Solver::MinimumSamples(); }
 
            
            void Unnormalize(Model* model) const {
                // Unnormalize model from the computed conditioning.
                Solver::Unnormalize(model, _N1, _N2);
            }
            
            
            /**
             * @brief The number of samples to test on
             **/
            std::size_t NumSamples() const {
                return (std::size_t)_x1.cols();
            }
            
            double getProsacBetaParam() const
            {
                return beta;
            }
            
            /**
             * @brief Computes possible models out of the given samples. The number of samples should be at least of MinimumSamples.
             **/
            void Fit(const std::vector<size_t> &samples, std::vector<Model> *models) const
            {
                const Mat x1 = ExtractColumns(_x1, samples);
                const Mat x2 = ExtractColumns(_x2, samples);
                Solver::Solve(x1, x2, models);
            }
            
            /**
             * @brief Computes the inliers over all samples for the given model that has been previously computed with Fit()
             **/
            int Score(const Model & model, InliersVec* isInlier) const
            {
                assert((int)isInlier->size() == _x1.cols());
                
                int nbInliers = 0;
                for (std::size_t j = 0; j < isInlier->size(); ++j) {
                    double error = Solver::Error(model, _x1.col(j), _x2.col(j));
                    if (error < inlierThreshold) {
                        ++nbInliers;
                        (*isInlier)[j] = true;
                    } else {
                        (*isInlier)[j] = false;
                    }
                }
                return nbInliers;
            }
            
        private:
            
            Mat _x1, _x2;       // Normalized input data
            Mat3 _N1,_N2;
            
            double beta; // beta is the probability that a match is declared inlier by mistake, i.e. the ratio of the "inlier"
            // surface by the total surface. The inlier surface is a disc with radius 1.96s for
            // homography/displacement computation, or a band with width 1.96*s*2 for epipolar geometry (s is the
            // detection noise), and the total surface is the surface of the image.
            
            // The threshold below which a sample error is considered an inlier
            double inlierThreshold;
            
        public:
            /*
             The following members are public as they may be configured by the user after the constructor.
             */
            
            double maxOutliersProportion; // Maximum allowed outliers proportion in the input data: used to compute T_N (can be as high as 0.95)
            // In this implementation, PROSAC won't stop before having reached the
            // corresponding inliers rate on the complete data set.
            
            int iMaxIter; // Maximum number of iterations (number of samples tested), Use -1 to compute it from dMaxOutliersProportion
            
            int iMaxLOIter; // Maximum number of local optimisations done on the inliers (LO-RANSAC).
            
            double eta0; // eta0 is the maximum probability that a solution with more than In_star inliers in Un_star exists and was not found
            // after k samples (typically set to 5%, see Sec. 2.2 of [Chum-Matas-05]).
        
            
            // probability that at least one of the random samples picked up by RANSAC is free of outliers
            double probability;
        };
        
        
    } // namespace robust
} // namespace openMVG
#endif // OPENMVG_ROBUST_ESTIMATOR_PROSAC_KERNEL_ADAPTATOR_H_
