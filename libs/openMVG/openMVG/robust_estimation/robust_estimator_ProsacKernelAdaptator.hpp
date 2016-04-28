
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

// Only c++11 has cbrt so use boost otherwise
#ifdef OPENMVG_HAVE_BOOST
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/cbrt.hpp>
#endif

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

namespace openMVG {
    namespace robust{

        
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
            
            double detHp;
            bool invertible;
            Hp.computeInverseAndDetWithCheck(invHp, detHp,invertible);
            if (!invertible) {
                return false;
            }
            if (detHp == 0.) {
                return false;
            }
            Mat3 Hq;
            Hq.col(0) = crossprod(crossprod(q1,q2),crossprod(q3,q4));
            Hq.col(1) = crossprod(crossprod(q1,q3),crossprod(q2,q4));
            Hq.col(2) = crossprod(crossprod(q1,q4),crossprod(q2,q3));
            if (Hq.determinant() == 0) {
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
            
            static void RectificationNormalizedToNuke(const Mat3 &rect_norm,
                                                      double aspectRatio,
                                                      double width, double height,
                                                      Mat3 *rect_nuke)
            {
                RectificationChangeBoundingBox(rect_norm, -1, 1./aspectRatio, 1, -1./aspectRatio,
                                               0., height, width, 0, rect_nuke);
            }

            
            static void Unnormalize(Mat3* model, int w1, int h1) {
                Mat3 m = *model;
                RectificationNormalizedToCImg(m, (double)w1 / (double)h1, w1, h1, model);
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
        
        /// computes the center and average distance to the center of the columns of M weighted by W
        inline void CenterScale(const Mat &M,
                                const Vec &W,
                                Vec2 *m,
                                double *scale)
        {
            
            assert(M.cols() == W.rows() && M.rows() == m->rows());
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
        
        struct FundamentalSolver {
            
            typedef Mat3 Model;
            
            static std::size_t MinimumSamples() { return openMVG::fundamental::kernel::SevenPointSolver::MINIMUM_SAMPLES; }
            
            static std::size_t MaximumModels() { return openMVG::fundamental::kernel::SevenPointSolver::MAX_MODELS; }
            
            enum CoDimensionEnum { CODIMENSION = 1 };
            
            static void FindCoeffs(const Vec9& f1,
                                   const Vec9& f2,
                                   Vec4 *coeffs)
            {
                /* Maple-generated version */
                /*
                 %VGG_SINGF_FROM_FF  Linearly combines two 3x3 matrices to a singular one.
                 %
                 %   a = vgg_singF_from_FF(F)  computes scalar(s) a such that given two 3x3 matrices F{1} and F{2},
                 %   it is det( a*F{1} + (1-a)*F{2} ) == 0.
                 
                 function a = vgg_singF_from_FF(F)
                 
                 % precompute determinants made from columns of F{1}, F{2}
                 for i1 = 1:2
                 for i2 = 1:2
                 for i3 = 1:2
                 D(i1,i2,i3) = det([F{i1}(:,1) F{i2}(:,2) F{i3}(:,3)]);
                 end
                 end
                 end
                 
                 % Solve The cubic equation for a
                 a = roots([-D(2,1,1)+D(1,2,2)+D(1,1,1)+D(2,2,1)+D(2,1,2)-D(1,2,1)-D(1,1,2)-D(2,2,2)
                 D(1,1,2)-2*D(1,2,2)-2*D(2,1,2)+D(2,1,1)-2*D(2,2,1)+D(1,2,1)+3*D(2,2,2)
                 D(2,2,1)+D(1,2,2)+D(2,1,2)-3*D(2,2,2)
                 D(2,2,2)]);
                 a = a(abs(imag(a))<10*eps);
                 
                 return
                 */
                /*
                 # -*- maple -*-
                 with(LinearAlgebra):
                 F1 := Matrix([[f10,f11,f12],[f13,f14,f15],[f16,f17,f18]]);
                 F2 := Matrix([[f20,f21,f22],[f23,f24,f25],[f26,f27,f28]]);
                 DFa := Determinant(alpha*F1+(1-alpha)*F2);
                 a := coeff(DFa,alpha,3);
                 b := coeff(DFa,alpha,2);
                 c := coeff(DFa,alpha,1);
                 d := coeff(DFa,alpha,0);
                 F111 := Transpose(Matrix([Column(F1,1),Column(F1,2),Column(F1,3)]));
                 F112 := Transpose(Matrix([Column(F1,1),Column(F1,2),Column(F2,3)]));
                 F121 := Transpose(Matrix([Column(F1,1),Column(F2,2),Column(F1,3)]));
                 F122 := Transpose(Matrix([Column(F1,1),Column(F2,2),Column(F2,3)]));
                 F211 := Transpose(Matrix([Column(F2,1),Column(F1,2),Column(F1,3)]));
                 F212 := Transpose(Matrix([Column(F2,1),Column(F1,2),Column(F2,3)]));
                 F221 := Transpose(Matrix([Column(F2,1),Column(F2,2),Column(F1,3)]));
                 F222 := Transpose(Matrix([Column(F2,1),Column(F2,2),Column(F2,3)]));
                 D111 := Determinant(F111);
                 D112 := Determinant(F112);
                 D121 := Determinant(F121);
                 D122 := Determinant(F122);
                 D211 := Determinant(F211);
                 D212 := Determinant(F212);
                 D221 := Determinant(F221);
                 D222 := Determinant(F222);
                 simplify(a-(-D211+D122+D111+D221+D212-D121-D112-D222));
                 simplify(b-(D112-2*D122-2*D212+D211-2*D221+D121+3*D222));
                 simplify(c-(D221+D122+D212-3*D222));
                 simplify(d-D222);
                 simplify(a+b+c+d-D111);
                 with(CodeGeneration):
                 C([coeff0=a,coeff1=b,coeff2=c,coeff3=d]);
                 */
                const double f10 = f1(0);
                const double f11 = f1(1);
                const double f12 = f1(2);
                const double f13 = f1(3);
                const double f14 = f1(4);
                const double f15 = f1(5);
                const double f16 = f1(6);
                const double f17 = f1(7);
                const double f18 = f1(8);
                const double f20 = f2(0);
                const double f21 = f2(1);
                const double f22 = f2(2);
                const double f23 = f2(3);
                const double f24 = f2(4);
                const double f25 = f2(5);
                const double f26 = f2(6);
                const double f27 = f2(7);
                const double f28 = f2(8);
                /* Let the compiler optimize the following code */
                (*coeffs)(0) = f10 * f25 * f17 - f10 * f25 * f27 + f26 * f14 * f12 - f26 * f14 * f22 - f26 * f24 * f12 - f23 * f17 * f12 + f23 * f17 * f22 + f23 * f27 * f12 + f23 * f11 * f18 - f23 * f11 * f28 - f23 * f21 * f18 + f16 * f11 * f15 - f16 * f11 * f25 - f16 * f21 * f15 + f16 * f21 * f25 - f16 * f14 * f12 + f16 * f14 * f22 + f16 * f24 * f12 - f16 * f24 * f22 - f26 * f11 * f15 + f26 * f11 * f25 + f26 * f21 * f15 - f20 * f14 * f18 + f20 * f14 * f28 + f20 * f24 * f18 + f20 * f15 * f17 - f20 * f15 * f27 - f20 * f25 * f17 + f13 * f17 * f12 - f13 * f17 * f22 - f13 * f27 * f12 + f13 * f27 * f22 - f13 * f11 * f18 + f13 * f11 * f28 + f13 * f21 * f18 - f13 * f21 * f28 + f10 * f14 * f18 - f10 * f14 * f28 - f10 * f24 * f18 + f10 * f24 * f28 - f10 * f15 * f17 + f10 * f15 * f27 - f26 * f21 * f25 + f26 * f24 * f22 - f20 * f24 * f28 + f20 * f25 * f27 - f23 * f27 * f22 + f23 * f21 * f28;
                (*coeffs)(1) = -f10 * f25 * f17 + 2 * f10 * f25 * f27 - f26 * f14 * f12 + 2 * f26 * f14 * f22 + 2 * f26 * f24 * f12 + f23 * f17 * f12 - 2 * f23 * f17 * f22 - 2 * f23 * f27 * f12 - f23 * f11 * f18 + 2 * f23 * f11 * f28 + 2 * f23 * f21 * f18 + f16 * f11 * f25 + f16 * f21 * f15 - 2 * f16 * f21 * f25 - f16 * f14 * f22 - f16 * f24 * f12 + 2 * f16 * f24 * f22 + f26 * f11 * f15 - 2 * f26 * f11 * f25 - 2 * f26 * f21 * f15 + f20 * f14 * f18 - 2 * f20 * f14 * f28 - 2 * f20 * f24 * f18 - f20 * f15 * f17 + 2 * f20 * f15 * f27 + 2 * f20 * f25 * f17 + f13 * f17 * f22 + f13 * f27 * f12 - 2 * f13 * f27 * f22 - f13 * f11 * f28 - f13 * f21 * f18 + 2 * f13 * f21 * f28 + f10 * f14 * f28 + f10 * f24 * f18 - 2 * f10 * f24 * f28 - f10 * f15 * f27 + 3 * f26 * f21 * f25 - 3 * f26 * f24 * f22 + 3 * f20 * f24 * f28 - 3 * f20 * f25 * f27 + 3 * f23 * f27 * f22 - 3 * f23 * f21 * f28;
                (*coeffs)(2) = 3 * f23 * f21 * f28 + f16 * f21 * f25 - f16 * f24 * f22 + f26 * f11 * f25 + f26 * f21 * f15 - 3 * f26 * f21 * f25 - f26 * f14 * f22 - f26 * f24 * f12 + 3 * f26 * f24 * f22 + f20 * f14 * f28 + f20 * f24 * f18 - 3 * f20 * f24 * f28 - f20 * f15 * f27 - f20 * f25 * f17 + 3 * f20 * f25 * f27 + f13 * f27 * f22 - f13 * f21 * f28 + f23 * f17 * f22 + f23 * f27 * f12 - 3 * f23 * f27 * f22 - f23 * f11 * f28 - f23 * f21 * f18 + f10 * f24 * f28 - f10 * f25 * f27;
                (*coeffs)(3) = f26 * f21 * f25 - f26 * f24 * f22 + f20 * f24 * f28 - f20 * f25 * f27 + f23 * f27 * f22 - f23 * f21 * f28;

            }
            
            static int SolvePolynomial(const Vec4 &coeffs, Vec3 *sol)
            {
                int nbSol = 0;
                
                // normalization should not be needed, since the null vectors were already normalized
                const double a = coeffs(0);
                const double b = coeffs(1);
                const double c = coeffs(2);
                const double d = coeffs(3);
                
                if( a == 0. ) {
                    if( b == 0. ) {
                        if (c != 0.) {// linear equation
                            (*sol)(nbSol) = -d/c;
                            ++nbSol;
                        }
                    } else {
                        // quadratic equation b.x² + c.x + d = 0
                        const double Delta = c*c - 4*b*d;
                        if (Delta == 0.) {
                            // one double real solution
                            (*sol)(nbSol) = -c/(2*b);
                            ++nbSol;
                        } else if( Delta > 0. ) {
                            // two real solutions
                            const double sqrtDelta = std::sqrt(Delta);
                            (*sol)(nbSol) = (-c-sqrtDelta)/(2*b);
                            ++nbSol;
                            (*sol)(nbSol) = (-c+sqrtDelta)/(2*b);
                            ++nbSol;
                        }
                    }
                } else {
                    const double a2 = std::pow(a,2);
                    const double a3 = a2*a;
                    const double b2 = std::pow(b,2);
                    const double b3 = b2*b;
                    
                    const double q = (3*a*c-b2)/(9*a2);
                    const double r = (9*a*b*c-27*a2*d-2*b3)/(54*a3);
                    
                    const double q3 = std::pow(q,3);
                    const double Delta = q3 + std::pow(r,2);
                    
                    if( Delta >= 0 ) {
                        const double sqrtDelta = std::sqrt(Delta);
                        // one real root and two imaginary
#ifdef OPENMVG_HAVE_BOOST
                        const double s = boost::math::cbrt(r+sqrtDelta);
                        const double t = boost::math::cbrt(r-sqrtDelta);
#else
                        const double s = std::cbrt(r+sqrtDelta);
                        const double t = std::cbrt(r-sqrtDelta);
#endif
                        
                        (*sol)(nbSol) = s + t - b/(3*a);
                        ++nbSol;
                    } else { // Delta < 0
                        // three real roots
                        const double rho = std::sqrt(-q3); // Delta<0, so q3<0
                        const double theta = std::acos(r / rho);
                        const double cbrtrho = std::sqrt(-q); // cubic root of rho is sqrt of -q
                        const double k0 = 2*cbrtrho;
                        const double k1 = theta/3.;
                        const double k2 = b/(3*a);
                        
                        const double pi = M_PI;
                        (*sol)(nbSol) = k0 * std::cos(k1 +2*pi/3) - k2;
                        ++nbSol;
                        (*sol)(nbSol) = k0 * std::cos(k1          ) - k2;
                        ++nbSol;
                        (*sol)(nbSol) = k0 * std::cos(k1 -2*pi/3) - k2;
                        ++nbSol;
                    }
                }
                
                (*sol).resize(nbSol);
                
                return nbSol;
            }
            
            static int FundamentalMatrixFromNullVectors(const Vec9 &f1,
                                                 const Vec9 &f2,
                                                 std::vector<Mat3> *F)
            {
                                
                // Get the cubic coefs
                Vec4 coeffs;
                FindCoeffs(f1, f2, &coeffs);
                
                // Solve the polynomial
                Vec3 sol;
                int nbSol = SolvePolynomial(coeffs, &sol);
                
                assert((int)sol.size() == nbSol);
                F->resize(nbSol);
                
                // Funamental matrix computation
                for( int k = 0; k < nbSol; ++k)
                {
                    // for each root form the fundamental matrix
                    double lambda = sol(k);
                    double mu = 1. - sol(k);
                    
                    Vec9 f = f1 * lambda + f2 * mu;

                    // normalize F using the norm_inf, since the Frobenius norm may cause overflows (don't make F(3,3) = 1, since it may be very close to 0)
                    double n = f.lpNorm<Eigen::Infinity>();
                    f /= n;
                    
                    (*F)[k].row(0) = f.segment(0,3);
                    (*F)[k].row(1) = f.segment(3,3);
                    (*F)[k].row(2) = f.segment(6,3);
            
                }
                
                return nbSol;
                // si une seule matrice trouvee : fmatrix (taille==9)
                // sinon (on a 2 ou 3 sols) : fmatrix est de taille 18 ou 27, et contient les 2 ou 3 matrices une apres l'autre
            }
            
            typedef Eigen::Matrix<double, 7, 9> Matrix79;
            
            static void ComputeMatrixA7(const Mat& X1,
                                        const Mat& X2,
                                        Matrix79 &mA)
            {

                assert(X1.cols() == 7 && X1.rows() == 2 && X2.rows() == 2 && X1.cols() == X2.cols() && mA.rows() == 7 && mA.cols() == 9);
                for (int i = 0; i < 7; ++i) {
                    double x1 = X1(0,i);
                    double y1 = X1(1,i);
                    double x2 = X2(0,i);
                    double y2 = X2(1,i);
 
                    mA(i,0) = x1 * x2;
                    mA(i,1) = y1 * x2;
                    mA(i,2) =      x2;
                    mA(i,3) = x1 * y2;
                    mA(i,4) = y1 * y2;
                    mA(i,5) =      y2;
                    mA(i,6) = x1;
                    mA(i,7) = y1;
                    mA(i,8) = 1;
                }
            }
            
            static int FundamentalMatrixFrom7Points(const Mat& X1,
                                                    const Mat& X2,
                                                    std::vector<Mat3> *F)
            {
                assert(X1.cols() == 7 && X1.rows() == 2 && X2.rows() == 2 && X1.cols() == X2.cols());
                Matrix79 mA;
                ComputeMatrixA7(X1, X2, mA);
                
                //// perform svd decomposition tA = uu.ss.tvv
                //SingularValueDecomposition tsvd(tA, false, true, false); // we only want U
                //// tA = uu . ss . tvv  =>  A = vv . ss . tuu = U . S . tV  => U = vv, V = uu, S = ss
                //matrix<double> V( tsvd.getU());
                
                // perform svd decomposition A = U.S.tV
                Eigen::JacobiSVD<Matrix79> svd(mA, Eigen::ComputeFullV);
                
                if (svd.rank() < 7) {
                    F->resize(0);
                    return 0; // degenerate configuration
                }
                
                // avoid matrix/vector copies, access result of SVD directly
                return FundamentalMatrixFromNullVectors(svd.matrixV().col(7), svd.matrixV().col(8), F);
            }
            
            
            static int FundamentalMatrixFrom8Points(const Mat& X1,
                                             const Mat& X2,
                                             const Vec& weights,
                                             bool normalize,
                                             Mat3 *F)
            {
                assert(X1.rows() == 2 && X2.rows() == 2 && X1.cols() == X2.cols() && weights.rows() == X1.cols());
                
                Vec2 center1, center2;
                double scale1 = 1.;
                double scale2 = 1.;
                center1.setZero();
                center2.setZero();
                const int n = X1.cols();
                if (normalize) {
                    CenterScale(X1, weights, &center1, &scale1);
                    CenterScale(X2, weights, &center2, &scale2);
                    if( scale1 < FLT_EPSILON || scale2 < FLT_EPSILON ) {
                        return 0;
                    }
                    scale1 = std::sqrt(2.)/scale1;
                    scale2 = std::sqrt(2.)/scale2;
                }
                
                
                Mat3 F0;
                // form a linear system Ax=0: for each selected pair of points m1 & m2,
                // the row of A(=a) represents the coefficients of equation: (m2, 1)'*F*(m1, 1) = 0
                // A_i = [m2x*m1x, m2x*m1y, m2x, m2y*m1x, m2y*m1y, m2y, m1x, m1y, 1]
                typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> ColMajorMat;
                ColMajorMat A(n,9);
                
                int nbInliers = 0;
                for (int i = 0; i < n; ++i) {
                    if (weights[i] > 0.) {
                        ++nbInliers;
                        Vec9 r; // r is the (nbInliers-1)-th row of A

                        Vec2 x1 = (X1.col(i) - center1) * scale1;
                        Vec2 x2 = (X2.col(i) - center2) * scale2;
                        
                        r(0) = x1(0)*x2(0);
                        r(1) = x1(1)*x2(0);
                        r(2) =       x2(0);
                        r(3) = x1(0)*x2(1);
                        r(4) = x1(1)*x2(1);
                        r(5) =       x2(1);
                        r(6) = x1(0);
                        r(7) = x1(1);
                        r(8) = 1;
                        // each line in A is weighted by w=weights[i]
                        A.row(nbInliers-1) = weights[i] * r;
                    }
                }
                if (nbInliers < 8) {
                    return 0;
                }
                A.resize(nbInliers,9);
                
                
                typedef Eigen::Matrix<double, 9, 1> Vec9;
                Eigen::JacobiSVD<ColMajorMat> A_svd(A ,Eigen::ComputeFullV);
                
                if (A_svd.rank() < 8) {
                    return 0; // degenerate case
                }
                
                Vec9 f = A_svd.matrixV().rightCols<1>();
                
                
                // take the last column of v as a solution of Af = 0
                F0.row(0) = f.segment(0,3);
                F0.row(1) = f.segment(3,3);
                F0.row(2) = f.segment(6,3);
                

                
                // make F0 singular (of rank 2) by decomposing it with SVD,
                // zeroing the last diagonal element of W and then composing the matrices back.

                {
                    // perform svd decomposition A = U.W.tV
                    Eigen::JacobiSVD<Mat3> tsvd(F0, Eigen::ComputeFullU | Eigen::ComputeFullV);
                    
                    if (tsvd.rank() < 2) {
                        return 0; // degenerate case
                    }
                    Vec3 W(tsvd.singularValues());
                    
                    Mat3 UW = tsvd.matrixU() * Eigen::DiagonalMatrix<double,3,3>(W(0),W(1),0.);
                    F0 = UW * tsvd.matrixV().transpose();
                }
                
                // apply the transformation that is inverse
                // to what we used to normalize the point coordinates
                if (normalize) {
                    Mat3 T1;
                    T1 << scale1, 0, -scale1*center1(0),
                    0, scale1, -scale1*center1(1),
                    0, 0, 1;
                    
                    Mat3 T2;
                    T2 << scale2, 0, -scale2*center2(0),
                    0, scale2, -scale2*center2(1),
                    0, 0, 1;
                    
                    // F0 <- T1'*F0*T0
                    Mat3 F0T1 = F0 * T1;
                    F0 = T2.transpose() * F0T1;
                }
                
                // normalize F using the norm_inf, since the Frobenius norm may cause overflows (don't make F(3,3) = 1, since it may be very close to 0)
                *F = (F0 / F0.lpNorm<Eigen::Infinity>());
                
                return 1;
            }

            
            /** 3D rigid transformation estimation (7 dof) with z=1
             * Compute a Scale Rotation and Translation rigid transformation.
             * This transformation provide a distortion-free transformation
             * using the following formulation Xb = S * R * Xa + t.
             * "Least-squares estimation of transformation parameters between two point patterns",
             * Shinji Umeyama, PAMI 1991, DOI: 10.1109/34.88573
             */
            static void ComputeModelFromMinimumSamples(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                FundamentalMatrixFrom7Points(x, y, Hs);
            }
            
            static int ComputeModelFromNSamples(const Mat& x, const Mat& y, const Vec& weights, Model* H)
            {
                return FundamentalMatrixFrom8Points(x, y , weights, true, H);
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
                    
                    scale1 = std::sqrt(2.) / scale1;
                    scale2 = std::sqrt(2.) / scale2;
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
                Eigen::JacobiSVD<ColMajorMat> svd(A ,Eigen::ComputeFullV);

                if (svd.rank() < 8) {
                    return 0; // degenerate case
                }
                
                Vec9 h = svd.matrixV().rightCols<1>();

                
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
            static void ComputeModelFromMinimumSamples(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                assert(x.cols() == y.cols() && x.rows() == y.rows() && x.rows() == 2);
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
                
                
            }
            
            static int ComputeModelFromNSamples(const Mat& x, const Mat& y, const Vec& weights, Model* H)
            {
                
                return Homography2DFromNPointsDLT(x, y, weights, true, H);
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
            
            static int Similarity2DFromNPointsWeighted(const Mat &x1,
                                                       const Mat &x2,
                                                       const Vec& weights,
                                                       Vec4 *similarity)
            {
                assert(x1.rows() == 2 && x2.rows() == 2 && x1.cols() == x2.cols() && weights.rows() == x1.cols());
                
                
                const int m = x1.rows(); // dimension
                const int n = x1.cols(); // number of measurements
                
                if (n < 2) {
                    return 0;
                }
                
                double sumW = weights.sum();
                if (sumW <= 0) {
                    return 0;
                }

                
                // required for demeaning ...
                const double one_over_n = 1. / (double)n;
                
                // computation of mean
                const Vec2 src_mean = (x1 * weights).rowwise().sum() * one_over_n;
                const Vec2 dst_mean = (x2 * weights).rowwise().sum() * one_over_n;
                
                // demeaning of src and dst points
                const Mat src_demean = x1.colwise() - src_mean;
                const Mat dst_demean = x2.colwise() - dst_mean;
                
                // Eq. (36)-(37)
                double src_var = 0;
                Mat2 sigma;
                sigma.setZero();
                int nbInliers = 0;
                for (int i = 0; i < n; ++i) {
                    if (weights(i) > 0) {
                        Vec2 dx1 = x1.col(i) - src_mean;
                        Vec2 dx2 = x2.col(i) - dst_mean;
                        src_var += (weights(i) * dx1.dot(dx1));
                        // covar can be updated using outer prod:
                        // (outer_prod (v1, v2)) [i] [j] = v1 [i] * v2 [j]
                        sigma += weights(i) * (dx2 * dx1.transpose());
                        ++nbInliers;
                    }
                }
                
                //const double src_var = src_demean.rowwise().squaredNorm().sum() * one_over_n;
                
                if (src_var == 0) {
                    return 0; // there must be at least 2 distinct points
                }
                
                src_var /= sumW;
                sigma /= sumW;
                
                // Eq. (38)
                //const Mat2 sigma = one_over_n * dst_demean * src_demean.transpose();
                
                Eigen::JacobiSVD<Mat2> svd(sigma, Eigen::ComputeFullU | Eigen::ComputeFullV);
                
                // Initialize the resulting transformation with an identity matrix...
                Mat3 Rt = Mat3::Identity(m+1,m+1);
                
                // Eq. (39)
                Vec S = Vec::Ones(m);
                if (sigma.determinant() < 0.) {
                    S(m - 1) = -1.;
                }
                
                // Eq. (40) and (43)
                const Vec& d = svd.singularValues();
                int rank = 0;
                for (int i = 0; i < m; ++i) {
                    if (!Eigen::internal::isMuchSmallerThan(d.coeff(i),d.coeff(0))) {
                        ++rank;
                    }
                }
                if (rank == m-1) {
                    if ( svd.matrixU().determinant() * svd.matrixV().determinant() > 0. ) {
                        Rt.block(0,0,m,m).noalias() = svd.matrixU() * svd.matrixV().transpose();
                    } else {
                        const double s = S(m - 1);
                        S(m - 1) = -1.;
                        Rt.block(0,0,m,m).noalias() = svd.matrixU() * S.asDiagonal() * svd.matrixV().transpose();
                        S(m - 1) = s;
                    }
                } else {
                    Rt.block(0,0,m,m).noalias() = svd.matrixU() * S.asDiagonal() * svd.matrixV().transpose();
                }
                
                
                // Eq. (42)
                const double c = 1. / src_var * svd.singularValues().dot(S);
                
                // Eq. (41)
                Vec2 t = dst_mean - c * Rt.topLeftCorner(m,m) * src_mean;
                // S = (tx, ty, c.sin(alpha), c.cos(alpha))
                (*similarity)(0) =  t(0);
                (*similarity)(1) =  t(1);
                (*similarity)(2) =  c * Rt(1,0);
                (*similarity)(3) =  c * Rt(0,0);
                return 1;
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
            
           
           
            static void ComputeModelFromMinimumSamples(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                Vec4 model;
                if (Similarity2DFromNPoints(x, y, &model)) {
                    Hs->push_back(model);
                }
            }
            
            static int ComputeModelFromNSamples(const Mat& x, const Mat& y, const Vec& weights, Model* H)
            {
                
                return Similarity2DFromNPointsWeighted(x, y, weights, H);
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
        
            static int translationFromNPoints(const Mat& x1, const Mat& x2, const Vec& weights, Vec2* translation)
            {
                assert(x1.rows() == 2 && x2.rows() == 2 && x1.cols() == x2.cols() && weights.rows() == x1.cols());
                const int n = x1.cols();
                if (n < 1) {
                    return 0; // there must be at least 1 match
                }
                
                int nbInliers = 0;
                double sum = 0.;
                for( int i = 0; i < n; ++i ) {
                    assert(weights[i] >= 0.);
                    if (weights[i] > 0.) {
                        sum += weights[i];
                        ++nbInliers;
                    }
                }
                if (nbInliers < 1) {
                    return 0;
                }
                assert(sum != 0);
                *translation = ((x2 - x1) * weights) / sum;
                return 1;
            }
            
            static void ComputeModelFromMinimumSamples(const Mat &x, const Mat &y, std::vector<Model> *Hs)
            {
                Vec2 model;
                model(0) = y(0) - x(0);
                model(1) = y(1) - x(1);
                Hs->push_back(model);
            }
            
            static int ComputeModelFromNSamples(const Mat& x, const Mat& y, const Vec& weights, Model* H)
            {
                
                return translationFromNPoints(x, y, weights, H);
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
        
      
        /// compute the value of the median element of the first n elements of an array
        /// get median in O(n) in the average case. Worst case is quadratic (O(n^2)).
        /// Similar to quicksort, but throw away useless "half" at each iteration.
        /// Warning: modifies the input array.
        /// Note: the Blum-Floyd-Pratt-Rivest-Tarjan (BFPRT) "Median of medians"
        /// algorithm gives worst-case O(n) time, but it's much slower.
        /// the order of the n first elements in the input array (parameter arr) is changed by this function
        inline double Median(Vec &arr, int n)
        {
            int low, high ;
            int median;
            int middle, ll, hh;
            
            low = 0 ; high = n-1 ; median = n/2; //(low + high) / 2;
            for (;;) {
                if (high <= low) /* One element only */
                {
                    double ret_value = arr[median];
                    return ret_value;
                }
                
                if (high == low + 1) {  /* Two elements only */
                    if (arr[low] > arr[high])
                        std::swap(arr[low], arr[high]) ;
                    
                    double ret_value = arr[median];
                    return ret_value;
                }
                
                /* Find median of low, middle and high items; swap into position low */
                middle = (low + high + 1) / 2; //(low + high) / 2;
                if (arr[middle] > arr[high])    std::swap(arr[middle], arr[high]) ;
                if (arr[low] > arr[high])       std::swap(arr[low], arr[high]) ;
                if (arr[middle] > arr[low])     std::swap(arr[middle], arr[low]) ;
                
                /* Swap low item (now in position middle) into position (low+1) */
                std::swap(arr[middle], arr[low+1]) ;
                
                /* Nibble from each end towards middle, swapping items when stuck */
                ll = low + 1;
                hh = high;
                for (;;) {
                    do ++ll; while (arr[low] > arr[ll]) ;
                    do --hh; while (arr[hh]  > arr[low]) ;
                    
                    if (hh < ll)
                        break;
                    
                    std::swap(arr[ll], arr[hh]) ;
                }
                
                /* Swap middle item (in position low) back into correct position */
                std::swap(arr[low], arr[hh]) ;
                
                /* Re-set active partition */
                if (hh <= median)
                    low = ll;
                if (hh >= median)
                    high = hh - 1;
            }
        }
        
        /// compute sigma_MAD from the first n elements of an error vector
        /// the input array (parameter arr) is changed by this function
        /// reference on sigmaMAD: http://en.wikipedia.org/wiki/Median_absolute_deviation
        inline double SigmaMAD(Vec &arr, int n)
        {
            double dMedian = Median(arr, n);
            
            for (int i = 0; i < n; ++i) {
                // the order of the elements of arr were change by the previous call to Median,
                // but they are still all there, let's reuse them.
                arr[i] = std::abs(dMedian - arr[i]);
            }
            
            return 1.4826 * Median(arr, n);
        }
        
        /// compute sigma_MAD from the inliers (indicated by the dataInliers argument) of an error vector.
        /// reference on sigmaMAD: http://en.wikipedia.org/wiki/Median_absolute_deviation
        inline double SigmaMAD(const Vec &errors, const InliersVec &inliers)
        {
            const int n = (int)inliers.size(); // the dataset size
            Vec errorsMAD(n);
            int j = 0;
            for (int i = 0; i < n; ++i) {
                if (inliers[i]) {
                    errorsMAD[j] = errors[i];
                    ++j;
                }
            }
            return SigmaMAD(errorsMAD, j); // only consider the j first elts of errorsMAD
        }

        
       
        
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
                
                double normalizedSigma = sigma * 2 / (double)(w1);
                
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
            void ComputeModelFromMinimumSamples(const std::vector<size_t> &samples, std::vector<Model> *models) const
            {
                const Mat x1 = ExtractColumns(_x1, samples);
                const Mat x2 = ExtractColumns(_x2, samples);
                Solver::ComputeModelFromMinimumSamples(x1, x2, models);
            }
            
            // Convenience function to call when they are a number of samples equal to MinmumSamples()
            // Do not call if you have more samples
            void ComputeModelFromMinimumSamples(std::vector<Model> *models) const
            {
                assert(_x1.cols() == (int)MinimumSamples());
                Solver::ComputeModelFromMinimumSamples(_x1, _x2, models);
            }
            
            // Computes a model from N samples with input weights. This is to be called only when there are
            // more samples than the minmum samples count
            int ComputeModelFromNSamples(const Mat& x1, const Mat& x2, const Vec& weights, Model* model) const
            {
                assert(_x1.cols() > (int)MinimumSamples());
                return Solver::ComputeModelFromNSamples(x1, x2, weights, model);
            }
            
            // Computes a model from N samples. This is to be called only when there are
            // more samples than the minmum samples count
            int ComputeModelFromAllSamples(Model* model) const
            {
                Vec weights(_x1.cols(), 1);
                weights.setOnes();
                assert(_x1.cols() > (int)MinimumSamples());
                return Solver::ComputeModelFromNSamples(_x1, _x2, weights, model);
            }
            
            /**
             * @brief Computes the inliers over all samples for the given model that has been previously computed with ComputeModelFromMinimumSamples()
             **/
            int ComputeInliersForModel(const Model & model, InliersVec* isInlier) const
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
            
            
            
            // LO-RANSAC
            /* from Chum et al. "ENHANCING RANSAC BY GENERALIZED MODEL OPTIMIZATION" ACCV'04:
             "Different methods of the best model optimization with respect to the two view geometry estimation
             were proposed and tested [1]. The following procedure performed the best.
             Constant number (twenty in our experiments) of samples are drawn only from Ik, while the verification
             is performed on the set of all data points U (so called ‘inner’ RANSAC). Since the proportion of
             inliers in Ik is high, there is no need for the size of sample to be minimal. The problem has shifted
             from minimizing the probability of including an outlier into the sample (the reason for choosing
             minimal sample size) to the problem of reduction of the influence of the noise on model parameters.
             The size of the sample is therefore selected to maximize the probability of drawing an α-sample.
             In a final step, model parameters are ‘polished’ by an iterative reweighted least squares technique."
             [1] O. Chum, J. Matas, and J. Kittler. Locally optimized ransac. In Proc. DAGM. Springer-Verlag, 2003.
             
             A wide-baseline configuration with a large number of outliers would benefit from implementing the
             above method.
             
             The following just implements the "simple" (number 2) method from [1], but it seems to be enough for
             a small-baseline stereo configuration.
             
             TODO: [1] says "Method 3 [Iterative] might be used in real-time procedures when a high number of
             inliers is expected."
             
             Note on radial distortion:
             Division model, Fitzgibbon CVPR’01: 9 point correspondences define the model
             LO Ransac:
             - Approximated by EG with no radial distortion
             - In LO step, estimate the full model from 9 (or more!) points 
             
             */
            bool OptimizeModel(const Model& model, const InliersVec& inliers, Model* optimizedModel) const
            {
                return MEstimatorIteration(model, inliers, optimizedModel);
            }
            
            /// full Mestimator
            /// returns the number of successful iterations
            int MEstimator(const Model &model, const InliersVec &inliers, int maxNbIterations, Model* optimizedModel, double *sigmaMAD_p = 0) const
            {
                const size_t n = inliers.size(); // the dataset size
                Vec errors(n);
                for (std::size_t i = 0; i < inliers.size(); ++i) {
                    if (inliers[i]) {
                        errors(i) = Solver::Error(model, _x1.col(i), _x2.col(i));
                    } else {
                        errors(i) = 0.;
                    }
                    
                }
                
                double sigmaMAD = SigmaMAD(errors, inliers);
                double sigmaMAD_old = 0.;
                bool succeeded;
                int i = 0;
                *optimizedModel = model;
                do {
                    if (sigmaMAD > 0.) {
                        succeeded = MEstimatorIterationFromErrors(*optimizedModel, errors, inliers, sigmaMAD, optimizedModel);
                    } else {
                        succeeded = false;
                        ++i; // sigmaMAD is 0: mark the model as optimized, although we didn't compute anything
                    }
                    if (succeeded) {
                        ++i;
                        
                        for (std::size_t i = 0; i < inliers.size(); ++i) {
                            if (inliers[i]) {
                                errors(i) = Solver::Error(*optimizedModel, _x1.col(i), _x2.col(i));
                            } else {
                                errors(i) = 0.;
                            }
                            
                        }
                        
                        sigmaMAD = SigmaMAD(errors, inliers);
                        sigmaMAD_old = sigmaMAD;
                    }
                } while (i < maxNbIterations && succeeded && sigmaMAD < sigmaMAD_old);
                if (sigmaMAD_p) {
                    *sigmaMAD_p = sigmaMAD;
                }
                return i;
            }
            
            
        private:
            
            
            /// Given matches and a model, compute weights for iterated
            /// least-squares or M-Estimator using the Pseudo-Huber function
            /// Ref.: [HZ] annex A6.8
            static void WeightsPseudoHuberFromErrors(const Mat& x1,
                                                     const Mat& x2,
                                                     const Vec &errors,
                                                     const InliersVec& inliers,
                                                     double sigma,
                                                     Vec *weights,
                                                     Mat* x1Out,
                                                     Mat* x2Out)
            {
                assert(errors.rows() == (int)inliers.size() && weights  && sigma > 0.);
                
                const double b = InlierThreshold<Solver::CODIMENSION>(sigma); // 95 percentile of the Gaussian
                
                int nInliers = std::accumulate(inliers.begin(), inliers.end(), 0);
                weights->resize(nInliers, 1);
                x1Out->resize(2, nInliers);
                x2Out->resize(2, nInliers);
                
                int inlierIndex = 0;
                for (int i = 0; i < errors.rows(); ++i) {
                    if (!inliers[i]) {
                        continue;
                    } else {
                        
                        // We use the Pseudo-Huber function
                        double d = std::abs(errors(i));
                        double d_over_b = d / b;
                        double cost = 2 * (b * b) * (std::sqrt(1 + d_over_b * d_over_b) - 1.);
                        if (d == 0. || cost <= 0.) {
                            (*weights)(inlierIndex) = 1. / sigma;
                        } else {
                            (*weights)(inlierIndex) = std::sqrt(cost) / (d * sigma);
                        }
                        x1Out->col(inlierIndex) = x1.col(i);
                        x2Out->col(inlierIndex) = x2.col(i);
                        ++inlierIndex;
                    }
                }
            }
            
            
            /// Compute a single iteration of the M-estimor robust optimization algorithm,
            /// M-estimator is implemented as reweighted least-squares. The weights are
            /// computed from a robust cost function. The residuals scale is
            /// evaluated using median absolute deviation (MAD).
            /// The cost function is the Pseudo-Huber cost function, which behaves like d^2
            /// for small d, and abs(d) for large d.
            /// Reference: Hartley & Zisserman sec. A6.8
            bool MEstimatorIterationFromErrors(const Model &model, const Vec& errors, const InliersVec &inliers, double sigmaMAD, Model* optimizedModel) const
            {
                // Iterating over it does iteratively reweighted least squares (IRLS)
                Vec weights;
                Mat x1Inlier,x2Inlier;
                WeightsPseudoHuberFromErrors(_x1, _x2, errors, inliers, sigmaMAD, &weights, &x1Inlier, &x2Inlier);
                assert(weights.rows() == x1Inlier.cols() && x1Inlier.cols() == x2Inlier.cols());
                *optimizedModel = model; //initialize model in case it needs a starting point
                int res = Solver::ComputeModelFromNSamples(x1Inlier, x2Inlier, weights, optimizedModel);
                return (res == 1);
            }
            
            bool MEstimatorIteration(const Model &model, const InliersVec &inliers, Model* optimizedModel, double *sigmaMAD_p = 0) const
            {
                const size_t n = inliers.size(); // the dataset size
                Vec errors(n);
                for (std::size_t i = 0; i < inliers.size(); ++i) {
                    if (inliers[i]) {
                        errors(i) = Solver::Error(model, _x1.col(i), _x2.col(i));
                    } else {
                        errors(i) = 0.;
                    }

                }
                double sigmaMAD = SigmaMAD(errors, inliers);
                if (sigmaMAD_p) {
                    *sigmaMAD_p = sigmaMAD;
                }
                // if sigmaMAD = 0. the model is already optimal
                if (sigmaMAD > 0.) {
                    return MEstimatorIterationFromErrors(model, errors, inliers, sigmaMAD, optimizedModel);
                }
                *optimizedModel = model;
                return false;
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
