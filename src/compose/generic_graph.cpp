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

#include "eino/compose/generic_graph.h"
#include "eino/compose/introspect.h"

namespace eino {
namespace compose {

// 对齐 eino/compose/generic_graph.go

// Graph内部实现基类
template<typename I, typename O>
class Graph<I, O>::Impl {
public:
    virtual ~Impl() = default;
    
    virtual std::error_code AddEdge(
        const std::string& start_node, 
        const std::string& end_node) = 0;
    
    virtual std::shared_ptr<Runnable<I, O>> Compile(
        std::shared_ptr<Context> ctx,
        const std::vector<GraphCompileOption>& opts) = 0;
    
    virtual components::Component GetComponent() const = 0;
};

// 具体图实现
template<typename I, typename O>
class GraphImpl : public Graph<I, O>::Impl {
public:
    GraphImpl(const NewGraphOptions& options)
        : options_(options) {
    }
    
    std::error_code AddEdge(
        const std::string& start_node,
        const std::string& end_node) override {
        
        // 检查节点是否存在
        if (nodes_.find(start_node) == nodes_.end()) {
            return std::make_error_code(std::errc::invalid_argument);
        }
        if (nodes_.find(end_node) == nodes_.end()) {
            return std::make_error_code(std::errc::invalid_argument);
        }
        
        // 添加边
        edges_[start_node].push_back(end_node);
        
        return {};
    }
    
    std::shared_ptr<Runnable<I, O>> Compile(
        std::shared_ptr<Context> ctx,
        const std::vector<GraphCompileOption>& opts) override {
        
        // 这里需要调用实际的图编译逻辑
        // 由于涉及复杂的类型系统，这里提供框架
        
        // 1. 准备编译选项
        GraphCompileOptions compile_opts;
        for (const auto& opt : opts) {
            if (opt) {
                opt(compile_opts);
            }
        }
        
        // 2. 构建图信息用于回调
        GraphInfo graph_info;
        graph_info.name = compile_opts.graph_name;
        graph_info.input_type = typeid(I).name();
        graph_info.output_type = typeid(O).name();
        graph_info.edges = edges_;
        graph_info.nodes = nodes_;
        
        // 3. 调用编译回调
        if (compile_opts.on_compile_finish) {
            compile_opts.on_compile_finish->OnFinish(ctx, graph_info);
        }
        
        // 4. 全局回调
        for (const auto& callback : GetGlobalGraphCompileCallbacks()) {
            callback->OnFinish(ctx, graph_info);
        }
        
        // 5. 创建可运行对象（占位符）
        // 实际实现需要根据图拓扑结构和节点类型构建执行计划
        return nullptr;
    }
    
    components::Component GetComponent() const override {
        return components::ComponentOfGraph;
    }

private:
    NewGraphOptions options_;
    std::map<std::string, GraphNodeInfo> nodes_;
    std::map<std::string, std::vector<std::string>> edges_;
    std::map<std::string, std::vector<std::string>> data_edges_;
};

// Graph构造函数
template<typename I, typename O>
Graph<I, O>::Graph(const std::vector<NewGraphOption>& opts) {
    NewGraphOptions options;
    for (const auto& opt : opts) {
        if (opt) {
            opt(options);
        }
    }
    
    impl_ = std::make_shared<GraphImpl<I, O>>(options);
}

// Graph::AddEdge实现
template<typename I, typename O>
std::error_code Graph<I, O>::AddEdge(
    const std::string& start_node, 
    const std::string& end_node) {
    return impl_->AddEdge(start_node, end_node);
}

// Graph::Compile实现
template<typename I, typename O>
std::shared_ptr<Runnable<I, O>> Graph<I, O>::Compile(
    std::shared_ptr<Context> ctx,
    const std::vector<GraphCompileOption>& opts) {
    return impl_->Compile(ctx, opts);
}

// CompileAnyGraph实现
template<typename I, typename O>
std::shared_ptr<Runnable<I, O>> CompileAnyGraph(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<void> graph,
    const std::vector<GraphCompileOption>& opts) {
    
    // 尝试转换为Graph<I, O>
    auto typed_graph = std::static_pointer_cast<Graph<I, O>>(graph);
    if (!typed_graph) {
        return nullptr;
    }
    
    return typed_graph->Compile(ctx, opts);
}

// 显式实例化常用类型
template class Graph<std::string, std::string>;
template class Graph<int, int>;
template class Graph<std::string, int>;
template class Graph<int, std::string>;

template std::shared_ptr<Runnable<std::string, std::string>> CompileAnyGraph<std::string, std::string>(
    std::shared_ptr<Context>, std::shared_ptr<void>, const std::vector<GraphCompileOption>&);

} // namespace compose
} // namespace eino
