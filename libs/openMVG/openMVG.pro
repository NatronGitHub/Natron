#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = openMVG
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

CONFIG += openmvg-flags

include(../../global.pri)
include(../../libs.pri)

win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
}

CONFIG -= warn_on

SOURCES += \
        openMVG/features/akaze/AKAZE.cpp \
        openMVG/features/liop/liop_descriptor.cpp \
        openMVG/linearProgramming/linearProgrammingMOSEK.cpp \
        openMVG/matching/kvld/kvld.cpp \
        openMVG/matching/kvld/algorithm.cpp \
        openMVG/multiview/conditioning.cpp \
        openMVG/multiview/essential.cpp \
        openMVG/multiview/projection.cpp \
        openMVG/multiview/solver_affine.cpp \
        openMVG/multiview/solver_essential_five_point.cpp \
        openMVG/multiview/solver_essential_kernel.cpp \
        openMVG/multiview/solver_fundamental_kernel.cpp \
        openMVG/multiview/solver_homography_kernel.cpp \
        openMVG/multiview/solver_resection_kernel.cpp \
        openMVG/multiview/translation_averaging_solver_l2_chordal.cpp \
        openMVG/multiview/translation_averaging_solver_softl1.cpp \
        openMVG/multiview/triangulation_nview.cpp \
        openMVG/multiview/triangulation.cpp \
        openMVG/numeric/numeric.cpp \
        openMVG/system/timer.cpp

#        openMVG/sfm/pipelines/global/GlobalSfM_rotation_averaging.cpp \
#        openMVG/sfm/pipelines/global/GlobalSfM_translation_averaging.cpp \
#        openMVG/sfm/pipelines/global/sfm_global_engine_relative_motions.cpp \
#        openMVG/sfm/pipelines/localization/SfM_Localizer_Single_3DTrackObservation_Database.cpp \
#        openMVG/sfm/pipelines/localization/SfM_Localizer.cpp \
#        openMVG/sfm/pipelines/sequential/sequential_SfM.cpp \
#        openMVG/sfm/pipelines/structure_from_known_poses/structure_estimator.cpp \
#        openMVG/sfm/pipelines/sfm_robust_model_estimation.cpp \
#        openMVG/sfm/sfm_data_BA_ceres.cpp \
#        openMVG/sfm/sfm_data_filters_frustum.cpp \
#        openMVG/sfm/sfm_data_io.cpp \
#        openMVG/sfm/sfm_data_triangulation.cpp \
#        openMVG/sfm/sfm_data_utils.cpp \
#        openMVG/multiview/rotation_averaging_l1.cpp \
#        openMVG/multiview/rotation_averaging_l2.cpp \
#        openMVG/matching/regions_matcher.cpp \
#        openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions_AllInMemory.cpp \
#        openMVG/matching_image_collection/Matcher_Regions_AllInMemory.cpp \
#        openMVG/linearProgramming/lInfinityCV/resection_kernel.cpp \

