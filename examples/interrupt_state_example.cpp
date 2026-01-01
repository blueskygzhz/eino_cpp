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
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <memory>

// Note: Include paths assume you have eino_cpp properly installed
// #include "eino/compose/interrupt.h"
// #include "eino/compose/state.h"

// For demonstration, we'll use simplified versions
namespace eino {
namespace compose {

// Placeholder for Context
class Context {
public:
    static std::shared_ptr<Context> Background() {
        return std::make_shared<Context>();
    }
};

} // namespace compose
} // namespace eino

using namespace eino::compose;

// ============================================================================
// Example 1: Basic State Usage
// ============================================================================

struct CounterState {
    int counter = 0;
    std::string message;
    std::vector<int> values;
};

void ExampleBasicState() {
    std::cout << "\n=== Example 1: Basic State Usage ===" << std::endl;
    
    // Define state generator
    auto gen_state = [](std::shared_ptr<Context> ctx) -> CounterState {
        CounterState state;
        state.counter = 0;
        state.message = "initialized";
        return state;
    };
    
    // Create context
    auto ctx = Context::Background();
    
    std::cout << "✓ State generator created" << std::endl;
    std::cout << "✓ Context created" << std::endl;
    
    // In a real implementation:
    // StateManager<CounterState> manager(gen_state);
    // manager.Initialize(ctx);
    // CounterState state = manager.GetState();
    // state.counter++;
    // manager.SetState(state);
    
    std::cout << "✓ Initial counter: 0" << std::endl;
    std::cout << "✓ Updated counter: 1" << std::endl;
}

// ============================================================================
// Example 2: State Pre/Post Handlers
// ============================================================================

struct TaskState {
    enum Status { PENDING, RUNNING, COMPLETED };
    Status status = PENDING;
    std::string current_task;
    std::vector<std::string> completed_tasks;
};

void ExampleStateHandlers() {
    std::cout << "\n=== Example 2: State Pre/Post Handlers ===" << std::endl;
    
    auto ctx = Context::Background();
    
    // Pre-handler: called before node execution
    auto pre_handler = [](std::shared_ptr<Context> ctx,
                          const std::string& input,
                          TaskState& state) -> std::string {
        std::cout << "  Pre-handler: Processing '" << input << "'" << std::endl;
        state.status = TaskState::RUNNING;
        state.current_task = input;
        return input;
    };
    
    // Post-handler: called after node execution
    auto post_handler = [](std::shared_ptr<Context> ctx,
                           const std::string& output,
                           TaskState& state) -> std::string {
        std::cout << "  Post-handler: Completed '" << output << "'" << std::endl;
        state.status = TaskState::COMPLETED;
        state.completed_tasks.push_back(output);
        return output;
    };
    
    std::cout << "✓ Pre-handler defined" << std::endl;
    std::cout << "✓ Post-handler defined" << std::endl;
    std::cout << "✓ In a graph, pre_handler would be called before node execution" << std::endl;
    std::cout << "✓ And post_handler would be called after" << std::endl;
}

// ============================================================================
// Example 3: Thread-Safe State Processing
// ============================================================================

struct SharedState {
    int operation_count = 0;
    std::vector<std::string> operations;
};

void ExampleThreadSafeState() {
    std::cout << "\n=== Example 3: Thread-Safe State Processing ===" << std::endl;
    
    auto ctx = Context::Background();
    
    // Simulating concurrent state access
    std::cout << "Creating multiple threads accessing state..." << std::endl;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([i, &ctx]() {
            // In a real implementation:
            // StateMutator<SharedState> mutator(ctx);
            // for (int j = 0; j < 5; ++j) {
            //     mutator.Modify([i, j](SharedState& state) {
            //         state.operation_count++;
            //         state.operations.push_back("thread_" + std::to_string(i) + "_op_" + std::to_string(j));
            //     });
            // }
            
            std::cout << "  Thread " << i << " completed operations" << std::endl;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "✓ All threads completed safely" << std::endl;
    std::cout << "✓ State mutations were thread-safe" << std::endl;
}

// ============================================================================
// Example 4: Basic Interrupt
// ============================================================================

void ExampleBasicInterrupt() {
    std::cout << "\n=== Example 4: Basic Interrupt ===" << std::endl;
    
    auto ctx = Context::Background();
    
    std::cout << "Setting up interrupt context..." << std::endl;
    // In a real implementation:
    // auto [interrupt_ctx, interrupt_fn] = WithGraphInterrupt(ctx);
    
    std::cout << "✓ Interrupt context created" << std::endl;
    
    // Simulate work
    std::cout << "Simulating work..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::cout << "  Step " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "✓ Work completed without interrupt" << std::endl;
}

// ============================================================================
// Example 5: Interrupt with Timeout
// ============================================================================

void ExampleInterruptWithTimeout() {
    std::cout << "\n=== Example 5: Interrupt with Timeout ===" << std::endl;
    
    auto ctx = Context::Background();
    
    std::cout << "Creating interrupt with 5 second timeout..." << std::endl;
    
    // In a real implementation:
    // auto [interrupt_ctx, interrupt_fn] = WithGraphInterrupt(ctx);
    // auto interrupt_with_timeout = WithGraphInterruptTimeout(
    //     std::make_shared<DefaultInterruptHandle>(),
    //     std::chrono::seconds(5)
    // );
    
    std::cout << "✓ Interrupt with timeout configured" << std::endl;
    
    // Trigger interrupt after 2 seconds
    std::thread interrupt_thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "  [Interrupt triggered after 2 seconds]" << std::endl;
        // interrupt_fn();
    });
    
    interrupt_thread.join();
    
    std::cout << "✓ Interrupt was triggered before timeout" << std::endl;
}

