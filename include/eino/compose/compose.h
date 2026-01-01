/*
 * Copyright 2024 CloudWeGo Authors
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

#ifndef EINO_CPP_COMPOSE_H_
#define EINO_CPP_COMPOSE_H_

// Include all compose module headers
#include "types.h"
#include "error.h"
#include "state.h"
#include "types_lambda.h"
#include "runnable.h"
#include "chain.h"
#include "graph.h"
#include "workflow.h"
#include "field_mapping.h"
#include "branch.h"
#include "chain_branch.h"

/**
 * @file compose.h
 * @brief Compose module for orchestrating AI components
 * 
 * This module provides tools for composing and orchestrating multiple AI components
 * together in various patterns:
 * 
 * - **Runnable**: Base interface for executable components
 * - **Chain**: Linear sequence of components
 * - **Graph**: Complex DAG orchestration with branching
 * - **Workflow**: Stateful composition with shared state
 * - **FieldMapping**: Data transformation and field mapping
 * 
 * ## Quick Start
 * 
 * ### Creating a Simple Chain
 * ```cpp
 * // Create components
 * auto prompt = NewLambdaRunnable<Input, PromptTemplate>(
 *     [](const Input& in) { return PromptTemplate(in.query); }
 * );
 * auto llm = NewLambdaRunnable<PromptTemplate, Response>(
 *     [](const PromptTemplate& pt) { return llm_client.call(pt); }
 * );
 * 
 * // Chain them together
 * auto chain = Chain2(prompt, llm);
 * 
 * // Execute
 * Input input{"What is AI?"};
 * Response result = chain->Invoke(input);
 * ```
 * 
 * ### Building a Graph with Branches
 * ```cpp
 * auto graph = std::make_shared<Graph<Input, Output>>();
 * 
 * // Add nodes
 * graph->AddNode("classifier", classifier_runnable);
 * graph->AddNode("summarizer", summarizer_runnable);
 * graph->AddNode("qa", qa_runnable);
 * 
 * // Add edges
 * graph->AddEdge("start", "classifier");
 * graph->AddEdge("classifier", "summarizer", "long");
 * graph->AddEdge("classifier", "qa", "short");
 * graph->AddEdge("summarizer", "end");
 * graph->AddEdge("qa", "end");
 * 
 * graph->Compile();
 * Output result = graph->Invoke(input);
 * ```
 * 
 * ### Streaming Data
 * ```cpp
 * auto chain = Chain2(input_gen, processor);
 * auto stream = chain->Stream(input);
 * 
 * // Process streamed results
 * Output item;
 * while (stream && stream->Read(item)) {
 *     ProcessOutput(item);
 * }
 * ```
 * 
 * ## Data Flow Patterns
 * 
 * Each Runnable supports four data flow patterns:
 * 
 * 1. **Invoke**: Single input → Single output (most common)
 * 2. **Stream**: Single input → Stream of outputs (chunked responses)
 * 3. **Collect**: Stream of inputs → Single output (aggregation)
 * 4. **Transform**: Stream of inputs → Stream of outputs (full pipeline)
 * 
 * This enables flexible composition and automatic adaptation.
 */

namespace eino {
namespace compose {

// Re-export commonly used types
using RunOption = Option;
// Note: GraphNode and GraphEdge are templates, cannot create non-template aliases
// Use them directly with template parameters

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_H_
