/*
 * Copyright 2024 CloudWeGo Authors
 *
 * Licensed under the Apache License, Version 2.0 (the \"License\");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an \"AS IS\" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EINO_CPP_SCHEMA_SELECT_H_
#define EINO_CPP_SCHEMA_SELECT_H_

// Aligns with eino/schema/select.go
//
// Select 提供多路选择功能，用于从多个流中并发接收数据
// 这是Go语言select语句的C++实现版本
//
// 核心功能：
// - 从多个流中同时等待数据
// - 返回第一个可用的数据项及其来源索引
// - 支持最多5个流的并发选择（与Go版本对齐）

#include "stream.h"
#include <vector>
#include <tuple>
#include <memory>

namespace eino {
namespace schema {

// MaxSelectNum 最大支持的select流数量
// 对齐: eino/schema/select.go maxSelectNum
constexpr int kMaxSelectNum = 5;

// ReceiveN 从多个流中接收数据（多路选择）
//
// 模板参数：
//   T - 流中元素的类型
//
// 参数：
//   chosen_list - 要监听的流索引列表（最多5个）
//   streams - 所有可用的流列表
//
// 返回值：
//   tuple<int, T, bool>:
//   - int: 返回数据的流索引（在chosen_list中的索引）
//   - T: 接收到的数据项
//   - bool: 是否成功接收（false表示流已关闭）
//
// 用法示例：
//   std::vector<int> chosen = {0, 2, 3};  // 监听第0、2、3个流
//   std::vector<std::shared_ptr<Stream<Message>>> streams;
//   auto [index, item, ok] = ReceiveN(chosen, streams);
//   if (ok) {
//       std::cout << "从流 " << index << " 收到数据" << std::endl;
//   }
//
// 对齐: eino/schema/select.go receiveN
template<typename T>
std::tuple<int, T, bool> ReceiveN(
    const std::vector<int>& chosen_list,
    const std::vector<std::shared_ptr<StreamReader<T>>>& streams);

// 内部实现：单流接收
template<typename T>
std::tuple<int, T, bool> ReceiveOne(
    const std::vector<int>& chosen_list,
    const std::vector<std::shared_ptr<StreamReader<T>>>& streams) {
    
    if (chosen_list.empty() || chosen_list[0] >= streams.size()) {
        return {-1, T(), false};
    }
    
    T item;
    bool ok = streams[chosen_list[0]]->Recv(item);
    return {chosen_list[0], item, ok};
}

// 内部实现：双流选择
template<typename T>
std::tuple<int, T, bool> ReceiveTwo(
    const std::vector<int>& chosen_list,
    const std::vector<std::shared_ptr<StreamReader<T>>>& streams) {
    
    if (chosen_list.size() < 2) {
        return ReceiveOne(chosen_list, streams);
    }
    
    // TODO: 实际实现需要使用 select/poll 机制
    // 这里简化为轮询第一个可用流
    T item;
    bool ok;
    
    // 尝试从第一个流接收（非阻塞）
    ok = streams[chosen_list[0]]->TryRecv(item);
    if (ok) return {chosen_list[0], item, true};
    
    // 尝试从第二个流接收（非阻塞）
    ok = streams[chosen_list[1]]->TryRecv(item);
    if (ok) return {chosen_list[1], item, true};
    
    // 如果都不可用，阻塞等待第一个
    ok = streams[chosen_list[0]]->Recv(item);
    return {chosen_list[0], item, ok};
}

// 主要实现
template<typename T>
std::tuple<int, T, bool> ReceiveN(
    const std::vector<int>& chosen_list,
    const std::vector<std::shared_ptr<StreamReader<T>>>& streams) {
    
    if (chosen_list.empty()) {
        return {-1, T(), false};
    }
    
    // 根据流数量选择不同的实现
    switch (chosen_list.size()) {
        case 1:
            return ReceiveOne(chosen_list, streams);
        case 2:
            return ReceiveTwo(chosen_list, streams);
        case 3:
        case 4:
        case 5: {
            // 多流选择：简化实现，轮询检查
            // TODO: 实际实现应使用更高效的多路复用机制
            T item;
            for (size_t i = 0; i < chosen_list.size(); ++i) {
                int idx = chosen_list[i];
                if (idx >= streams.size()) continue;
                
                if (streams[idx]->TryRecv(item)) {
                    return {idx, item, true};
                }
            }
            
            // 如果都不可用，阻塞等待第一个
            bool ok = streams[chosen_list[0]]->Recv(item);
            return {chosen_list[0], item, ok};
        }
        default:
            throw std::invalid_argument(
                "ReceiveN supports at most " + std::to_string(kMaxSelectNum) + " streams");
    }
}

}  // namespace schema
}  // namespace eino

#endif  // EINO_CPP_SCHEMA_SELECT_H_
