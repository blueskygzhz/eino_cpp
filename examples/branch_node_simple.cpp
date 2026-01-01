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

/**
 * BranchNode 简单示例
 * 
 * 本示例展示如何使用 branch_node 进行条件分支判断
 * 场景：用户年龄检查
 * - Branch 0: age >= 18 (成年人)
 * - Default: age < 18 (未成年)
 */

#include "../include/eino/compose/branch_node.h"
#include <iostream>
#include <map>
#include <any>

using namespace eino::compose;

int main() {
    std::cout << "\n=====================================" << std::endl;
    std::cout << "   BranchNode 简单示例 - 年龄检查" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        // 步骤 1: 创建 BranchNode 配置
        std::cout << "\n[步骤 1] 创建 BranchNode 配置..." << std::endl;
        BranchNodeConfig config;
        
        // 添加条件: age >= 18
        config.AddSingleCondition(BranchOperator::GreaterOrEqual);
        std::cout << "  ✓ 添加条件: age >= 18" << std::endl;
        
        // 步骤 2: 创建 BranchNode
        std::cout << "\n[步骤 2] 创建 BranchNode..." << std::endl;
        auto branch_node = BranchNode<
            std::map<std::string, std::any>,
            std::map<std::string, std::any>
        >::New(nullptr, config);
        std::cout << "  ✓ BranchNode 创建成功" << std::endl;
        
        // 步骤 3: 测试用例 1 - 成年人 (age = 25)
        std::cout << "\n[步骤 3] 测试用例 1: age = 25" << std::endl;
        {
            std::map<std::string, std::any> input;
            std::map<std::string, std::any> condition;
            
            condition["left"] = static_cast<int64_t>(25);   // 用户年龄
            condition["right"] = static_cast<int64_t>(18);  // 阈值
            input["0"] = condition;
            
            std::cout << "  输入: age = 25, threshold = 18" << std::endl;
            
            auto output = branch_node->Invoke(nullptr, input);
            int64_t selected = std::any_cast<int64_t>(output["selected"]);
            
            std::cout << "  输出: Branch " << selected;
            if (selected == 0) {
                std::cout << " → 成年人 (条件满足: 25 >= 18)" << std::endl;
            } else {
                std::cout << " → 未成年 (错误!)" << std::endl;
            }
        }
        
        // 步骤 4: 测试用例 2 - 未成年 (age = 15)
        std::cout << "\n[步骤 4] 测试用例 2: age = 15" << std::endl;
        {
            std::map<std::string, std::any> input;
            std::map<std::string, std::any> condition;
            
            condition["left"] = static_cast<int64_t>(15);   // 用户年龄
            condition["right"] = static_cast<int64_t>(18);  // 阈值
            input["0"] = condition;
            
            std::cout << "  输入: age = 15, threshold = 18" << std::endl;
            
            auto output = branch_node->Invoke(nullptr, input);
            int64_t selected = std::any_cast<int64_t>(output["selected"]);
            
            std::cout << "  输出: Branch " << selected;
            if (selected == 1) {
                std::cout << " → 未成年 (默认分支: 15 < 18)" << std::endl;
            } else {
                std::cout << " → 成年人 (错误!)" << std::endl;
            }
        }
        
        // 步骤 5: 测试用例 3 - 边界值 (age = 18)
        std::cout << "\n[步骤 5] 测试用例 3: age = 18 (边界值)" << std::endl;
        {
            std::map<std::string, std::any> input;
            std::map<std::string, std::any> condition;
            
            condition["left"] = static_cast<int64_t>(18);   // 用户年龄
            condition["right"] = static_cast<int64_t>(18);  // 阈值
            input["0"] = condition;
            
            std::cout << "  输入: age = 18, threshold = 18" << std::endl;
            
            auto output = branch_node->Invoke(nullptr, input);
            int64_t selected = std::any_cast<int64_t>(output["selected"]);
            
            std::cout << "  输出: Branch " << selected;
            if (selected == 0) {
                std::cout << " → 成年人 (条件满足: 18 >= 18)" << std::endl;
            } else {
                std::cout << " → 未成年 (错误!)" << std::endl;
            }
        }
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "   ✅ 所有测试完成!" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        // 运行步骤总结
        std::cout << "\n【运行步骤总结】" << std::endl;
        std::cout << "1. 创建 BranchNodeConfig，添加条件 (age >= 18)" << std::endl;
        std::cout << "2. 使用配置创建 BranchNode" << std::endl;
        std::cout << "3. 准备输入数据 (包含 left 和 right 操作数)" << std::endl;
        std::cout << "4. 调用 Invoke() 执行条件判断" << std::endl;
        std::cout << "5. 从输出中获取 selected 字段，表示选中的分支索引" << std::endl;
        std::cout << "   - Branch 0: 条件满足" << std::endl;
        std::cout << "   - Branch 1: 条件不满足 (默认分支)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
