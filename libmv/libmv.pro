#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = LibMV
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt



DEFINES += CERES_HAVE_PTHREAD CERES_NO_SUITESPARSE CERES_NO_CXSPARSE CERES_HAVE_RWLOCK

# Comment to make ceres use a lapack library
DEFINES += CERES_NO_LAPACK

# Uncomment to make ceres use openmp
#DEFINES += CERES_USE_OPENMP

#If undefined, make sure to add to sources all the files in ceres/internal/ceres/generated
DEFINES += CERES_RESTRICT_SCHUR_SPECIALIZATION
DEFINES += WITH_LIBMV_GUARDED_ALLOC GOOGLE_GLOG_DLL_DECL= LIBMV_NO_FAST_DETECTOR=

c++11 {
   DEFINES += CERES_STD_UNORDERED_MAP
   DEFINES += CERES_STD_SHARED_PTR
} else {
   # Use boost::shared_ptr and boost::unordered_map
   CONFIG += boost
   DEFINES += CERES_BOOST_SHARED_PTR
   DEFINES += CERES_BOOST_UNORDERED_MAP
}

include(../global.pri)
include(../config.pri)

INCLUDEPATH += $$PWD/../libs/ceres/config
INCLUDEPATH += $$PWD/../libs/ceres/include
INCLUDEPATH += $$PWD/../libs/ceres/internal
INCLUDEPATH += $$PWD/../libs/gflags
INCLUDEPATH += $$PWD/../libs/gflags/src
INCLUDEPATH += $$PWD/../libs/gflags/src/gflags
INCLUDEPATH += $$PWD/../libs/glog/src
INCLUDEPATH += $$PWD/../libs/libmv/third_party
INCLUDEPATH += $$PWD/../libs/Eigen3
INCLUDEPATH += $$PWD/../libs/libmv


win32* {
     INCLUDEPATH += $$PWD/../libs/glog/src/windows
}

!win32* {
    INCLUDEPATH += $$PWD/../libs/glog/src
}

win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
        INCLUDEPATH += $$PWD/../libs/libmv/third_party/msinttypes
}


