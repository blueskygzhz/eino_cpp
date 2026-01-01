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

#ifndef EINO_CPP_SCHEMA_SCHEMA_H_
#define EINO_CPP_SCHEMA_SCHEMA_H_

// Main schema types
#include "eino/schema/types.h"

// Stream utilities
#include "eino/schema/stream.h"

namespace eino {
namespace schema {

// Forward declarations for Message utilities
std::vector<Message> ConcatMessages(const std::vector<Message*>& messages);

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_SCHEMA_H_
