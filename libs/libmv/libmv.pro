#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = LibMV
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

CONFIG += libmv-flags gflags-flags

include(../../global.pri)
include(../../libs.pri)
include(../../config.pri)

CONFIG -= warn_on

win32-msvc* {
        INCLUDEPATH += third_party/msinttypes
}


SOURCES += \
        libmv/autotrack/autotrack.cc \
        libmv/autotrack/predict_tracks.cc \
        libmv/autotrack/tracks.cc \
        libmv/image/array_nd.cc \
        libmv/image/convolve.cc \
        libmv/multiview/conditioning.cc \
        libmv/multiview/homography.cc \
        libmv/multiview/projection.cc \
        libmv/numeric/numeric.cc \
        libmv/tracking/track_region.cc \
        libmv/simple_pipeline/detect.cc

HEADERS += \
        libmv/autotrack/autotrack.h \
        libmv/autotrack/callbacks.h \
        libmv/autotrack/frame_accessor.h \
        libmv/autotrack/marker.h \
        libmv/autotrack/model.h \
        libmv/autotrack/predict_tracks.h \
        libmv/autotrack/quad.h \
        libmv/autotrack/reconstruction.h \
        libmv/autotrack/region.h \
        libmv/autotrack/tracks.h \
        libmv/base/aligned_malloc.h \
        libmv/base/id_generator.h \
        libmv/base/scoped_ptr.h \
        libmv/base/vector.h \
        libmv/base/vector_utils.h \
        libmv/image/array_nd.h \
        libmv/image/convolve.h \
        libmv/image/correlation.h \
        libmv/image/image_converter.h \
        libmv/image/image_drawing.h \
        libmv/image/image.h \
        libmv/image/sample.h \
        libmv/image/tuple.h \
        libmv/logging/logging.h \
        libmv/multiview/conditioning.h \
        libmv/multiview/euclidean_resection.h \
        libmv/multiview/fundamental.h \
        libmv/multiview/homography_error.h \
        libmv/multiview/homography.h \
        libmv/multiview/homography_parameterization.h \
        libmv/multiview/nviewtriangulation.h \
        libmv/multiview/panography.h \
        libmv/multiview/panography_kernel.h \
        libmv/multiview/projection.h \
        libmv/multiview/resection.h \
        libmv/multiview/triangulation.h \
        libmv/multiview/two_view_kernel.h \
        libmv/numeric/dogleg.h \
        libmv/numeric/function_derivative.h \
        libmv/numeric/levenberg_marquardt.h \
        libmv/numeric/numeric.h \
        libmv/numeric/poly.h \
        libmv/simple_pipeline/bundle.h \
        libmv/simple_pipeline/callbacks.h \
        libmv/simple_pipeline/camera_intrinsics.h \
        libmv/simple_pipeline/camera_intrinsics_impl.h \
        libmv/simple_pipeline/detect.h \
        libmv/simple_pipeline/distortion_models.h \
        libmv/simple_pipeline/initialize_reconstruction.h \
        libmv/simple_pipeline/intersect.h \
        libmv/simple_pipeline/keyframe_selection.h \
        libmv/simple_pipeline/modal_solver.h \
        libmv/simple_pipeline/pipeline.h \
        libmv/simple_pipeline/reconstruction.h \
        libmv/simple_pipeline/reconstruction_scale.h \
        libmv/simple_pipeline/resect.h \
        libmv/simple_pipeline/tracks.h \
        libmv/tracking/brute_region_tracker.h \
        libmv/tracking/hybrid_region_tracker.h \
        libmv/tracking/kalman_filter.h \
        libmv/tracking/klt_region_tracker.h \
        libmv/tracking/pyramid_region_tracker.h \
        libmv/tracking/region_tracker.h \
        libmv/tracking/retrack_region_tracker.h \
        libmv/tracking/track_region.h \
        libmv/tracking/trklt_region_tracker.h \

win32-msvc* {

HEADERS += \
        third_party/msinttypes/inttypes.h \
        third_party/msinttypes/stdint.h
}
