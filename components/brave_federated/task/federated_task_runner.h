/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_FEDERATED_TASK_RUNNER_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_FEDERATED_TASK_RUNNER_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_federated/task/model.h"
#include "brave/components/brave_federated/task/typing.h"
#include "brave/components/brave_federated/util/linear_algebra_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_federated {

class Model;
class Task;
using ModelWeights = std::tuple<Weights, float>;

class FederatedTaskRunner final {
 public:
  FederatedTaskRunner(Task task, std::unique_ptr<Model> model);
  ~FederatedTaskRunner();

  Model* GetModel();

  absl::optional<TaskResult> Run();

  void SetTrainingData(DataSet training_data);
  void SetTestData(DataSet test_data);

  void SetWeights(ModelWeights weights);
  ModelWeights GetWeights();

 private:
  PerformanceReport Evaluate();
  PerformanceReport Train();

  Task task_;
  std::unique_ptr<Model> model_;
  DataSet training_data_;
  DataSet test_data_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_TASK_FEDERATED_TASK_RUNNER_H_
