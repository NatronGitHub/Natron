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

#include "libmv/autotrack/tracks.h"

#include <algorithm>
#include <vector>
#include <iterator>
#include <set>

#include "libmv/numeric/numeric.h"

namespace mv {

Tracks::Tracks(const Tracks& other) {
  markers_ = other.markers_;
}

Tracks::Tracks(const TrackMarkersMap& markers) : markers_(markers) {}

bool Tracks::GetMarker(int clip, int frame, int track, Marker* marker) const {
  TrackMarkersMap::const_iterator foundTrack = markers_.find(track);
  if (foundTrack == markers_.end()) {
    return false;
  }
  ClipMarkersMap::const_iterator foundClip = foundTrack->second.find(clip);
  if (foundClip == foundTrack->second.end()) {
    return false;
  }
  FrameMarkerMap::const_iterator foundFrame = foundClip->second.find(frame);
  if (foundFrame == foundClip->second.end()) {
    return false;
  } else {
    *marker = foundFrame->second;
    return true;
  }
}

void Tracks::GetMarkersForTrack(int track, vector<Marker>* markers) const {
  TrackMarkersMap::const_iterator foundTrack = markers_.find(track);
  if (foundTrack == markers_.end()) {
    return;
  }
  for (ClipMarkersMap::const_iterator it = foundTrack->second.begin(); it != foundTrack->second.end(); ++it) {
    for (FrameMarkerMap::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
      markers->push_back(it2->second);
    }
  }
}

void Tracks::GetMarkersForTrackInClip(int clip,
                                      int track,
                                      vector<Marker>* markers) const {
  TrackMarkersMap::const_iterator foundTrack = markers_.find(track);
  if (foundTrack == markers_.end()) {
    return;
  }
  ClipMarkersMap::const_iterator foundClip = foundTrack->second.find(clip);
  if (foundClip == foundTrack->second.end()) {
    return;
  }
  for (FrameMarkerMap::const_iterator it2 = foundClip->second.begin(); it2 != foundClip->second.end(); ++it2) {
    markers->push_back(it2->second);
  }
}

void Tracks::GetMarkersInFrame(int clip,
                               int frame,
                               vector<Marker>* markers) const {
  for (TrackMarkersMap::const_iterator it = markers_.begin(); it!=markers_.end(); ++it) {
    ClipMarkersMap::const_iterator foundClip = it->second.find(clip);
    if (foundClip == it->second.end()) {
      continue;
    }
    FrameMarkerMap::const_iterator foundFrame = foundClip->second.find(frame);
    if (foundFrame == foundClip->second.end()) {
      continue;
    } else {
      markers->push_back(foundFrame->second);
    }
  }
}

struct MarkerIteratorCompareLess
{
  bool operator() (const FrameMarkerMap::const_iterator & lhs,
                   const FrameMarkerMap::const_iterator & rhs) const
  {
    if (lhs->second.track < rhs->second.track) {
      return true;
    } else if (lhs->second.track > rhs->second.track) {
      return false;
    } else {
      if (lhs->second.clip < rhs->second.clip) {
        return true;
      } else if (lhs->second.clip > rhs->second.clip) {
        return false;
      } else {
        if (lhs->second.frame < rhs->second.frame) {
          return true;
        } else if (lhs->second.frame > rhs->second.frame) {
          return false;
        } else {
          return false;
        }
      }
    }
  }
};

typedef std::set<FrameMarkerMap::const_iterator, MarkerIteratorCompareLess> FrameMarkersIteratorSet;

void Tracks::GetMarkersForTracksInBothImages(int clip1, int frame1,
                                             int clip2, int frame2,
                                             vector<Marker>* markers) const {
  FrameMarkersIteratorSet image1_tracks;
  FrameMarkersIteratorSet image2_tracks;

  // Collect the tracks in each of the two images.

  for (TrackMarkersMap::const_iterator it = markers_.begin(); it!=markers_.end(); ++it) {
    ClipMarkersMap::const_iterator foundClip = it->second.find(clip1);
    if (foundClip == it->second.end()) {
      continue;
    }
    FrameMarkerMap::const_iterator foundFrame = foundClip->second.find(frame1);
    if (foundFrame == foundClip->second.end()) {
      continue;
    } else {
      image1_tracks.insert(foundFrame);
    }
  }

  for (TrackMarkersMap::const_iterator it = markers_.begin(); it!=markers_.end(); ++it) {
    ClipMarkersMap::const_iterator foundClip = it->second.find(clip2);
    if (foundClip == it->second.end()) {
      continue;
    }
    FrameMarkerMap::const_iterator foundFrame = foundClip->second.find(frame2);
    if (foundFrame == foundClip->second.end()) {
      continue;
    } else {
      image2_tracks.insert(foundFrame);
    }
  }

  // Intersect the two sets to find the tracks of interest.

  for (FrameMarkersIteratorSet::const_iterator it = image1_tracks.begin(); it != image1_tracks.end(); ++it) {
    FrameMarkersIteratorSet::const_iterator foundInImage2Tracks = image2_tracks.find(*it);
    if (foundInImage2Tracks != image2_tracks.end()) {
      markers->push_back((*it)->second);
    }
  }
}

void Tracks::AddMarker(const Marker& marker) {
  ClipMarkersMap& tracks = markers_[marker.track];
  FrameMarkerMap& frames = tracks[marker.clip];
  frames[marker.frame] = marker;
}

void Tracks::SetMarkers(TrackMarkersMap* markers) {
  std::swap(markers_, *markers);
}

bool Tracks::RemoveMarker(int clip, int frame, int track) {
  TrackMarkersMap::iterator foundTrack = markers_.find(track);
  if (foundTrack == markers_.end()) {
    return false;
  }
  ClipMarkersMap::iterator foundClip = foundTrack->second.find(clip);
  if (foundClip == foundTrack->second.end()) {
    return false;
  }
  FrameMarkerMap::iterator foundFrame = foundClip->second.find(frame);
  if (foundFrame == foundClip->second.end()) {
    return false;
  } else {
    foundClip->second.erase(foundFrame);
    return true;
  }
}

void Tracks::RemoveMarkersForTrack(int track) {
  TrackMarkersMap::iterator foundTrack = markers_.find(track);
  if (foundTrack == markers_.end()) {
    return;
  }
  markers_.erase(foundTrack);
}


int Tracks::MaxClip() const {
  int max_clip = 0;
  for (TrackMarkersMap::const_iterator it = markers_.begin(); it!=markers_.end(); ++it) {
    if (!it->second.empty()) {
      max_clip = std::max(it->second.rbegin()->first, max_clip);
    }
  }
  return max_clip;
}

int Tracks::MaxFrame(int clip) const {
  int max_frame = 0;
  for (TrackMarkersMap::const_iterator it = markers_.begin(); it!=markers_.end(); ++it) {
    ClipMarkersMap::const_iterator foundClip = it->second.find(clip);
    if (foundClip == it->second.end()) {
      continue;
    }
    if (!foundClip->second.empty()) {
      max_frame = std::max(foundClip->second.rbegin()->first, max_frame);
    }
  }

  return max_frame;
}

int Tracks::MaxTrack() const {
  if (!markers_.empty()) {
    return markers_.rbegin()->first;
  } else {
    return 0;
  }
}

int Tracks::NumMarkers() const {
  return markers_.size();
}

}  // namespace mv
