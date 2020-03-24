//
// Created by rgrandia on 13.03.20.
//

#pragma once

#include <ocs2_oc/oc_solver/SolverSynchronizedModule.h>

#include "ocs2_switched_model_interface/core/ComModelBase.h"
#include "ocs2_switched_model_interface/core/KinematicsModelBase.h"
#include "ocs2_switched_model_interface/core/SwitchedModel.h"
#include "ocs2_switched_model_interface/foot_planner/SplineCpg.h"

namespace switched_model {

struct SwingTrajectoryPlannerSettings {
  scalar_t liftOffVelocity = 0.0;
  scalar_t touchDownVelocity = 0.0;
  scalar_t swingHeight = 0.1;
  scalar_t touchdownAfterHorizon = 0.2;  // swing time added beyond the horizon if there is no touchdown in the current mode schedule
  scalar_t errorGain = 0.0;        // proportional gain for returning to the planned swing trajectory. 10-90%-rise_time ~= 2.2 / errorGain
                                   // alternatively can be measured as (velocity feedback) / (tracking error) ([m/s] / [m])
  scalar_t swingTimeScale = 0.15;  // swing phases shorter than this time will be scaled down in height and velocity
};

SwingTrajectoryPlannerSettings loadSwingTrajectorySettings(const std::string& filename, bool verbose = true);

class SwingTrajectoryPlanner {
 public:
  SwingTrajectoryPlanner(SwingTrajectoryPlannerSettings settings, const ComModelBase<scalar_t>& comModel,
                         const KinematicsModelBase<scalar_t>& kinematicsModel);

  void update(scalar_t initTime, scalar_t finalTime, const comkino_state_t& currentState, const ocs2::ModeSchedule& modeSchedule,
              scalar_t terrainHeight);

  void update(const ocs2::ModeSchedule& modeSchedule, const std::array<scalar_array_t, NUM_CONTACT_POINTS>& liftOffHeightSequence,
              const std::array<scalar_array_t, NUM_CONTACT_POINTS>& touchDownHeightSequence);

  scalar_t getZvelocityConstraint(size_t leg, scalar_t time) const;

  scalar_t getZpositionConstraint(size_t leg, scalar_t time) const;

 private:
  void updateFeetTrajectories(scalar_t initTime, scalar_t finalTime, const std::array<vector3_t, NUM_CONTACT_POINTS>& currentFeetPositions,
                              const ocs2::ModeSchedule& modeSchedule, scalar_t terrainHeight);

  void updateErrorTrajectories(scalar_t initTime, const std::array<vector3_t, NUM_CONTACT_POINTS>& currentFeetPositions,
                               const ocs2::ModeSchedule& modeSchedule);

  SwingTrajectoryPlannerSettings settings_;
  std::unique_ptr<ComModelBase<scalar_t>> comModel_;
  std::unique_ptr<KinematicsModelBase<scalar_t>> kinematicsModel_;

  struct contactHistory {
    scalar_t time;
    scalar_t height;
  };
  std::array<contactHistory, NUM_CONTACT_POINTS> lastContacts_;
  std::array<std::vector<SplineCpg>, NUM_CONTACT_POINTS> feetHeightTrajectories_;
  std::array<std::vector<scalar_t>, NUM_CONTACT_POINTS> feetHeightTrajectoriesEvents_;

  // Error correction
  scalar_t initTime_;
  std::array<scalar_t, NUM_CONTACT_POINTS> initialErrors_;

 public:
  /** Helper functions */
  enum class FootPhaseType { Stance, Swing };
  struct FootPhase {
    FootPhaseType type;
    scalar_t startTime;  // times are NaN if they cannot be identified at the boundaries
    scalar_t endTime;
  };
  static std::vector<FootPhase> extractFootPhases(const std::vector<scalar_t>& eventTimes, const std::vector<bool>& contactFlags);

  /**
   * Extracts for each leg the contact sequence over the motion phase sequence.
   * @param phaseIDsStock
   * @return contactFlagStock
   */
  static std::array<std::vector<bool>, NUM_CONTACT_POINTS> extractContactFlags(const std::vector<size_t>& phaseIDsStock);
};

}  // namespace switched_model
