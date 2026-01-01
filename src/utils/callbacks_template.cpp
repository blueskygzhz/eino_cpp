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

#include "eino/utils/callbacks_template.h"

namespace eino {
namespace utils {
namespace callbacks {

HandlerHelper& HandlerHelper::Prompt(std::shared_ptr<PromptCallbackHandler> handler) {
    prompt_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::ChatModel(std::shared_ptr<ModelCallbackHandler> handler) {
    chat_model_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::Embedding(std::shared_ptr<EmbeddingCallbackHandler> handler) {
    embedding_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::Indexer(std::shared_ptr<IndexerCallbackHandler> handler) {
    indexer_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::Retriever(std::shared_ptr<RetrieverCallbackHandler> handler) {
    retriever_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::Loader(std::shared_ptr<LoaderCallbackHandler> handler) {
    loader_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::Transformer(std::shared_ptr<TransformerCallbackHandler> handler) {
    transformer_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::Tool(std::shared_ptr<ToolCallbackHandler> handler) {
    tool_handler_ = handler;
    return *this;
}

HandlerHelper& HandlerHelper::ToolsNode(std::shared_ptr<ToolsNodeCallbackHandlers> handler) {
    tools_node_handler_ = handler;
    return *this;
}

std::shared_ptr<eino::callbacks::Handler> HandlerHelper::Build() {
    // Create a composite handler that delegates to component-specific handlers
    // This is a simplified implementation
    // In practice, you would create a handler that routes callbacks based on component type
    return nullptr;  // TODO: Implement composite handler
}

} // namespace callbacks
} // namespace utils
} // namespace eino
