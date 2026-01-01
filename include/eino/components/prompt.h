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

#ifndef EINO_CPP_COMPONENTS_PROMPT_H_
#define EINO_CPP_COMPONENTS_PROMPT_H_

#include "../compose/runnable.h"
#include "../schema/types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace eino {
namespace components {

using json = nlohmann::json;

// ChatTemplate formats input variables into messages for chat models
// Input: map of variables, Output: vector of Message
class ChatTemplate : public compose::Runnable<std::map<std::string, json>, std::vector<schema::Message>> {
public:
    virtual ~ChatTemplate() = default;
    
    // Format formats input variables into messages
    virtual std::vector<schema::Message> Format(
        std::shared_ptr<compose::Context> ctx,
        const std::map<std::string, json>& variables,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) = 0;
};

// PromptTemplate is a basic prompt template implementation
// Supports simple variable substitution with {variable_name} syntax
class PromptTemplate : public ChatTemplate {
public:
    PromptTemplate() = default;
    explicit PromptTemplate(const std::string& template_str);
    explicit PromptTemplate(const std::vector<std::string>& templates);
    
    virtual ~PromptTemplate() = default;
    
    // Set the template string
    void SetTemplate(const std::string& template_str);
    
    // Add a template string
    void AddTemplate(const std::string& template_str);
    
    // Get the number of templates
    size_t GetTemplateCount() const;
    
    // Format implementation
    std::vector<schema::Message> Format(
        std::shared_ptr<compose::Context> ctx,
        const std::map<std::string, json>& variables,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    // Compose Runnable interface implementations
    std::vector<schema::Message> Invoke(
        std::shared_ptr<compose::Context> ctx,
        const std::map<std::string, json>& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::shared_ptr<compose::StreamReader<std::vector<schema::Message>>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const std::map<std::string, json>& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::vector<schema::Message> Collect(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::map<std::string, json>>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;
    
    std::shared_ptr<compose::StreamReader<std::vector<schema::Message>>> Transform(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<compose::StreamReader<std::map<std::string, json>>> input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) override;

private:
    std::vector<std::string> templates_;
    
    // Helper function to substitute variables in a template
    std::string SubstituteVariables(
        const std::string& template_str,
        const std::map<std::string, json>& variables);
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_PROMPT_H_