SOURCES += \
         ../libs/ceres/internal/ceres/array_utils.cc \
        ../libs/ceres/internal/ceres/blas.cc \
        ../libs/ceres/internal/ceres/block_evaluate_preparer.cc \
        ../libs/ceres/internal/ceres/block_jacobian_writer.cc \
        ../libs/ceres/internal/ceres/block_jacobi_preconditioner.cc \
        ../libs/ceres/internal/ceres/block_random_access_dense_matrix.cc \
        ../libs/ceres/internal/ceres/block_random_access_diagonal_matrix.cc \
        ../libs/ceres/internal/ceres/block_random_access_matrix.cc \
        ../libs/ceres/internal/ceres/block_random_access_sparse_matrix.cc \
        ../libs/ceres/internal/ceres/block_sparse_matrix.cc \
        ../libs/ceres/internal/ceres/block_structure.cc \
        ../libs/ceres/internal/ceres/callbacks.cc \
        ../libs/ceres/internal/ceres/canonical_views_clustering.cc \
        ../libs/ceres/internal/ceres/c_api.cc \
        ../libs/ceres/internal/ceres/cgnr_solver.cc \
        ../libs/ceres/internal/ceres/compressed_col_sparse_matrix_utils.cc \
        ../libs/ceres/internal/ceres/compressed_row_jacobian_writer.cc \
        ../libs/ceres/internal/ceres/compressed_row_sparse_matrix.cc \
        ../libs/ceres/internal/ceres/conditioned_cost_function.cc \
        ../libs/ceres/internal/ceres/conjugate_gradients_solver.cc \
        ../libs/ceres/internal/ceres/coordinate_descent_minimizer.cc \
        ../libs/ceres/internal/ceres/corrector.cc \
        ../libs/ceres/internal/ceres/covariance.cc \
        ../libs/ceres/internal/ceres/covariance_impl.cc \
        ../libs/ceres/internal/ceres/cxsparse.cc \
        ../libs/ceres/internal/ceres/dense_normal_cholesky_solver.cc \
        ../libs/ceres/internal/ceres/dense_qr_solver.cc \
        ../libs/ceres/internal/ceres/dense_sparse_matrix.cc \
        ../libs/ceres/internal/ceres/detect_structure.cc \
        ../libs/ceres/internal/ceres/dogleg_strategy.cc \
        ../libs/ceres/internal/ceres/dynamic_compressed_row_jacobian_writer.cc \
        ../libs/ceres/internal/ceres/dynamic_compressed_row_sparse_matrix.cc \
        ../libs/ceres/internal/ceres/evaluator.cc \
        ../libs/ceres/internal/ceres/file.cc \
        ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_d_d_d.cc \
        ../libs/ceres/internal/ceres/generated/schur_eliminator_d_d_d.cc \
        ../libs/ceres/internal/ceres/gradient_checking_cost_function.cc \
        ../libs/ceres/internal/ceres/gradient_problem.cc \
        ../libs/ceres/internal/ceres/gradient_problem_solver.cc \
        ../libs/ceres/internal/ceres/implicit_schur_complement.cc \
        ../libs/ceres/internal/ceres/incomplete_lq_factorization.cc \
        ../libs/ceres/internal/ceres/iterative_schur_complement_solver.cc \
        ../libs/ceres/internal/ceres/lapack.cc \
        ../libs/ceres/internal/ceres/levenberg_marquardt_strategy.cc \
        ../libs/ceres/internal/ceres/linear_least_squares_problems.cc \
        ../libs/ceres/internal/ceres/linear_operator.cc \
        ../libs/ceres/internal/ceres/linear_solver.cc \
        ../libs/ceres/internal/ceres/line_search.cc \
        ../libs/ceres/internal/ceres/line_search_direction.cc \
        ../libs/ceres/internal/ceres/line_search_minimizer.cc \
        ../libs/ceres/internal/ceres/line_search_preprocessor.cc \
        ../libs/ceres/internal/ceres/local_parameterization.cc \
        ../libs/ceres/internal/ceres/loss_function.cc \
        ../libs/ceres/internal/ceres/low_rank_inverse_hessian.cc \
        ../libs/ceres/internal/ceres/minimizer.cc \
        ../libs/ceres/internal/ceres/normal_prior.cc \
        ../libs/ceres/internal/ceres/parameter_block_ordering.cc \
        ../libs/ceres/internal/ceres/partitioned_matrix_view.cc \
        ../libs/ceres/internal/ceres/polynomial.cc \
        ../libs/ceres/internal/ceres/preconditioner.cc \
        ../libs/ceres/internal/ceres/preprocessor.cc \
        ../libs/ceres/internal/ceres/problem.cc \
        ../libs/ceres/internal/ceres/problem_impl.cc \
        ../libs/ceres/internal/ceres/program.cc \
        ../libs/ceres/internal/ceres/reorder_program.cc \
        ../libs/ceres/internal/ceres/residual_block.cc \
        ../libs/ceres/internal/ceres/residual_block_utils.cc \
        ../libs/ceres/internal/ceres/schur_complement_solver.cc \
        ../libs/ceres/internal/ceres/schur_eliminator.cc \
        ../libs/ceres/internal/ceres/schur_jacobi_preconditioner.cc \
        ../libs/ceres/internal/ceres/scratch_evaluate_preparer.cc \
        ../libs/ceres/internal/ceres/single_linkage_clustering.cc \
        ../libs/ceres/internal/ceres/solver.cc \
        ../libs/ceres/internal/ceres/solver_utils.cc \
        ../libs/ceres/internal/ceres/sparse_matrix.cc \
        ../libs/ceres/internal/ceres/sparse_normal_cholesky_solver.cc \
        ../libs/ceres/internal/ceres/split.cc \
        ../libs/ceres/internal/ceres/stringprintf.cc \
        ../libs/ceres/internal/ceres/suitesparse.cc \
        ../libs/ceres/internal/ceres/triplet_sparse_matrix.cc \
        ../libs/ceres/internal/ceres/trust_region_minimizer.cc \
        ../libs/ceres/internal/ceres/trust_region_preprocessor.cc \
        ../libs/ceres/internal/ceres/trust_region_strategy.cc \
        ../libs/ceres/internal/ceres/types.cc \
        ../libs/ceres/internal/ceres/visibility_based_preconditioner.cc \
        ../libs/ceres/internal/ceres/visibility.cc \
        ../libs/ceres/internal/ceres/wall_time.cc \
        ../libs/libmv/libmv/autotrack/autotrack.cc \
        ../libs/libmv/libmv/autotrack/predict_tracks.cc \
        ../libs/libmv/libmv/autotrack/tracks.cc \
        ../libs/libmv/libmv/image/array_nd.cc \
        ../libs/libmv/libmv/image/convolve.cc \
        ../libs/libmv/libmv/multiview/homography.cc \
        ../libs/libmv/libmv/tracking/track_region.cc \
        ../libs/gflags/src/gflags.cc \
        ../libs/gflags/src/gflags_completions.cc \
        ../libs/gflags/src/gflags_reporting.cc




