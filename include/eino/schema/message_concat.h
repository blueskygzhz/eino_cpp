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

#ifndef EINO_CPP_SCHEMA_MESSAGE_CONCAT_H_
#define EINO_CPP_SCHEMA_MESSAGE_CONCAT_H_

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <algorithm>
#include <stdexcept>

#include "eino/schema/types.h"

namespace eino {
namespace schema {

// ConcatToolCalls merges multiple tool calls with the same index
// Used in stream mode where tool calls are split across chunks
std::vector<ToolCall> ConcatToolCalls(const std::vector<ToolCall>& chunks);

// ConcatAssistantMultiContent merges contiguous text and audio parts in assistant output
// - Text parts: merge consecutive text parts into one
// - Audio parts (base64): merge consecutive base64 audio chunks
// - Other parts: keep as-is
std::vector<MessageOutputPart> ConcatAssistantMultiContent(
    const std::vector<MessageOutputPart>& parts);

// ConcatExtra merges extra maps from multiple messages
// Handles nested map merging using internal concat logic
std::map<std::string, json> ConcatExtra(
    const std::vector<std::map<std::string, json>>& extra_list);

// ConcatMessages concatenates messages with the same role and name
// Useful for merging messages from a stream into a single complete message
//
// Example:
//   std::vector<Message*> msgs;
//   for (auto chunk : stream) {
//       msgs.push_back(chunk);
//   }
//   Message* full_msg = ConcatMessages(msgs);
//
// Rules:
// - All messages must have the same role and name
// - Content strings are concatenated
// - Reasoning content is concatenated
// - Tool calls with same index are merged
// - ResponseMeta keeps the last valid FinishReason
// - TokenUsage keeps the maximum values
// - LogProbs are accumulated
// - Extra maps are merged
Message ConcatMessages(const std::vector<Message*>& msgs);

// ConcatMessageArray concatenates arrays of messages
// Each position in the array is independently concatenated
//
// Example:
//   std::vector<std::vector<Message*>> arrays = {
//       {msg1_0, msg1_1},  // First chunk array
//       {msg2_0, msg2_1},  // Second chunk array
//   };
//   auto result = ConcatMessageArray(arrays);  // [merged_0, merged_1]
std::vector<Message> ConcatMessageArray(
    const std::vector<std::vector<Message*>>& message_arrays);

// ConcatMessageStream reads all messages from a stream and concatenates them
// Automatically handles EOF and errors
//
// Example:
//   StreamReader<Message*>* stream = GetMessageStream();
//   Message* full_msg = ConcatMessageStream(stream);
//
// template<typename T>
// Message ConcatMessageStream(StreamReader<T>* stream) {
//     std::vector<Message*> msgs;
//     T msg;
//     while (stream->Next(msg)) {
//         msgs.push_back(msg);
//     }
//     stream->Close();
//     return ConcatMessages(msgs);
// }

// Helper: check if a MessageOutputPart is base64 audio
inline bool IsBase64AudioPart(const MessageOutputPart& part) {
    return part.type == ChatMessagePartType::kAudioURL &&
           part.audio != nullptr &&
           part.audio->common.base64_data != nullptr &&
           part.audio->common.url == nullptr;
}

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_MESSAGE_CONCAT_H_
