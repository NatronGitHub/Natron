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

#include "libmv/autotrack/autotrack.h"
#include "libmv/autotrack/quad.h"
#include "libmv/autotrack/frame_accessor.h"
#include "libmv/autotrack/predict_tracks.h"
#include "libmv/base/scoped_ptr.h"
#include "libmv/logging/logging.h"
#include "libmv/numeric/numeric.h"

namespace mv {

namespace {

class DisableChannelsTransform : public FrameAccessor::Transform {
 public:
  DisableChannelsTransform(int disabled_channels)
      : disabled_channels_(disabled_channels) {  }

  int64_t key() const {
    return disabled_channels_;
  }

  void run(const FloatImage& input, FloatImage* output) const {
    bool disable_red   = (disabled_channels_ & Marker::CHANNEL_R) != 0,
         disable_green = (disabled_channels_ & Marker::CHANNEL_G) != 0,
         disable_blue  = (disabled_channels_ & Marker::CHANNEL_B) != 0;

    LG << "Disabling channels: "
       << (disable_red   ? "R " : "")
       << (disable_green ? "G " : "")
       << (disable_blue  ? "B" : "");

    // It's important to rescale the resultappropriately so that e.g. if only
    // blue is selected, it's not zeroed out.
    float scale = (disable_red   ? 0.0f : 0.2126f) +
                  (disable_green ? 0.0f : 0.7152f) +
                  (disable_blue  ? 0.0f : 0.0722f);

    output->Resize(input.Height(), input.Width(), 1);
    for (int y = 0; y < input.Height(); y++) {
      for (int x = 0; x < input.Width(); x++) {
        float r = disable_red   ? 0.0f : input(y, x, 0);
        float g = disable_green ? 0.0f : input(y, x, 1);
        float b = disable_blue  ? 0.0f : input(y, x, 2);
        (*output)(y, x, 0) = (0.2126f * r + 0.7152f * g + 0.0722f * b) / scale;
      }
    }
  }

 private:
  // Bitfield representing visible channels, bits are from Marker::Channel.
  int disabled_channels_;
};

template<typename QuadT, typename ArrayT>
void QuadToArrays(const QuadT& quad, ArrayT* x, ArrayT* y) {
  for (int i = 0; i < 4; ++i) {
    x[i] = quad.coordinates(i, 0);
    y[i] = quad.coordinates(i, 1);
  }
}

void MarkerToArrays(const Marker& marker, double* x, double* y) {
  Quad2Df offset_quad = marker.patch;
  Vec2f origin = marker.search_region.Rounded().min;
  offset_quad.coordinates.rowwise() -= origin.transpose();
  QuadToArrays(offset_quad, x, y);
  x[4] = marker.center.x() - origin(0);
  y[4] = marker.center.y() - origin(1);
}

struct GetImageForMarkerArgs
{
    const Marker* marker;
    FrameAccessor::GetImageTypeEnum sourceType;
    FloatImage* image;
    FrameAccessor::Key key;

