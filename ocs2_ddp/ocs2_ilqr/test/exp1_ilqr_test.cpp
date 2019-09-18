/******************************************************************************
Copyright (c) 2017, Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <gtest/gtest.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include <ocs2_ilqr/ILQR_MT.h>
#include <ocs2_ilqr/ILQR_ST.h>

#include <ocs2_oc/test/EXP1.h>

using namespace ocs2;

enum { STATE_DIM = 2, INPUT_DIM = 1 };

TEST(exp1_ilqr_test, exp1_ilqr_test) {
  ILQR_Settings ilqrSettings;
  ilqrSettings.ddpSettings_.displayInfo_ = false;
  ilqrSettings.ddpSettings_.displayShortSummary_ = true;
  ilqrSettings.ddpSettings_.absTolODE_ = 1e-10;
  ilqrSettings.ddpSettings_.relTolODE_ = 1e-7;
  ilqrSettings.ddpSettings_.maxNumStepsPerSecond_ = 10000;
  ilqrSettings.ddpSettings_.nThreads_ = 3;
  ilqrSettings.ddpSettings_.maxNumIterations_ = 30;
  ilqrSettings.ddpSettings_.lsStepsizeGreedy_ = true;
  ilqrSettings.ddpSettings_.noStateConstraints_ = true;
  ilqrSettings.ddpSettings_.minLearningRate_ = 0.0001;
  ilqrSettings.ddpSettings_.minRelCost_ = 5e-4;
  ilqrSettings.ddpSettings_.checkNumericalStability_ = false;
  ilqrSettings.ddpSettings_.useFeedbackPolicy_ = false;
  ilqrSettings.ddpSettings_.debugPrintRollout_ = false;

  ilqrSettings.rolloutSettings_.absTolODE_ = 1e-11;
  ilqrSettings.rolloutSettings_.relTolODE_ = 1e-8;
  ilqrSettings.rolloutSettings_.maxNumStepsPerSecond_ = 10000;

  // switching times
  std::vector<double> switchingTimes{0.2262, 1.0176};
  std::vector<size_t> subsystemsSequence{0, 1, 2};
  std::shared_ptr<EXP1_LogicRules> logicRules(new EXP1_LogicRules(switchingTimes, subsystemsSequence));

  double startTime = 0.0;
  double finalTime = 3.0;

  // partitioning times
  std::vector<double> partitioningTimes;
  partitioningTimes.push_back(startTime);
  partitioningTimes.push_back(switchingTimes[0]);
  partitioningTimes.push_back(switchingTimes[1]);
  partitioningTimes.push_back(finalTime);

  EXP1_System::state_vector_t initState(2.0, 3.0);

  /******************************************************************************************************/
  /******************************************************************************************************/
  /******************************************************************************************************/
  // system dynamics
  EXP1_System systemDynamics(logicRules);

  // system derivatives
  EXP1_SystemDerivative systemDerivative(logicRules);

  // system constraints
  EXP1_SystemConstraint systemConstraint;

  // system cost functions
  EXP1_CostFunction systemCostFunction(logicRules);

  // system operatingTrajectories
  Eigen::Matrix<double, STATE_DIM, 1> stateOperatingPoint = Eigen::Matrix<double, STATE_DIM, 1>::Zero();
  Eigen::Matrix<double, INPUT_DIM, 1> inputOperatingPoint = Eigen::Matrix<double, INPUT_DIM, 1>::Zero();
  EXP1_SystemOperatingTrajectories operatingTrajectories(stateOperatingPoint, inputOperatingPoint);

  /******************************************************************************************************/
  /******************************************************************************************************/
  /******************************************************************************************************/
  // ILQR - single-threaded version
  ILQR_ST<STATE_DIM, INPUT_DIM> ilqrST(&systemDynamics, &systemDerivative, &systemConstraint, &systemCostFunction, &operatingTrajectories,
                                       ilqrSettings, logicRules);

  // ILQR - multi-threaded version
  //	ILQR_MT<STATE_DIM, INPUT_DIM> ilqrMT(
  //			&systemDynamics, &systemDerivative,
  //			&systemConstraint, &systemCostFunction,
  //			&operatingTrajectories, ilqrSettings, logicRules);

  // run single_threaded core ILQR
  if (ilqrSettings.ddpSettings_.displayInfo_ || ilqrSettings.ddpSettings_.displayShortSummary_)
    std::cerr << "\n>>> single-threaded ILQR" << std::endl;
  ilqrST.run(startTime, initState, finalTime, partitioningTimes);

  // run multi-threaded ILQR
  //	if (ilqrSettings.ddpSettings_.displayInfo_ || ilqrSettings.ddpSettings_.displayShortSummary_)
  //		std::cerr << "\n>>> multi-threaded ILQR" << std::endl;
  //	ilqrMT.run(startTime, initState, finalTime, partitioningTimes);

  /******************************************************************************************************/
  /******************************************************************************************************/
  /******************************************************************************************************/
  // get solution
  ILQR_BASE<STATE_DIM, INPUT_DIM>::primal_solution_t solutionST = ilqrST.primalSolution(finalTime);
  //	ILQR_BASE<STATE_DIM, INPUT_DIM>::primal_solution_t solutionMT = ilqrMT.primalSolution(finalTime);

  // get performance indices
  double totalCost_st, totalCost_mt;
  double constraint1ISE_st, constraint1ISE_mt;
  double constraint2ISE_st, constraint2ISE_mt;
  ilqrST.getPerformanceIndeces(totalCost_st, constraint1ISE_st, constraint2ISE_st);
  //	ilqrMT.getPerformanceIndeces(totalCost_mt, constraint1ISE_mt, constraint2ISE_mt);

  /******************************************************************************************************/
  /******************************************************************************************************/
  /******************************************************************************************************/
  const double expectedCost = 5.4399;
  ASSERT_LT(fabs(totalCost_st - expectedCost), 10 * ilqrSettings.ddpSettings_.minRelCost_)
      << "MESSAGE: ILQR_ST failed in the EXP1's cost test!";
  //	ASSERT_LT(fabs(totalCost_mt - expectedCost), 10*ilqrSettings.ddpSettings_.minRelCost_) <<
  //			"MESSAGE: ILQR_MT failed in the EXP1's cost test!";

  const double expectedISE1 = 0.0;
  ASSERT_LT(fabs(constraint1ISE_st - expectedISE1), 10 * ilqrSettings.ddpSettings_.minRelConstraint1ISE_)
      << "MESSAGE: ILQR_ST failed in the EXP1's type-1 constraint ISE test!";
  //	ASSERT_LT(fabs(constraint1ISE_mt - expectedISE1), 10*ilqrSettings.ddpSettings_.minRelConstraint1ISE_) <<
  //			"MESSAGE: ILQR_MT failed in the EXP1's type-1 constraint ISE test!";

  const double expectedISE2 = 0.0;
  ASSERT_LT(fabs(constraint2ISE_st - expectedISE2), 10 * ilqrSettings.ddpSettings_.minRelConstraint1ISE_)
      << "MESSAGE: ILQR_ST failed in the EXP1's type-2 constraint ISE test!";
  //	ASSERT_LT(fabs(constraint2ISE_mt - expectedISE2), 10*ilqrSettings.ddpSettings_.minRelConstraint1ISE_) <<
  //			"MESSAGE: ILQR_MT failed in the EXP1's type-2 constraint ISE test!";

  double ctrlFinalTime;
  if (ilqrSettings.ddpSettings_.useFeedbackPolicy_) {
	  ctrlFinalTime = dynamic_cast<ILQR_ST<STATE_DIM, INPUT_DIM>::linear_controller_t*>(solutionST.controllerPtr_.get())->timeStamp_.back();
  } else {
	  ctrlFinalTime = dynamic_cast<ILQR_ST<STATE_DIM, INPUT_DIM>::feedforward_controller_t*>(solutionST.controllerPtr_.get())->timeStamp_.back();
  }
  ASSERT_DOUBLE_EQ(solutionST.timeTrajectory_.back(), finalTime) << "MESSAGE: ILQR_ST failed in policy final time of trajectory!";
  ASSERT_DOUBLE_EQ(ctrlFinalTime, finalTime) << "MESSAGE: ILQR_ST failed in policy final time of controller!";
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
