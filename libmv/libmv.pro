#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = LibMV
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

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


HEADERS += \
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


win32-msvc* {

HEADERS += \
        ../libs/libmv/third_party/msinttypes/inttypes.h \
        ../libs/libmv/third_party/msinttypes/stdint.h
}
