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

#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "eino/compose/chain.h"
#include "eino/compose/runnable.h"

using json = nlohmann::json;

int main() {
    std::cout << "=== Eino C++ Basic Chain Example ===" << std::endl;
    
    try {
        // Create two simple steps
        auto step1 = eino::compose::NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<eino::compose::Context> ctx, 
               const std::string& input,
               const std::vector<eino::compose::Option>& opts) {
                std::cout << "[Step 1] Input: " << input << std::endl;
                return "processed_" + input;
            }
        );
        
        auto step2 = eino::compose::NewLambdaRunnable<std::string, std::string>(
            [](std::shared_ptr<eino::compose::Context> ctx,
               const std::string& input,
               const std::vector<eino::compose::Option>& opts) {
                std::cout << "[Step 2] Input: " << input << std::endl;
                return input + "_final";
            }
        );
        
        // Create chain from two steps
        std::shared_ptr<eino::compose::Runnable<std::string, std::string>> s1 = step1;
        std::shared_ptr<eino::compose::Runnable<std::string, std::string>> s2 = step2;
        
        auto chain = eino::compose::NewChain<std::string, std::string, std::string>(s1, s2);
        
        std::cout << "\nChain created successfully!" << std::endl;
        
        // Compile chain
        chain->Compile();
        std::cout << "Chain compiled successfully!" << std::endl;
        
        // Execute chain
        auto ctx = eino::compose::Context::Background();
        auto empty_opts = std::vector<eino::compose::Option>();
        
        std::string result = chain->Invoke(ctx, "test_input", empty_opts);
        std::cout << "\nChain execution result: " << result << std::endl;
        
        // Expected: "processed_test_input_final"
        if (result == "processed_test_input_final") {
            std::cout << "✓ Chain execution successful!" << std::endl;
        } else {
            std::cout << "✗ Chain execution failed (unexpected result)" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
