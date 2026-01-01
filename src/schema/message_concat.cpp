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

#include "eino/schema/message_concat.h"
#include <sstream>
#include <algorithm>

namespace eino {
namespace schema {

std::vector<ToolCall> ConcatToolCalls(const std::vector<ToolCall>& chunks) {
    std::vector<ToolCall> merged;
    std::map<int, std::vector<size_t>> index_map;  // index -> chunk positions
    
    // Group by index
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& tc = chunks[i];
        if (tc.index == nullptr) {
            merged.push_back(tc);
        } else {
            index_map[*tc.index].push_back(i);
        }
    }
    
    // Merge tool calls with same index
    for (const auto& [idx, positions] : index_map) {
        ToolCall tool_call;
        tool_call.index = new int(idx);
        
        std::string tool_id, tool_type, tool_name;
        std::ostringstream args_builder;
        
        for (size_t pos : positions) {
            const auto& chunk = chunks[pos];
            
            // Validate and merge ID
            if (!chunk.id.empty()) {
                if (tool_id.empty()) {
                    tool_id = chunk.id;
                } else if (tool_id != chunk.id) {
                    throw std::runtime_error(
                        "Cannot concat ToolCalls with different IDs: '" + 
                        tool_id + "' vs '" + chunk.id + "'");
                }
            }
            
            // Validate and merge Type
            if (!chunk.type.empty()) {
                if (tool_type.empty()) {
                    tool_type = chunk.type;
                } else if (tool_type != chunk.type) {
                    throw std::runtime_error(
                        "Cannot concat ToolCalls with different types: '" + 
                        tool_type + "' vs '" + chunk.type + "'");
                }
            }
            
            // Validate and merge Function Name
            if (!chunk.function.name.empty()) {
                if (tool_name.empty()) {
                    tool_name = chunk.function.name;
                } else if (tool_name != chunk.function.name) {
                    throw std::runtime_error(
                        "Cannot concat ToolCalls with different names: '" + 
                        tool_name + "' vs '" + chunk.function.name + "'");
                }
            }
            
            // Concatenate arguments
            if (!chunk.function.arguments.empty()) {
                args_builder << chunk.function.arguments;
            }
        }
        
        tool_call.id = tool_id;
        tool_call.type = tool_type;
        tool_call.function.name = tool_name;
        tool_call.function.arguments = args_builder.str();
        
        merged.push_back(tool_call);
    }
    
    // Sort by index
    if (merged.size() > 1) {
        std::stable_sort(merged.begin(), merged.end(), 
            [](const ToolCall& a, const ToolCall& b) {
                if (a.index == nullptr && b.index == nullptr) return false;
                if (a.index == nullptr) return true;
                if (b.index == nullptr) return false;
                return *a.index < *b.index;
            });
    }
    
