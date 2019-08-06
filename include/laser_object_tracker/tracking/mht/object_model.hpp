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

#ifndef LASER_OBJECT_TRACKER_TRACKING_MHT_OBJECT_MODEL_HPP
#define LASER_OBJECT_TRACKER_TRACKING_MHT_OBJECT_MODEL_HPP

#include <list>

#include <Eigen/Core>

#include <opencv2/core/eigen.hpp>
#include <opencv2/video/tracking.hpp>

#include <mht/except.h>
#include <mht/matrix.h>
#include <mht/mdlmht.h>

#include "laser_object_tracker/feature_extraction/features/object.hpp"
#include "laser_object_tracker/tracking/mht/track.hpp"

namespace laser_object_tracker {
namespace tracking {
namespace mht {
cv::KalmanFilter buildKalmanFilter(int state_dimensions,
                                   int measurement_dimensions,
                                   const Eigen::MatrixXd& transition_matrix,
                                   const Eigen::MatrixXd& measurement_matrix,
                                   const Eigen::MatrixXd& measurement_noise_covariance,
                                   const Eigen::MatrixXd& initial_state_covariance,
                                   const Eigen::MatrixXd& process_noise_covariance);

cv::KalmanFilter copyKalmanFilter(const cv::KalmanFilter& kalman_filter);

double calculateMahalanobisDistance(const cv::KalmanFilter& kalman_filter,
                                    const cv::Mat& measurement);

double calculateLogLikelihood(const cv::KalmanFilter& kalman_filter,
                              double mahalanobis_distance);

double angleBetweenAngles(double target, double source);

double absAngleBetweenAngles(double target, double source);

double assignmentCost(const feature_extraction::features::Segment2D& lhs,
                      const feature_extraction::features::Segment2D& rhs);

class ObjectReport : public MDL_REPORT {
 public:
  ObjectReport(double false_alarm_log_likelihood,
               const feature_extraction::features::Object& object,
               int frame_number,
               size_t corner_id)
      : false_alarm_log_likelihood_(false_alarm_log_likelihood),
        object_(object),
        frame_number_(frame_number),
        corner_id_(corner_id) {}

  double getFalseAlarmLogLikelihood() const {
    return false_alarm_log_likelihood_;
  }

  const feature_extraction::features::Object& getObject() const {
    return object_;
  }

  int getFrameNumber() const {
    return frame_number_;
  }

  size_t getCornerId() const {
    return corner_id_;
  }

 private:
  double false_alarm_log_likelihood_;

  feature_extraction::features::Object object_;

  int frame_number_;
  size_t corner_id_;
};

class ObjectState : public MDL_STATE {
 public:
  static constexpr int STATE_DIMENSION = 4;
  static constexpr int MEASUREMENT_DIMENSION = 2;

  using ReferencePointType = feature_extraction::features::Object::ReferencePointType;
  using ReferencePointSource = feature_extraction::features::Object::ReferencePointSource;

  using State = Eigen::Matrix<double, STATE_DIMENSION, 1>;
  using Measurement = Eigen::Matrix<double, MEASUREMENT_DIMENSION, 1>;

  using StateTransition = Eigen::Matrix<double, STATE_DIMENSION, STATE_DIMENSION>;
  using MeasurementMatrix = Eigen::Matrix<double, MEASUREMENT_DIMENSION, STATE_DIMENSION>;

  using MeasurementNoiseCovariance = Eigen::Matrix<double, MEASUREMENT_DIMENSION, MEASUREMENT_DIMENSION>;
  using InitialStateCovariance = Eigen::Matrix<double, STATE_DIMENSION, STATE_DIMENSION>;
  using ProcessNoiseCovariance = Eigen::Matrix<double, STATE_DIMENSION, STATE_DIMENSION>;

  ObjectState(MODEL* model,
              double time_step,
              double log_likelihood,
              int times_skipped,
              ReferencePointType reference_point_type,
              const ReferencePointSource& reference_point_source,
              const cv::KalmanFilter& kalman_filter,
              const State& state)
      : ObjectState(model,
                    time_step,
                    log_likelihood,
                    times_skipped,
                    reference_point_type,
                    reference_point_source,
                    kalman_filter) {
    cv::eigen2cv(state, kalman_filter_.statePre);
    cv::eigen2cv(state, kalman_filter_.statePost);
  }

