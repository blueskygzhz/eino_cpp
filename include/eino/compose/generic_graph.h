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

#ifndef EINO_COMPOSE_GENERIC_GRAPH_H_
#define EINO_COMPOSE_GENERIC_GRAPH_H_

#include <memory>
#include <functional>
#include <string>

#include "eino/compose/runnable.h"
#include "eino/compose/graph_compile_options.h"

namespace eino {
namespace compose {

// 对齐 eino/compose/generic_graph.go

/**
 * @brief 状态生成器类型
 * 
 * 对应 Go: GenLocalState[S any]
 */
template<typename S>
using GenLocalState = std::function<std::shared_ptr<S>(std::shared_ptr<Context>)>;

/**
 * @brief 新图选项配置
 * 
 * 对应 Go: newGraphOptions
 */
struct NewGraphOptions {
    std::function<std::shared_ptr<void>(std::shared_ptr<Context>)> with_state;
    std::string state_type;
};

/**
 * @brief 新图选项函数类型
 * 
 * 对应 Go: NewGraphOption
 */
using NewGraphOption = std::function<void(NewGraphOptions&)>;

/**
 * @brief 设置状态生成器选项
 * 
 * 对应 Go: WithGenLocalState[S any](gls GenLocalState[S]) NewGraphOption
 * 
 * 用于在节点间共享状态
 * 
 * 示例:
 * @code
 * struct TestState {
 *   std::shared_ptr<UserInfo> user_info;
 *   std::map<std::string, std::any> kvs;
 * };
 * 
 * auto gen_state = [](std::shared_ptr<Context> ctx) {
 *   return std::make_shared<TestState>();
 * };
 * 
 * auto graph = NewGraph<std::string, std::string>(
 *   WithGenLocalState<TestState>(gen_state)
 * );
 * @endcode
 */
template<typename S>
NewGraphOption WithGenLocalState(GenLocalState<S> gls) {
    return [gls](NewGraphOptions& opts) {
        opts.with_state = [gls](std::shared_ptr<Context> ctx) -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(gls(ctx));
        };
        opts.state_type = typeid(S).name();
    };
}

/**
 * @brief 泛型图类
 * 
 * 对应 Go: Graph[I, O any]
 * 
 * 模板参数:
 * - I: 图输入类型
 * - O: 图输出类型
 * 
 * 用法示例:
 * @code
 * auto graph = NewGraph<std::string, std::string>();
 * graph->AddNode("node1", some_node);
 * graph->AddNode("node2", some_node);
 * graph->AddEdge("node1", "node2");
 * 
 * auto runnable = graph->Compile(ctx, WithGraphName("my_graph"));
 * auto result = runnable->Invoke(ctx, "input");
 * @endcode
 */
template<typename I, typename O>
class Graph {
public:
    Graph(const std::vector<NewGraphOption>& opts = {});
    ~Graph() = default;

    /**
     * @brief 添加边
     * 
     * 对应 Go: AddEdge(startNode, endNode string) error
     * 
     * 边表示从起始节点到结束节点的数据流
     * 前一个节点的输出类型必须设置为下一个节点的输入类型
     * 
     * 注意: startNode和endNode必须先通过AddNode添加
     * 
     * @param start_node 起始节点键
     * @param end_node 结束节点键
     * @return 错误码
     */
    std::error_code AddEdge(const std::string& start_node, const std::string& end_node);

    /**
     * @brief 编译图
     * 
     * 对应 Go: Compile(ctx context.Context, opts ...GraphCompileOption) (Runnable[I, O], error)
     * 
     * 将原始图编译成可执行形式
     * 
     * @param ctx 上下文
     * @param opts 编译选项
     * @return 可运行对象
     */
    std::shared_ptr<Runnable<I, O>> Compile(
        std::shared_ptr<Context> ctx,
        const std::vector<GraphCompileOption>& opts = {});

    // 内部实现接口（由具体图类实现）
    class Impl;
    std::shared_ptr<Impl> impl_;
};

/**
 * @brief 创建新图
 * 
 * 对应 Go: NewGraph[I, O any](opts ...NewGraphOption) *Graph[I, O]
 * 
 * @param opts 新图选项
 * @return 图实例
 */
template<typename I, typename O>
std::shared_ptr<Graph<I, O>> NewGraph(const std::vector<NewGraphOption>& opts = {}) {
    return std::make_shared<Graph<I, O>>(opts);
}

/**
 * @brief 编译任意图
 * 
 * 对应 Go: compileAnyGraph[I, O any](ctx context.Context, g AnyGraph, opts ...GraphCompileOption) (Runnable[I, O], error)
 * 
 * 内部辅助函数，处理图编译的通用逻辑
 */
template<typename I, typename O>
std::shared_ptr<Runnable<I, O>> CompileAnyGraph(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<void> graph,
    const std::vector<GraphCompileOption>& opts);

} // namespace compose
} // namespace eino

#endif // EINO_COMPOSE_GENERIC_GRAPH_H_