    GetImageForMarkerArgs()
    : marker(0)
    , sourceType(FrameAccessor::eGetImageTypeSource)
    , image(0)
    , key(0)
    {

    }
};

void GetImageForMarker(std::list<GetImageForMarkerArgs>& args, FrameAccessor* frame_accessor) {

    if (args.empty()) {
        return;
    }
    std::list<FrameAccessor::GetImageArgs> accessorArgs;
    std::vector<Region> regions(args.size());

    libmv::scoped_ptr<FrameAccessor::Transform> transform = NULL;

    std::size_t i = 0;
    for (std::list<GetImageForMarkerArgs>::const_iterator it = args.begin(); it != args.end(); ++it, ++i) {
        // TODO(sergey): Currently we pass float region to the accessor,
        // but we don't want the accessor to decide the rounding, so we
        // do rounding here.
        // Ideally we would need to pass IntRegion to the frame accessor.
        regions[i] = it->marker->search_region.Rounded();

        bool needsTransform = false;
        if (!transform.get() && it->marker->disabled_channels != 0 && it->sourceType == FrameAccessor::eGetImageTypeSource) {
            needsTransform = true;
            transform.reset(new DisableChannelsTransform(it->marker->disabled_channels));
        }

        FrameAccessor::GetImageArgs access;
        access.clip = it->marker->clip;
        access.frame = it->marker->frame;
        access.sourceType = it->sourceType;
        access.input_mode = FrameAccessor::MONO;
        access.downscale = 0; // No downscale for now.
        access.region = &regions[i];
        access.transform = needsTransform ? transform.get() : 0;
        access.destination = 0;
        access.destinationKey = 0;
        accessorArgs.push_back(access);
    }
    frame_accessor->GetImage(accessorArgs);

    assert(args.size() == accessorArgs.size());
    std::list<GetImageForMarkerArgs>::iterator ito = args.begin();
    for (std::list<FrameAccessor::GetImageArgs>::iterator it = accessorArgs.begin(); it != accessorArgs.end(); ++it, ++ito) {
        ito->image = it->destination;
        ito->key = it->destinationKey;
    }
}

}  // namespace

bool AutoTrack::TrackMarker(Marker* tracked_marker,
                            TrackRegionResult* result,
                            KalmanFilterState* predictionState,
                            const TrackRegionOptions* track_options) {
    
  // Try to predict the location of the second marker.
    bool predicted_position;
    if (predictionState) {
        predicted_position = predictionState->PredictForward(tracked_marker->frame, tracked_marker);
    } else {
        predicted_position = PredictMarkerPosition(tracks_, tracked_marker);
    }
  if (predicted_position) {
    LG << "Succesfully predicted!";
  } else {
    LG << "Prediction failed; trying to track anyway.";
  }

  Marker reference_marker;
  tracks_.GetMarker(tracked_marker->reference_clip,
                    tracked_marker->reference_frame,
                    tracked_marker->track,
                    &reference_marker);

  // Convert markers into the format expected by TrackRegion.
  double x1[5], y1[5];
  MarkerToArrays(reference_marker, x1, y1);

  double x2[5], y2[5];
  MarkerToArrays(*tracked_marker, x2, y2);

  // TODO(keir): Technically this could take a smaller slice from the source
  // image instead of taking one the size of the search window.
  std::list<GetImageForMarkerArgs> getImageArgs;
  {
      GetImageForMarkerArgs args;
      args.marker = &reference_marker;
      args.sourceType = FrameAccessor::eGetImageTypeSource;
      getImageArgs.push_back(args);
  }
  {
      GetImageForMarkerArgs args;
      args.marker = tracked_marker;
      args.sourceType = FrameAccessor::eGetImageTypeSource;
      getImageArgs.push_back(args);
  }
  {
      // If non-null, this is used as the pattern mask. It should match the size of
      // image1, even though only values inside the image1 quad are examined. The
      // values must be in the range 0.0 to 0.1.
      GetImageForMarkerArgs args;
      args.marker = &reference_marker;
      args.sourceType = FrameAccessor::eGetImageTypeMask;
      getImageArgs.push_back(args);
  }

  GetImageForMarker(getImageArgs, frame_accessor_);



  FloatImage* reference_image = 0, *tracked_image = 0, *reference_mask = 0;
  FrameAccessor::Key reference_key, tracked_key, reference_mask_key;
  assert(getImageArgs.size() == 3);
  {
      std::list<GetImageForMarkerArgs>::iterator it = getImageArgs.begin();
      reference_image = it->image;
      reference_key = it->key;
      ++it;

      tracked_image = it->image;
      tracked_key = it->key;
      ++it;

      reference_mask = it->image;
      reference_mask_key = it->key;
  }

  if (!reference_key) {
    LG << "Couldn't get frame for reference marker: " << reference_marker;
    return false;
  }


  if (!tracked_key) {
    frame_accessor_->ReleaseImage(reference_key);
    LG << "Couldn't get frame for tracked marker: " << tracked_marker;
    return false;
  }


  // Store original position befoer tracking, so we can claculate offset later.
  Vec2f original_center = tracked_marker->center;

  // Do the tracking!
  TrackRegionOptions local_track_region_options;
  if (track_options) {
    local_track_region_options = *track_options;
  }
  if (reference_mask_key != NULL) {
    LG << "Using mask for reference marker: " << reference_marker;
    local_track_region_options.image1_mask = reference_mask;
  }
  local_track_region_options.num_extra_points = 1;  // For center point.
  local_track_region_options.attempt_refine_before_brute = predicted_position;
  TrackRegion(*reference_image,
              *tracked_image,
              x1, y1,
              local_track_region_options,
              x2, y2,
              result);

  // Copy results over the tracked marker.
  Vec2f tracked_origin = tracked_marker->search_region.Rounded().min;
  for (int i = 0; i < 4; ++i) {
    tracked_marker->patch.coordinates(i, 0) = x2[i] + tracked_origin[0];
    tracked_marker->patch.coordinates(i, 1) = y2[i] + tracked_origin[1];
  }
  tracked_marker->center(0) = x2[4] + tracked_origin[0];
  tracked_marker->center(1) = y2[4] + tracked_origin[1];
  Vec2f delta = tracked_marker->center - original_center;
  tracked_marker->search_region.Offset(delta);
  tracked_marker->source = Marker::TRACKED;
  tracked_marker->status = Marker::UNKNOWN;
  tracked_marker->reference_clip  = reference_marker.clip;
  tracked_marker->reference_frame = reference_marker.frame;

  // Release the images and masks from the accessor cache.
  frame_accessor_->ReleaseImage(reference_key);
  frame_accessor_->ReleaseImage(tracked_key);

  if (reference_mask) {
      frame_accessor_->ReleaseImage(reference_mask_key);
  }

  // Update the kalman filter with the new measurement
  if (predictionState && result->is_usable()) {
    predictionState->Update(*tracked_marker);
  }
    
  // TODO(keir): Possibly the return here should get removed since the results
  // are part of TrackResult. However, eventually the autotrack stuff will have
  // extra status (e.g. prediction fail, etc) that should get included.
  return true;
}

void AutoTrack::AddMarker(const Marker& marker) {
  tracks_.AddMarker(marker);
}

void AutoTrack::SetMarkers(TrackMarkersMap* markers) {
  tracks_.SetMarkers(markers);
}

bool AutoTrack::GetMarker(int clip, int frame, int track,
                          Marker* markers) const {
  return tracks_.GetMarker(clip, frame, track, markers);
}

void AutoTrack::DetectAndTrack(const DetectAndTrackOptions& options) {
  int num_clips = frame_accessor_->NumClips();
  for (int clip = 0; clip < num_clips; ++clip) {
    int num_frames = frame_accessor_->NumFrames(clip);
    vector<Marker> previous_frame_markers;
    // Q: How to decide track #s when detecting?
    // Q: How to match markers from previous frame? set of prev frame tracks?
    // Q: How to decide what markers should get tracked and which ones should not?
    for (int frame = 0; frame < num_frames; ++frame) {
      if (Cancelled()) {
        LG << "Got cancel message while detecting and tracking...";
        return;
      }
      // First, get or detect markers for this frame.
      vector<Marker> this_frame_markers;
      tracks_.GetMarkersInFrame(clip, frame, &this_frame_markers);
      LG << "Clip " << clip << ", frame " << frame << " have "
         << this_frame_markers.size();
      if (this_frame_markers.size() < options.min_num_features) {
        DetectFeaturesInFrame(clip, frame);
        this_frame_markers.clear();
        tracks_.GetMarkersInFrame(clip, frame, &this_frame_markers);
        LG << "... detected " << this_frame_markers.size() << " features.";
      }
      if (previous_frame_markers.empty()) {
        LG << "First frame; skipping tracking stage.";
        previous_frame_markers.swap(this_frame_markers);
        continue;
      }
      // Second, find tracks that should get tracked forward into this frame.
      // To avoid tracking markers that are already tracked to this frame, make
      // a sorted set of the tracks that exist in the last frame.
      vector<int> tracks_in_this_frame;
      for (int i = 0; i < this_frame_markers.size(); ++i) {
        tracks_in_this_frame.push_back(this_frame_markers[i].track);
      }
      std::sort(tracks_in_this_frame.begin(),
                tracks_in_this_frame.end());

      // Find tracks in the previous frame that are not in this one.
      vector<Marker*> previous_frame_markers_to_track;
      int num_skipped = 0;
      for (int i = 0; i < previous_frame_markers.size(); ++i) {
        if (std::binary_search(tracks_in_this_frame.begin(),
                               tracks_in_this_frame.end(),
                               previous_frame_markers[i].track)) {
          num_skipped++;
        } else {
          previous_frame_markers_to_track.push_back(&previous_frame_markers[i]);
        }
      }

      // Finally track the markers from the last frame into this one.
      // TODO(keir): Use OMP.
      for (int i = 0; i < previous_frame_markers_to_track.size(); ++i) {
        Marker this_frame_marker = *previous_frame_markers_to_track[i];
        this_frame_marker.frame = frame;
        LG << "Tracking: " << this_frame_marker;
        TrackRegionResult result;
        TrackMarker(&this_frame_marker, &result);
        if (result.is_usable()) {
          LG << "Success: " << this_frame_marker;
          AddMarker(this_frame_marker);
          this_frame_markers.push_back(this_frame_marker);
        } else {
          LG << "Failed to track: " << this_frame_marker;
        }
      }
      // Put the markers from this frame
      previous_frame_markers.swap(this_frame_markers);
    }
  }
}

}  // namespace mv
