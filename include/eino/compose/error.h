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

#ifndef EINO_CPP_COMPOSE_ERROR_H_
#define EINO_CPP_COMPOSE_ERROR_H_

#include <exception>
#include <string>
#include <vector>
#include <sstream>
#include <typeinfo>
#include <memory>

namespace eino {
namespace compose {

// NodePath represents the execution path through the graph
class NodePath {
public:
    NodePath() = default;
    explicit NodePath(const std::string& initial_node) {
        path_.push_back(initial_node);
    }

    void AddNode(const std::string& node) {
        path_.insert(path_.begin(), node);
    }

    const std::vector<std::string>& GetPath() const {
        return path_;
    }

    std::string ToString() const {
        if (path_.empty()) {
            return "[]";
        }
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < path_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << path_[i];
        }
        oss << "]";
        return oss.str();
    }

private:
    std::vector<std::string> path_;
};

// ComposeException is the base exception for compose module
class ComposeException : public std::exception {
public:
    explicit ComposeException(const std::string& message) : message_(message) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }

protected:
    std::string message_;
};

// InternalError represents errors that occur during graph/chain execution
class InternalError : public ComposeException {
public:
    enum class Type {
        NodeRunError,
        GraphRunError,
    };

    InternalError(Type type, const std::string& message, const NodePath& path = NodePath())
        : ComposeException(FormatMessage(type, message, path)), type_(type), node_path_(path) {}

    Type GetType() const { return type_; }
    const NodePath& GetNodePath() const { return node_path_; }

private:
    static std::string FormatMessage(Type type, const std::string& msg, const NodePath& path) {
        std::ostringstream oss;
        oss << "[" << (type == Type::NodeRunError ? "NodeRunError" : "GraphRunError") << "] " << msg;
        if (!path.GetPath().empty()) {
            oss << "\nnode path: " << path.ToString();
        }
        return oss.str();
    }

    Type type_;
    NodePath node_path_;
};

// TypeMismatchError represents type conversion errors
class TypeMismatchError : public ComposeException {
public:
    TypeMismatchError(const std::type_info& expected, const std::type_info& got)
        : ComposeException(FormatMessage(expected, got)),
          expected_(&expected), got_(&got) {}

    const std::type_info& GetExpectedType() const { return *expected_; }
    const std::type_info& GetGotType() const { return *got_; }

private:
    static std::string FormatMessage(const std::type_info& expected, const std::type_info& got) {
        std::ostringstream oss;
        oss << "Type mismatch. Expected: " << expected.name() << ", Got: " << got.name();
        return oss.str();
    }

    const std::type_info* expected_;
    const std::type_info* got_;
};

// RuntimeError represents runtime errors
class RuntimeError : public ComposeException {
public:
    explicit RuntimeError(const std::string& message) : ComposeException(message) {}
};

// MaxStepsExceededError represents when graph execution exceeds max steps
class MaxStepsExceededError : public ComposeException {
public:
    explicit MaxStepsExceededError(int max_steps)
        : ComposeException("Graph execution exceeds max steps: " + std::to_string(max_steps)) {}
};

// ValidationError represents validation failures
class ValidationError : public ComposeException {
public:
    explicit ValidationError(const std::string& message) 
        : ComposeException("Validation error: " + message) {}
};

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_ERROR_H_
