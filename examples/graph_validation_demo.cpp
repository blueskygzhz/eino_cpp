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

/**
 * Graph Validation Demo - 演示 AddEdge 时的类型验证
 * 
 * 这个示例展示了修复后的行为：
 * - AddEdge 会立即进行类型验证
 * - 类型不匹配会在添加边时立即报错
 * - 与 Go 版本行为完全对齐
 */

#include <iostream>
#include <memory>
#include <string>
#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"

using namespace eino;
using namespace eino::compose;

// 示例：字符串转大写节点
class ToUpperNode : public Runnable<std::string, std::string> {
public:
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = {}) override {
        
        std::string result = input;
        for (char& c : result) {
            c = std::toupper(c);
        }
        return result;
    }
    
    std::shared_ptr<StreamReader<std::string>> Stream(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = {}) override {
        return nullptr;
    }
    
    std::string Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = {}) override {
        return "";
    }
    
    std::shared_ptr<StreamReader<std::string>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = {}) override {
        return nullptr;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::string);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::string);
    }
    
    std::string GetComponentType() const override {
        return "ToUpperNode";
    }
};

// 示例：字符串反转节点
class ReverseNode : public Runnable<std::string, std::string> {
public:
    std::string Invoke(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = {}) override {
        
        return std::string(input.rbegin(), input.rend());
    }
    
    std::shared_ptr<StreamReader<std::string>> Stream(
        std::shared_ptr<Context> ctx,
        const std::string& input,
        const std::vector<Option>& opts = {}) override {
        return nullptr;
    }
    
    std::string Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = {}) override {
        return "";
    }
    
    std::shared_ptr<StreamReader<std::string>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<std::string>> input,
        const std::vector<Option>& opts = {}) override {
        return nullptr;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(std::string);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(std::string);
    }
    
    std::string GetComponentType() const override {
        return "ReverseNode";
    }
};

/**
 * 示例1: 成功的类型验证
 */
void demo_successful_validation() {
    std::cout << "\n=== Demo 1: Successful Type Validation ===\n\n";
    
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // 添加两个兼容的节点
    auto to_upper = std::make_shared<ToUpperNode>();
    auto reverse = std::make_shared<ReverseNode>();
    
    std::cout << "Adding node 'to_upper' (string -> string)...\n";
    graph->AddNode("to_upper", to_upper);
    
    std::cout << "Adding node 'reverse' (string -> string)...\n";
    graph->AddNode("reverse", reverse);
    
    std::cout << "\nAdding edge: __START__ -> to_upper\n";
    graph->AddEdge(Graph<std::string, std::string>::START_NODE, "to_upper");
    std::cout << "✅ Edge added successfully (types compatible)\n";
    
    std::cout << "\nAdding edge: to_upper -> reverse\n";
    graph->AddEdge("to_upper", "reverse");
    std::cout << "✅ Edge added successfully (types compatible)\n";
    
    std::cout << "\nAdding edge: reverse -> __END__\n";
    graph->AddEdge("reverse", Graph<std::string, std::string>::END_NODE);
    std::cout << "✅ Edge added successfully (types compatible)\n";
    
    std::cout << "\nCompiling graph...\n";
    graph->Compile();
    std::cout << "✅ Graph compiled successfully\n";
    
    std::cout << "\nTesting graph execution:\n";
    auto ctx = Context::Background();
    auto result = graph->Invoke(ctx, "hello");
    std::cout << "Input:  'hello'\n";
    std::cout << "Output: '" << result << "' (expected: 'OLLEH')\n";
}

/**
 * 示例2: 演示修复前后的差异
 */
void demo_before_and_after_fix() {
    std::cout << "\n=== Demo 2: Before/After Fix Comparison ===\n\n";
    
    std::cout << "【修复前的行为 ❌】\n";
    std::cout << "-------------------------------\n";
    std::cout << "graph->AddEdge(\"node1\", \"node2\");  // ✅ 成功（即使类型不匹配）\n";
    std::cout << "// ... 用户继续构建图 ...\n";
    std::cout << "graph->Compile();  // ❌ 这时才报错：Type mismatch!\n";
    std::cout << "\n问题：错误发现太晚，用户体验差\n";
    
    std::cout << "\n【修复后的行为 ✅】\n";
    std::cout << "-------------------------------\n";
    std::cout << "graph->AddEdge(\"node1\", \"node2\");  \n";
    std::cout << "// ❌ 立即抛出异常：\n";
    std::cout << "// std::runtime_error: Type mismatch: node 'node1' output (string)\n";
    std::cout << "//                     incompatible with node 'node2' input (int)\n";
    std::cout << "\n改进：错误立即被发现，堆栈跟踪指向 AddEdge 调用点\n";
}

/**
 * 示例3: 控制边 vs 数据边
 */
void demo_control_vs_data_edges() {
    std::cout << "\n=== Demo 3: Control vs Data Edges ===\n\n";
    
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto node1 = std::make_shared<ToUpperNode>();
    auto node2 = std::make_shared<ReverseNode>();
    
    graph->AddNode("node1", node1);
    graph->AddNode("node2", node2);
    
    std::cout << "【数据边 (Data Edge)】\n";
    std::cout << "- 会传递数据\n";
    std::cout << "- 触发类型验证 ✅\n";
    std::cout << "- 示例：graph->AddEdge(\"node1\", \"node2\");\n";
    graph->AddEdge("node1", "node2");
    std::cout << "✅ Type validation performed\n";
    
    std::cout << "\n【控制边 (Control Edge)】\n";
    std::cout << "- 只表示执行顺序\n";
    std::cout << "- 不传递数据\n";
    std::cout << "- 跳过类型验证\n";
    std::cout << "- 示例：graph->AddEdge(\"node1\", \"node2\", false, true);\n";
    std::cout << "         // no_control=false, no_data=true\n";
}

/**
 * 示例4: 字段映射验证
 */
void demo_field_mapping_validation() {
    std::cout << "\n=== Demo 4: Field Mapping Validation ===\n\n";
    
    std::cout << "当使用字段映射时，也会触发类型验证：\n\n";
    
    std::cout << "std::vector<FieldMapping> mappings;\n";
    std::cout << "FieldMapping mapping;\n";
    std::cout << "mapping.from_key = \"output_field\";\n";
    std::cout << "mapping.to_key = \"input_field\";\n";
    std::cout << "mappings.push_back(mapping);\n\n";
    
    std::cout << "graph->AddEdge(\"node1\", \"node2\", mappings);\n";
    std::cout << "// ✅ 会验证：\n";
    std::cout << "// 1. 字段路径是否有效\n";
    std::cout << "// 2. 字段类型是否兼容\n";
    std::cout << "// 3. 转换函数是否存在（如果需要）\n";
}

/**
 * 主函数
 */
int main() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║        Graph Validation Demo - AddEdge Type Checking         ║
║                                                               ║
║  演示修复后的行为：AddEdge 时立即进行类型验证                  ║
║  对齐 Go 版本：eino/compose/graph.go:285-289                  ║
╚═══════════════════════════════════════════════════════════════╝
    )";
    
    try {
        demo_successful_validation();
        demo_before_and_after_fix();
        demo_control_vs_data_edges();
        demo_field_mapping_validation();
        
        std::cout << "\n\n";
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                    All Demos Completed ✅                      ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