HEADERS += \
        openMVG/cameras/Camera_Common.hpp \
        openMVG/cameras/Camera_Intrinsics.hpp \
        openMVG/cameras/Camera_Pinhole_Brown.hpp \
        openMVG/cameras/Camera_Pinhole_Radial.hpp \
        openMVG/cameras/Camera_Pinhole.hpp \
        openMVG/cameras/Camera_undistort_image.hpp \
        openMVG/cameras/cameras.hpp \
        openMVG/cameras/PinholeCamera.hpp \
        openMVG/cameras/Camera_IO.hpp \
        openMVG/color_harmonization/global_quantile_gain_offset_alignment.hpp \
        openMVG/color_harmonization/selection_fullFrame.hpp \
        openMVG/color_harmonization/selection_interface.hpp \
        openMVG/color_harmonization/selection_matchedPoints.hpp \
        openMVG/color_harmonization/selection_VLDSegment.hpp \
        openMVG/features/akaze/AKAZE.hpp \
        openMVG/features/akaze/mldb_descriptor.hpp \
        openMVG/features/akaze/msurf_descriptor.hpp \
        openMVG/features/descriptor.hpp \
        openMVG/features/feature.hpp \
        openMVG/features/features.hpp \
        openMVG/features/image_describer.hpp \
        openMVG/features/image_describer_akaze.hpp \
        openMVG/features/io_regions_type.hpp \
        openMVG/features/keypointSet.hpp \
        openMVG/features/regions_factory.hpp \
        openMVG/features/regions.hpp \
        openMVG/features/liop/liop_descriptor.hpp \
        openMVG/geometry/frustum.hpp \
        openMVG/geometry/half_space_intersection.hpp \
        openMVG/geometry/pose3.hpp \
        openMVG/geometry/rigid_transformation3D_srt.hpp \
        openMVG/geometry/Similarity3.hpp \
        openMVG/image/image_concat.hpp \
        openMVG/image/image_container.hpp \
        openMVG/image/image_converter.hpp \
        openMVG/image/image_convolution_base.hpp \
        openMVG/image/image_convolution.hpp \
        openMVG/image/image_diffusion.hpp \
        openMVG/image/image_drawing.hpp \
        openMVG/image/image_filtering.hpp \
        openMVG/image/image_resampling.hpp \
        openMVG/image/image.hpp \
        openMVG/image/image_warping.hpp \
        openMVG/image/pixel_types.hpp \
        openMVG/image/sample.hpp \
        openMVG/linearProgramming/bisectionLP.hpp \
        openMVG/linearProgramming/linearProgramming.hpp \
        openMVG/linearProgramming/linearProgrammingInterface.hpp \
        openMVG/linearProgramming/linearProgrammingMOSEK.hpp \
        openMVG/linearProgramming/linearProgrammingOSI_X.hpp \
        openMVG/linearProgramming/lInfinityCV/global_translations_fromTij.hpp \
        openMVG/linearProgramming/lInfinityCV/global_translations_fromTriplets.hpp \
        openMVG/linearProgramming/lInfinityCV/lInfinityCV.hpp \
        openMVG/linearProgramming/lInfinityCV/resection_kernel.hpp \
        openMVG/linearProgramming/lInfinityCV/tijsAndXis_From_xi_Ri_noise.hpp \
        openMVG/linearProgramming/lInfinityCV/tijsAndXis_From_xi_Ri.hpp \
        openMVG/linearProgramming/lInfinityCV/triangulation.hpp \
        openMVG/linearProgramming/lInfinityCV/triplet_tijsAndXis_kernel.hpp \
        openMVG/matching/cascade_hasher.hpp \
        openMVG/matching/indMatch_utils.hpp \
        openMVG/matching/indMatch.hpp \
        openMVG/matching/indMatchDecoratorXY.hpp \
        openMVG/matching/kvld/algorithm.h \
        openMVG/matching/kvld/kvld_draw.h \
        openMVG/matching/kvld/kvld.h \
        openMVG/matching/matcher_brute_force.hpp \
        openMVG/matching/matcher_cascade_hashing.hpp \
        openMVG/matching/matcher_type.hpp \
        openMVG/matching/matching_filters.hpp \
        openMVG/matching/matching_interface.hpp \
        openMVG/matching/metric_hamming.hpp \
        openMVG/matching/metric.hpp \
        openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions_AllInMemory.hpp \
        openMVG/matching_image_collection/F_ACRobust.hpp \
        openMVG/matching_image_collection/Geometric_Filter_utils.hpp \
        openMVG/matching_image_collection/GeometricFilter.hpp \
        openMVG/matching_image_collection/H_ACRobust.hpp \
        openMVG/matching_image_collection/Matcher.hpp \
        openMVG/matching_image_collection/Pair_Builder.hpp \
        openMVG/multiview/conditioning.hpp \
        openMVG/multiview/essential.hpp \
        openMVG/multiview/projection.hpp \
        openMVG/multiview/rotation_averaging_common.hpp \
        openMVG/multiview/solver_affine.hpp \
        openMVG/multiview/solver_essential_kernel.hpp \
        openMVG/multiview/solver_fundamental_kernel.hpp \
        openMVG/multiview/solver_homography_kernel.hpp \
        openMVG/multiview/solver_resection_kernel.hpp \
        openMVG/multiview/solver_resection_p3p.hpp \
        openMVG/multiview/solver_translation_knownRotation_kernel.hpp \
        openMVG/multiview/translation_averaging_common.hpp \
        openMVG/multiview/translation_averaging_solver.hpp \
        openMVG/multiview/triangulation_nview.hpp \
        openMVG/multiview/triangulation.hpp \
        openMVG/multiview/two_view_kernel.hpp \
        openMVG/numeric/accumulator_trait.hpp \
        openMVG/numeric/lm.hpp \
        openMVG/numeric/math_trait.hpp \
        openMVG/numeric/numeric.h \
        openMVG/numeric/poly.h \
        openMVG/robust_estimation/guided_matching.hpp \
        openMVG/robust_estimation/rand_sampling.hpp \
        openMVG/robust_estimation/robust_estimator_ACRansac.hpp \
        openMVG/robust_estimation/robust_estimator_ACRansacKernelAdaptator.hpp \
        openMVG/robust_estimation/robust_estimator_lineKernel_test.hpp \
        openMVG/robust_estimation/robust_estimator_LMeds.hpp \
        openMVG/robust_estimation/robust_estimator_MaxConsensus.hpp \
        openMVG/robust_estimation/robust_estimator_Ransac.hpp \
        openMVG/robust_estimation/robust_ransac_tools.hpp \
        openMVG/robust_estimation/score_evaluator.hpp \
        openMVG/stl/dynamic_bitset.hpp \
        openMVG/stl/hash.hpp \
        openMVG/stl/indexed_sort.hpp \
        openMVG/stl/split.hpp \
        openMVG/stl/stl.hpp \
        openMVG/stl/stlMap.hpp \
        openMVG/system/timer.hpp \
        openMVG/tracks/tracks.hpp \
        openMVG/types.hpp \
        openMVG/version.hpp \
        openMVG/robust_estimation/robust_estimator_Prosac.hpp \
        openMVG/robust_estimation/robust_estimator_ProsacKernelAdaptator.hpp

