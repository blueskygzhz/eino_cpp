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

#ifndef EINO_CPP_ADK_FILESYSTEM_BACKEND_H_
#define EINO_CPP_ADK_FILESYSTEM_BACKEND_H_

// File system backend interface and types.
// Aligned with Go: adk/filesystem/backend.go
//
// Provides a pluggable, unified file backend protocol interface for
// file operations (read, write, edit, grep, glob, ls).

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "eino/schema/stream.h"

namespace eino {
namespace adk {
namespace filesystem {

// FileInfo represents basic file metadata information.
// Aligned with Go: filesystem.FileInfo
struct FileInfo {
    // Path is the path of the file or directory.
    std::string path;
    // IsDir indicates whether the entry is a directory.
    bool is_dir = false;
    // Size is the file size in bytes.
    int64_t size = 0;
    // ModifiedAt is the last modification time in ISO 8601 format.
    std::string modified_at;
};

// GrepMatch represents a single pattern match result.
// Aligned with Go: filesystem.GrepMatch
struct GrepMatch {
    std::string content;
    // Path is the file path where the match was found.
    std::string path;
    // Line is the 1-based line number of the match.
    int line = 0;
};

// LsInfoRequest contains parameters for listing file information.
// Aligned with Go: filesystem.LsInfoRequest
struct LsInfoRequest {
    std::string path;
};

// ReadRequest contains parameters for reading file content.
// Aligned with Go: filesystem.ReadRequest
struct ReadRequest {
    std::string file_path;
    int offset = 0;   // Starting line number (1-based), defaults to 1
    int limit = 0;     // Max lines to read, 0 = entire file
};

// GrepRequest contains parameters for searching file content.
// Aligned with Go: filesystem.GrepRequest
struct GrepRequest {
    // Search Parameters
    std::string pattern;       // Regex pattern (ripgrep syntax)
    std::string path;          // Optional directory scope

    // File Filtering
    std::string glob;          // Optional glob pattern for file filtering
    std::string file_type;     // Optional file type filter (e.g., "js", "py")

    // Search Options
    bool case_insensitive = false;
    bool enable_multiline = false;

    // Context Display
    int after_lines = 0;
    int before_lines = 0;
};

// GlobInfoRequest contains parameters for glob pattern matching.
// Aligned with Go: filesystem.GlobInfoRequest
struct GlobInfoRequest {
    std::string pattern;
    std::string path;
};

// WriteRequest contains parameters for writing file content.
// Aligned with Go: filesystem.WriteRequest
struct WriteRequest {
    std::string file_path;
    std::string content;
};

// EditRequest contains parameters for editing file content.
// Aligned with Go: filesystem.EditRequest
struct EditRequest {
    std::string file_path;
    std::string old_string;
    std::string new_string;
    bool replace_all = false;
};

// FileContent holds the content read from a file.
// Aligned with Go: filesystem.FileContent
struct FileContent {
    std::string content;
};

// Backend is a pluggable, unified file backend protocol interface.
// Aligned with Go: filesystem.Backend
class Backend {
public:
    virtual ~Backend() = default;

    // LsInfo lists file information under the given path.
    virtual std::pair<std::vector<FileInfo>, std::string> LsInfo(
        const LsInfoRequest& req) = 0;

    // Read reads file content with support for line-based offset and limit.
    virtual std::pair<FileContent, std::string> Read(
        const ReadRequest& req) = 0;

    // GrepRaw searches for content matching the specified pattern.
    virtual std::pair<std::vector<GrepMatch>, std::string> GrepRaw(
        const GrepRequest& req) = 0;

    // GlobInfo returns file information matching the glob pattern.
    virtual std::pair<std::vector<FileInfo>, std::string> GlobInfo(
        const GlobInfoRequest& req) = 0;

    // Write creates or updates file content.
    virtual std::string Write(const WriteRequest& req) = 0;

    // Edit replaces string occurrences in a file.
    virtual std::string Edit(const EditRequest& req) = 0;
};

// ExecuteRequest contains parameters for executing a command.
// Aligned with Go: filesystem.ExecuteRequest
struct ExecuteRequest {
    std::string command;
    bool run_in_background = false;
};

// ExecuteResponse contains the response result of command execution.
// Aligned with Go: filesystem.ExecuteResponse
struct ExecuteResponse {
    std::string output;
    int exit_code = 0;
    bool truncated = false;
};

// Shell interface for command execution.
// Aligned with Go: filesystem.Shell
class Shell {
public:
    virtual ~Shell() = default;
    virtual std::pair<ExecuteResponse, std::string> Execute(
        const ExecuteRequest& input) = 0;
};

}  // namespace filesystem
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_FILESYSTEM_BACKEND_H_