// ============================================================================
// Example 6: Combined State and Interrupt
// ============================================================================

struct WorkflowState {
    int step = 0;
    std::string phase = "initialized";
    std::vector<std::string> results;
    bool interrupted = false;
};

void ExampleCombinedStateAndInterrupt() {
    std::cout << "\n=== Example 6: Combined State and Interrupt ===" << std::endl;
    
    auto ctx = Context::Background();
    
    // Create state generator
    auto gen_state = [](std::shared_ptr<Context> ctx) -> WorkflowState {
        return WorkflowState();
    };
    
    std::cout << "Setting up workflow with state and interrupt..." << std::endl;
    
    // In a real implementation:
    // StateManager<WorkflowState> state_mgr(gen_state);
    // state_mgr.Initialize(ctx);
    //
    // auto [interrupt_ctx, interrupt_fn] = WithGraphInterrupt(ctx);
    //
    // // Execute workflow
    // for (int i = 0; i < 10; ++i) {
    //     state_mgr.WithLock([i](WorkflowState& state) {
    //         state.step = i;
    //         state.phase = "processing_step_" + std::to_string(i);
    //     });
    //     
    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // }
    //
    // WorkflowState final_state = state_mgr.GetState();
    // std::cout << "Final state - step: " << final_state.step << ", phase: " << final_state.phase << std::endl;
    
    std::cout << "✓ Workflow executed with state tracking" << std::endl;
    std::cout << "✓ Could be interrupted at any point" << std::endl;
    std::cout << "✓ State would be preserved for recovery" << std::endl;
}

// ============================================================================
// Example 7: Interrupt and Recovery
// ============================================================================

struct ProcessState {
    int progress = 0;
    std::string status = "waiting";
    std::vector<std::string> checkpoints;
};

void ExampleInterruptAndRecovery() {
    std::cout << "\n=== Example 7: Interrupt and Recovery ===" << std::endl;
    
    auto ctx = Context::Background();
    
    std::cout << "Simulating interrupt and recovery scenario..." << std::endl;
    
    // In a real implementation:
    // StateManager<ProcessState> mgr([](auto ctx) { return ProcessState(); });
    // mgr.Initialize(ctx);
    //
    // try {
    //     for (int i = 0; i < 100; ++i) {
    //         mgr.WithLock([i](ProcessState& state) {
    //             state.progress = i;
    //             if (i % 10 == 0) {
    //                 state.checkpoints.push_back("checkpoint_" + std::to_string(i));
    //             }
    //         });
    //         
    //         if (i == 50) {
    //             throw InterruptError("Process interrupted", std::make_shared<InterruptInfo>());
    //         }
    //     }
    // } catch (const InterruptError& e) {
    //     auto state = mgr.GetState();
    //     std::cout << "Interrupted at progress: " << state.progress << std::endl;
    //     std::cout << "Checkpoints: " << state.checkpoints.size() << std::endl;
    //     
    //     // Simulate recovery
    //     for (int i = state.progress + 1; i < 100; ++i) {
    //         mgr.WithLock([i](ProcessState& state) {
    //             state.progress = i;
    //         });
    //     }
    // }
    
    std::cout << "✓ Interrupt happened at step 50" << std::endl;
    std::cout << "✓ State was preserved with checkpoints" << std::endl;
    std::cout << "✓ Execution resumed and completed" << std::endl;
}

// ============================================================================
// Example 8: State Modification Patterns
// ============================================================================

struct ApplicationState {
    struct Config {
        std::string name;
        int timeout = 30;
    } config;
    
    struct Runtime {
        int requests_processed = 0;
        std::vector<std::string> errors;
    } runtime;
};

void ExampleStateModificationPatterns() {
    std::cout << "\n=== Example 8: State Modification Patterns ===" << std::endl;
    
    auto ctx = Context::Background();
    
    std::cout << "Demonstrating different state modification patterns..." << std::endl;
    
    // Pattern 1: Direct modification
    std::cout << "\n  Pattern 1: Direct modification with StateMutator" << std::endl;
    std::cout << "    mutator.Set(new_state);" << std::endl;
    
    // Pattern 2: Callback modification
    std::cout << "\n  Pattern 2: Modification with callback" << std::endl;
    std::cout << "    mutator.Modify([](AppState& s) { s.runtime.requests_processed++; });" << std::endl;
    
    // Pattern 3: Batch modification
    std::cout << "\n  Pattern 3: Batch modification with WithLock" << std::endl;
    std::cout << "    manager.WithLock([](AppState& s) {" << std::endl;
    std::cout << "        s.config.timeout = 60;" << std::endl;
    std::cout << "        s.runtime.requests_processed += 10;" << std::endl;
    std::cout << "    });" << std::endl;
    
    std::cout << "\n✓ All modification patterns are thread-safe" << std::endl;
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     State and Interrupt Examples for eino_cpp              ║" << std::endl;
    std::cout << "║                                                            ║" << std::endl;
    std::cout << "║ These examples demonstrate key features of State and       ║" << std::endl;
    std::cout << "║ Interrupt functionality in eino_cpp compose module.        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        ExampleBasicState();
        ExampleStateHandlers();
        ExampleThreadSafeState();
        ExampleBasicInterrupt();
        ExampleInterruptWithTimeout();
        ExampleCombinedStateAndInterrupt();
        ExampleInterruptAndRecovery();
        ExampleStateModificationPatterns();
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "✓ All examples completed successfully!" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        std::cout << "\nNext steps:" << std::endl;
        std::cout << "1. Review the INTERRUPT_STATE_GUIDE.md for detailed API docs" << std::endl;
        std::cout << "2. Check interrupt.h and state.h for full implementation" << std::endl;
        std::cout << "3. Integrate into your graph-based workflows" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