#        openMVG/matching/matcher_kdtree_flann.hpp \
#        openMVG/matching/regions_matcher.hpp \
#        openMVG/matching_image_collection/Matcher_Regions_AllInMemory.hpp \
#        openMVG/multiview/rotation_averaging_l1.hpp \
#        openMVG/multiview/rotation_averaging_l2.hpp \
#        openMVG/multiview/rotation_averaging.hpp \
#        openMVG/graph/connectedComponent.hpp \
#        openMVG/graph/graph_builder.hpp \
#        openMVG/graph/graph_graphviz_export.hpp \
#        openMVG/graph/graph.hpp \
#        openMVG/graph/triplet_finder.hpp \
#        openMVG/sfm/pipelines/global/GlobalSfM_rotation_averaging.hpp \
#        openMVG/sfm/pipelines/global/GlobalSfM_rotation_averaging/score_evaluator.hpp \
#        openMVG/sfm/pipelines/global/GlobalSfM_translation_averaging.hpp \
#        openMVG/sfm/pipelines/global/sfm_global_engine_relative_motions.hpp \
#        openMVG/sfm/pipelines/global/sfm_global_reindex.hpp \
#        openMVG/sfm/pipelines/global/triplet_t_ACRansac_kernelAdaptator.hpp \
#        openMVG/sfm/pipelines/localization/SfM_Localizer_Single_3DTrackObservation_Database.hpp \
#        openMVG/sfm/pipelines/localization/SfM_Localizer.hpp \
#        openMVG/sfm/pipelines/sequential/sequential_SfM.hpp \
#        openMVG/sfm/pipelines/structure_from_known_poses/structure_estimator.hpp \
#        openMVG/sfm/pipelines/sfm_engine.hpp \
#        openMVG/sfm/pipelines/sfm_features_provider.hpp \
#        openMVG/sfm/pipelines/sfm_matches_provider.hpp \
#        openMVG/sfm/pipelines/sfm_regions_provider.hpp \
#        openMVG/sfm/pipelines/sfm_robust_model_estimation.hpp \
#        openMVG/sfm/sfm_data_BA_ceres_camera_functor.hpp \
#        openMVG/sfm/sfm_data_BA_ceres.hpp \
#        openMVG/sfm/sfm_data_BA.hpp \
#        openMVG/sfm/sfm_data_filters_frustum.hpp \
#        openMVG/sfm/sfm_data_filters.hpp \
#        openMVG/sfm/sfm_data_io_baf.hpp \
#        openMVG/sfm/sfm_data_io_cereal.hpp \
#        openMVG/sfm/sfm_data_io_ply.hpp \
#        openMVG/sfm/sfm_data_io.hpp \
#        openMVG/sfm/sfm_data_triangulation.hpp \
#        openMVG/sfm/sfm_data_utils.hpp \
#        openMVG/sfm/sfm_data.hpp \
#        openMVG/sfm/sfm_filters.hpp \
#        openMVG/sfm/sfm_landmark.hpp \
#        openMVG/sfm/sfm_view.hpp \
#        openMVG/sfm/sfm.hpp \


