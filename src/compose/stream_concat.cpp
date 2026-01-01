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

#include "eino/compose/stream_concat.h"

#include "eino/schema/message.h"

namespace eino {
namespace compose {

void StreamChunkConcatRegistry::RegisterDefaultConcatFuncs() {
    // Register concat function for string (concatenation)
    RegisterConcatFunc<std::string>(
        [](const std::vector<std::string>& items) {
            std::string result;
            for (const auto& s : items) {
                result += s;
            }
            return result;
        });
    
    // Register concat function for vectors (append)
    RegisterConcatFunc<std::vector<std::string>>(
        [](const std::vector<std::vector<std::string>>& items) {
            std::vector<std::string> result;
            for (const auto& vec : items) {
                result.insert(result.end(), vec.begin(), vec.end());
            }
            return result;
        });
    
    RegisterConcatFunc<std::vector<int>>(
        [](const std::vector<std::vector<int>>& items) {
            std::vector<int> result;
            for (const auto& vec : items) {
                result.insert(result.end(), vec.begin(), vec.end());
            }
            return result;
        });
    
    // Register concat function for Message (combine content)
    RegisterConcatFunc<schema::Message>(
        [](const std::vector<schema::Message>& items) {
            if (items.empty()) {
                return schema::Message();
            }
            
            schema::Message result = items[0];
            for (size_t i = 1; i < items.size(); ++i) {
                // Concatenate content
                result.content += items[i].content;
                
                // Merge tool calls if any
                if (!items[i].tool_calls.empty()) {
                    result.tool_calls.insert(
                        result.tool_calls.end(),
                        items[i].tool_calls.begin(),
                        items[i].tool_calls.end());
                }
                
                // Keep the last response_metadata
                if (!items[i].response_metadata.empty()) {
                    result.response_metadata = items[i].response_metadata;
                }
            }
            
            return result;
        });
    
    // Register concat function for ChatModelResponse
    RegisterConcatFunc<schema::ChatModelResponse>(
        [](const std::vector<schema::ChatModelResponse>& items) {
            if (items.empty()) {
                return schema::ChatModelResponse();
            }
            
            schema::ChatModelResponse result = items[0];
            
            for (size_t i = 1; i < items.size(); ++i) {
                // Concatenate message content
                result.message.content += items[i].message.content;
                
                // Merge tool calls
                if (!items[i].message.tool_calls.empty()) {
                    result.message.tool_calls.insert(
                        result.message.tool_calls.end(),
                        items[i].message.tool_calls.begin(),
                        items[i].message.tool_calls.end());
                }
                
                // Update usage (sum tokens)
                result.usage.prompt_tokens += items[i].usage.prompt_tokens;
                result.usage.completion_tokens += items[i].usage.completion_tokens;
                result.usage.total_tokens += items[i].usage.total_tokens;
                
                // Keep the last response_metadata
                if (!items[i].message.response_metadata.empty()) {
                    result.message.response_metadata = items[i].message.response_metadata;
                }
            }
            
            return result;
        });
}

} // namespace compose
} // namespace eino
