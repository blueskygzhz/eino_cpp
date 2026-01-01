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

#ifndef EINO_CPP_ADK_SESSION_H_
#define EINO_CPP_ADK_SESSION_H_

// Session Management for Agent Output Storage
// ============================================
// Aligns with eino/adk/chatmodel.go:542-555

#include "types.h"
#include "../schema/types.h"
#include <string>
#include <memory>

namespace eino {
namespace adk {

// SetOutputToSession stores agent output to session
// Aligns with: setOutputToSession in eino/adk/chatmodel.go:542-555
//
// Go signature:
//   func setOutputToSession(ctx context.Context, msg Message, 
//                           msgStream MessageStream, outputKey string) error
//
// Parameters:
//   ctx: execution context
//   msg: single message (non-streaming), can be nullptr if msg_stream is set
//   msg_stream: message stream (streaming), can be nullptr if msg is set
//   output_key: session key to store the output
//
// Behavior:
//   - If msg is not nullptr: store msg.Content to session[outputKey]
//   - If msg_stream is not nullptr: concatenate stream and store Content
//
// Returns:
//   Error string if failed, empty string on success
//
// Usage:
//   auto err = SetOutputToSession(ctx, msg, nullptr, "agent_response");
//   if (!err.empty()) {
//       // handle error
//   }
std::string SetOutputToSession(
    void* ctx,
    schema::Message* msg,
    schema::StreamReader<schema::Message>* msg_stream,
    const std::string& output_key);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_SESSION_H_