#Add these source files if doing bundle adjustements, these are template specializations for common cases (6-parameter cameras and 3-parameter points)
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_2.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_3.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_4.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_2_d.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_3.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_4.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_9.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_3_d.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_3.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_4.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_8.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_9.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_4_d.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_2_d_d.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_2.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_3.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_4.cc \
#                ../libs/ceres/internal/ceres/generated/partitioned_matrix_view_4_4_d.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_2_2.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_2_3.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_2_4.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_2_d.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_3_3.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_3_4.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_3_9.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_3_d.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_4_3.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_4_4.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_4_8.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_4_9.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_4_d.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_2_d_d.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_4_4_2.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_4_4_3.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_4_4_4.cc \
#                ../libs/ceres/internal/ceres/generated/schur_eliminator_4_4_d.cc

HEADERS += \
        ../libs/ceres/include/ceres/autodiff_cost_function.h \
        ../libs/ceres/include/ceres/autodiff_local_parameterization.h \
        ../libs/ceres/include/ceres/c_api.h \
        ../libs/ceres/include/ceres/ceres.h \
        ../libs/ceres/include/ceres/conditioned_cost_function.h \
        ../libs/ceres/include/ceres/cost_function.h \
        ../libs/ceres/include/ceres/cost_function_to_functor.h \
        ../libs/ceres/include/ceres/covariance.h \
        ../libs/ceres/include/ceres/crs_matrix.h \
        ../libs/ceres/include/ceres/dynamic_autodiff_cost_function.h \
        ../libs/ceres/include/ceres/dynamic_numeric_diff_cost_function.h \
        ../libs/ceres/include/ceres/fpclassify.h \
        ../libs/ceres/include/ceres/gradient_checker.h \
        ../libs/ceres/include/ceres/gradient_problem.h \
        ../libs/ceres/include/ceres/gradient_problem_solver.h \
        ../libs/ceres/include/ceres/internal/autodiff.h \
        ../libs/ceres/include/ceres/internal/disable_warnings.h \
        ../libs/ceres/include/ceres/internal/eigen.h \
        ../libs/ceres/include/ceres/internal/fixed_array.h \
        ../libs/ceres/include/ceres/internal/macros.h \
        ../libs/ceres/include/ceres/internal/manual_constructor.h \
        ../libs/ceres/include/ceres/internal/numeric_diff.h \
        ../libs/ceres/include/ceres/internal/port.h \
        ../libs/ceres/include/ceres/internal/reenable_warnings.h \
        ../libs/ceres/include/ceres/internal/scoped_ptr.h \
        ../libs/ceres/include/ceres/internal/variadic_evaluate.h \
        ../libs/ceres/include/ceres/iteration_callback.h \
        ../libs/ceres/include/ceres/jet.h \
        ../libs/ceres/include/ceres/local_parameterization.h \
        ../libs/ceres/include/ceres/loss_function.h \
        ../libs/ceres/include/ceres/normal_prior.h \
        ../libs/ceres/include/ceres/numeric_diff_cost_function.h \
        ../libs/ceres/include/ceres/ordered_groups.h \
        ../libs/ceres/include/ceres/problem.h \
        ../libs/ceres/include/ceres/rotation.h \
        ../libs/ceres/include/ceres/sized_cost_function.h \
        ../libs/ceres/include/ceres/solver.h \
        ../libs/ceres/include/ceres/types.h \
        ../libs/ceres/include/ceres/version.h \
        ../libs/ceres/internal/ceres/array_utils.h \
        ../libs/ceres/internal/ceres/blas.h \
        ../libs/ceres/internal/ceres/block_evaluate_preparer.h \
        ../libs/ceres/internal/ceres/block_jacobian_writer.h \
        ../libs/ceres/internal/ceres/block_jacobi_preconditioner.h \
        ../libs/ceres/internal/ceres/block_random_access_dense_matrix.h \
        ../libs/ceres/internal/ceres/block_random_access_diagonal_matrix.h \
        ../libs/ceres/internal/ceres/block_random_access_matrix.h \
        ../libs/ceres/internal/ceres/block_random_access_sparse_matrix.h \
        ../libs/ceres/internal/ceres/block_sparse_matrix.h \
        ../libs/ceres/internal/ceres/block_structure.h \
        ../libs/ceres/internal/ceres/callbacks.h \
        ../libs/ceres/internal/ceres/canonical_views_clustering.h \
        ../libs/ceres/internal/ceres/casts.h \
        ../libs/ceres/internal/ceres/cgnr_linear_operator.h \
        ../libs/ceres/internal/ceres/cgnr_solver.h \
        ../libs/ceres/internal/ceres/collections_port.h \
        ../libs/ceres/internal/ceres/compressed_col_sparse_matrix_utils.h \
        ../libs/ceres/internal/ceres/compressed_row_jacobian_writer.h \
        ../libs/ceres/internal/ceres/compressed_row_sparse_matrix.h \
        ../libs/ceres/internal/ceres/conjugate_gradients_solver.h \
        ../libs/ceres/internal/ceres/coordinate_descent_minimizer.h \
        ../libs/ceres/internal/ceres/corrector.h \
        ../libs/ceres/internal/ceres/covariance_impl.h \
        ../libs/ceres/internal/ceres/cxsparse.h \
        ../libs/ceres/internal/ceres/dense_jacobian_writer.h \
        ../libs/ceres/internal/ceres/dense_normal_cholesky_solver.h \
        ../libs/ceres/internal/ceres/dense_qr_solver.h \
        ../libs/ceres/internal/ceres/dense_sparse_matrix.h \
        ../libs/ceres/internal/ceres/detect_structure.h \
        ../libs/ceres/internal/ceres/dogleg_strategy.h \
        ../libs/ceres/internal/ceres/dynamic_compressed_row_finalizer.h \
        ../libs/ceres/internal/ceres/dynamic_compressed_row_jacobian_writer.h \
        ../libs/ceres/internal/ceres/dynamic_compressed_row_sparse_matrix.h \
        ../libs/ceres/internal/ceres/evaluator.h \
        ../libs/ceres/internal/ceres/execution_summary.h \
        ../libs/ceres/internal/ceres/file.h \
        ../libs/ceres/internal/ceres/gradient_checking_cost_function.h \
        ../libs/ceres/internal/ceres/gradient_problem_evaluator.h \
        ../libs/ceres/internal/ceres/graph_algorithms.h \
        ../libs/ceres/internal/ceres/graph.h \
        ../libs/ceres/internal/ceres/implicit_schur_complement.h \
        ../libs/ceres/internal/ceres/incomplete_lq_factorization.h \
        ../libs/ceres/internal/ceres/integral_types.h \
        ../libs/ceres/internal/ceres/iterative_schur_complement_solver.h \
        ../libs/ceres/internal/ceres/lapack.h \
        ../libs/ceres/internal/ceres/levenberg_marquardt_strategy.h \
        ../libs/ceres/internal/ceres/linear_least_squares_problems.h \
        ../libs/ceres/internal/ceres/linear_operator.h \
        ../libs/ceres/internal/ceres/linear_solver.h \
        ../libs/ceres/internal/ceres/line_search_direction.h \
        ../libs/ceres/internal/ceres/line_search.h \
        ../libs/ceres/internal/ceres/line_search_minimizer.h \
        ../libs/ceres/internal/ceres/line_search_preprocessor.h \
        ../libs/ceres/internal/ceres/low_rank_inverse_hessian.h \
        ../libs/ceres/internal/ceres/map_util.h \
        ../libs/ceres/internal/ceres/minimizer.h \
        ../libs/ceres/internal/ceres/mutex.h \
        ../libs/ceres/internal/ceres/parameter_block.h \
        ../libs/ceres/internal/ceres/parameter_block_ordering.h \
        ../libs/ceres/internal/ceres/partitioned_matrix_view.h \
        ../libs/ceres/internal/ceres/partitioned_matrix_view_impl.h \
        ../libs/ceres/internal/ceres/polynomial.h \
        ../libs/ceres/internal/ceres/preconditioner.h \
        ../libs/ceres/internal/ceres/preprocessor.h \
        ../libs/ceres/internal/ceres/problem_impl.h \
        ../libs/ceres/internal/ceres/program_evaluator.h \
        ../libs/ceres/internal/ceres/program.h \
        ../libs/ceres/internal/ceres/random.h \
        ../libs/ceres/internal/ceres/reorder_program.h \
        ../libs/ceres/internal/ceres/residual_block.h \
        ../libs/ceres/internal/ceres/residual_block_utils.h \
        ../libs/ceres/internal/ceres/schur_complement_solver.h \
        ../libs/ceres/internal/ceres/schur_eliminator.h \
        ../libs/ceres/internal/ceres/schur_eliminator_impl.h \
        ../libs/ceres/internal/ceres/schur_jacobi_preconditioner.h \
        ../libs/ceres/internal/ceres/scratch_evaluate_preparer.h \
        ../libs/ceres/internal/ceres/single_linkage_clustering.h \
        ../libs/ceres/internal/ceres/small_blas.h \
        ../libs/ceres/internal/ceres/solver_utils.h \
        ../libs/ceres/internal/ceres/sparse_matrix.h \
        ../libs/ceres/internal/ceres/sparse_normal_cholesky_solver.h \
        ../libs/ceres/internal/ceres/split.h \
        ../libs/ceres/internal/ceres/stl_util.h \
        ../libs/ceres/internal/ceres/stringprintf.h \
        ../libs/ceres/internal/ceres/suitesparse.h \
        ../libs/ceres/internal/ceres/triplet_sparse_matrix.h \
        ../libs/ceres/internal/ceres/trust_region_minimizer.h \
        ../libs/ceres/internal/ceres/trust_region_preprocessor.h \
        ../libs/ceres/internal/ceres/trust_region_strategy.h \
        ../libs/ceres/internal/ceres/visibility_based_preconditioner.h \
        ../libs/ceres/internal/ceres/visibility.h \
        ../libs/ceres/internal/ceres/wall_time.h \
        ../libs/libmv/libmv/autotrack/autotrack.h \
        ../libs/libmv/libmv/autotrack/callbacks.h \
        ../libs/libmv/libmv/autotrack/frame_accessor.h \
        ../libs/libmv/libmv/autotrack/marker.h \
        ../libs/libmv/libmv/autotrack/model.h \
        ../libs/libmv/libmv/autotrack/predict_tracks.h \
        ../libs/libmv/libmv/autotrack/quad.h \
        ../libs/libmv/libmv/autotrack/reconstruction.h \
        ../libs/libmv/libmv/autotrack/region.h \
        ../libs/libmv/libmv/autotrack/tracks.h \
        ../libs/libmv/libmv/base/aligned_malloc.h \
        ../libs/libmv/libmv/base/id_generator.h \
        ../libs/libmv/libmv/base/scoped_ptr.h \
        ../libs/libmv/libmv/base/vector.h \
        ../libs/libmv/libmv/base/vector_utils.h \
        ../libs/libmv/libmv/image/array_nd.h \
        ../libs/libmv/libmv/image/convolve.h \
        ../libs/libmv/libmv/image/correlation.h \
        ../libs/libmv/libmv/image/image_converter.h \
        ../libs/libmv/libmv/image/image_drawing.h \
        ../libs/libmv/libmv/image/image.h \
        ../libs/libmv/libmv/image/sample.h \
        ../libs/libmv/libmv/image/tuple.h \
        ../libs/libmv/libmv/logging/logging.h \
        ../libs/libmv/libmv/multiview/conditioning.h \
        ../libs/libmv/libmv/multiview/euclidean_resection.h \
        ../libs/libmv/libmv/multiview/fundamental.h \
        ../libs/libmv/libmv/multiview/homography_error.h \
        ../libs/libmv/libmv/multiview/homography.h \
        ../libs/libmv/libmv/multiview/homography_parameterization.h \
        ../libs/libmv/libmv/multiview/nviewtriangulation.h \
        ../libs/libmv/libmv/multiview/panography.h \
        ../libs/libmv/libmv/multiview/panography_kernel.h \
        ../libs/libmv/libmv/multiview/projection.h \
        ../libs/libmv/libmv/multiview/resection.h \
        ../libs/libmv/libmv/multiview/triangulation.h \
        ../libs/libmv/libmv/multiview/two_view_kernel.h \
        ../libs/libmv/libmv/numeric/dogleg.h \
        ../libs/libmv/libmv/numeric/function_derivative.h \
        ../libs/libmv/libmv/numeric/levenberg_marquardt.h \
        ../libs/libmv/libmv/numeric/numeric.h \
        ../libs/libmv/libmv/numeric/poly.h \
        ../libs/libmv/libmv/simple_pipeline/bundle.h \
        ../libs/libmv/libmv/simple_pipeline/callbacks.h \
        ../libs/libmv/libmv/simple_pipeline/camera_intrinsics.h \
        ../libs/libmv/libmv/simple_pipeline/camera_intrinsics_impl.h \
        ../libs/libmv/libmv/simple_pipeline/detect.h \
        ../libs/libmv/libmv/simple_pipeline/distortion_models.h \
        ../libs/libmv/libmv/simple_pipeline/initialize_reconstruction.h \
        ../libs/libmv/libmv/simple_pipeline/intersect.h \
        ../libs/libmv/libmv/simple_pipeline/keyframe_selection.h \
        ../libs/libmv/libmv/simple_pipeline/modal_solver.h \
        ../libs/libmv/libmv/simple_pipeline/pipeline.h \
        ../libs/libmv/libmv/simple_pipeline/reconstruction.h \
        ../libs/libmv/libmv/simple_pipeline/reconstruction_scale.h \
        ../libs/libmv/libmv/simple_pipeline/resect.h \
        ../libs/libmv/libmv/simple_pipeline/tracks.h \
        ../libs/libmv/libmv/tracking/brute_region_tracker.h \
        ../libs/libmv/libmv/tracking/hybrid_region_tracker.h \
        ../libs/libmv/libmv/tracking/kalman_filter.h \
        ../libs/libmv/libmv/tracking/klt_region_tracker.h \
        ../libs/libmv/libmv/tracking/pyramid_region_tracker.h \
        ../libs/libmv/libmv/tracking/region_tracker.h \
        ../libs/libmv/libmv/tracking/retrack_region_tracker.h \
        ../libs/libmv/libmv/tracking/track_region.h \
        ../libs/libmv/libmv/tracking/trklt_region_tracker.h \
        ../libs/gflags/config.h \
        ../libs/gflags/gflags/gflags_completions.h \
        ../libs/gflags/gflags/gflags_declare.h \
        ../libs/gflags/gflags/gflags.h \
        ../libs/gflags/mutex.h \
        ../libs/gflags/util.h

