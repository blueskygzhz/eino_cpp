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

#include "eino/schema/stream_copy.h"

namespace eino {
namespace schema {

// Implementation notes:
//
// The Copy functionality is implemented in the header using templates.
// This file is kept for potential future non-template implementations
// and to maintain consistency with the eino_cpp module structure.
//
// Key alignment points with eino schema.StreamReader.Copy():
//
// 1. Creates n independent copies of a StreamReader
// 2. Each copy can read from the original stream independently
// 3. Original StreamReader becomes unusable after Copy
// 4. Uses a parent-child structure to share data efficiently
// 5. Each child maintains its own read position
// 6. Data is loaded once and shared among all children
// 7. When all children are closed, the source is closed
//
// Go reference: eino/schema/stream.go lines 230-253 and 618-700
//
// Usage example:
//
//   auto sr = StreamReaderFromArray<int>({1, 2, 3});
//   auto copies = CopyStreamReader(sr, 2);
//   
//   auto sr1 = copies[0];
//   auto sr2 = copies[1];
//   
//   int val1, val2;
//   sr1->Recv(val1);  // Read independently
//   sr2->Recv(val2);  // Each has its own position
//

} // namespace schema
} // namespace eino
