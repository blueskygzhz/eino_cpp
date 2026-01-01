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

#include "eino/adk/session.h"
#include "eino/adk/context.h"
#include "eino/schema/message_concat.h"
#include <vector>

namespace eino {
namespace adk {

// =============================================================================
// SetOutputToSession - Store agent output to session
// Aligns with: eino/adk/chatmodel.go:542-555
// =============================================================================

std::string SetOutputToSession(
    void* ctx,
    schema::Message* msg,
    schema::StreamReader<schema::Message>* msg_stream,
    const std::string& output_key) {
    
    if (output_key.empty()) {
        return "output_key is empty";
    }
    
    std::string content;
    
    // Case 1: Non-streaming message - aligns with chatmodel.go:543-546
    if (msg) {
        content = msg->content;
        AddSessionValue(ctx, output_key, new std::string(content));
        return "";  // Success
    }
    
    // Case 2: Streaming message - aligns with chatmodel.go:548-554
    if (msg_stream) {
        // Concatenate all chunks from stream - aligns with chatmodel.go:548
        std::vector<schema::Message*> chunks;
        schema::Message* chunk = nullptr;
        
        while (msg_stream->Read(chunk)) {
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
        
        if (chunks.empty()) {
            return "message stream is empty";
        }
        
        // Concatenate messages - aligns with chatmodel.go:548
        auto concatenated = schema::ConcatMessages(chunks);
        if (!concatenated) {
            return "failed to concatenate message stream";
        }
        
        content = concatenated->content;
        
        // Store to session - aligns with chatmodel.go:553
        AddSessionValue(ctx, output_key, new std::string(content));
        return "";  // Success
    }
    
    return "both msg and msg_stream are null";
}

}  // namespace adk
}  // namespace eino