#SOURCES += \
#        dependencies/osi_clp/Clp/src/CbcOrClpParam.cpp \
#        dependencies/osi_clp/Clp/src/Clp_C_Interface.cpp \
#        dependencies/osi_clp/Clp/src/ClpCholeskyBase.cpp \
#        dependencies/osi_clp/Clp/src/ClpCholeskyDense.cpp \
#        dependencies/osi_clp/Clp/src/ClpCholeskyTaucs.cpp \
#        dependencies/osi_clp/Clp/src/ClpConstraint.cpp \
#        dependencies/osi_clp/Clp/src/ClpConstraintLinear.cpp \
#        dependencies/osi_clp/Clp/src/ClpConstraintQuadratic.cpp \
#        dependencies/osi_clp/Clp/src/ClpDualRowDantzig.cpp \
#        dependencies/osi_clp/Clp/src/ClpDualRowPivot.cpp \
#        dependencies/osi_clp/Clp/src/ClpDualRowSteepest.cpp \
#        dependencies/osi_clp/Clp/src/ClpDummyMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpDynamicExampleMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpDynamicMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpEventHandler.cpp \
#        dependencies/osi_clp/Clp/src/ClpFactorization.cpp \
#        dependencies/osi_clp/Clp/src/ClpGubDynamicMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpGubMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpHelperFunctions.cpp \
#        dependencies/osi_clp/Clp/src/ClpInterior.cpp \
#        dependencies/osi_clp/Clp/src/ClpLinearObjective.cpp \
#        dependencies/osi_clp/Clp/src/ClpLsqr.cpp \
#        dependencies/osi_clp/Clp/src/ClpMain.cpp \
#        dependencies/osi_clp/Clp/src/ClpMatrixBase.cpp \
#        dependencies/osi_clp/Clp/src/ClpMessage.cpp \
#        dependencies/osi_clp/Clp/src/ClpModel.cpp \
#        dependencies/osi_clp/Clp/src/ClpNetworkBasis.cpp \
#        dependencies/osi_clp/Clp/src/ClpNetworkMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpNode.cpp \
#        dependencies/osi_clp/Clp/src/ClpNonLinearCost.cpp \
#        dependencies/osi_clp/Clp/src/ClpObjective.cpp \
#        dependencies/osi_clp/Clp/src/ClpPackedMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpPdco.cpp \
#        dependencies/osi_clp/Clp/src/ClpPdcoBase.cpp \
#        dependencies/osi_clp/Clp/src/ClpPlusMinusOneMatrix.cpp \
#        dependencies/osi_clp/Clp/src/ClpPredictorCorrector.cpp \
#        dependencies/osi_clp/Clp/src/ClpPresolve.cpp \
#        dependencies/osi_clp/Clp/src/ClpPrimalColumnDantzig.cpp \
#        dependencies/osi_clp/Clp/src/ClpPrimalColumnPivot.cpp \
#        dependencies/osi_clp/Clp/src/ClpPrimalColumnSteepest.cpp \
#        dependencies/osi_clp/Clp/src/ClpQuadraticObjective.cpp \
#        dependencies/osi_clp/Clp/src/ClpSimplex.cpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexDual.cpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexNonlinear.cpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexOther.cpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexPrimal.cpp \
#        dependencies/osi_clp/Clp/src/ClpSolve.cpp \
#        dependencies/osi_clp/Clp/src/Idiot.cpp \
#        dependencies/osi_clp/Clp/src/IdiSolve.cpp \
#        dependencies/osi_clp/Clp/src/MyEventHandler.cpp \
#        dependencies/osi_clp/Clp/src/MyMessageHandler.cpp \
#        dependencies/osi_clp/Clp/src/IdiSolve.cpp \
#        dependencies/osi_clp/Clp/src/OsiClp/OsiClpSolverInterface.cpp

