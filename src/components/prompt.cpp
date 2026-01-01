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

#include "eino/components/prompt.h"
#include <sstream>
#include <regex>

namespace eino {
namespace components {

PromptTemplate::PromptTemplate(const std::string& template_str) {
    templates_.push_back(template_str);
}

PromptTemplate::PromptTemplate(const std::vector<std::string>& templates)
    : templates_(templates) {
}

void PromptTemplate::SetTemplate(const std::string& template_str) {
    templates_.clear();
    templates_.push_back(template_str);
}

void PromptTemplate::AddTemplate(const std::string& template_str) {
    templates_.push_back(template_str);
}

size_t PromptTemplate::GetTemplateCount() const {
    return templates_.size();
}

std::string PromptTemplate::SubstituteVariables(
    const std::string& template_str,
    const std::map<std::string, json>& variables) {
    std::string result = template_str;
    
    // Replace {variable_name} with values from the map
    for (const auto& pair : variables) {
        const std::string& key = pair.first;
        const json& value = pair.second;
        std::string pattern = "{" + key + "}";
        std::string replacement;
        
        if (value.is_string()) {
            replacement = value.dump();
            replacement = replacement.substr(1, replacement.length() - 2);  // Remove quotes
        } else if (value.is_number()) {
            replacement = value.dump();
        } else {
            replacement = value.dump();
        }
        
        size_t pos = 0;
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            result.replace(pos, pattern.length(), replacement);
            pos += replacement.length();
        }
    }
    
    return result;
}

std::vector<schema::Message> PromptTemplate::Format(
    std::shared_ptr<compose::Context> ctx,
    const std::map<std::string, json>& variables,
    const std::vector<compose::Option>& opts) {
    std::vector<schema::Message> messages;
    
    for (const auto& template_str : templates_) {
        std::string formatted = SubstituteVariables(template_str, variables);
        // By default, treat each template as a user message
        messages.push_back(schema::UserMessage(formatted));
    }
    
    return messages;
}

std::vector<schema::Message> PromptTemplate::Invoke(
    std::shared_ptr<compose::Context> ctx,
    const std::map<std::string, json>& input,
    const std::vector<compose::Option>& opts) {
    return Format(ctx, input, opts);
}

std::shared_ptr<compose::StreamReader<std::vector<schema::Message>>> PromptTemplate::Stream(
    std::shared_ptr<compose::Context> ctx,
    const std::map<std::string, json>& input,
    const std::vector<compose::Option>& opts) {
    // Create a simple stream reader with the formatted messages
    auto messages = Format(ctx, input, opts);
    auto result = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Message>>>();
    result->Add(messages);
    return result;
}

std::vector<schema::Message> PromptTemplate::Collect(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<std::map<std::string, json>>> input,
    const std::vector<compose::Option>& opts) {
    std::vector<schema::Message> result;
    std::map<std::string, json> vars;
    
    while (input->Read(vars)) {
        auto messages = Format(ctx, vars, opts);
        result.insert(result.end(), messages.begin(), messages.end());
    }
    
    return result;
}

std::shared_ptr<compose::StreamReader<std::vector<schema::Message>>> PromptTemplate::Transform(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<compose::StreamReader<std::map<std::string, json>>> input,
    const std::vector<compose::Option>& opts) {
    auto result = std::make_shared<compose::SimpleStreamReader<std::vector<schema::Message>>>();
    std::map<std::string, json> vars;
    
    while (input->Read(vars)) {
        auto messages = Format(ctx, vars, opts);
        result->Add(messages);
    }
    
    return result;
}

} // namespace components
} // namespace eino