  ObjectState(MODEL* model,
              double time_step,
              double log_likelihood,
              int times_skipped,
              ReferencePointType reference_point_type,
              const ReferencePointSource& reference_point_source,
              const cv::KalmanFilter& kalman_filter)
      : MDL_STATE(model),
        time_step_(time_step),
        log_likelihood_(log_likelihood),
        times_skipped_(times_skipped),
        reference_point_type_(reference_point_type),
        kalman_filter_(copyKalmanFilter(kalman_filter)) {
    initializeWithPointSource(reference_point_source);
  }

  ObjectState(MODEL* model,
              double time_step,
              double log_likelihood,
              int times_skipped,
              ReferencePointType reference_point_type,
              const ReferencePointSource& reference_point_source,
              cv::KalmanFilter&& kalman_filter,
              const State& state)
      : MDL_STATE(model),
        time_step_(time_step),
        log_likelihood_(log_likelihood),
        times_skipped_(times_skipped),
        reference_point_type_(reference_point_type),
        kalman_filter_(std::move(kalman_filter)) {
    cv::eigen2cv(state, kalman_filter_.statePre);
    cv::eigen2cv(state, kalman_filter_.statePost);
    initializeWithPointSource(reference_point_source);
  }

  ObjectState(const ObjectState& other)
      : MDL_STATE(other.getMdl()),
        time_step_(other.time_step_),
        log_likelihood_(other.log_likelihood_),
        times_skipped_(other.times_skipped_),
        reference_point_type_(other.reference_point_type_),
        kalman_filter_(copyKalmanFilter(other.kalman_filter_)),
        segment_1_(other.segment_1_),
        segment_2_(other.segment_2_),
        is_second_initialized_(other.is_second_initialized_) {}

  double getLogLikelihood() override {
    return log_likelihood_;
  }

  void setLogLikelihood(double log_likelihood) {
    log_likelihood_ = log_likelihood;
  }

  void predict() {
    kalman_filter_.predict();
  }

  void update(const Measurement& measurement) {
    cv::Mat cv_measurement;
    cv::eigen2cv(measurement, cv_measurement);
    kalman_filter_.correct(cv_measurement);
  }

  void incrementTimesSkipped() {
    ++times_skipped_;
  }

  void resetTimesSkipped() {
    times_skipped_ = 0;
  }

  int getTimesSkipped() const {
    return times_skipped_;
  }

  ReferencePointType getReferencePointType() const {
    return reference_point_type_;
  }

  std::pair<const feature_extraction::features::Segment2D*,
            const feature_extraction::features::Segment2D*> updateReferencePointSource(const ReferencePointSource& source);

  double getX() const {
    return kalman_filter_.statePost.at<double>(0);
  }

  double getXPredicted() const {
    return kalman_filter_.statePre.at<double>(0);
  }

  void setXPredicted(double x) {
    kalman_filter_.statePre.at<double>(0) = x;
  }

  double getY() const {
    return kalman_filter_.statePost.at<double>(1);
  }

  double getYPredicted() const {
    return kalman_filter_.statePre.at<double>(1);
  }

  void setYPredicted(double y) {
    kalman_filter_.statePre.at<double>(1) = y;
  }

  double getVelocityX() const {
    return kalman_filter_.statePost.at<double>(2);
  }

  double getVelocityY() const {
    return kalman_filter_.statePost.at<double>(3);
  }

  const cv::KalmanFilter& getKalmanFilter() const {
    return kalman_filter_;
  }

 private:
  void initializeWithPointSource(const ReferencePointSource& source);

  double time_step_;

  double log_likelihood_;

  int times_skipped_;

  ReferencePointType reference_point_type_;

  cv::KalmanFilter kalman_filter_;

  feature_extraction::features::Segment2D segment_1_, segment_2_;

  bool is_second_initialized_ = false;
};

class ObjectModel : public MODEL {
 public:
  ObjectModel(double time_step,
              double max_mahalanobis_distance,
              double skip_decay_rate,
              double start_likelihood,
              double detect_likelihood,
              const ObjectState::MeasurementNoiseCovariance& measurement_noise_covariance,
              const ObjectState::InitialStateCovariance& initial_state_covariance,
              const ObjectState::ProcessNoiseCovariance& process_noise_covariance)
      : MODEL(),
        time_step_(time_step),
        max_mahalanobis_distance_(max_mahalanobis_distance),
        skip_decay_rate_(skip_decay_rate),
        start_log_likelihood_(std::log(start_likelihood)),
        skip_log_likelihood_(std::log(1 - detect_likelihood)),
        detect_log_likelihood_(std::log(detect_likelihood)),
        measurement_noise_covariance_(measurement_noise_covariance),
        initial_state_covariance_(initial_state_covariance),
        process_noise_covariance_(process_noise_covariance) {
    state_transition_ << 1.0, 0.0, time_step_, 0.0,
        0.0, 1.0, 0.0, time_step_,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0;

    measurement_matrix_ << 1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0;
  }

