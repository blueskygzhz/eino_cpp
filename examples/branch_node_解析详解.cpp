/*
 * BranchNode 节点引用解析详解
 * 
 * 问题：node_a 输出和 node_b 输出里面都没有 node 自己的信息，
 *       BranchNode 怎么正确解析出自己需要的值？
 * 
 * 答案：通过特殊的输入格式和路径解析机制
 */

#include <iostream>
#include <map>
#include <any>
#include <string>
#include "../include/eino/compose/branch_node.h"

using namespace eino::compose;

int main() {
    std::cout << "======================================================================" << std::endl;
    std::cout << "  BranchNode 节点引用解析机制详解" << std::endl;
    std::cout << "======================================================================" << std::endl;
    
    // ========================================================================
    // 场景设置
    // ========================================================================
    std::cout << "\n【场景】" << std::endl;
    std::cout << "- Node A 输出: {\"age\": 25, \"name\": \"Alice\"}" << std::endl;
    std::cout << "- Node B 输出: {\"score\": 85, \"vip\": true}" << std::endl;
    std::cout << "- BranchNode 需要判断: (node_a.age >= 18) AND (node_b.vip == true)" << std::endl;
    
    // ========================================================================
    // 第 1 步：创建 BranchNode 配置（声明需要引用哪些节点）
    // ========================================================================
    std::cout << "\n【步骤 1】创建 BranchNode 配置" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;
    
    BranchNodeConfig config;
    
    std::cout << "\n配置条件: (node_a.age >= 18) AND (node_b.vip == true)" << std::endl;
    std::cout << "\n关键点：使用 OperandConfig::FromNode() 声明节点引用" << std::endl;
    
    std::vector<SingleClauseConfig> clauses = {
        SingleClauseConfig(
            BranchOperator::GreaterOrEqual,
            OperandConfig::FromNode("node_a", {"age"}),   // ← 声明：需要 node_a 的 age
            OperandConfig::FromLiteral(static_cast<int64_t>(18))
        ),
        SingleClauseConfig(
            BranchOperator::Equal,
            OperandConfig::FromNode("node_b", {"vip"}),   // ← 声明：需要 node_b 的 vip
            OperandConfig::FromLiteral(true)
        )
    };
    config.AddMultiConditionWithOperands(clauses, ClauseRelation::AND);
    
    std::cout << "\n✓ 配置中记录了:" << std::endl;
    std::cout << "  - 左操作数需要从 'node_a' 的 'age' 字段获取" << std::endl;
    std::cout << "  - 另一个操作数需要从 'node_b' 的 'vip' 字段获取" << std::endl;
    
    // ========================================================================
    // 第 2 步：模拟 Node A 和 Node B 的输出
    // ========================================================================
    std::cout << "\n【步骤 2】模拟上游节点的输出" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;
    
    // Node A 的输出（注意：这里没有 "node_a" 这个键）
    std::map<std::string, std::any> node_a_output;
    node_a_output["age"] = static_cast<int64_t>(25);
    node_a_output["name"] = std::string("Alice");
    
    std::cout << "\nNode A 输出内容:" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    \"age\": 25," << std::endl;
    std::cout << "    \"name\": \"Alice\"" << std::endl;
    std::cout << "  }" << std::endl;
    std::cout << "\n注意：输出中没有 'node_a' 这个键！" << std::endl;
    
    // Node B 的输出（注意：这里也没有 "node_b" 这个键）
    std::map<std::string, std::any> node_b_output;
    node_b_output["score"] = static_cast<int64_t>(85);
    node_b_output["vip"] = true;
    
    std::cout << "\nNode B 输出内容:" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    \"score\": 85," << std::endl;
    std::cout << "    \"vip\": true" << std::endl;
    std::cout << "  }" << std::endl;
    std::cout << "\n注意：输出中也没有 'node_b' 这个键！" << std::endl;
    
    // ========================================================================
    // 第 3 步：构造 BranchNode 的输入（关键步骤！）
    // ========================================================================
    std::cout << "\n【步骤 3】构造 BranchNode 的输入（★ 关键步骤 ★）" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;
    
    std::cout << "\n需要将各节点的输出包装成特殊格式:" << std::endl;
    std::cout << "\n输入格式：" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    \"node_a\": <node_a的完整输出>," << std::endl;
    std::cout << "    \"node_b\": <node_b的完整输出>" << std::endl;
    std::cout << "  }" << std::endl;
    
    std::map<std::string, std::any> branch_input;
    branch_input["node_a"] = node_a_output;  // ← 用 "node_a" 作为键
    branch_input["node_b"] = node_b_output;  // ← 用 "node_b" 作为键
    
    std::cout << "\n实际构造的输入：" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    \"node_a\": {" << std::endl;
    std::cout << "      \"age\": 25," << std::endl;
    std::cout << "      \"name\": \"Alice\"" << std::endl;
    std::cout << "    }," << std::endl;
    std::cout << "    \"node_b\": {" << std::endl;
    std::cout << "      \"score\": 85," << std::endl;
    std::cout << "      \"vip\": true" << std::endl;
    std::cout << "    }" << std::endl;
    std::cout << "  }" << std::endl;
    
    std::cout << "\n✓ 关键点：" << std::endl;
    std::cout << "  1. 使用节点名称（\"node_a\", \"node_b\"）作为顶层键" << std::endl;
    std::cout << "  2. 节点的原始输出作为值" << std::endl;
    std::cout << "  3. 这样形成了一个两层的嵌套结构" << std::endl;
    
    // ========================================================================
    // 第 4 步：BranchNode 内部解析过程
    // ========================================================================
    std::cout << "\n【步骤 4】BranchNode 内部解析过程" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;
    
    std::cout << "\n当 BranchNode 执行 Invoke() 时，内部做了以下操作：" << std::endl;
    
    std::cout << "\n1️⃣  读取配置中的引用信息：" << std::endl;
    std::cout << "   - 条件 1 左操作数: FromNode(\"node_a\", {\"age\"})" << std::endl;
    std::cout << "   - 条件 2 左操作数: FromNode(\"node_b\", {\"vip\"})" << std::endl;
    
    std::cout << "\n2️⃣  调用 ResolveValueSource() 解析值：" << std::endl;
    std::cout << "   对于 FromNode(\"node_a\", {\"age\"})：" << std::endl;
    std::cout << "   - 构造路径: [\"node_a\", \"age\"]" << std::endl;
    std::cout << "   - 调用 TakeMapValue(input, [\"node_a\", \"age\"], out)" << std::endl;
    std::cout << "   - 先找到 input[\"node_a\"] → {age: 25, name: \"Alice\"}" << std::endl;
    std::cout << "   - 再找到 input[\"node_a\"][\"age\"] → 25" << std::endl;
    std::cout << "   - 返回: 25" << std::endl;
    
    std::cout << "\n3️⃣  同样解析其他引用：" << std::endl;
    std::cout << "   对于 FromNode(\"node_b\", {\"vip\"})：" << std::endl;
    std::cout << "   - 构造路径: [\"node_b\", \"vip\"]" << std::endl;
    std::cout << "   - input[\"node_b\"][\"vip\"] → true" << std::endl;
    std::cout << "   - 返回: true" << std::endl;
    
    std::cout << "\n4️⃣  执行条件判断：" << std::endl;
    std::cout << "   - (25 >= 18) → true" << std::endl;
    std::cout << "   - (true == true) → true" << std::endl;
    std::cout << "   - true AND true → true" << std::endl;
    std::cout << "   - 结果: Branch 0 匹配" << std::endl;
    
    // ========================================================================
    // 第 5 步：实际执行验证
    // ========================================================================
    std::cout << "\n【步骤 5】实际执行验证" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;
    
    auto branch_node = BranchNode<
        std::map<std::string, std::any>,
        std::map<std::string, std::any>
    >::New(nullptr, config);
    
    std::cout << "\n执行 BranchNode::Invoke()..." << std::endl;
    
    auto output = branch_node->Invoke(nullptr, branch_input);
    int64_t selected = std::any_cast<int64_t>(output["selected"]);
    
    std::cout << "\n✅ 执行结果: Branch " << selected << std::endl;
    std::cout << "\n条件判断成功！BranchNode 正确解析了 node_a 和 node_b 的输出。" << std::endl;
    
    // ========================================================================
    // 核心机制总结
    // ========================================================================
    std::cout << "\n======================================================================" << std::endl;
    std::cout << "  核心机制总结" << std::endl;
    std::cout << "======================================================================" << std::endl;
    
    std::cout << "\n【问题】" << std::endl;
    std::cout << "node_a 输出 {age: 25} 和 node_b 输出 {vip: true}" << std::endl;
    std::cout << "它们都没有包含 'node_a' 或 'node_b' 这样的键" << std::endl;
    std::cout << "BranchNode 怎么知道哪个是 node_a，哪个是 node_b？" << std::endl;
    
    std::cout << "\n【答案】" << std::endl;
    std::cout << "通过特殊的输入包装格式：" << std::endl;
    std::cout << "\n1️⃣  配置阶段 - 声明引用" << std::endl;
    std::cout << "   OperandConfig::FromNode(\"node_a\", {\"age\"})" << std::endl;
    std::cout << "   └─ 告诉 BranchNode：我需要从名为 'node_a' 的数据中提取 'age'" << std::endl;
    
    std::cout << "\n2️⃣  输入构造阶段 - 包装数据" << std::endl;
    std::cout << "   branch_input[\"node_a\"] = node_a_output" << std::endl;
    std::cout << "   └─ 使用节点名称作为键，将节点输出作为值" << std::endl;
    
    std::cout << "\n3️⃣  解析阶段 - 路径查找" << std::endl;
    std::cout << "   ResolveValueSource() 看到 FromNode(\"node_a\", {\"age\"})" << std::endl;
    std::cout << "   └─ 构造路径 [\"node_a\", \"age\"]" << std::endl;
    std::cout << "   └─ 从 input[\"node_a\"][\"age\"] 提取值" << std::endl;
    
    std::cout << "\n【关键代码】" << std::endl;
    std::cout << "\n// 步骤 1: 配置引用" << std::endl;
    std::cout << "OperandConfig::FromNode(\"node_a\", {\"age\"})" << std::endl;
    
    std::cout << "\n// 步骤 2: 包装输入" << std::endl;
    std::cout << "branch_input[\"node_a\"] = node_a_output;" << std::endl;
    std::cout << "branch_input[\"node_b\"] = node_b_output;" << std::endl;
    
    std::cout << "\n// 步骤 3: BranchNode 内部解析" << std::endl;
    std::cout << "// ResolveValueSource() 函数:" << std::endl;
    std::cout << "// 1. 构造完整路径: [\"node_a\", \"age\"]" << std::endl;
    std::cout << "// 2. 调用 TakeMapValue(input, path, out)" << std::endl;
    std::cout << "// 3. 递归查找: input[\"node_a\"][\"age\"] → 25" << std::endl;
    
    // ========================================================================
    // 多层级路径示例
    // ========================================================================
    std::cout << "\n======================================================================" << std::endl;
    std::cout << "  多层级路径示例" << std::endl;
    std::cout << "======================================================================" << std::endl;
    
    std::cout << "\n假设 Node A 输出嵌套结构：" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    \"result\": {" << std::endl;
    std::cout << "      \"data\": {" << std::endl;
    std::cout << "        \"score\": 95" << std::endl;
    std::cout << "      }" << std::endl;
    std::cout << "    }" << std::endl;
    std::cout << "  }" << std::endl;
    
    std::cout << "\n引用方式：" << std::endl;
    std::cout << "  OperandConfig::FromNode(\"node_a\", {\"result\", \"data\", \"score\"})" << std::endl;
    
    std::cout << "\nBranchNode 输入格式：" << std::endl;
    std::cout << "  {" << std::endl;
    std::cout << "    \"node_a\": {" << std::endl;
    std::cout << "      \"result\": {" << std::endl;
    std::cout << "        \"data\": {" << std::endl;
    std::cout << "          \"score\": 95" << std::endl;
    std::cout << "        }" << std::endl;
    std::cout << "      }" << std::endl;
    std::cout << "    }" << std::endl;
    std::cout << "  }" << std::endl;
    
    std::cout << "\n解析路径：" << std::endl;
    std::cout << "  [\"node_a\", \"result\", \"data\", \"score\"]" << std::endl;
    std::cout << "  └─ input[\"node_a\"][\"result\"][\"data\"][\"score\"] → 95" << std::endl;
    
    // ========================================================================
    // 总结
    // ========================================================================
    std::cout << "\n======================================================================" << std::endl;
    std::cout << "  ✅ 总结" << std::endl;
    std::cout << "======================================================================" << std::endl;
    
    std::cout << "\n【核心思想】" << std::endl;
    std::cout << "通过在输入中使用节点名称作为键，将节点输出包装成嵌套结构，" << std::endl;
    std::cout << "使得 BranchNode 能够根据配置中的节点引用信息，" << std::endl;
    std::cout << "正确地从输入中提取出所需的值。" << std::endl;
    
    std::cout << "\n【三个关键点】" << std::endl;
    std::cout << "1. 配置中使用 FromNode(\"节点名\", {路径}) 声明引用" << std::endl;
    std::cout << "2. 输入中使用 input[\"节点名\"] = 节点输出 进行包装" << std::endl;
    std::cout << "3. BranchNode 内部通过路径解析提取值" << std::endl;
    
    std::cout << "\n【实现细节】" << std::endl;
    std::cout << "- ResolveValueSource(): 解析 ValueSource，提取实际值" << std::endl;
    std::cout << "- TakeMapValue(): 递归遍历嵌套 map，按路径提取值" << std::endl;
    std::cout << "- ConvertInputWithReferences(): 批量解析所有条件的操作数" << std::endl;
    
    return 0;
}
