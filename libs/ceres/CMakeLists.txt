# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

# Ceres Solver 2.2.0, built as upstream's CMake build does with
# MINIGLOG=ON SCHUR_SPECIALIZATIONS=OFF LAPACK=OFF SUITESPARSE=OFF EIGENSPARSE=OFF
# USE_CUDA=OFF (see config/ceres/internal/config.h and README.Natron).
set(ceres_HEADERS
    config/ceres/internal/config.h
    config/ceres/internal/export.h
    include/ceres/autodiff_cost_function.h
    include/ceres/autodiff_first_order_function.h
    include/ceres/autodiff_manifold.h
    include/ceres/c_api.h
    include/ceres/ceres.h
    include/ceres/conditioned_cost_function.h
    include/ceres/constants.h
    include/ceres/context.h
    include/ceres/cost_function_to_functor.h
    include/ceres/cost_function.h
    include/ceres/covariance.h
    include/ceres/crs_matrix.h
    include/ceres/cubic_interpolation.h
    include/ceres/dynamic_autodiff_cost_function.h
    include/ceres/dynamic_cost_function_to_functor.h
    include/ceres/dynamic_cost_function.h
    include/ceres/dynamic_numeric_diff_cost_function.h
    include/ceres/evaluation_callback.h
    include/ceres/first_order_function.h
    include/ceres/gradient_checker.h
    include/ceres/gradient_problem_solver.h
    include/ceres/gradient_problem.h
    include/ceres/internal/array_selector.h
    include/ceres/internal/autodiff.h
    include/ceres/internal/disable_warnings.h
    include/ceres/internal/eigen.h
    include/ceres/internal/euler_angles.h
    include/ceres/internal/fixed_array.h
    include/ceres/internal/householder_vector.h
    include/ceres/internal/integer_sequence_algorithm.h
    include/ceres/internal/jet_traits.h
    include/ceres/internal/line_parameterization.h
    include/ceres/internal/memory.h
    include/ceres/internal/numeric_diff.h
    include/ceres/internal/parameter_dims.h
    include/ceres/internal/port.h
    include/ceres/internal/reenable_warnings.h
    include/ceres/internal/sphere_manifold_functions.h
    include/ceres/internal/variadic_evaluate.h
    include/ceres/iteration_callback.h
    include/ceres/jet_fwd.h
    include/ceres/jet.h
    include/ceres/line_manifold.h
    include/ceres/loss_function.h
    include/ceres/manifold.h
    include/ceres/normal_prior.h
    include/ceres/numeric_diff_cost_function.h
    include/ceres/numeric_diff_first_order_function.h
    include/ceres/numeric_diff_options.h
    include/ceres/ordered_groups.h
    include/ceres/problem.h
    include/ceres/product_manifold.h
    include/ceres/rotation.h
    include/ceres/sized_cost_function.h
    include/ceres/solver.h
    include/ceres/sphere_manifold.h
    include/ceres/tiny_solver_autodiff_function.h
    include/ceres/tiny_solver_cost_function_adapter.h
    include/ceres/tiny_solver.h
    include/ceres/types.h
    include/ceres/version.h
    internal/ceres/accelerate_sparse.h
    internal/ceres/array_utils.h
    internal/ceres/block_evaluate_preparer.h
    internal/ceres/block_jacobi_preconditioner.h
    internal/ceres/block_jacobian_writer.h
    internal/ceres/block_random_access_dense_matrix.h
    internal/ceres/block_random_access_diagonal_matrix.h
    internal/ceres/block_random_access_matrix.h
    internal/ceres/block_random_access_sparse_matrix.h
    internal/ceres/block_sparse_matrix.h
    internal/ceres/block_structure.h
    internal/ceres/callbacks.h
    internal/ceres/canonical_views_clustering.h
    internal/ceres/casts.h
    internal/ceres/cgnr_solver.h
    internal/ceres/compressed_col_sparse_matrix_utils.h
    internal/ceres/compressed_row_jacobian_writer.h
    internal/ceres/compressed_row_sparse_matrix.h
    internal/ceres/concurrent_queue.h
    internal/ceres/conjugate_gradients_solver.h
    internal/ceres/context_impl.h
    internal/ceres/coordinate_descent_minimizer.h
    internal/ceres/corrector.h
    internal/ceres/covariance_impl.h
    internal/ceres/cuda_block_sparse_crs_view.h
    internal/ceres/cuda_block_structure.h
    internal/ceres/cuda_buffer.h
    internal/ceres/cuda_partitioned_block_sparse_crs_view.h
    internal/ceres/cuda_sparse_matrix.h
    internal/ceres/cuda_vector.h
    internal/ceres/dense_cholesky.h
    internal/ceres/dense_jacobian_writer.h
    internal/ceres/dense_normal_cholesky_solver.h
    internal/ceres/dense_qr_solver.h
    internal/ceres/dense_qr.h
    internal/ceres/dense_sparse_matrix.h
    internal/ceres/detect_structure.h
    internal/ceres/dogleg_strategy.h
    internal/ceres/dynamic_compressed_row_finalizer.h
    internal/ceres/dynamic_compressed_row_jacobian_writer.h
    internal/ceres/dynamic_compressed_row_sparse_matrix.h
    internal/ceres/dynamic_sparse_normal_cholesky_solver.h
    internal/ceres/eigen_vector_ops.h
    internal/ceres/eigensparse.h
    internal/ceres/evaluator.h
    internal/ceres/execution_summary.h
    internal/ceres/fake_bundle_adjustment_jacobian.h
    internal/ceres/file.h
    internal/ceres/float_suitesparse.h
    internal/ceres/function_sample.h
    internal/ceres/gradient_checking_cost_function.h
    internal/ceres/gradient_problem_evaluator.h
    internal/ceres/graph_algorithms.h
    internal/ceres/graph.h
    internal/ceres/implicit_schur_complement.h
    internal/ceres/inner_product_computer.h
    internal/ceres/invert_psd_matrix.h
    internal/ceres/is_close.h
    internal/ceres/iterative_refiner.h
    internal/ceres/iterative_schur_complement_solver.h
    internal/ceres/levenberg_marquardt_strategy.h
    internal/ceres/line_search_direction.h
    internal/ceres/line_search_minimizer.h
    internal/ceres/line_search_preprocessor.h
    internal/ceres/line_search.h
    internal/ceres/linear_least_squares_problems.h
    internal/ceres/linear_operator.h
    internal/ceres/linear_solver.h
    internal/ceres/low_rank_inverse_hessian.h
    internal/ceres/map_util.h
    internal/ceres/miniglog/glog/logging.h
    internal/ceres/minimizer.h
    internal/ceres/pair_hash.h
    internal/ceres/parallel_for.h
    internal/ceres/parallel_invoke.h
    internal/ceres/parallel_utils.h
    internal/ceres/parallel_vector_ops.h
    internal/ceres/parameter_block_ordering.h
    internal/ceres/parameter_block.h
    internal/ceres/partition_range_for_parallel_for.h
    internal/ceres/partitioned_matrix_view_impl.h
    internal/ceres/partitioned_matrix_view.h
    internal/ceres/polynomial.h
    internal/ceres/power_series_expansion_preconditioner.h
    internal/ceres/preconditioner.h
    internal/ceres/preprocessor.h
    internal/ceres/problem_impl.h
    internal/ceres/program_evaluator.h
    internal/ceres/program.h
    internal/ceres/reorder_program.h
    internal/ceres/residual_block_utils.h
    internal/ceres/residual_block.h
    internal/ceres/schur_complement_solver.h
    internal/ceres/schur_eliminator_impl.h
    internal/ceres/schur_eliminator.h
    internal/ceres/schur_jacobi_preconditioner.h
    internal/ceres/schur_templates.h
    internal/ceres/scoped_thread_token.h
    internal/ceres/scratch_evaluate_preparer.h
    internal/ceres/single_linkage_clustering.h
    internal/ceres/small_blas_generic.h
    internal/ceres/small_blas.h
    internal/ceres/solver_utils.h
    internal/ceres/sparse_cholesky.h
    internal/ceres/sparse_matrix.h
    internal/ceres/sparse_normal_cholesky_solver.h
    internal/ceres/stl_util.h
    internal/ceres/stringprintf.h
    internal/ceres/subset_preconditioner.h
    internal/ceres/suitesparse.h
    internal/ceres/thread_pool.h
    internal/ceres/thread_token_provider.h
    internal/ceres/triplet_sparse_matrix.h
    internal/ceres/trust_region_minimizer.h
    internal/ceres/trust_region_preprocessor.h
    internal/ceres/trust_region_step_evaluator.h
    internal/ceres/trust_region_strategy.h
    internal/ceres/visibility_based_preconditioner.h
    internal/ceres/visibility.h
    internal/ceres/wall_time.h
)
set(ceres_SOURCES
    internal/ceres/accelerate_sparse.cc
    internal/ceres/array_utils.cc
    internal/ceres/block_evaluate_preparer.cc
    internal/ceres/block_jacobi_preconditioner.cc
    internal/ceres/block_jacobian_writer.cc
    internal/ceres/block_random_access_dense_matrix.cc
    internal/ceres/block_random_access_diagonal_matrix.cc
    internal/ceres/block_random_access_matrix.cc
    internal/ceres/block_random_access_sparse_matrix.cc
    internal/ceres/block_sparse_matrix.cc
    internal/ceres/block_structure.cc
    internal/ceres/c_api.cc
    internal/ceres/callbacks.cc
    internal/ceres/canonical_views_clustering.cc
    internal/ceres/cgnr_solver.cc
    internal/ceres/compressed_col_sparse_matrix_utils.cc
    internal/ceres/compressed_row_jacobian_writer.cc
    internal/ceres/compressed_row_sparse_matrix.cc
    internal/ceres/conditioned_cost_function.cc
    internal/ceres/context_impl.cc
    internal/ceres/context.cc
    internal/ceres/coordinate_descent_minimizer.cc
    internal/ceres/corrector.cc
    internal/ceres/cost_function.cc
    internal/ceres/covariance_impl.cc
    internal/ceres/covariance.cc
    internal/ceres/cuda_block_sparse_crs_view.cc
    internal/ceres/cuda_block_structure.cc
    internal/ceres/cuda_partitioned_block_sparse_crs_view.cc
    internal/ceres/cuda_sparse_matrix.cc
    internal/ceres/cuda_vector.cc
    internal/ceres/dense_cholesky.cc
    internal/ceres/dense_normal_cholesky_solver.cc
    internal/ceres/dense_qr_solver.cc
    internal/ceres/dense_qr.cc
    internal/ceres/dense_sparse_matrix.cc
    internal/ceres/detect_structure.cc
    internal/ceres/dogleg_strategy.cc
    internal/ceres/dynamic_compressed_row_jacobian_writer.cc
    internal/ceres/dynamic_compressed_row_sparse_matrix.cc
    internal/ceres/dynamic_sparse_normal_cholesky_solver.cc
    internal/ceres/eigensparse.cc
    internal/ceres/evaluation_callback.cc
    internal/ceres/evaluator.cc
    internal/ceres/fake_bundle_adjustment_jacobian.cc
    internal/ceres/file.cc
    internal/ceres/first_order_function.cc
    internal/ceres/float_suitesparse.cc
    internal/ceres/function_sample.cc
    internal/ceres/generated/partitioned_matrix_view_d_d_d.cc
    internal/ceres/generated/schur_eliminator_d_d_d.cc
    internal/ceres/gradient_checker.cc
    internal/ceres/gradient_checking_cost_function.cc
    internal/ceres/gradient_problem_solver.cc
    internal/ceres/gradient_problem.cc
    internal/ceres/implicit_schur_complement.cc
    internal/ceres/inner_product_computer.cc
    internal/ceres/is_close.cc
    internal/ceres/iteration_callback.cc
    internal/ceres/iterative_refiner.cc
    internal/ceres/iterative_schur_complement_solver.cc
    internal/ceres/levenberg_marquardt_strategy.cc
    internal/ceres/line_search_direction.cc
    internal/ceres/line_search_minimizer.cc
    internal/ceres/line_search_preprocessor.cc
    internal/ceres/line_search.cc
    internal/ceres/linear_least_squares_problems.cc
    internal/ceres/linear_operator.cc
    internal/ceres/linear_solver.cc
    internal/ceres/loss_function.cc
    internal/ceres/low_rank_inverse_hessian.cc
    internal/ceres/manifold.cc
    internal/ceres/miniglog/glog/logging.cc
    internal/ceres/minimizer.cc
    internal/ceres/normal_prior.cc
    internal/ceres/parallel_invoke.cc
    internal/ceres/parallel_utils.cc
    internal/ceres/parallel_vector_ops.cc
    internal/ceres/parameter_block_ordering.cc
    internal/ceres/partitioned_matrix_view.cc
    internal/ceres/polynomial.cc
    internal/ceres/power_series_expansion_preconditioner.cc
    internal/ceres/preconditioner.cc
    internal/ceres/preprocessor.cc
    internal/ceres/problem_impl.cc
    internal/ceres/problem.cc
    internal/ceres/program.cc
    internal/ceres/reorder_program.cc
    internal/ceres/residual_block_utils.cc
    internal/ceres/residual_block.cc
    internal/ceres/schur_complement_solver.cc
    internal/ceres/schur_eliminator.cc
    internal/ceres/schur_jacobi_preconditioner.cc
    internal/ceres/schur_templates.cc
    internal/ceres/scratch_evaluate_preparer.cc
    internal/ceres/single_linkage_clustering.cc
    internal/ceres/solver_utils.cc
    internal/ceres/solver.cc
    internal/ceres/sparse_cholesky.cc
    internal/ceres/sparse_matrix.cc
    internal/ceres/sparse_normal_cholesky_solver.cc
    internal/ceres/stringprintf.cc
    internal/ceres/subset_preconditioner.cc
    internal/ceres/suitesparse.cc
    internal/ceres/thread_pool.cc
    internal/ceres/thread_token_provider.cc
    internal/ceres/triplet_sparse_matrix.cc
    internal/ceres/trust_region_minimizer.cc
    internal/ceres/trust_region_preprocessor.cc
    internal/ceres/trust_region_step_evaluator.cc
    internal/ceres/trust_region_strategy.cc
    internal/ceres/types.cc
    internal/ceres/visibility_based_preconditioner.cc
    internal/ceres/visibility.cc
    internal/ceres/wall_time.cc
)
add_library(ceres STATIC ${ceres_HEADERS} ${ceres_SOURCES})
# internal/ceres/miniglog provides glog/logging.h: the bundled Ceres needs
# neither glog nor gflags.
target_include_directories(ceres SYSTEM
    PUBLIC
        ../Eigen3 config include internal/ceres/miniglog
    PRIVATE
        internal
)
target_compile_options(ceres PRIVATE ${NATRON_THIRD_PARTY_NO_WARNINGS})
# MAX_LOG_LEVEL: miniglog prints LOG(n) and VLOG(n) for n <= MAX_LOG_LEVEL
# (FATAL=-3, ERROR=-2, WARNING=-1, INFO=0, VLOG(1)...). 0 prints LOG(INFO) and
# above, but not VLOG(1) and above, as the bundled glog did: Natron never called
# InitGoogleLogging(), so glog wrote to stderr, with verbosity 0. It is PUBLIC,
# since the macros are expanded in libmv too.
target_compile_definitions(ceres PUBLIC MAX_LOG_LEVEL=0)
# Ceres runs its multi-threaded parts with std::thread.
find_package(Threads REQUIRED)
target_link_libraries(ceres PUBLIC Threads::Threads)
set_target_properties(ceres PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(Ceres::ceres ALIAS ceres)
