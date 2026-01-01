/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EINO_CPP_ADK_PROMPTS_H_
#define EINO_CPP_ADK_PROMPTS_H_

#include <string>

namespace eino {
namespace adk {
namespace prompts {

// ============================================================================
// Deep Agent Prompts
// 对齐 eino/adk/prebuilt/deep/prompt.go
// ============================================================================

// WriteTodosPrompt provides guidance on using the write_todos tool
// 对齐 eino/adk/prebuilt/deep/prompt.go:30-45
extern const char* kWriteTodosPrompt;

// TaskPrompt provides guidance on using the task tool for subagent spawning
// 对齐 eino/adk/prebuilt/deep/prompt.go:46-75
extern const char* kTaskPrompt;

// BaseAgentPrompt is the base prompt for all Deep agents
// 对齐 eino/adk/prebuilt/deep/prompt.go:76
extern const char* kBaseAgentPrompt;

// TaskToolDescription is the full description template for the task tool
// 对齐 eino/adk/prebuilt/deep/prompt.go:77-179
extern const char* kTaskToolDescription;

// WriteTodosToolDescription provides detailed instructions for the write_todos tool
// 对齐 eino/adk/prebuilt/deep/prompt.go:180-最后
extern const char* kWriteTodosToolDescription;

// ============================================================================
// Plan-Execute Agent Prompts
// 对齐 eino/adk/prebuilt/planexecute/plan_execute.go
// ============================================================================

// PlannerPrompt is the prompt template for the planner agent
// 对齐 eino/adk/prebuilt/planexecute/plan_execute.go:120-145
extern const char* kPlannerPrompt;

// ExecutorPrompt is the prompt template for the executor agent
// 对齐 eino/adk/prebuilt/planexecute/plan_execute.go:147-155
extern const char* kExecutorPrompt;

// ReplannerPrompt is the prompt template for the replanner agent
// 对齐 eino/adk/prebuilt/planexecute/plan_execute.go:157-189
extern const char* kReplannerPrompt;

} // namespace prompts
} // namespace adk
} // namespace eino

#endif // EINO_CPP_ADK_PROMPTS_H_
