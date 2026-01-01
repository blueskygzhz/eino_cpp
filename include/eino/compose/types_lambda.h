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

#ifndef EINO_CPP_COMPOSE_TYPES_LAMBDA_H_
#define EINO_CPP_COMPOSE_TYPES_LAMBDA_H_

#include <functional>
#include <memory>
#include <vector>
#include <stdexcept>
#include "runnable.h"

namespace eino {
namespace compose {

// Lambda function types with options
template<typename I, typename O, typename TOption = Option>
using Invoke = std::function<O(
    std::shared_ptr<Context>,
    const I&,
    const std::vector<TOption>&)>;

template<typename I, typename O, typename TOption = Option>
using Stream = std::function<std::shared_ptr<StreamReader<O>>(
    std::shared_ptr<Context>,
    const I&,
    const std::vector<TOption>&)>;

template<typename I, typename O, typename TOption = Option>
using Collect = std::function<O(
    std::shared_ptr<Context>,
    std::shared_ptr<StreamReader<I>>,
    const std::vector<TOption>&)>;

template<typename I, typename O, typename TOption = Option>
using Transform = std::function<std::shared_ptr<StreamReader<O>>(
    std::shared_ptr<Context>,
    std::shared_ptr<StreamReader<I>>,
    const std::vector<TOption>&)>;

// Lambda function types without options
template<typename I, typename O>
using InvokeWOOpt = std::function<O(
    std::shared_ptr<Context>,
    const I&)>;

template<typename I, typename O>
using StreamWOOpt = std::function<std::shared_ptr<StreamReader<O>>(
    std::shared_ptr<Context>,
    const I&)>;

template<typename I, typename O>
using CollectWOOpt = std::function<O(
    std::shared_ptr<Context>,
    std::shared_ptr<StreamReader<I>>)>;

template<typename I, typename O>
using TransformWOOpt = std::function<std::shared_ptr<StreamReader<O>>(
    std::shared_ptr<Context>,
    std::shared_ptr<StreamReader<I>>)>;

// Lambda node type - represents a lambda function node in the composition graph
class Lambda {
public:
    enum class Type {
        Invokable,
        Streamable,
        Collectable,
        Transformable,
    };

    Lambda() : type_(Type::Invokable) {}
    
    explicit Lambda(Type type) : type_(type) {}

    virtual ~Lambda() = default;

    Type GetType() const { return type_; }

    bool IsValid() const {
        return true;
    }

protected:
    Type type_;
};

// Lambda options
struct LambdaOpts {
    bool enable_callback = false;
};

// Factory functions for creating Lambda nodes

template<typename I, typename O>
std::shared_ptr<Lambda> InvokableLambda(
    Invoke<I, O> func) {
    return std::make_shared<Lambda>(Lambda::Type::Invokable);
}

template<typename I, typename O, typename TOption>
std::shared_ptr<Lambda> InvokableLambdaWithOption(
    Invoke<I, O, TOption> func) {
    return std::make_shared<Lambda>(Lambda::Type::Invokable);
}

template<typename I, typename O>
std::shared_ptr<Lambda> StreamableLambda(
    Stream<I, O> func) {
    return std::make_shared<Lambda>(Lambda::Type::Streamable);
}

template<typename I, typename O>
std::shared_ptr<Lambda> CollectableLambda(
    Collect<I, O> func) {
    return std::make_shared<Lambda>(Lambda::Type::Collectable);
}

template<typename I, typename O>
std::shared_ptr<Lambda> TransformableLambda(
    Transform<I, O> func) {
    return std::make_shared<Lambda>(Lambda::Type::Transformable);
}

// Convenience factory for simple invoke functions without options
template<typename I, typename O>
std::shared_ptr<Lambda> AnyLambda(InvokeWOOpt<I, O> func) {
    return std::make_shared<Lambda>(Lambda::Type::Invokable);
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_TYPES_LAMBDA_H_
