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

#ifndef EINO_CPP_FLOW_FLOW_H_
#define EINO_CPP_FLOW_FLOW_H_

// Retriever flows
#include "retriever/parent_retriever.h"
#include "retriever/multi_query_retriever.h"
#include "retriever/router_retriever.h"

// Indexer flows
#include "indexer/parent_indexer.h"

// Agent flows
#include "agent/react.h"

/*
 * Eino Flow Framework
 * 
 * The flow framework provides pre-built, composable components for common AI/ML workflows.
 * 
 * Key Components:
 * 
 * 1. Retriever Flows:
 *    - ParentRetriever: Retrieves parent documents based on sub-document search results
 *    - MultiQueryRetriever: Generates multiple query variations and fuses results
 *    - RouterRetriever: Routes queries to different retrievers and fuses results
 * 
 * 2. Indexer Flows:
 *    - ParentIndexer: Handles parent-child document relationships during indexing
 * 
 * 3. Agent Flows:
 *    - ReActAgent: Implements the ReAct (Reasoning + Acting) pattern
 */

#endif // EINO_CPP_FLOW_FLOW_H_
