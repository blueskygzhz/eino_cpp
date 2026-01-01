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

#ifndef EINO_CPP_COMPONENTS_CALLBACK_EXTRA_H_
#define EINO_CPP_COMPONENTS_CALLBACK_EXTRA_H_

#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace eino {
namespace components {

using json = nlohmann::json;

// CallbackExtra provides component-specific metadata for callbacks
// Each component type has its own callback_extra metadata format
// This is a base interface for component callback metadata

struct CallbackExtra {
    // Component input information
    struct Input {
        std::string type_name;      // Type name of component input
        std::string description;    // Human-readable description
        json schema;               // JSON schema describing input structure
    };
    
    // Component output information
    struct Output {
        std::string type_name;      // Type name of component output
        std::string description;    // Human-readable description
        json schema;               // JSON schema describing output structure
    };
    
    Input input;
    Output output;
    
    // Extended metadata for this component type
    std::map<std::string, json> extra;
};

// ChatModelCallbackExtra - Metadata for chat model components
struct ChatModelCallbackExtra : public CallbackExtra {
    // ChatModel specific metadata
    struct ModelInfo {
        std::string model_name;
        std::string provider;
        std::map<std::string, std::string> capabilities;  // streaming, tool_calling, etc.
    };
    
    ModelInfo model_info;
};

// EmbeddingCallbackExtra - Metadata for embedding components
struct EmbeddingCallbackExtra : public CallbackExtra {
    // Embedding specific metadata
    struct EmbeddingInfo {
        std::string model_name;
        int embedding_dimension;
        std::string distance_metric;  // cosine, euclidean, etc.
    };
    
    EmbeddingInfo embedding_info;
};

// ToolCallbackExtra - Metadata for tool components
struct ToolCallbackExtra : public CallbackExtra {
    // Tool specific metadata
    struct ToolInfo {
        std::string tool_name;
        std::string tool_description;
        bool is_streaming_runnable;
        bool is_invokable;
    };
    
    ToolInfo tool_info;
};

// RetrieverCallbackExtra - Metadata for retriever components
struct RetrieverCallbackExtra : public CallbackExtra {
    // Retriever specific metadata
    struct RetrieverInfo {
        std::string retriever_type;  // vector, bm25, hybrid, etc.
        int top_k;
        float score_threshold;
    };
    
    RetrieverInfo retriever_info;
};

// IndexerCallbackExtra - Metadata for indexer components
struct IndexerCallbackExtra : public CallbackExtra {
    // Indexer specific metadata
    struct IndexerInfo {
        std::string index_type;
        int batch_size;
    };
    
    IndexerInfo indexer_info;
};

// PromptCallbackExtra - Metadata for prompt template components
struct PromptCallbackExtra : public CallbackExtra {
    // Prompt specific metadata
    struct PromptInfo {
        std::vector<std::string> input_variables;
        std::vector<std::string> output_variables;
    };
    
    PromptInfo prompt_info;
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_CALLBACK_EXTRA_H_
