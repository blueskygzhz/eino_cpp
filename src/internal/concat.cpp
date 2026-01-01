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

#include "eino/internal/concat.h"
#include "eino/schema/types.h"

namespace eino {
namespace internal {

// Register default concat functions for common types
namespace {

// Initialize default concat functions
void InitializeDefaultConcatFuncs() {
    // String concat is already specialized
    
    // Basic types use last value
    RegisterStreamChunkConcatFunc<int>(UseLast<int>);
    RegisterStreamChunkConcatFunc<int64_t>(UseLast<int64_t>);
    RegisterStreamChunkConcatFunc<double>(UseLast<double>);
    RegisterStreamChunkConcatFunc<float>(UseLast<float>);
    RegisterStreamChunkConcatFunc<bool>(UseLast<bool>);
    
    // Message concat function
    RegisterStreamChunkConcatFunc<schema::Message>([](const std::vector<schema::Message>& messages) {
        if (messages.empty()) {
            throw std::runtime_error("Cannot concat empty message vector");
        }
        
        schema::Message merged = messages[0];
        
        for (size_t i = 1; i < messages.size(); ++i) {
            const auto& msg = messages[i];
            
            // Check role consistency
            if (msg.role != merged.role) {
                throw std::runtime_error("Cannot concat messages with different roles");
            }
            
            // Merge content
            merged.content += msg.content;
            
            // Merge reasoning content
            if (!msg.reasoning_content.empty()) {
                merged.reasoning_content += msg.reasoning_content;
            }
            
            // Merge tool calls
            merged.tool_calls.insert(merged.tool_calls.end(),
                                    msg.tool_calls.begin(),
                                    msg.tool_calls.end());
            
            // Merge extra metadata
            for (const auto& pair : msg.extra) {
                merged.extra[pair.first] = pair.second;
            }
            
            // Update response metadata
            if (msg.response_meta) {
                if (!merged.response_meta) {
                    merged.response_meta = msg.response_meta;
                } else {
                    if (!msg.response_meta->finish_reason.empty()) {
                        merged.response_meta->finish_reason = msg.response_meta->finish_reason;
                    }
                    if (msg.response_meta->usage) {
                        if (!merged.response_meta->usage) {
                            merged.response_meta->usage = msg.response_meta->usage;
                        } else {
                            // Merge usage information
                            merged.response_meta->usage->prompt_tokens = 
                                std::max(merged.response_meta->usage->prompt_tokens,
                                        msg.response_meta->usage->prompt_tokens);
                            merged.response_meta->usage->completion_tokens = 
                                std::max(merged.response_meta->usage->completion_tokens,
                                        msg.response_meta->usage->completion_tokens);
                            merged.response_meta->usage->total_tokens = 
                                std::max(merged.response_meta->usage->total_tokens,
                                        msg.response_meta->usage->total_tokens);
                        }
                    }
                }
            }
        }
        
        return merged;
    });
}

// Static initializer
static bool initialized = (InitializeDefaultConcatFuncs(), true);

} // anonymous namespace

} // namespace internal
} // namespace eino