    return merged;
}

std::vector<MessageOutputPart> ConcatAssistantMultiContent(
    const std::vector<MessageOutputPart>& parts) {
    
    if (parts.empty()) {
        return parts;
    }
    
    std::vector<MessageOutputPart> merged;
    size_t i = 0;
    
    while (i < parts.size()) {
        const auto& current = parts[i];
        size_t start = i;
        
        if (current.type == ChatMessagePartType::kText) {
            // Text merging: find contiguous text block
            size_t end = start + 1;
            while (end < parts.size() && parts[end].type == ChatMessagePartType::kText) {
                ++end;
            }
            
            if (end == start + 1) {
                merged.push_back(current);
            } else {
                // Merge multiple text parts
                std::ostringstream text_builder;
                for (size_t k = start; k < end; ++k) {
                    text_builder << parts[k].text;
                }
                
                MessageOutputPart merged_part;
                merged_part.type = ChatMessagePartType::kText;
                merged_part.text = text_builder.str();
                merged.push_back(merged_part);
            }
            i = end;
            
        } else if (IsBase64AudioPart(current)) {
            // Audio merging: find contiguous audio block
            size_t end = start + 1;
            while (end < parts.size() && IsBase64AudioPart(parts[end])) {
                ++end;
            }
            
            if (end == start + 1) {
                merged.push_back(current);
            } else {
                // Merge multiple audio parts
                std::ostringstream b64_builder;
                std::string mime_type;
                std::vector<std::map<std::string, json>> extra_list;
                
                for (size_t k = start; k < end; ++k) {
                    const auto& audio_part = parts[k].audio;
                    if (audio_part->common.base64_data != nullptr) {
                        b64_builder << *audio_part->common.base64_data;
                    }
                    if (mime_type.empty()) {
                        mime_type = audio_part->common.mime_type;
                    }
                    if (!audio_part->common.extra.empty()) {
                        extra_list.push_back(audio_part->common.extra);
                    }
                }
                
                MessageOutputPart merged_part;
                merged_part.type = ChatMessagePartType::kAudioURL;
                merged_part.audio = std::make_shared<MessageOutputAudio>();
                
                std::string* b64_data = new std::string(b64_builder.str());
                merged_part.audio->common.base64_data = b64_data;
                merged_part.audio->common.mime_type = mime_type;
                
                if (!extra_list.empty()) {
                    merged_part.audio->common.extra = ConcatExtra(extra_list);
                }
                
                merged.push_back(merged_part);
            }
            i = end;
            
        } else {
            // Non-mergeable part
            merged.push_back(current);
            ++i;
        }
    }
    
    return merged;
}

std::map<std::string, json> ConcatExtra(
    const std::vector<std::map<std::string, json>>& extra_list) {
    
    if (extra_list.empty()) {
        return {};
    }
    
    if (extra_list.size() == 1) {
        return extra_list[0];
    }
    
    // Simple merge: later values override earlier ones
    // For advanced merging (nested maps, arrays), use internal::ConcatItems
    std::map<std::string, json> result;
    for (const auto& extra : extra_list) {
        for (const auto& [key, value] : extra) {
            result[key] = value;
        }
    }
    
    return result;
}

Message ConcatMessages(const std::vector<Message*>& msgs) {
    if (msgs.empty()) {
        throw std::invalid_argument("Cannot concat empty message list");
    }
    
    Message result;
    std::vector<std::string> contents;
    size_t content_len = 0;
    std::vector<std::string> reasoning_contents;
    size_t reasoning_len = 0;
    std::vector<ToolCall> tool_calls;
    std::vector<ChatMessagePart> multi_content_parts;
    std::vector<MessageOutputPart> assistant_gen_parts;
    std::vector<std::map<std::string, json>> extra_list;
    
    for (size_t idx = 0; idx < msgs.size(); ++idx) {
        const auto* msg = msgs[idx];
        if (msg == nullptr) {
            throw std::runtime_error("Unexpected nil chunk at index " + std::to_string(idx));
        }
        
        // Validate and set Role
        if (!result.GetRoleString().empty()) {
            if (result.role != msg->role) {
                throw std::runtime_error(
                    "Cannot concat messages with different roles: '" + 
                    result.GetRoleString() + "' vs '" + msg->GetRoleString() + "'");
            }
        } else {
            result.role = msg->role;
        }
        
        // Validate and set Name
        if (!msg->name.empty()) {
            if (result.name.empty()) {
                result.name = msg->name;
            } else if (result.name != msg->name) {
                throw std::runtime_error(
                    "Cannot concat messages with different names: '" + 
                    result.name + "' vs '" + msg->name + "'");
            }
        }
        
        // Validate and set ToolCallID
        if (!msg->tool_call_id.empty()) {
            if (result.tool_call_id.empty()) {
                result.tool_call_id = msg->tool_call_id;
            } else if (result.tool_call_id != msg->tool_call_id) {
                throw std::runtime_error(
                    "Cannot concat messages with different tool_call_ids: '" + 
                    result.tool_call_id + "' vs '" + msg->tool_call_id + "'");
            }
        }
        
        // Validate and set ToolName
        if (!msg->tool_name.empty()) {
            if (result.tool_name.empty()) {
                result.tool_name = msg->tool_name;
            } else if (result.tool_name != msg->tool_name) {
                throw std::runtime_error(
                    "Cannot concat messages with different tool_names: '" + 
                    result.tool_name + "' vs '" + msg->tool_name + "'");
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
            reasoning_len += msg->reasoning_content.size();
        }
        
        // Accumulate tool calls
        if (!msg->tool_calls.empty()) {
            tool_calls.insert(tool_calls.end(), 
                             msg->tool_calls.begin(), 
                             msg->tool_calls.end());
        }
        
        // Accumulate extra
        if (!msg->extra.empty()) {
            extra_list.push_back(msg->extra);
        }
        
        // Accumulate deprecated MultiContent
        if (!msg->multi_content.empty()) {
            multi_content_parts.insert(multi_content_parts.end(),
                                      msg->multi_content.begin(),
                                      msg->multi_content.end());
        }
        
        // Accumulate AssistantGenMultiContent
        if (!msg->assistant_gen_multi_content.empty()) {
            assistant_gen_parts.insert(assistant_gen_parts.end(),
                                      msg->assistant_gen_multi_content.begin(),
                                      msg->assistant_gen_multi_content.end());
        }
        
        // Initialize ResponseMeta if needed
        if (msg->response_meta != nullptr && result.response_meta == nullptr) {
            result.response_meta = std::make_shared<ResponseMeta>();
        }
        
        // Merge ResponseMeta
        if (msg->response_meta != nullptr && result.response_meta != nullptr) {
            // Keep last valid FinishReason
            if (!msg->response_meta->finish_reason.empty()) {
                result.response_meta->finish_reason = msg->response_meta->finish_reason;
            }
            
            // Merge Usage
            if (msg->response_meta->usage != nullptr) {
                if (result.response_meta->usage == nullptr) {
                    result.response_meta->usage = std::make_shared<TokenUsage>();
                }
                
                auto& ru = *result.response_meta->usage;
                auto& mu = *msg->response_meta->usage;
                
                ru.prompt_tokens = std::max(ru.prompt_tokens, mu.prompt_tokens);
                ru.completion_tokens = std::max(ru.completion_tokens, mu.completion_tokens);
                ru.total_tokens = std::max(ru.total_tokens, mu.total_tokens);
                ru.prompt_token_details.cached_tokens = std::max(
                    ru.prompt_token_details.cached_tokens,
                    mu.prompt_token_details.cached_tokens);
            }
            
            // Accumulate LogProbs
            if (msg->response_meta->logprobs != nullptr) {
                if (result.response_meta->logprobs == nullptr) {
                    result.response_meta->logprobs = std::make_shared<LogProbs>();
                }
                
                result.response_meta->logprobs->content.insert(
                    result.response_meta->logprobs->content.end(),
                    msg->response_meta->logprobs->content.begin(),
                    msg->response_meta->logprobs->content.end());
            }
        }
    }
    
    // Build final content
    if (!contents.empty()) {
        std::ostringstream builder;
        for (const auto& c : contents) {
            builder << c;
        }
        result.content = builder.str();
    }
    
    // Build final reasoning content
    if (!reasoning_contents.empty()) {
        std::ostringstream builder;
        for (const auto& rc : reasoning_contents) {
            builder << rc;
        }
        result.reasoning_content = builder.str();
    }
    
    // Merge tool calls
    if (!tool_calls.empty()) {
        result.tool_calls = ConcatToolCalls(tool_calls);
    }
    
    // Merge extra
    if (!extra_list.empty()) {
        auto merged_extra = ConcatExtra(extra_list);
        if (!merged_extra.empty()) {
            result.extra = merged_extra;
        }
    }
    
    // Set deprecated MultiContent
    if (!multi_content_parts.empty()) {
        result.multi_content = multi_content_parts;
    }
    
    // Merge AssistantGenMultiContent
    if (!assistant_gen_parts.empty()) {
        result.assistant_gen_multi_content = ConcatAssistantMultiContent(assistant_gen_parts);
    }
    
    return result;
}

std::vector<Message> ConcatMessageArray(
    const std::vector<std::vector<Message*>>& message_arrays) {
    
    if (message_arrays.empty()) {
        return {};
    }
    
    size_t array_len = message_arrays[0].size();
    std::vector<Message> result(array_len);
    std::vector<std::vector<Message*>> slices_to_concat(array_len);
    
    // Group messages by position
    for (const auto& ma : message_arrays) {
        if (ma.size() != array_len) {
            throw std::runtime_error(
                "Unexpected array length. Got " + std::to_string(ma.size()) + 
                ", expected " + std::to_string(array_len));
        }
        
        for (size_t i = 0; i < array_len; ++i) {
            if (ma[i] != nullptr) {
                slices_to_concat[i].push_back(ma[i]);
            }
        }
    }
    
    // Concat each position
    for (size_t i = 0; i < array_len; ++i) {
        if (slices_to_concat[i].empty()) {
            result[i] = Message();  // Empty message
        } else if (slices_to_concat[i].size() == 1) {
            result[i] = *slices_to_concat[i][0];
        } else {
            result[i] = ConcatMessages(slices_to_concat[i]);
        }
    }
    
    return result;
}

} // namespace schema
} // namespace eino