  int beginNewStates(MDL_STATE *state, MDL_REPORT *report) override {
    return 1;
  }

  double getEndLogLikelihood(MDL_STATE *state) override {
    auto object_state = dynamic_cast<ObjectState&>(*state);
    return std::log(getEndProbability(object_state));
  }

  double getContinueLogLikelihood(MDL_STATE *state) override {
    auto object_state = dynamic_cast<ObjectState&>(*state);
    return std::log(1.0 - getEndProbability(object_state));
  }

  double getSkipLogLikelihood(MDL_STATE *state) override {
    return skip_log_likelihood_;
  }

  double getDetectLogLikelihood(MDL_STATE *state) override {
    return detect_log_likelihood_;
  }

  MDL_STATE *getNewState(int i, MDL_STATE *state, MDL_REPORT *report) override;

 private:
  double getEndProbability(const ObjectState& state) const;
  double mahalanobisDistance(const ObjectState& state,
                             const ObjectReport& report) const;
  cv::KalmanFilter getKalmanFilter() const;

  double time_step_;
  double max_mahalanobis_distance_;

  // The higher rate, the slower decay
  double skip_decay_rate_;

  double start_log_likelihood_;
  double skip_log_likelihood_;
  double detect_log_likelihood_;

  ObjectState::MeasurementNoiseCovariance  measurement_noise_covariance_;
  ObjectState::InitialStateCovariance initial_state_covariance_;
  ObjectState::ProcessNoiseCovariance process_noise_covariance_;

  ObjectState::StateTransition state_transition_;
  ObjectState::MeasurementMatrix measurement_matrix_;

 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class ObjectFalseAlarm : public DLISTnode {
 public:
  explicit ObjectFalseAlarm(const ObjectReport* report)
      : DLISTnode(),
        x_(report->getObject().getReferencePoint().x()),
        y_(report->getObject().getReferencePoint().x()),
        frame_number_(report->getFrameNumber()),
        corner_id_(report->getCornerId()) {}

 protected:
  MEMBERS_FOR_DLISTnode(ObjectFalseAlarm)

 private:
  double x_, y_;
  int frame_number_;
  size_t corner_id_;
};

class ObjectTracker : public MDL_MHT {
 public:
  ObjectTracker(double false_alarm_likelihood,
                int max_depth,
                double min_g_hypo_ratio,
                int max_g_hypos,
                const ptrDLIST_OF<MODEL>& models)
      : MDL_MHT(max_depth,
                min_g_hypo_ratio,
                max_g_hypos),
        false_alarm_log_likelihood_(std::log(false_alarm_likelihood)) {
    m_modelList.appendCopy(models);
  }

  const std::list<Track>& getTracks() const {
    return tracks_;
  }

  const std::list<ObjectFalseAlarm>& getFalseAlarms() const {
    return false_alarms_;
  }

 protected:
  void measure(const std::list<REPORT*>& new_reports) override;

  void startTrack(int i, int i1, MDL_STATE* state, MDL_REPORT* report) override;

  void continueTrack(int i, int i1, MDL_STATE* state, MDL_REPORT* report) override;

  void skipTrack(int i, int i1, MDL_STATE* state) override;

  void endTrack(int i, int i1) override;

  void falseAlarm(int i, MDL_REPORT* report) override;

 private:
  Track* findTrack(int id);

  void verify(int track_id,
              double state_x,
              double state_y,
              double velocity_x,
              double velocity_y,
              double report_x,
              double report_y,
              double likelihood,
              int frame,
              size_t corner_id);

  double false_alarm_log_likelihood_;
  std::list<Track> tracks_;
  std::list<ObjectFalseAlarm> false_alarms_;
};

}  // namespace mht
}  // namespace tracking
}  // namespace laser_object_tracker

#endif  // LASER_OBJECT_TRACKER_TRACKING_MHT_OBJECT_MODEL_HPP