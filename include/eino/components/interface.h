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

#ifndef EINO_CPP_COMPONENTS_INTERFACE_H_
#define EINO_CPP_COMPONENTS_INTERFACE_H_

#include <string>
#include <memory>

namespace eino {
namespace components {

// Component type constants
extern const char* kComponentOfPrompt;
extern const char* kComponentOfChatModel;
extern const char* kComponentOfEmbedding;
extern const char* kComponentOfIndexer;
extern const char* kComponentOfRetriever;
extern const char* kComponentOfLoader;
extern const char* kComponentOfTransformer;
extern const char* kComponentOfTool;

// Typer interface for getting the type name of a component
class Typer {
public:
    virtual ~Typer() = default;
    
    // GetType returns the type name of the component
    virtual std::string GetType() const = 0;
};

// GetType returns the type of a component if it implements Typer
inline std::string GetType(const std::shared_ptr<Typer>& component) {
    if (component) {
        return component->GetType();
    }
    return "";
}

// Checker interface for component callback aspect status
class Checker {
public:
    virtual ~Checker() = default;
    
    // IsCallbacksEnabled returns whether callbacks are enabled
    virtual bool IsCallbacksEnabled() const = 0;
};

// IsCallbacksEnabled checks if a component has callbacks enabled
inline bool IsCallbacksEnabled(const std::shared_ptr<Checker>& component) {
    if (component) {
        return component->IsCallbacksEnabled();
    }
    return false;
}

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_INTERFACE_H_