win32 {
SOURCES += \
        ../libs/glog/src/logging.cc \
        ../libs/glog/src/raw_logging.cc \
        ../libs/glog/src/utilities.cc \
        ../libs/glog/src/vlog_is_on.cc \
        ../libs/glog/src/windows/port.cc \
        ../libs/gflags/windows_port.cc

HEADERS += \
        ../libs/glog/src/utilities.h \
        ../libs/glog/src/stacktrace_generic-inl.h \
        ../libs/glog/src/stacktrace.h \
        ../libs/glog/src/stacktrace_x86_64-inl.h \
        ../libs/glog/src/base/googleinit.h \
        ../libs/glog/src/base/mutex.h \
        ../libs/glog/src/base/commandlineflags.h \
        ../libs/glog/src/stacktrace_powerpc-inl.h \
        ../libs/glog/src/stacktrace_x86-inl.h \
        ../libs/glog/src/config.h \
        ../libs/glog/src/stacktrace_libunwind-inl.h \
        ../libs/glog/src/windows/glog/raw_logging.h \
        ../libs/glog/src/windows/glog/vlog_is_on.h \
        ../libs/glog/src/windows/glog/logging.h \
        ../libs/glog/src/windows/glog/log_severity.h \
        ../libs/glog/src/windows/port.h \
        ../libs/glog/src/windows/config.h \
        ../libs/gflags/windows_port.h

} else {
SOURCES += \
        ../libs/glog/src/demangle.cc \
        ../libs/glog/src/logging.cc \
        ../libs/glog/src/raw_logging.cc \
        ../libs/glog/src/signalhandler.cc \
        ../libs/glog/src/symbolize.cc \
        ../libs/glog/src/utilities.cc \
        ../libs/glog/src/vlog_is_on.cc


HEADERS += \
        ../libs/glog/src/base/commandlineflags.h \
        ../libs/glog/src/base/googleinit.h \
        ../libs/glog/src/base/mutex.h \
        ../libs/glog/src/config_freebsd.h \
        ../libs/glog/src/config.h \
        ../libs/glog/src/config_hurd.h \
        ../libs/glog/src/config_linux.h \
        ../libs/glog/src/config_mac.h \
        ../libs/glog/src/demangle.h \
        ../libs/glog/src/glog/logging.h \
        ../libs/glog/src/glog/log_severity.h \
        ../libs/glog/src/glog/raw_logging.h \
        ../libs/glog/src/glog/vlog_is_on.h \
        ../libs/glog/src/stacktrace_generic-inl.h \
        ../libs/glog/src/stacktrace.h \
        ../libs/glog/src/stacktrace_libunwind-inl.h \
        ../libs/glog/src/stacktrace_powerpc-inl.h \
        ../libs/glog/src/stacktrace_x86_64-inl.h \
        ../libs/glog/src/stacktrace_x86-inl.h \
        ../libs/glog/src/symbolize.h \
        ../libs/glog/src/utilities.h
}

win32-msvc* {

HEADERS += \
        ../libs/libmv/third_party/msinttypes/inttypes.h \
        ../libs/libmv/third_party/msinttypes/stdint.h
}
