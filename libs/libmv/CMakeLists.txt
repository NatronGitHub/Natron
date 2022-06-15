# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2022 The Natron developers
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

set(mv_HEADERS
    libmv/autotrack/autotrack.h
    libmv/autotrack/callbacks.h
    libmv/autotrack/frame_accessor.h
    libmv/autotrack/marker.h
    libmv/autotrack/model.h
    libmv/autotrack/predict_tracks.h
    libmv/autotrack/quad.h
    libmv/autotrack/reconstruction.h
    libmv/autotrack/region.h
    libmv/autotrack/tracks.h
    libmv/base/aligned_malloc.h
    libmv/base/id_generator.h
    libmv/base/scoped_ptr.h
    libmv/base/vector.h
    libmv/base/vector_utils.h
    libmv/image/array_nd.h
    libmv/image/convolve.h
    libmv/image/correlation.h
    libmv/image/image_converter.h
    libmv/image/image_drawing.h
    libmv/image/image.h
    libmv/image/sample.h
    libmv/image/tuple.h
    libmv/logging/logging.h
    libmv/multiview/conditioning.h
    libmv/multiview/euclidean_resection.h
    libmv/multiview/fundamental.h
    libmv/multiview/homography_error.h
    libmv/multiview/homography.h
    libmv/multiview/homography_parameterization.h
    libmv/multiview/nviewtriangulation.h
    libmv/multiview/panography.h
    libmv/multiview/panography_kernel.h
    libmv/multiview/projection.h
    libmv/multiview/resection.h
    libmv/multiview/triangulation.h
    libmv/multiview/two_view_kernel.h
    libmv/numeric/dogleg.h
    libmv/numeric/function_derivative.h
    libmv/numeric/levenberg_marquardt.h
    libmv/numeric/numeric.h
    libmv/numeric/poly.h
    libmv/simple_pipeline/bundle.h
    libmv/simple_pipeline/callbacks.h
    libmv/simple_pipeline/camera_intrinsics.h
    libmv/simple_pipeline/camera_intrinsics_impl.h
    libmv/simple_pipeline/detect.h
    libmv/simple_pipeline/distortion_models.h
    libmv/simple_pipeline/initialize_reconstruction.h
    libmv/simple_pipeline/intersect.h
    libmv/simple_pipeline/keyframe_selection.h
    libmv/simple_pipeline/modal_solver.h
    libmv/simple_pipeline/pipeline.h
    libmv/simple_pipeline/reconstruction.h
    libmv/simple_pipeline/reconstruction_scale.h
    libmv/simple_pipeline/resect.h
    libmv/simple_pipeline/tracks.h
    libmv/tracking/brute_region_tracker.h
    libmv/tracking/hybrid_region_tracker.h
    libmv/tracking/kalman_filter.h
    libmv/tracking/klt_region_tracker.h
    libmv/tracking/pyramid_region_tracker.h
    libmv/tracking/region_tracker.h
    libmv/tracking/retrack_region_tracker.h
    libmv/tracking/track_region.h
    libmv/tracking/trklt_region_tracker.h
)
if(MSVC)
    set(mv_HEADERS
        third_party/msinttypes/inttypes.h
        third_party/msinttypes/stdint.h
)
endif()
set(mv_SOURCES
    libmv/autotrack/autotrack.cc
    libmv/autotrack/predict_tracks.cc
    libmv/autotrack/tracks.cc
    libmv/image/array_nd.cc
    libmv/image/convolve.cc
    libmv/multiview/conditioning.cc
    libmv/multiview/homography.cc
    libmv/multiview/projection.cc
    libmv/numeric/numeric.cc
    libmv/tracking/track_region.cc
)
add_library(mv STATIC ${mv_HEADERS} ${mv_SOURCES})
target_include_directories(mv PUBLIC third_party .)
if(MSVC)
    target_include_directories(mv PUBLIC third_party/msinttypes)
endif()
target_compile_definitions(mv PUBLIC LIBMV_NO_FAST_DETECTOR=)
target_link_libraries(mv PUBLIC ceres)
set_target_properties(mv PROPERTIES POSITION_INDEPENDENT_CODE ON)
