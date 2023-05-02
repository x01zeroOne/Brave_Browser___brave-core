/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/typing.h"

#include <vector>

#include "brave/components/brave_federated/task/model.h"

namespace brave_federated {

Task::Task(TaskId task_id,
           TaskType type,
           std::string token,
           std::vector<Weights> parameters,
           std::map<std::string, float> config)
    : task_id_(task_id),
      type_(type),
      token_(token),
      parameters_(parameters),
      config_(config) {}
Task::Task(const Task& other) = default;
Task::~Task() = default;

const TaskId& Task::GetId() const {
  return task_id_;
}

const TaskType& Task::GetType() const {
  return type_;
}

const std::string& Task::GetToken() const {
  return token_;
}

const std::vector<Weights>& Task::GetParameters() const {
  return parameters_;
}

const std::map<std::string, float>& Task::GetConfig() const {
  return config_;
}

TaskResult::TaskResult(Task task, PerformanceReport report)
    : task_(task), report_(report) {}

TaskResult::TaskResult(const TaskResult& other) = default;
TaskResult::~TaskResult() = default;

const Task& TaskResult::GetTask() const {
  return task_;
}

const PerformanceReport& TaskResult::GetReport() const {
  return report_;
}

TaskResultResponse::TaskResultResponse(bool success) : success_(success) {}

bool TaskResultResponse::IsSuccessful() {
  return success_;
}

TaskResultResponse::~TaskResultResponse() = default;

}  // namespace brave_federated
