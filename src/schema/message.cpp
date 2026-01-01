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

#include "../../include/eino/schema/types.h"
#include "../../include/eino/schema/message_concat.h"
#include <stdexcept>
#include <sstream>
#include <map>

namespace eino {
namespace schema {

// Helper function to concat ToolCalls with the same index
// Aligns with eino/schema/message.go concatToolCalls (lines 869-926)
std::vector<ToolCall> ConcatToolCalls(const std::vector<ToolCall>& chunks) {
    std::vector<ToolCall> merged;
    std::map<int, std::vector<size_t>> indexed_chunks;  // index -> chunk indices
    
    // Group chunks by index
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        if (chunk.index == nullptr) {
            // No index, add directly to merged
            merged.push_back(chunk);
        } else {
            indexed_chunks[*chunk.index].push_back(i);
        }
    }
    
    // Merge chunks with the same index
    for (const auto& entry : indexed_chunks) {
        int index = entry.first;
        const auto& chunk_indices = entry.second;
        
        if (chunk_indices.empty()) {
            continue;
        }
        
        // Start with first chunk
        ToolCall result_call = chunks[chunk_indices[0]];
        result_call.index = new int(index);
        
        // Accumulate arguments
        std::ostringstream args_builder;
        std::string tool_id, tool_type, tool_name;
        
        for (size_t idx : chunk_indices) {
            const auto& chunk = chunks[idx];
            
            // Check ID consistency
            if (!chunk.id.empty()) {
                if (tool_id.empty()) {
                    tool_id = chunk.id;
                } else if (tool_id != chunk.id) {
                    throw std::runtime_error(
                        "cannot concat ToolCalls with different tool id: '" + 
                        tool_id + "' '" + chunk.id + "'");
                }
            }
            
            // Check Type consistency
            if (!chunk.type.empty()) {
                if (tool_type.empty()) {
                    tool_type = chunk.type;
                } else if (tool_type != chunk.type) {
                    throw std::runtime_error(
                        "cannot concat ToolCalls with different tool type: '" + 
                        tool_type + "' '" + chunk.type + "'");
                }
            }
            
            // Check Name consistency
            if (!chunk.function.name.empty()) {
                if (tool_name.empty()) {
                    tool_name = chunk.function.name;
                } else if (tool_name != chunk.function.name) {
                    throw std::runtime_error(
                        "cannot concat ToolCalls with different function name: '" + 
                        tool_name + "' '" + chunk.function.name + "'");
                }
            }
            
            // Accumulate arguments
            args_builder << chunk.function.arguments;
        }
        
        // Build final ToolCall
        result_call.id = tool_id;
        result_call.type = tool_type;
        result_call.function.name = tool_name;
        result_call.function.arguments = args_builder.str();
        
        merged.push_back(result_call);
    }
    
