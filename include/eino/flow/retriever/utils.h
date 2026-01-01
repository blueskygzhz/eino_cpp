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

#ifndef EINO_CPP_FLOW_RETRIEVER_UTILS_H_
#define EINO_CPP_FLOW_RETRIEVER_UTILS_H_

#include "../../components/retriever.h"
#include "../../schema/types.h"
#include "../../compose/context.h"
#include "../../callbacks/callbacks.h"
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <exception>

namespace eino {
namespace flow {
namespace retriever {
namespace utils {

// RetrieveTask represents a task for retrieving documents
// Aligns with: eino/flow/retriever/utils/utils.go:RetrieveTask
struct RetrieveTask {
    // Task identification
    std::string name;
    
    // Retriever to use for this task
    std::shared_ptr<components::Retriever> retriever;
    
    // Query string
    std::string query;
    
    // Retrieval options
    std::vector<compose::Option> retrieve_options;
    
    // Results (populated after execution)
    std::vector<schema::Document> result;
    
    // Error (if any occurred)
    std::string error;
    bool has_error = false;
    
    RetrieveTask() = default;
    
    RetrieveTask(const std::string& n, 
                 std::shared_ptr<components::Retriever> r,
                 const std::string& q,
                 const std::vector<compose::Option>& opts = {})
        : name(n), retriever(r), query(q), retrieve_options(opts) {}
};

// ConcurrentRetrieveWithCallback executes multiple retrieve tasks concurrently
// Aligns with: eino/flow/retriever/utils/utils.go:ConcurrentRetrieveWithCallback
inline void ConcurrentRetrieveWithCallback(
    std::shared_ptr<compose::Context> ctx,
    std::vector<std::shared_ptr<RetrieveTask>>& tasks) {
    
    if (tasks.empty()) {
        return;
    }
    
    std::vector<std::thread> threads;
    threads.reserve(tasks.size());
    
    for (auto& task : tasks) {
        threads.emplace_back([ctx, task]() {
            try {
                // Create retriever-specific context with run info
                auto retriever_ctx = ctx;  // Clone or reuse context
                
                // Trigger OnStart callback
                if (ctx && ctx->GetCallbackManager()) {
                    callbacks::RunInfo run_info;
                    run_info.component = "Retriever";
                    run_info.type = task->retriever->GetType();
                    run_info.name = run_info.type + run_info.component;
                    
                    ctx->GetCallbackManager()->OnStart(retriever_ctx, run_info, task->query);
                }
                
                // Execute retrieval
                task->result = task->retriever->Retrieve(retriever_ctx, task->query, task->retrieve_options);
                
                // Trigger OnEnd callback
                if (ctx && ctx->GetCallbackManager()) {
                    callbacks::RunInfo run_info;
                    run_info.component = "Retriever";
                    run_info.type = task->retriever->GetType();
                    run_info.name = run_info.type + run_info.component;
                    
                    ctx->GetCallbackManager()->OnEnd(retriever_ctx, run_info, task->result);
                }
                
            } catch (const std::exception& e) {
                // Capture error
                task->error = std::string("retrieve error, query: ") + task->query + 
                             ", error: " + e.what();
                task->has_error = true;
                
                // Trigger OnError callback
                if (ctx && ctx->GetCallbackManager()) {
                    callbacks::RunInfo run_info;
                    run_info.component = "Retriever";
                    run_info.type = task->retriever ? task->retriever->GetType() : "Unknown";
                    run_info.name = run_info.type + run_info.component;
                    
                    ctx->GetCallbackManager()->OnError(ctx, run_info, task->error);
                }
                
            } catch (...) {
                // Capture unknown error
                task->error = std::string("retrieve panic, query: ") + task->query + 
                             ", error: unknown exception";
                task->has_error = true;
                
                // Trigger OnError callback
                if (ctx && ctx->GetCallbackManager()) {
                    callbacks::RunInfo run_info;
                    run_info.component = "Retriever";
                    run_info.type = task->retriever ? task->retriever->GetType() : "Unknown";
                    run_info.name = run_info.type + run_info.component;
                    
                    ctx->GetCallbackManager()->OnError(ctx, run_info, task->error);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// Helper function to create context with retriever run info
// Aligns with: eino/flow/retriever/utils/utils.go:ctxWithRetrieverRunInfo
inline std::shared_ptr<compose::Context> CtxWithRetrieverRunInfo(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<components::Retriever> retriever) {
    
    if (!ctx) {
        ctx = std::make_shared<compose::Context>();
    }
    
    callbacks::RunInfo run_info;
    run_info.component = "Retriever";
    
    if (retriever) {
        run_info.type = retriever->GetType();
    }
    
    run_info.name = run_info.type + run_info.component;
    
    // Reuse existing callback handlers
    // (Implementation depends on context callback management)
    
    return ctx;
}

} // namespace utils
} // namespace retriever
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_RETRIEVER_UTILS_H_
