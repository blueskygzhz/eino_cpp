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

#ifndef EINO_CPP_CALLBACKS_HANDLER_TEMPLATE_H_
#define EINO_CPP_CALLBACKS_HANDLER_TEMPLATE_H_

#include <memory>
#include <string>
#include <map>

#include "eino/callbacks/interface.h"
#include "eino/callbacks/aspect_inject.h"
#include "eino/components/component.h"

namespace eino {
namespace callbacks {

// Component-specific handler interfaces
// These are called based on the component type

// ChatModelHandler - Handler for ChatModel components
class ChatModelHandler {
public:
    virtual ~ChatModelHandler() = default;
    
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    virtual void OnError(const RunInfo& info, const std::string& error) {}
    virtual void OnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) {}
    virtual void OnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) {}
};

// ToolHandler - Handler for Tool components  
class ToolHandler {
public:
    virtual ~ToolHandler() = default;
    
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    virtual void OnError(const RunInfo& info, const std::string& error) {}
};

// EmbeddingHandler - Handler for Embedding components
class EmbeddingHandler {
public:
    virtual ~EmbeddingHandler() = default;
    
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    virtual void OnError(const RunInfo& info, const std::string& error) {}
};

// RetrieverHandler - Handler for Retriever components
class RetrieverHandler {
public:
    virtual ~RetrieverHandler() = default;
    
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    virtual void OnError(const RunInfo& info, const std::string& error) {}
};

// IndexerHandler - Handler for Indexer components
class IndexerHandler {
public:
    virtual ~IndexerHandler() = default;
    
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    virtual void OnError(const RunInfo& info, const std::string& error) {}
};

// PromptHandler - Handler for Prompt components
class PromptHandler {
public:
    virtual ~PromptHandler() = default;
    
    virtual void OnStart(const RunInfo& info, const CallbackInput& input) {}
    virtual void OnEnd(const RunInfo& info, const CallbackOutput& output) {}
    virtual void OnError(const RunInfo& info, const std::string& error) {}
};

// HandlerTemplate - Routes callbacks to component-specific handlers
// This is the C++ equivalent of Go's handlerTemplate
//
// Usage:
//   auto handler = HandlerTemplate::Builder()
//       .WithChatModel(my_chatmodel_handler)
//       .WithTool(my_tool_handler)
//       .Build();
//
// The HandlerTemplate implements the generic Handler interface
// and routes calls to the appropriate component-specific handler
// based on RunInfo.Component
//
class HandlerTemplate : public HandlerWithTiming {
public:
    HandlerTemplate() = default;
    
    // Set component-specific handlers
    void SetChatModelHandler(std::shared_ptr<ChatModelHandler> handler) {
        chatmodel_handler_ = handler;
    }
    
    void SetToolHandler(std::shared_ptr<ToolHandler> handler) {
        tool_handler_ = handler;
    }
    
    void SetEmbeddingHandler(std::shared_ptr<EmbeddingHandler> handler) {
        embedding_handler_ = handler;
    }
    
    void SetRetrieverHandler(std::shared_ptr<RetrieverHandler> handler) {
        retriever_handler_ = handler;
    }
    
    void SetIndexerHandler(std::shared_ptr<IndexerHandler> handler) {
        indexer_handler_ = handler;
    }
    
    void SetPromptHandler(std::shared_ptr<PromptHandler> handler) {
        prompt_handler_ = handler;
    }
    
    // Handler interface implementation - routes to component-specific handlers
    void OnStart(const RunInfo& info, const CallbackInput& input) override {
        RouteOnStart(info, input);
    }
    
    void OnEnd(const RunInfo& info, const CallbackOutput& output) override {
        RouteOnEnd(info, output);
    }
    
    void OnError(const RunInfo& info, const std::string& error) override {
        RouteOnError(info, error);
    }
    
    void OnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) override {
        RouteOnStartWithStreamInput(info, input);
    }
    
    void OnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) override {
        RouteOnEndWithStreamOutput(info, output);
    }
    
    // TimingChecker implementation
    bool Check(CallbackTiming timing) override {
        // Check if any handler is registered
        return chatmodel_handler_ || tool_handler_ || 
               embedding_handler_ || retriever_handler_ ||
               indexer_handler_ || prompt_handler_;
    }
    
    // Builder pattern
    class Builder {
    public:
        Builder() : template_(std::make_shared<HandlerTemplate>()) {}
        
        Builder& WithChatModel(std::shared_ptr<ChatModelHandler> handler) {
            template_->SetChatModelHandler(handler);
            return *this;
        }
        
        Builder& WithTool(std::shared_ptr<ToolHandler> handler) {
            template_->SetToolHandler(handler);
            return *this;
        }
        
        Builder& WithEmbedding(std::shared_ptr<EmbeddingHandler> handler) {
            template_->SetEmbeddingHandler(handler);
            return *this;
        }
        
        Builder& WithRetriever(std::shared_ptr<RetrieverHandler> handler) {
            template_->SetRetrieverHandler(handler);
            return *this;
        }
        
        Builder& WithIndexer(std::shared_ptr<IndexerHandler> handler) {
            template_->SetIndexerHandler(handler);
            return *this;
        }
        
        Builder& WithPrompt(std::shared_ptr<PromptHandler> handler) {
            template_->SetPromptHandler(handler);
            return *this;
        }
        
        std::shared_ptr<HandlerTemplate> Build() {
            return template_;
        }
        
    private:
        std::shared_ptr<HandlerTemplate> template_;
    };
    