    return merged;
}

// ConcatMessages concatenates messages with the same role and name
// Aligns with eino/schema/message.go ConcatMessages (lines 1081-1260)
// This is critical for streaming message assembly
std::shared_ptr<Message> ConcatMessages(const std::vector<std::shared_ptr<Message>>& msgs) {
    if (msgs.empty()) {
        throw std::runtime_error("no messages to concat");
    }
    
    auto result = std::make_shared<Message>();
    
    std::vector<std::string> contents;
    size_t content_len = 0;
    std::vector<std::string> reasoning_contents;
    size_t reasoning_content_len = 0;
    std::vector<ToolCall> tool_calls;
    std::vector<ChatMessagePart> multi_content_parts;
    std::vector<MessageOutputPart> assistant_gen_multi_content_parts;
    std::vector<std::map<std::string, json>> extra_list;
    
    // Process each message chunk
    for (size_t idx = 0; idx < msgs.size(); ++idx) {
        const auto& msg = msgs[idx];
        
        if (!msg) {
            throw std::runtime_error(
                "unexpected nil chunk in message stream, index: " + std::to_string(idx));
        }
        
        // Check role consistency
        if (result->GetRoleString().empty()) {
            result->role = msg->role;
        } else if (result->role != msg->role) {
            throw std::runtime_error(
                "cannot concat messages with different roles: '" + 
                result->GetRoleString() + "' '" + msg->GetRoleString() + "'");
        }
        
        // Check name consistency
        if (!msg->name.empty()) {
            if (result->name.empty()) {
                result->name = msg->name;
            } else if (result->name != msg->name) {
                throw std::runtime_error(
                    "cannot concat messages with different names: '" + 
                    result->name + "' '" + msg->name + "'");
            }
        }
        
        // Check tool_call_id consistency
        if (!msg->tool_call_id.empty()) {
            if (result->tool_call_id.empty()) {
                result->tool_call_id = msg->tool_call_id;
            } else if (result->tool_call_id != msg->tool_call_id) {
                throw std::runtime_error(
                    "cannot concat messages with different toolCallIDs: '" + 
                    result->tool_call_id + "' '" + msg->tool_call_id + "'");
            }
        }
        
        // Check tool_name consistency
        if (!msg->tool_name.empty()) {
            if (result->tool_name.empty()) {
                result->tool_name = msg->tool_name;
            } else if (result->tool_name != msg->tool_name) {
                throw std::runtime_error(
                    "cannot concat messages with different toolNames: '" + 
                    result->tool_name + "' '" + msg->tool_name + "'");
            }
        }
        
        // Accumulate content
        if (!msg->content.empty()) {
            contents.push_back(msg->content);
            content_len += msg->content.size();
        }
        
        // Accumulate reasoning content
        if (!msg->reasoning_content.empty()) {
            reasoning_contents.push_back(msg->reasoning_content);
            reasoning_content_len += msg->reasoning_content.size();
        }
        
        // Accumulate tool calls
        if (!msg->tool_calls.empty()) {
            tool_calls.insert(tool_calls.end(), msg->tool_calls.begin(), msg->tool_calls.end());
        }
        
        // Accumulate multi_content (deprecated)
        if (!msg->multi_content.empty()) {
            multi_content_parts.insert(multi_content_parts.end(), 
                msg->multi_content.begin(), msg->multi_content.end());
        }
        
        // Accumulate assistant_gen_multi_content
        if (!msg->assistant_gen_multi_content.empty()) {
            assistant_gen_multi_content_parts.insert(
                assistant_gen_multi_content_parts.end(),
                msg->assistant_gen_multi_content.begin(), 
                msg->assistant_gen_multi_content.end());
        }
        
        // Accumulate extra
        if (!msg->extra.empty()) {
            extra_list.push_back(msg->extra);
        }
        
        // Take response_meta from last message (Go line 1191-1193)
        if (msg->response_meta) {
            result->response_meta = msg->response_meta;
        }
    }
    
    // Merge content
    if (!contents.empty()) {
        std::ostringstream content_builder;
        content_builder.str().reserve(content_len);
        for (const auto& c : contents) {
            content_builder << c;
        }
        result->content = content_builder.str();
    }
    
    // Merge reasoning content
    if (!reasoning_contents.empty()) {
        std::ostringstream reasoning_builder;
        reasoning_builder.str().reserve(reasoning_content_len);
        for (const auto& rc : reasoning_contents) {
            reasoning_builder << rc;
        }
        result->reasoning_content = reasoning_builder.str();
    }
    
    // Merge tool calls
    if (!tool_calls.empty()) {
        result->tool_calls = ConcatToolCalls(tool_calls);
    }
    
    // Merge multi_content (deprecated - simple concatenation)
    if (!multi_content_parts.empty()) {
        result->multi_content = multi_content_parts;
    }
    
    // Merge assistant_gen_multi_content
    // Use ConcatAssistantMultiContent to merge text/audio properly
    if (!assistant_gen_multi_content_parts.empty()) {
        result->assistant_gen_multi_content = ConcatAssistantMultiContent(assistant_gen_multi_content_parts);
    }
    
    // Merge extra (simple merge - last one wins for conflicting keys)
    if (!extra_list.empty()) {
        for (const auto& extra_map : extra_list) {
            for (const auto& entry : extra_map) {
                result->extra[entry.first] = entry.second;
            }
        }
    }
    
    return result;
}

// ConcatMessageStream concatenates messages from a stream reader
// Aligns with eino/schema/message.go ConcatMessageStream (lines 1262-1280)
std::shared_ptr<Message> ConcatMessageStream(std::shared_ptr<StreamReader<Message>> stream) {
    std::vector<std::shared_ptr<Message>> msgs;
    
    Message msg;
    while (stream->Next(msg)) {
        msgs.push_back(std::make_shared<Message>(msg));
    }
    
    stream->Close();
    
    return ConcatMessages(msgs);
}

// Message::String() implementation
std::string Message::String() const {
    std::ostringstream sb;
    sb << "role: " << GetRoleString();
    
    if (!name.empty()) {
        sb << "\nname: " << name;
    }
    
    if (!content.empty()) {
        sb << "\ncontent: " << content;
    }
    
    if (!reasoning_content.empty()) {
        sb << "\nreasoning_content: " << reasoning_content;
    }
    
    // Multi-content (old style)
    if (!multi_content.empty()) {
        sb << "\nmulti_content: [" << multi_content.size() << " parts]";
    }
    
    // Multi-content (new style - input)
    if (!user_input_multi_content.empty()) {
        sb << "\nuser_input_multi_content: [" << user_input_multi_content.size() << " parts]";
    }
    
    // Multi-content (new style - output)
    if (!assistant_gen_multi_content.empty()) {
        sb << "\nassistant_gen_multi_content: [" << assistant_gen_multi_content.size() << " parts]";
    }
    
    if (!tool_calls.empty()) {
        sb << "\ntool_calls:\n";
        for (const auto& tc : tool_calls) {
            if (tc.index != nullptr) {
                sb << "index[" << *tc.index << "]:";
            }
            sb << "  id: " << tc.id 
               << ", type: " << tc.type 
               << ", function: " << tc.function.name 
               << "(" << tc.function.arguments << ")\n";
        }
    }
    
    if (!tool_call_id.empty()) {
        sb << "\ntool_call_id: " << tool_call_id;
    }
    
    if (!tool_name.empty()) {
        sb << "\ntool_call_name: " << tool_name;
    }
    
    if (response_meta) {
        sb << "\nfinish_reason: " << response_meta->finish_reason;
        if (response_meta->usage) {
            sb << "\nusage: prompt=" << response_meta->usage->prompt_tokens
               << ", completion=" << response_meta->usage->completion_tokens
               << ", total=" << response_meta->usage->total_tokens;
        }
    }
    
    return sb.str();
}

}  // namespace schema
}  // namespace eino
