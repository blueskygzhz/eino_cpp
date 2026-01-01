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

#ifndef EINO_CPP_COMPONENTS_PREBUILT_SIMPLE_LOADER_H_
#define EINO_CPP_COMPONENTS_PREBUILT_SIMPLE_LOADER_H_

#include "../document.h"
#include <string>
#include <memory>

namespace eino {
namespace components {

// SimpleLoader loads documents from a URI
// This is a basic implementation that can be extended for specific document types
class SimpleLoader : public Loader {
public:
    SimpleLoader() = default;
    virtual ~SimpleLoader() = default;
    
    // Invoke loads documents from a source
    std::vector<schema::Document> Invoke(
        std::shared_ptr<compose::Context> ctx,
        const schema::Source& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    // Stream loads documents from a source with streaming
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const schema::Source& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::vector<schema::Document> Collect(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<schema::Source>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> Transform(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<schema::Source>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;

protected:
    // Load from file path (can be overridden)
    virtual std::vector<schema::Document> LoadFromFile(const std::string& file_path);
    
    // Load from directory path (can be overridden)
    virtual std::vector<schema::Document> LoadFromDirectory(const std::string& dir_path);
    
    // Load from URI (can be overridden)
    virtual std::vector<schema::Document> LoadFromURI(const std::string& uri);
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_PREBUILT_SIMPLE_LOADER_H_
