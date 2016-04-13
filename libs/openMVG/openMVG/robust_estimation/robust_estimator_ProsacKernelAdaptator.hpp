
// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_ROBUST_ESTIMATOR_PROSAC_KERNEL_ADAPTATOR_H_
#define OPENMVG_ROBUST_ESTIMATOR_PROSAC_KERNEL_ADAPTATOR_H_

#include <vector>
#include <cassert>
#include <cmath>
#include <cfloat>
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
        
        inline Vec3 crossprod(const Vec3& p1, const Vec3& p2) {
            return p1.cross(p2);
        }
        
        /**
         * \brief Compute a homography from 4 points correspondences
         * \param p1 source point
         * \param p2 source point
         * \param p3 source point
         * \param p4 source point
         * \param q1 target point
         * \param q2 target point
         * \param q3 target point
         * \param q4 target point
         * \return the homography matrix that maps pi's to qi's
         *
         Using four point-correspondences pi ↔ pi^, we can set up an equation system to solve for the homography matrix H.
         An algorithm to obtain these parameters requiring only the inversion of a 3 × 3 equation system is as follows.
         From the four point-correspondences pi ↔ pi^ with (i ∈ {1, 2, 3, 4}),
         compute h1 = (p1 × p2 ) × (p3 × p4 ), h2 = (p1 × p3 ) × (p2 × p4 ), h3 = (p1 × p4 ) × (p2 × p3 ).
         Also compute h1^ , h2^ , h3^ using the same principle from the points pi^.
         Now, the homography matrix H can be obtained easily from
         H · [h1 h2 h3] = [h1^ h2^ h3^],
         which only requires the inversion of the matrix [h1 h2 h3].
         
         Algo from:
         http://www.dirk-farin.net/phd/text/AB_EfficientComputationOfHomographiesFromFourCorrespondences.pdf
         */
        inline bool
        homography_from_four_points(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, const Vec3 &p4,
                                    const Vec3 &q1, const Vec3 &q2, const Vec3 &q3, const Vec3 &q4,
                                    Mat3 *H)
        {
            Mat3 invHp;
            Mat3 Hp;
            Hp.col(0) = crossprod(crossprod(p1,p2),crossprod(p3,p4));
            Hp.col(1) = crossprod(crossprod(p1,p3),crossprod(p2,p4));
            Hp.col(2) = crossprod(crossprod(p1,p4),crossprod(p2,p3));
            
            double detHp = Hp.determinant();
            if (detHp == 0.) {
                return false;
            }
            Mat3 Hq;
            Hq.col(0) = crossprod(crossprod(q1,q2),crossprod(q3,q4));
            Hq.col(1) = crossprod(crossprod(q1,q3),crossprod(q2,q4));
            Hq.col(2) = crossprod(crossprod(q1,q4),crossprod(q2,q3));
            
            double detHq;
            bool invertible;
            Hp.computeInverseAndDetWithCheck(invHp, detHq,invertible);
            if (!invertible) {
                return false;
            }
            *H = Hq * invHp;
            return true;
        }
        
        
        /**
         In non-normalized coordinates, (0, 0) is the center of the upper-left pixel
         and (w-1, h-1) is the center of the lower-right pixel.
         
         In normalized coordinates, (-1, h/w) is the upper-left corner of the upper-left pixel
         and (1, -h/w) is the lowerright corner of the lower-righ pixel.
         
         image width is w in non-normalized units, 2 in normalized units
         image height is h in non-normalized units, 2h/w in normalized units
         
         normalization matrix =
         [ 2/w,    0, -(w - 1)/w]
         [   0, -2/w,  (h - 1)/w]
         [   0,    0,          1]
         **/
        struct Mat3ModelNormalizer {
            
            static void Normalize(double *x, double *y, int w, int h)
            {
                *x = (2. / w) * *x - (w - 1.) / w;
                *y = -(2. / w) * *y + (h - 1.) / w;
            }
            
            static void Normalize(Mat* normalizedPoints, int w, int h)
            {
                assert(normalizedPoints->rows() == 2);
                for (int i = 0; i < normalizedPoints->cols(); ++i) {
                    Normalize(&(*normalizedPoints)(0,i), &(*normalizedPoints)(1,i), w, h);
                }
            }
            
            static void Normalize(Mat* x1, Mat* x2, int w1, int h1, int /*w2*/, int /*h2*/)
            {
                Mat3ModelNormalizer::Normalize(x1, w1, h1);
                Mat3ModelNormalizer::Normalize(x2, w1, h1);
            }
            
            static void RectificationChangeBoundingBox(const Mat3 &rect,
                                                       double current_up_left_x, double current_up_left_y,
                                                       double current_bottom_right_x, double current_bottom_right_y,
                                                       double new_up_left_x, double new_up_left_y,
                                                       double new_bottom_right_x, double new_bottom_right_y,
                                                       Mat3 *rect_new)
            {
                assert(current_up_left_x != current_bottom_right_x && current_up_left_y != current_bottom_right_y);
                assert(new_up_left_x != new_bottom_right_x && new_up_left_y != new_bottom_right_y);
                
                // Compute transformation A from new bounding box to current bounding box.
                Mat3 A;
                
                homography_from_four_points(Vec3(     new_up_left_x,      new_up_left_y, 1),
                                            Vec3(new_bottom_right_x,      new_up_left_y, 1),
                                            Vec3(new_bottom_right_x, new_bottom_right_y, 1),
                                            Vec3(     new_up_left_x, new_bottom_right_y, 1),
                                            Vec3(     current_up_left_x,      current_up_left_y, 1.),
                                            Vec3(current_bottom_right_x,      current_up_left_y, 1.),
                                            Vec3(current_bottom_right_x, current_bottom_right_y, 1.),
                                            Vec3(     current_up_left_x, current_bottom_right_y, 1.),
                                            &A);
                
                
                Mat3 Ainv;
                double detA;
                bool invertible;
                A.computeInverseAndDetWithCheck(Ainv, detA,invertible, 0.);
                assert(invertible);
                assert(detA != 0.); // homography_from_four_points always returns a non-singular matrix
                
                Mat3 rect_A = rect * A;
                *rect_new = Ainv * rect_A;
            }
            
            static void RectificationNormalizedToCImg(const Mat3 &rect_norm,
                                                      double aspectRatio,
                                                      double width, double height,
                                                      Mat3 *rect_cimg)
            {
                RectificationChangeBoundingBox(rect_norm, -1, 1./aspectRatio, 1, -1./aspectRatio,
                                               -0.5, -0.5, width-0.5, height-0.5, rect_cimg);
            }
            
            static void Unnormalize(Mat3* model, int w1, int h1) {
                RectificationNormalizedToCImg(*model, (double)w1 / (double)h1, w1, h1, model);
                // Unnormalize model from the computed conditioning.
                //*model = t2inv * (*model) * t1;
                
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
            
            static void Normalize(Mat* x1, Mat* x2, int w1, int h1, int w2, int h2)
            {
                Mat3ModelNormalizer::Normalize(x1, x2, w1, h1, w2, h2);
            }
            
            static void Unnormalize(Model* model, int w1, int h1) {
                Mat3ModelNormalizer::Unnormalize(model, w1, h1);
            }

        };

        
        struct Homography2DSolver {
            
            typedef Mat3 Model;
            
            static std::size_t MinimumSamples() { return 4; }
            
            static std::size_t MaximumModels() { return 1; }
            
            enum CoDimensionEnum { CODIMENSION = 2 };
            
            
            /// computes the center and average distance to the center of the columns of M weighted by W
            static void CenterScale(const Mat &M,
                             const Vec &W,
                             Vec2 *m,
                             double *scale)
            {
                
                assert(M.cols() == W.cols() && M.rows() == m->rows());
                double sumW = W.sum();
                *scale = 0.;
                if (sumW == 0.) {
                    m->setZero();
                    return;
                }
                *m = (M * W) / sumW;
                for (int i = 0; i < M.cols(); ++i) {
                    *scale += W(i) * (M.col(i) - *m).norm();
                }
                assert(sumW != 0);
                *scale /= sumW;
            }
            
            /// compute H-matrix using DLT method [HZ, Sec. 4.1, p89]
            static int Homography2DFromNPointsDLT(const Mat &X1,
                                                  const Mat &X2,
                                                  const Vec& weights,
                                                  bool normalize,
                                                  Mat3 *H)
            {
                
                assert(X1.rows() == 2 && X2.rows() == 2 && X1.cols() == X2.cols() && X1.cols() == weights.rows());
                const int n = X1.cols();
                Vec2 center1, center2;
                double scale1 = 1.;
                double scale2 = 1.;
                center1.setZero();
                center2.setZero();
                if (n < 4) {
                    return 0; // there must be at least 4 matches
                }
                if (normalize) {
                    CenterScale(X1, weights, &center1, &scale1);
                    CenterScale(X2, weights, &center2, &scale2);
                    if( scale1 < FLT_EPSILON || scale2 < FLT_EPSILON ) {
                        return 0;
                    }
                    
                    scale1 = std::sqrt(2.)/scale1;
                    scale2 = std::sqrt(2.)/scale2;
                }
                
                
                
                // form a linear system Ax=0: for each selected pair of points m1 & m2,
                // X_i  = (x_i,  y_i,  w_i)
                // X'_i = (x'_i, y'_i, w'_i)
                // A_i = [    O^T     -w'_i.X_i^T  y'_i.X_i^T]
                //       [ w'_i.X_i^T      O^T    -x'_i.X_i^T]
                //     ( h1 ) (first row of H)
                // x = ( h2 ) (second row of H)
                //     ( h3 ) (third row of H)
                // SVD  Decomposition A = UDV^T (destructive wrt A)
                // h is the column of V associated with the smallest singular value of A
                // or h is the eigenvector associated with the smallest eigenvalue of AtA
                typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> ColMajorMat;
                ColMajorMat A(2*n,9);
                
                A.setZero();
                int nbInliers = 0;
                for( int i = 0; i < n; ++i ) {
                    if (weights[i] > 0.) {
                        Vec3 x1h((X1(0,i) - center1(0)) * scale1, (X1(1,i) - center1(1)) * scale1, 1.);
                        Vec2 x2 = (X2.col(i) - center2) * scale2;
                        const double w = weights(i); // each equation is weighted by weights[i]
                        
                        
                        A(nbInliers * 2, 3) = -w * x1h(0);
                        A(nbInliers * 2, 4) = -w * x1h(1);
                        A(nbInliers * 2, 5) = -w * x1h(2);
                        
                        A(nbInliers * 2, 6) = w * x2(1) * x1h(0);
                        A(nbInliers * 2, 7) = w * x2(1) * x1h(1);
                        A(nbInliers * 2, 8) = w * x2(1) * x1h(2);
                        
                        A(nbInliers * 2 + 1, 0) = w * x1h(0);
                        A(nbInliers * 2 + 1, 1) = w * x1h(1);
                        A(nbInliers * 2 + 1, 2) = w * x1h(2);
                        
                        A(nbInliers * 2 + 1, 6) = -w * x2(0) * x1h(0);
                        A(nbInliers * 2 + 1, 7) = -w * x2(0) * x1h(1);
                        A(nbInliers * 2 + 1, 8) = -w * x2(0) * x1h(2);
                        
                        ++nbInliers;
                    }
                }
                if (nbInliers < 4) {
                    return 0;
                }
                A.resize(2*nbInliers,9);
                                 
                typedef Eigen::Matrix<double, 9, 1> Vec9;
                Eigen::JacobiSVD<ColMajorMat> svd(A ,Eigen::ComputeThinU | Eigen::ComputeThinV);
                
                Vec9 b;
                b.setZero();
                Vec9 h = svd.solve(b);
                if (svd.rank() < 8) {
                    return 0; // degenerate case
                }
                
                // take the last column of v as a solution of Af = 0
                H->row(0) = h.segment(0,3);
                H->row(1) = h.segment(3,3);
                H->row(2) = h.segment(6,3);
                
                // apply the transformation that is inverse
                // to what we used to normalize the point coordinates
                if (normalize) {
                    Mat3 T1;
                    T1.setZero();
                    T1(0,0) = scale1;
                    T1(0,2) = -scale1*center1(0);
                    T1(1,1) = scale1;
                    T1(1,2) = -scale1*center1(1);
                    T1(2,2) = 1.;
        
                    Mat3 invT2;
                    invT2.setZero();
                    invT2(0,0) = 1./scale2;
                    invT2(0,2) = center2(0);
                    invT2(1,1) = 1/scale2;
                    invT2(1,2) = center2(1);
                    invT2(2,2) = 1.;
                    // H <- inverse(T1)*H*T0
                    Mat3 HT1 = *H * T1;
                    *H = invT2 * HT1;
                }
                
                // normalize H using the norm_inf, since the Frobenius norm may cause overflows (don't make H(3,3) = 1, since it may be very close to 0)
                double norm = H->lpNorm<Eigen::Infinity>();
                assert(norm != 0);
                *H /= norm;
                
                return 1;
            }
            
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
                assert(x.cols() == y.cols() && x.rows() == y.rows() && x.rows() == 2);
                if (x.cols() == 4) {
                    // Convert to homogeneous coords
                    Mat p(3,4), q(3,4);
                    for (int i = 0; i < 4; ++i) {
                        p(0,i) = x(0,i);
                        p(1,i) = x(1,i);
                        p(2,i) = 1.;
                        
                        q(0,i) = y(0,i);
                        q(1,i) = y(1,i);
                        q(2,i) = 1.;
                    }
                    Mat3 h;
                    if (homography_from_four_points(p.col(0), p.col(1), p.col(2), p.col(3),
                                                    q.col(0), q.col(1), q.col(2), q.col(3),&h)) {
                        Hs->push_back(h);
                    }
                    
                } else {
                    Vec weights = Vec::Ones(x.cols(), 1);
                    Mat3 h;
                    if (Homography2DFromNPointsDLT(x, y, weights, true, &h)) {
                        Hs->push_back(h);
                    }
                }
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
                
                Vec2 t(x1 * h1 + y1 * h2 + h3 - x1 * x2 * h7 - y1 * x2 * h8 - x2 * h9,
                       x1 * h4 + y1 * h5 + h6 - x1 * y2 * h7 - y1 * y2 * h8 - y2 * h9);
                
                double j1 = h1-h7*x2, j2 = h2-h8*x2, j3 = -h7*x1-h8*y1-h9, j4 = h4-h7*y2, j5 =h5-h8*y2;
                                
                Mat J(2,4);
                J << j1, j2, j3, 0,
                    j4, j5, 0, j3;
                
                *dX = J.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(t);

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
            
            static void Normalize(Mat* x1, Mat* x2, int w1, int h1, int w2, int h2)
            {
                Mat3ModelNormalizer::Normalize(x1, x2, w1, h1, w2, h2);
            }
            
            static void Unnormalize(Model* model, int w1, int h1) {
                Mat3ModelNormalizer::Unnormalize(model, w1, h1);
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
                                               Vec4 *similarity)
            {
                assert(x1.rows() == 2 && x2.rows() == 2 && x1.cols() == x2.cols());
                Mat3 Rt = Eigen::umeyama(x1, x2);
                                
                // S = (tx, ty, c.sin(alpha), c.cos(alpha))
                (*similarity)(0) =  Rt(0, 2);
                (*similarity)(1) =  Rt(1, 2);
                (*similarity)(2) =  Rt(1, 0);
                (*similarity)(3) =  Rt(0, 0);
                
                return 1;

                
            }
            
            /*static int Similarity2DFromNPointsWeighted(const Mat &x1,
                                        const Mat &x2,
                                        const Vec& weights,
                                        Vec4 *similarity)
            {
                assert(x1.rows() == 2 && x2.rows() == 2 && x1.cols() == x2.cols() && weights.rows() == x1.cols());
                
                Mat x1_w,x2_w;
                double sumW = 0;
                for (int i = 0; i < weights.rows();++i) {
                    if (weights[i] > 0.) {
                        x1_w.resize(x1.rows(), x1_w.cols() + 1);
                        x2_w.resize(x2.rows(), x2_w.cols() + 1);
                        sumW += weights[i];
                        x1_w(0, i) = x1(0, i) * weights[i];
                        x1_w(1, i) = x1(1, i) * weights[i];
                        x2_w(0, i) = x2(0, i) * weights[i];
                        x2_w(1, i) = x2(1, i) * weights[i];
                    }
                }
                if (sumW <= 0) {
                    return 0;
                }
                
                x1_w /= sumW;
                x2_w /= sumW;
                
                return Similarity2DFromNPoints(x1_w, x2_w, similarity);
                
            }*/
           
            static void Solve(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                Vec4 model;
                if (Similarity2DFromNPoints(x, y, &model)) {
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
            
            static void Normalize(Mat* x1, Mat* x2, int w1, int h1, int /*w2*/, int /*h2*/)
            {
                // Use same scale for both axis because this is a similarity
                double s1 = (double)std::max(w1,h1);
                //double s2 = (double)std::max(w2,h2);
                for (int i = 0; i < x1->cols(); ++i) {
                    (*x1)(0,i) /= s1;
                    (*x1)(1,i) /= s1;
                    
                    (*x2)(0,i) /= s1;
                    (*x2)(1,i) /= s1;
                }
            }
            
            
            
            static void Unnormalize(Model* model, int w1, int h1) {
                
                double s1 = (double)std::max(w1,h1);
                (*model)(0) *= s1;
                (*model)(1) *= s1;
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
            
            static void Normalize(Mat* x1, Mat* x2, int w1, int h1, int /*w2*/, int /*h2*/)
            {
                double s1 = (double)std::max(w1,h1);
                //double s2 = (double)std::max(w2,h2);
                for (int i = 0; i < x1->cols(); ++i) {
                    (*x1)(0,i) /= s1;
                    (*x1)(1,i) /= s1;
                    
                    (*x2)(0,i) /= s1;
                    (*x2)(1,i) /= s1;

                }

            }
           
            static void Unnormalize(Model* model, int w1, int h1) {
                
                double s1 = (double)std::max(w1,h1);
                (*model)(0) *= s1;
                (*model)(1) *= s1;
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
            , _w1(w1)
            , _h1(h1)
            , _w2(w2)
            , _h2(h2)
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
                
                Solver::Normalize(&_x1, &_x2, w1, h1, w2, h2);
                
                assert(w1 != 0 && h1 != 0 && w2 != 0 && h2 != 0);
                double normalizedSigma = sigma / (double)(w1 * h1);
                
                inlierThreshold = InlierThreshold<Solver::CODIMENSION>(normalizedSigma);
                beta = Solver::Beta(inlierThreshold, w1, h1);
            }

            static std::size_t MinimumSamples() { return Solver::MinimumSamples(); }
            
            static std::size_t MaximumModels() { return Solver::MinimumSamples(); }
 
            
            void Unnormalize(Model* model) const {
                // Unnormalize model from the computed conditioning.
                Solver::Unnormalize(model, _w1, _h1);
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
            int _w1,_h1,_w2,_h2;
            
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
