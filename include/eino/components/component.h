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

#ifndef EINO_CPP_COMPONENTS_COMPONENT_H_
#define EINO_CPP_COMPONENTS_COMPONENT_H_

// Include all component interfaces

#include "document.h"      // Loader, Transformer, Parser
#include "model.h"         // BaseChatModel, ChatModel, ToolCallingChatModel
#include "embedding.h"     // Embedder
#include "retriever.h"     // Retriever
#include "tool.h"          // BaseTool, InvokableTool, StreamableTool, ToolsNode
#include "prompt.h"        // ChatTemplate, PromptTemplate
#include "indexer.h"       // Indexer
#include "interface.h"     // Component interface utilities

#endif // EINO_CPP_COMPONENTS_COMPONENT_H_