private:
    // Routing logic based on component type
    void RouteOnStart(const RunInfo& info, const CallbackInput& input) {
        auto component = GetComponentType(info);
        
        switch (component) {
            case components::Component::kChatModel:
                if (chatmodel_handler_) {
                    chatmodel_handler_->OnStart(info, input);
                }
                break;
            case components::Component::kTool:
                if (tool_handler_) {
                    tool_handler_->OnStart(info, input);
                }
                break;
            case components::Component::kEmbedding:
                if (embedding_handler_) {
                    embedding_handler_->OnStart(info, input);
                }
                break;
            case components::Component::kRetriever:
                if (retriever_handler_) {
                    retriever_handler_->OnStart(info, input);
                }
                break;
            case components::Component::kIndexer:
                if (indexer_handler_) {
                    indexer_handler_->OnStart(info, input);
                }
                break;
            case components::Component::kPromptTemplate:
                if (prompt_handler_) {
                    prompt_handler_->OnStart(info, input);
                }
                break;
            default:
                break;
        }
    }
    
    void RouteOnEnd(const RunInfo& info, const CallbackOutput& output) {
        auto component = GetComponentType(info);
        
        switch (component) {
            case components::Component::kChatModel:
                if (chatmodel_handler_) {
                    chatmodel_handler_->OnEnd(info, output);
                }
                break;
            case components::Component::kTool:
                if (tool_handler_) {
                    tool_handler_->OnEnd(info, output);
                }
                break;
            case components::Component::kEmbedding:
                if (embedding_handler_) {
                    embedding_handler_->OnEnd(info, output);
                }
                break;
            case components::Component::kRetriever:
                if (retriever_handler_) {
                    retriever_handler_->OnEnd(info, output);
                }
                break;
            case components::Component::kIndexer:
                if (indexer_handler_) {
                    indexer_handler_->OnEnd(info, output);
                }
                break;
            case components::Component::kPromptTemplate:
                if (prompt_handler_) {
                    prompt_handler_->OnEnd(info, output);
                }
                break;
            default:
                break;
        }
    }
    
    void RouteOnError(const RunInfo& info, const std::string& error) {
        auto component = GetComponentType(info);
        
        switch (component) {
            case components::Component::kChatModel:
                if (chatmodel_handler_) {
                    chatmodel_handler_->OnError(info, error);
                }
                break;
            case components::Component::kTool:
                if (tool_handler_) {
                    tool_handler_->OnError(info, error);
                }
                break;
            case components::Component::kEmbedding:
                if (embedding_handler_) {
                    embedding_handler_->OnError(info, error);
                }
                break;
            case components::Component::kRetriever:
                if (retriever_handler_) {
                    retriever_handler_->OnError(info, error);
                }
                break;
            case components::Component::kIndexer:
                if (indexer_handler_) {
                    indexer_handler_->OnError(info, error);
                }
                break;
            case components::Component::kPromptTemplate:
                if (prompt_handler_) {
                    prompt_handler_->OnError(info, error);
                }
                break;
            default:
                break;
        }
    }
    
    void RouteOnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) {
        auto component = GetComponentType(info);
        
        if (component == components::Component::kChatModel && chatmodel_handler_) {
            chatmodel_handler_->OnStartWithStreamInput(info, input);
        }
    }
    
    void RouteOnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) {
        auto component = GetComponentType(info);
        
        if (component == components::Component::kChatModel && chatmodel_handler_) {
            chatmodel_handler_->OnEndWithStreamOutput(info, output);
        }
    }
    
    // Extract component type from RunInfo
    components::Component GetComponentType(const RunInfo& info) {
        // Check if component is stored in extra field
        auto it = info.extra.find("component");
        if (it != info.extra.end()) {
            try {
                return static_cast<components::Component>(std::stoi(it->second));
            } catch (...) {
                return components::Component::kUnknown;
            }
        }
        
        // Fallback: parse from run_type string
        if (info.run_type == "chatmodel") {
            return components::Component::kChatModel;
        } else if (info.run_type == "tool") {
            return components::Component::kTool;
        } else if (info.run_type == "embedding") {
            return components::Component::kEmbedding;
        } else if (info.run_type == "retriever") {
            return components::Component::kRetriever;
        } else if (info.run_type == "indexer") {
            return components::Component::kIndexer;
        } else if (info.run_type == "prompt") {
            return components::Component::kPromptTemplate;
        }
        
        return components::Component::kUnknown;
    }
    
    std::shared_ptr<ChatModelHandler> chatmodel_handler_;
    std::shared_ptr<ToolHandler> tool_handler_;
    std::shared_ptr<EmbeddingHandler> embedding_handler_;
    std::shared_ptr<RetrieverHandler> retriever_handler_;
    std::shared_ptr<IndexerHandler> indexer_handler_;
    std::shared_ptr<PromptHandler> prompt_handler_;
};

// Convenience function for creating HandlerTemplate
inline std::shared_ptr<HandlerTemplate> NewHandlerTemplate() {
    return std::make_shared<HandlerTemplate>();
}

} // namespace callbacks
} // namespace eino

#endif // EINO_CPP_CALLBACKS_HANDLER_TEMPLATE_H_
