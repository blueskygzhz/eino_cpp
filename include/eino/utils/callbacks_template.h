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

#ifndef EINO_CPP_UTILS_CALLBACKS_TEMPLATE_H_
#define EINO_CPP_UTILS_CALLBACKS_TEMPLATE_H_

#include "../callbacks/interface.h"
#include "../components/component.h"
#include <memory>
#include <map>

namespace eino {
namespace utils {
namespace callbacks {

// Forward declarations for component-specific handlers
class PromptCallbackHandler;
class ModelCallbackHandler;
class EmbeddingCallbackHandler;
class IndexerCallbackHandler;
class RetrieverCallbackHandler;
class LoaderCallbackHandler;
class TransformerCallbackHandler;
class ToolCallbackHandler;
class ToolsNodeCallbackHandlers;

// HandlerHelper is a builder for creating callbacks with specific handlers for different components
class HandlerHelper {
public:
    HandlerHelper() = default;
    ~HandlerHelper() = default;
    
    // Set handlers for different components
    HandlerHelper& Prompt(std::shared_ptr<PromptCallbackHandler> handler);
    HandlerHelper& ChatModel(std::shared_ptr<ModelCallbackHandler> handler);
    HandlerHelper& Embedding(std::shared_ptr<EmbeddingCallbackHandler> handler);
    HandlerHelper& Indexer(std::shared_ptr<IndexerCallbackHandler> handler);
    HandlerHelper& Retriever(std::shared_ptr<RetrieverCallbackHandler> handler);
    HandlerHelper& Loader(std::shared_ptr<LoaderCallbackHandler> handler);
    HandlerHelper& Transformer(std::shared_ptr<TransformerCallbackHandler> handler);
    HandlerHelper& Tool(std::shared_ptr<ToolCallbackHandler> handler);
    HandlerHelper& ToolsNode(std::shared_ptr<ToolsNodeCallbackHandlers> handler);
    
    // Build and return the final handler
    std::shared_ptr<eino::callbacks::Handler> Build();
    
private:
    std::shared_ptr<PromptCallbackHandler> prompt_handler_;
    std::shared_ptr<ModelCallbackHandler> chat_model_handler_;
    std::shared_ptr<EmbeddingCallbackHandler> embedding_handler_;
    std::shared_ptr<IndexerCallbackHandler> indexer_handler_;
    std::shared_ptr<RetrieverCallbackHandler> retriever_handler_;
    std::shared_ptr<LoaderCallbackHandler> loader_handler_;
    std::shared_ptr<TransformerCallbackHandler> transformer_handler_;
    std::shared_ptr<ToolCallbackHandler> tool_handler_;
    std::shared_ptr<ToolsNodeCallbackHandlers> tools_node_handler_;
    
    std::map<components::Component, std::shared_ptr<eino::callbacks::Handler>> compose_templates_;
};

// Component-specific callback handler interfaces

class PromptCallbackHandler {
public:
    virtual ~PromptCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class ModelCallbackHandler {
public:
    virtual ~ModelCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class EmbeddingCallbackHandler {
public:
    virtual ~EmbeddingCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class IndexerCallbackHandler {
public:
    virtual ~IndexerCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class RetrieverCallbackHandler {
public:
    virtual ~RetrieverCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class LoaderCallbackHandler {
public:
    virtual ~LoaderCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class TransformerCallbackHandler {
public:
    virtual ~TransformerCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class ToolCallbackHandler {
public:
    virtual ~ToolCallbackHandler() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

class ToolsNodeCallbackHandlers {
public:
    virtual ~ToolsNodeCallbackHandlers() = default;
    
    virtual void OnStart(void* ctx, const std::string& run_info) {}
    virtual void OnEnd(void* ctx, const std::string& run_info) {}
    virtual void OnError(void* ctx, const std::string& error) {}
};

// Create a new HandlerHelper
inline std::shared_ptr<HandlerHelper> NewHandlerHelper() {
    return std::make_shared<HandlerHelper>();
}

} // namespace callbacks
} // namespace utils
} // namespace eino

#endif // EINO_CPP_UTILS_CALLBACKS_TEMPLATE_H_