#HEADERS += \
#        dependencies/osi_clp/Clp/src/CbcOrClpParam.hpp \
#        dependencies/osi_clp/Clp/src/Clp_C_Interface.hpp \
#        dependencies/osi_clp/Clp/src/ClpCholeskyBase.hpp \
#        dependencies/osi_clp/Clp/src/ClpCholeskyDense.hpp \
#        dependencies/osi_clp/Clp/src/ClpCholeskyTaucs.hpp \
#        dependencies/osi_clp/Clp/src/ClpConstraint.hpp \
#        dependencies/osi_clp/Clp/src/ClpConfig.hpp \
#        dependencies/osi_clp/Clp/src/ClpConstraintLinear.hpp \
#        dependencies/osi_clp/Clp/src/ClpConstraintQuadratic.hpp \
#        dependencies/osi_clp/Clp/src/ClpDualRowDantzig.hpp \
#        dependencies/osi_clp/Clp/src/ClpDualRowPivot.hpp \
#        dependencies/osi_clp/Clp/src/ClpDualRowSteepest.hpp \
#        dependencies/osi_clp/Clp/src/ClpDummyMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpDynamicExampleMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpDynamicMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpEventHandler.hpp \
#        dependencies/osi_clp/Clp/src/ClpFactorization.hpp \
#        dependencies/osi_clp/Clp/src/ClpGubDynamicMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpGubMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpHelperFunctions.hpp \
#        dependencies/osi_clp/Clp/src/ClpInterior.hpp \
#        dependencies/osi_clp/Clp/src/ClpLinearObjective.hpp \
#        dependencies/osi_clp/Clp/src/ClpLsqr.hpp \
#        dependencies/osi_clp/Clp/src/ClpMain.hpp \
#        dependencies/osi_clp/Clp/src/ClpMatrixBase.hpp \
#        dependencies/osi_clp/Clp/src/ClpMessage.hpp \
#        dependencies/osi_clp/Clp/src/ClpModel.hpp \
#        dependencies/osi_clp/Clp/src/ClpNetworkBasis.hpp \
#        dependencies/osi_clp/Clp/src/ClpNetworkMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpNode.hpp \
#        dependencies/osi_clp/Clp/src/ClpNonLinearCost.hpp \
#        dependencies/osi_clp/Clp/src/ClpObjective.hpp \
#        dependencies/osi_clp/Clp/src/ClpPackedMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpParameters.hpp \
#        dependencies/osi_clp/Clp/src/ClpPdco.hpp \
#        dependencies/osi_clp/Clp/src/ClpPdcoBase.hpp \
#        dependencies/osi_clp/Clp/src/ClpPlusMinusOneMatrix.hpp \
#        dependencies/osi_clp/Clp/src/ClpPredictorCorrector.hpp \
#        dependencies/osi_clp/Clp/src/ClpPresolve.hpp \
#        dependencies/osi_clp/Clp/src/ClpPrimalColumnDantzig.hpp \
#        dependencies/osi_clp/Clp/src/ClpPrimalColumnPivot.hpp \
#        dependencies/osi_clp/Clp/src/ClpPrimalColumnSteepest.hpp \
#        dependencies/osi_clp/Clp/src/ClpQuadraticObjective.hpp \
#        dependencies/osi_clp/Clp/src/ClpSimplex.hpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexDual.hpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexNonlinear.hpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexOther.hpp \
#        dependencies/osi_clp/Clp/src/ClpSimplexPrimal.hpp \
#        dependencies/osi_clp/Clp/src/ClpSolve.hpp \
#        dependencies/osi_clp/Clp/src/Idiot.hpp \
#        dependencies/osi_clp/Clp/src/IdiSolve.hpp \
#        dependencies/osi_clp/Clp/src/MyEventHandler.hpp \
#        dependencies/osi_clp/Clp/src/MyMessageHandler.hpp \
#        dependencies/osi_clp/Clp/src/IdiSolve.hpp \
#        dependencies/osi_clp/Clp/src/OsiClp/OsiClpSolverInterface.hpp \
#        dependencies/osi_clp/Clp/src/config_clp_default.h \
#        dependencies/osi_clp/Clp/src/config_default.h


