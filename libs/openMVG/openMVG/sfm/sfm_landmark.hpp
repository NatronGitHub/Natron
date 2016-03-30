// Copyright (c) 2015 Pierre Moulon.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_SFM_LANDMARK_HPP
#define OPENMVG_SFM_LANDMARK_HPP

#include "openMVG/numeric/numeric.h"

#ifndef OPENMVG_NO_SERIALIZATION
#include <cereal/cereal.hpp> // Serialization
#endif

namespace openMVG {
namespace sfm {

/// Define 3D-2D tracking data: 3D landmark with it's 2D observations
struct Observation
{
  Observation():id_feat(UndefinedIndexT) {  }
  Observation(const Vec2 & p, IndexT idFeat): x(p), id_feat(idFeat) {}

  Vec2 x;
  IndexT id_feat;

#ifndef OPENMVG_NO_SERIALIZATION
  // Serialization
  template <class Archive>
  void save( Archive & ar) const
  {
    ar(cereal::make_nvp("id_feat", id_feat ));
    const std::vector<double> pp = { x(0), x(1) };
    ar(cereal::make_nvp("x", pp));
  }

  // Serialization
  template <class Archive>
  void load( Archive & ar)
  {
    ar(cereal::make_nvp("id_feat", id_feat ));
    std::vector<double> p(2);
    ar(cereal::make_nvp("x", p));
    x = Eigen::Map<const Vec2>(&p[0]);
  }
#endif // #ifndef OPENMVG_NO_SERIALIZATION
};
/// Observations are indexed by their View_id
typedef Hash_Map<IndexT, Observation> Observations;

/// Define a landmark (a 3D point, with it's 2d observations)
struct Landmark
{
  Observations obs;
  Vec3 X;

#ifndef OPENMVG_NO_SERIALIZATION
  // Serialization
  template <class Archive>
  void save( Archive & ar) const
  {
    const std::vector<double> point = { X(0), X(1), X(2) };
    ar(cereal::make_nvp("X", point ));
    ar(cereal::make_nvp("observations", obs));
  }

  template <class Archive>
  void load( Archive & ar)
  {
    std::vector<double> point(3);
    ar(cereal::make_nvp("X", point ));
    X = Eigen::Map<const Vec3>(&point[0]);
    ar(cereal::make_nvp("observations", obs));
  }
#endif // #ifndef OPENMVG_NO_SERIALIZATION
};

} // namespace sfm
} // namespace openMVG

#endif // OPENMVG_SFM_LANDMARK_HPP
