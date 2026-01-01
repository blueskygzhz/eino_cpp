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

#include "eino/schema/types.h"
#include <sstream>

namespace eino {
namespace schema {

std::string Message::String() const {
    std::ostringstream oss;
    
    // Role and content
    oss << GetRoleString() << ": " << content;
    
    // Reasoning content
    if (!reasoning_content.empty()) {
        oss << "\nreasoning content:\n" << reasoning_content;
    }
    
    // Tool calls
    if (!tool_calls.empty()) {
        oss << "\ntool_calls:\n";
        for (const auto& tc : tool_calls) {
            if (tc.index) {
                oss << "index[" << *tc.index << "]:";
            }
            oss << "id=" << tc.id << ", type=" << tc.type 
                << ", function_name=" << tc.function.name 
                << ", function_args=" << tc.function.arguments << "\n";
        }
    }
    
    // Tool call ID
    if (!tool_call_id.empty()) {
        oss << "\ntool_call_id: " << tool_call_id;
    }
    
    // Tool name
    if (!tool_name.empty()) {
        oss << "\ntool_call_name: " << tool_name;
    }
    
    // Response metadata
    if (response_meta) {
        oss << "\nfinish_reason: " << response_meta->finish_reason;
        if (response_meta->usage) {
            oss << "\nusage: prompt_tokens=" << response_meta->usage->prompt_tokens
                << ", completion_tokens=" << response_meta->usage->completion_tokens
                << ", total_tokens=" << response_meta->usage->total_tokens;
        }
    }
    
    return oss.str();
}

// Concatenate messages with the same role
std::vector<Message> ConcatMessages(const std::vector<Message*>& messages) {
    std::vector<Message> result;
    
    if (messages.empty()) {
        return result;
    }
    
    Message merged = *messages[0];
    
    for (size_t i = 1; i < messages.size(); ++i) {
        const auto& msg = messages[i];
        
        // Check role consistency
        if (msg->role != merged.role) {
            // Different role - cannot merge
            result.push_back(merged);
            merged = *msg;
            continue;
        }
        
        // Merge content
        merged.content += msg->content;
        
        // Merge reasoning content
        if (!msg->reasoning_content.empty()) {
            merged.reasoning_content += msg->reasoning_content;
        }
        
        // Merge tool calls
        merged.tool_calls.insert(merged.tool_calls.end(),
                                msg->tool_calls.begin(),
                                msg->tool_calls.end());
        
        // Merge extra metadata
        for (const auto& pair : msg->extra) {
            merged.extra[pair.first] = pair.second;
        }
        
        // Update response metadata
        if (msg->response_meta) {
            if (!merged.response_meta) {
                merged.response_meta = msg->response_meta;
            } else {
                if (!msg->response_meta->finish_reason.empty()) {
                    merged.response_meta->finish_reason = msg->response_meta->finish_reason;
                }
                if (msg->response_meta->usage) {
                    if (!merged.response_meta->usage) {
                        merged.response_meta->usage = msg->response_meta->usage;
                    } else {
                        // Merge usage information
                        merged.response_meta->usage->prompt_tokens = 
                            std::max(merged.response_meta->usage->prompt_tokens,
                                    msg->response_meta->usage->prompt_tokens);
                        merged.response_meta->usage->completion_tokens = 
                            std::max(merged.response_meta->usage->completion_tokens,
                                    msg->response_meta->usage->completion_tokens);
                        merged.response_meta->usage->total_tokens = 
                            std::max(merged.response_meta->usage->total_tokens,
                                    msg->response_meta->usage->total_tokens);
                    }
                }
            }
        }
    }
    
    result.push_back(merged);
    return result;
}

} // namespace schema
} // namespace eino
