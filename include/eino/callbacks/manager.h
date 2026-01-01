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

#ifndef EINO_CPP_CALLBACKS_MANAGER_H_
#define EINO_CPP_CALLBACKS_MANAGER_H_

#include <memory>
#include <vector>
#include <any>
#include <map>
#include <string>

#include "eino/callbacks/interface.h"
#include "eino/callbacks/aspect_inject.h"

namespace eino {
namespace callbacks {

// Context keys for callback management
struct CtxManagerKey {};
struct CtxRunInfoKey {};

// CallbackManager manages callback handlers in the execution context
// It stores both global and local handlers, along with current RunInfo
class CallbackManager {
public:
    CallbackManager() = default;
    
    CallbackManager(const RunInfo& run_info, 
                    const std::vector<std::shared_ptr<Handler>>& handlers)
        : run_info_(run_info)
        , handlers_(handlers) {
        // Copy global handlers
        const auto& global = GlobalHandlerManager::GetInstance().GetGlobalHandlers();
        global_handlers_.reserve(global.size());
        for (const auto& h : global) {
            global_handlers_.push_back(h);
        }
    }
    
    // Create a new manager with updated RunInfo
    std::shared_ptr<CallbackManager> WithRunInfo(const RunInfo& info) const {
        auto mgr = std::make_shared<CallbackManager>();
        mgr->global_handlers_ = global_handlers_;
        mgr->handlers_ = handlers_;
        mgr->run_info_ = info;
        return mgr;
    }
    
    // Get all handlers (global + local)
    std::vector<std::shared_ptr<Handler>> GetAllHandlers() const {
        std::vector<std::shared_ptr<Handler>> all;
        all.reserve(global_handlers_.size() + handlers_.size());
        all.insert(all.end(), handlers_.begin(), handlers_.end());
        all.insert(all.end(), global_handlers_.begin(), global_handlers_.end());
        return all;
    }
    
    const RunInfo& GetRunInfo() const { return run_info_; }
    const std::vector<std::shared_ptr<Handler>>& GetHandlers() const { return handlers_; }
    const std::vector<std::shared_ptr<HandlerWithTiming>>& GetGlobalHandlers() const { 
        return global_handlers_; 
    }
    
    // Check if there are any handlers
    bool HasHandlers() const {
        return !handlers_.empty() || !global_handlers_.empty();
    }
    
private:
    std::vector<std::shared_ptr<HandlerWithTiming>> global_handlers_;
    std::vector<std::shared_ptr<Handler>> handlers_;
    RunInfo run_info_;
};

// Context type for callback propagation
// In C++, we use std::any for flexible context storage
using Context = std::map<std::string, std::any>;

// Context helper functions

// Create context with CallbackManager
inline Context CtxWithManager(const Context& ctx, std::shared_ptr<CallbackManager> mgr) {
    Context new_ctx = ctx;
    new_ctx["_callback_manager"] = mgr;
    return new_ctx;
}

// Get CallbackManager from context
inline std::shared_ptr<CallbackManager> ManagerFromCtx(const Context& ctx) {
    auto it = ctx.find("_callback_manager");
    if (it != ctx.end()) {
        try {
            return std::any_cast<std::shared_ptr<CallbackManager>>(it->second);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
    return nullptr;
}

// Create a new CallbackManager instance
inline std::shared_ptr<CallbackManager> NewManager(
    const RunInfo& info,
    const std::vector<std::shared_ptr<Handler>>& handlers = {}) {
    
    if (handlers.empty() && 
        GlobalHandlerManager::GetInstance().GetGlobalHandlers().empty()) {
        return nullptr;
    }
    
    return std::make_shared<CallbackManager>(info, handlers);
}

// Initialize callbacks in context
inline Context InitCallbacks(
    const Context& ctx,
    const RunInfo& info,
    const std::vector<std::shared_ptr<Handler>>& handlers = {}) {
    
    auto mgr = NewManager(info, handlers);
    if (mgr) {
        return CtxWithManager(ctx, mgr);
    }
    
    return CtxWithManager(ctx, nullptr);
}

// Ensure RunInfo is present in context
inline Context EnsureRunInfo(
    const Context& ctx,
    const std::string& type,
    int component) {
    
    auto cbm = ManagerFromCtx(ctx);
    if (!cbm) {
        RunInfo info;
        info.run_type = type;
        info.run_id = std::to_string(reinterpret_cast<uintptr_t>(&ctx));
        info.extra["component"] = std::to_string(component);
        return InitCallbacks(ctx, info);
    }
    
    // Already has manager
    return ctx;
}

// Reuse existing handlers with new RunInfo
inline Context ReuseHandlers(const Context& ctx, const RunInfo& info) {
    auto cbm = ManagerFromCtx(ctx);
    if (!cbm) {
        return InitCallbacks(ctx, info);
    }
    
    auto new_mgr = cbm->WithRunInfo(info);
    return CtxWithManager(ctx, new_mgr);
}

// Append additional handlers to context
inline Context AppendHandlers(
    const Context& ctx,
    const RunInfo& info,
    const std::vector<std::shared_ptr<Handler>>& handlers) {
    
    auto cbm = ManagerFromCtx(ctx);
    if (!cbm) {
        return InitCallbacks(ctx, info, handlers);
    }
    
    // Merge handlers
    auto existing = cbm->GetHandlers();
    std::vector<std::shared_ptr<Handler>> all_handlers;
    all_handlers.reserve(existing.size() + handlers.size());
    all_handlers.insert(all_handlers.end(), existing.begin(), existing.end());
    all_handlers.insert(all_handlers.end(), handlers.begin(), handlers.end());
    
    return InitCallbacks(ctx, info, all_handlers);
}

// Get handlers from context (convenience function)
inline std::vector<std::shared_ptr<Handler>> GetHandlersFromContext(const Context& ctx) {
    auto mgr = ManagerFromCtx(ctx);
    if (mgr) {
        return mgr->GetAllHandlers();
    }
    return {};
}

// Store RunInfo in context
inline Context CtxWithRunInfo(const Context& ctx, const RunInfo& info) {
    Context new_ctx = ctx;
    new_ctx["_run_info"] = info;
    return new_ctx;
}

// Get RunInfo from context
inline RunInfo* RunInfoFromCtx(const Context& ctx) {
    auto it = ctx.find("_run_info");
    if (it != ctx.end()) {
        try {
            return std::any_cast<RunInfo>(&it->second);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
    return nullptr;
}

} // namespace callbacks
} // namespace eino

#endif // EINO_CPP_CALLBACKS_MANAGER_H_