#SOURCES += \
#        dependencies/osi_clp/CoinUtils/src/CoinAlloc.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinBuild.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinDenseFactorization.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinDenseVector.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinError.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFactorization1.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFactorization2.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFactorization3.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFactorization4.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFileIO.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFinite.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinIndexedVector.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinLpIO.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinMessage.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinMessageHandler.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinModel.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinModelUseful.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinModelUseful2.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinMpsIO.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinOslFactorization.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinOslFactorization2.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinOslFactorization3.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPackedMatrix.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPackedVector.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPackedVectorBase.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinParam.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinParamUtils.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPostsolveMatrix.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPrePostsolveMatrix.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveDoubleton.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveDual.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveDupcol.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveEmpty.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveFixed.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveForcing.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveHelperFunctions.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveImpliedFree.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveIsolated.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveMatrix.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveMonitor.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolvePsdebug.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveSingleton.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveSubst.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveTighten.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveTripleton.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveUseless.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveZeros.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSearchTree.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinShallowPackedVector.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSimpFactorization.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSnapshot.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinStructuredModel.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartBasis.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartDual.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartPrimalDual.cpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartVector.cpp



#HEADERS += \
#        dependencies/osi_clp/CoinUtils/src/Coin_C_defines.h \
#        dependencies/osi_clp/CoinUtils/src/CoinAlloc.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinBuild.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinDistance.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinDenseFactorization.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinDenseVector.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinError.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFactorization.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFileIO.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFinite.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinFloatEqual.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinHelperFunctions.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinIndexedVector.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinLpIO.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinMessage.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinMessageHandler.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinModel.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinModelUseful.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinMpsIO.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinOslC.h \
#        dependencies/osi_clp/CoinUtils/src/CoinOslFactorization.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPackedMatrix.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPackedVector.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPackedVectorBase.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinParam.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPragma.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPostsolveMatrix.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPrePostsolveMatrix.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveDoubleton.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveDual.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveDupcol.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveEmpty.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveFixed.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveForcing.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveImpliedFree.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveIsolated.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveMatrix.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveMonitor.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolvePsdebug.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveSingleton.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveSubst.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveTighten.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveTripleton.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveUseless.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinPresolveZeros.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSearchTree.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinShallowPackedVector.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSignal.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSimpFactorization.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSmartPtr.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSnapshot.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinSort.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinStructuredModel.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinTime.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinTypes.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinUtility.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinUtilsConfig.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStart.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartBasis.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartDual.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartPrimalDual.hpp \
#        dependencies/osi_clp/CoinUtils/src/CoinWarmStartVector.hpp \
#        dependencies/osi_clp/CoinUtils/src/config_coinutils_default.h \
#        dependencies/osi_clp/CoinUtils/src/config_default.h



#SOURCES += \
#        dependencies/osi_clp/Osi/src/Osi/OsiAuxInfo.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiBranchingObject.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiChooseVariable.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiColCut.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiCut.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiCuts.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiNames.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiPresolve.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiRowCut.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiRowCutDebugger.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiSolverBranch.cpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiSolverInterface.cpp \
##        dependencies/osi_clp/Osi/src/OsiMsk/OsiMskSolverInterface.cpp


#HEADERS += \
#        dependencies/osi_clp/Osi/src/Osi/config_default.h \
#        dependencies/osi_clp/Osi/src/Osi/config_osi_default.h \
#        dependencies/osi_clp/Osi/src/Osi/OsiAuxInfo.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiBranchingObject.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiChooseVariable.hpp\
#        dependencies/osi_clp/Osi/src/Osi/OsiCollections.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiColCut.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiCut.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiCuts.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiPresolve.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiRowCut.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiRowCutDebugger.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiSolverBranch.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiSolverInterface.hpp \
#        dependencies/osi_clp/Osi/src/Osi/OsiSolverParameters.hpp \
##        dependencies/osi_clp/Osi/src/OsiMsk/OsiMskSolverInterface.hpp
