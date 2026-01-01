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

#include "../include/eino/adk/types.h"
#include "../include/eino/schema/types.h"

namespace eino {
namespace adk {

// GetMessage implementation - aligns with eino adk.MessageVariant.GetMessage()
std::pair<Message, std::string> MessageVariant::GetMessage() const {
    // Non-streaming: return message directly
    if (!is_streaming) {
        return {message, ""};
    }
    
    // Streaming: concatenate all chunks
    if (!message_stream) {
        return {nullptr, "message_stream is null for streaming variant"};
    }
    
    std::vector<Message> messages;
    try {
        while (true) {
            // Receive chunk from stream
            Message chunk = message_stream->Recv();
            if (!chunk) {
                break;  // EOF
            }
            messages.push_back(chunk);
        }
    } catch (const std::exception& e) {
        return {nullptr, std::string("error receiving message stream: ") + e.what()};
    }
    
    // Concatenate all messages
    if (messages.empty()) {
        return {nullptr, ""};
    }
    
    // Use schema::ConcatMessages to concatenate
    try {
        Message concat_msg = schema::ConcatMessages(messages);
        return {concat_msg, ""};
    } catch (const std::exception& e) {
        return {nullptr, std::string("failed to concat messages: ") + e.what()};
    }
}

}  // namespace adk
}  // namespace eino
