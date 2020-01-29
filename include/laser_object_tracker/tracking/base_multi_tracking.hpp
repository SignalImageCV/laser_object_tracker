/*********************************************************************
*
* BSD 3-Clause License
*
*  Copyright (c) 2019, Piotr Pokorski
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice, this
*     list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3. Neither the name of the copyright holder nor the names of its
*     contributors may be used to endorse or promote products derived from
*     this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#ifndef LASER_OBJECT_TRACKER_TRACKING_BASE_MULTI_TRACKING_HPP
#define LASER_OBJECT_TRACKER_TRACKING_BASE_MULTI_TRACKING_HPP

#include <Eigen/Core>

#include <ros/time.h>

#include "laser_object_tracker/feature_extraction/features/features.hpp"

namespace laser_object_tracker {
namespace tracking {

template<class FeatureType, class TrackType>
class BaseMultiTracking {
 public:
  using FeatureT = FeatureType;
  using TrackT = TrackType;

  using Container = std::vector<TrackT>;
  using value_type = typename Container::value_type;
  using reference = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using iterator = typename Container::iterator;
  using const_iterator = typename Container::const_iterator;
  using difference_type = typename Container::difference_type;
  using size_type = typename Container::size_type;

  virtual void predict() = 0;

  virtual const Container& update(const std::vector<FeatureT>& measurements) = 0;

  const Container& getTracks() const {
    return tracks_;
  }

  iterator begin() {
    return tracks_.begin();
  }

  const_iterator begin() const {
    return tracks_.begin();
  }

  const_iterator cbegin() const {
    return tracks_.cbegin();
  }

  iterator end() {
    return tracks_.end();
  }

  const_iterator end() const {
    return tracks_.end();
  }

  const_iterator cend() const {
    return tracks_.cend();
  }

  size_type size() const {
    return tracks_.size();
  }

  bool empty() const {
    return tracks_.empty();
  }

  virtual ~BaseMultiTracking() = default;

 protected:
  Container tracks_;
};
}  // namespace tracking
}  // namespace laser_object_tracker

#endif //LASER_OBJECT_TRACKER_TRACKING_BASE_MULTI_TRACKING_HPP
