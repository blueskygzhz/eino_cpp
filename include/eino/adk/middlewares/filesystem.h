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

#ifndef EINO_CPP_ADK_MIDDLEWARES_FILESYSTEM_H_
#define EINO_CPP_ADK_MIDDLEWARES_FILESYSTEM_H_

// Filesystem middleware type aliases.
// Aligned with Go: adk/middlewares/filesystem/backend.go
//
// Re-exports types from adk/filesystem for convenience.

#include "eino/adk/filesystem/backend.h"

namespace eino {
namespace adk {
namespace middlewares {
namespace filesystem_mw {

// Type aliases from adk/filesystem, aligned with Go: middlewares/filesystem/backend.go
using FileInfo = adk::filesystem::FileInfo;
using GrepMatch = adk::filesystem::GrepMatch;
using LsInfoRequest = adk::filesystem::LsInfoRequest;
using ReadRequest = adk::filesystem::ReadRequest;
using GrepRequest = adk::filesystem::GrepRequest;
using GlobInfoRequest = adk::filesystem::GlobInfoRequest;
using WriteRequest = adk::filesystem::WriteRequest;
using EditRequest = adk::filesystem::EditRequest;
using FileContent = adk::filesystem::FileContent;

}  // namespace filesystem_mw
}  // namespace middlewares
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_MIDDLEWARES_FILESYSTEM_H_
