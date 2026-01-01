#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "eino/adk/types.h"
#include "eino/adk/context.h"

using namespace eino::adk;

int main() {
    int test_count = 0;
    int passed_count = 0;
    
    std::cout << "\n========== ADK Unit Tests ==========\n" << std::endl;
    
    // Test 1: MessageVariant
    {
        test_count++;
        std::cout << "Test 1: MessageVariant creation... ";
        try {
            MessageVariant msg_var;
            msg_var.is_streaming = false;
            msg_var.message = nullptr;
            if (!msg_var.is_streaming && msg_var.message == nullptr) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 2: TransferToAgentAction
    {
        test_count++;
        std::cout << "Test 2: TransferToAgentAction creation... ";
        try {
            TransferToAgentAction action;
            action.dest_agent_name = "target_agent";
            if (action.dest_agent_name == "target_agent") {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 3: AgentOutput
    {
        test_count++;
        std::cout << "Test 3: AgentOutput creation... ";
        try {
            AgentOutput output;
            output.message_output = nullptr;
            output.customized_output = nullptr;
            if (output.message_output == nullptr && output.customized_output == nullptr) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 4: AgentAction
    {
        test_count++;
        std::cout << "Test 4: AgentAction creation... ";
        try {
            AgentAction action;
            action.exit = false;
            if (!action.exit && action.interrupted == nullptr) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 5: RunStep
    {
        test_count++;
        std::cout << "Test 5: RunStep creation... ";
        try {
            RunStep step;
            step.agent_name = "test_agent";
            if (step.String() == "test_agent") {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 6: RunStep::Equals
    {
        test_count++;
        std::cout << "Test 6: RunStep::Equals... ";
        try {
            RunStep step1, step2;
            step1.agent_name = "agent_a";
            step2.agent_name = "agent_a";
            if (step1.Equals(step2)) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (equals failed)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 7: AgentEvent
    {
        test_count++;
        std::cout << "Test 7: AgentEvent creation... ";
        try {
            AgentEvent event;
            event.agent_name = "agent_x";
            event.error_msg = "";
            if (!event.HasError() && event.agent_name == "agent_x") {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 8: AgentEvent with error
    {
        test_count++;
        std::cout << "Test 8: AgentEvent with error... ";
        try {
            AgentEvent event;
            event.error_msg = "Test error";
            if (event.HasError()) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (error detection failed)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 9: AgentInput
    {
        test_count++;
        std::cout << "Test 9: AgentInput creation... ";
        try {
            AgentInput input;
            input.enable_streaming = false;
            if (!input.enable_streaming && input.messages.empty()) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 10: ChatModelAgentState
    {
        test_count++;
        std::cout << "Test 10: ChatModelAgentState creation... ";
        try {
            ChatModelAgentState state;
            if (state.messages.empty()) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (state not empty)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 11: InterruptInfo
    {
        test_count++;
        std::cout << "Test 11: InterruptInfo creation... ";
        try {
            InterruptInfo interrupt;
            interrupt.data = nullptr;
            if (interrupt.data == nullptr) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (value mismatch)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    // Test 12: RunSession creation
    {
        test_count++;
        std::cout << "Test 12: RunSession creation... ";
        try {
            auto session = std::make_shared<RunSession>();
            if (session != nullptr && session->GetEvents().empty()) {
                std::cout << "✓ PASS" << std::endl;
                passed_count++;
            } else {
                std::cout << "✗ FAIL (session invalid)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ FAIL (exception: " << e.what() << ")" << std::endl;
        }
    }
    
    std::cout << "\n========== Results ==========\n";
    std::cout << "Passed: " << passed_count << "/" << test_count << std::endl;
    std::cout << "Failed: " << (test_count - passed_count) << "/" << test_count << std::endl;
    std::cout << "\n";
    
    return (test_count - passed_count) == 0 ? 0 : 1;
}
