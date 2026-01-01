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

#include "eino/components/prebuilt/simple_loader.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>

namespace eino {
namespace components {

std::vector<schema::Document> SimpleLoader::LoadFromFile(const std::string& file_path) {
    std::vector<schema::Document> docs;
    
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return docs;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        if (!content.empty()) {
            schema::Document doc;
            doc.id = file_path;
            doc.page_content = content;
            doc.metadata["source"] = file_path;
            docs.push_back(doc);
        }
    } catch (...) {
        // Silently handle errors
    }
    
    return docs;
}

std::vector<schema::Document> SimpleLoader::LoadFromDirectory(const std::string& dir_path) {
    std::vector<schema::Document> docs;
    
    try {
        DIR* dir = opendir(dir_path.c_str());
        if (dir == nullptr) {
            return docs;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type != DT_REG) continue;
            
            std::string file_path = dir_path + "/" + entry->d_name;
            auto file_docs = LoadFromFile(file_path);
            docs.insert(docs.end(), file_docs.begin(), file_docs.end());
        }
        
        closedir(dir);
    } catch (...) {
        // Silently handle errors
    }
    
    return docs;
}

std::vector<schema::Document> SimpleLoader::LoadFromURI(const std::string& uri) {
    if (uri.find("://") != std::string::npos) {
        // Remote URI - not supported in this simple implementation
        return std::vector<schema::Document>();
    }
    
    // Assume it's a local file path
    struct stat buffer;
    if (stat(uri.c_str(), &buffer) == 0) {
        if (S_ISDIR(buffer.st_mode)) {
            return LoadFromDirectory(uri);
        } else if (S_ISREG(buffer.st_mode)) {
            return LoadFromFile(uri);
        }
    }
    
    return std::vector<schema::Document>();
}

std::vector<schema::Document> SimpleLoader::Invoke(
    std::shared_ptr<compose::Context> ctx,
    const schema::Source& input,
    const std::vector<compose::Option>& opts) {
    return LoadFromURI(input.uri);
}

std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> SimpleLoader::Stream(
    std::shared_ptr<compose::Context> ctx,
    const schema::Source& input,
    const std::vector<compose::Option>& opts) {
    auto docs = LoadFromURI(input.uri);
    auto result = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Document>>>();
    result->Add(docs);
    return result;
}

std::vector<schema::Document> SimpleLoader::Collect(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<schema::Source>> input,
    const std::vector<compose::Option>& opts) {
    std::vector<schema::Document> result;
    schema::Source source;
    
    while (input->Read(source)) {
        auto docs = LoadFromURI(source.uri);
        result.insert(result.end(), docs.begin(), docs.end());
    }
    
    return result;
}

std::shared_ptr<compose::StreamReader<std::vector<schema::Document>>> SimpleLoader::Transform(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<schema::Source>> input,
    const std::vector<compose::Option>& opts) {
    auto result = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Document>>>();
    schema::Source source;
    
    while (input->Read(source)) {
        auto docs = LoadFromURI(source.uri);
        result->Add(docs);
    }
    
    return result;
}

} // namespace components
} // namespace eino
