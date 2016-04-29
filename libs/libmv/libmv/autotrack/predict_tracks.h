// Copyright (c) 2014 libmv authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// Author: mierle@gmail.com (Keir Mierle)

#ifndef LIBMV_AUTOTRACK_PREDICT_TRACKS_H_
#define LIBMV_AUTOTRACK_PREDICT_TRACKS_H_

#include "libmv/tracking/kalman_filter.h"
#include "libmv/autotrack/marker.h"

namespace mv {

class Tracks;
struct Marker;

typedef mv::KalmanFilter<double, 6, 2> TrackerKalman;
    
class KalmanFilterState {
    TrackerKalman::State _state;
    int _stateFrame;
    bool _hasInitializedState;
    int _frameStep;
    Marker _previousMarker;
    
public:
    
    KalmanFilterState() : _state(), _stateFrame(0), _hasInitializedState(false), _frameStep(1), _previousMarker() {}
    
    // Initialize the Kalman state.
    void Init(const Marker& first_marker, int frameStep);
    
    // predict forward until target_frame is reached
    bool PredictForward(const int target_frame,
                        Marker* predicted_marker);
    
    // returns true if update was succesful
    bool Update(const Marker& measured_marker);
};


    
// Predict the position of the given marker, and update it accordingly. The
// existing position will be overwritten.
bool PredictMarkerPosition(const Tracks& tracks, Marker* marker);

}  // namespace mv

#endif  // LIBMV_AUTOTRACK_PREDICT_TRACKS_H_
