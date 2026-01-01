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

#ifndef EINO_CPP_ADK_AGENT_BASE_H_
#define EINO_CPP_ADK_AGENT_BASE_H_

// ADK Agent Execution Helpers
// ============================
// NOTE: In Go version of eino, there is NO AgentBase abstract class.
// Agents directly implement the Agent interface and internally use Compose.
// 
// This file provides utility functions for Agent implementations,
// NOT a base class to inherit from.
//
// Architecture principle:
// - Agent: High-level interface for end users
// - Compose: Low-level execution engine (Graph/Chain/Runnable)
// - Integration: Agents dynamically build Compose structures at execution time
//   using lazy build pattern (buildRunFunc), NOT by inheriting from a base class

#include "agent.h"
#include "../compose/runnable.h"
#include "../compose/state.h"
#include "async_iterator.h"
#include <memory>
#include <string>
#include <thread>

namespace eino {
namespace adk {
namespace internal {

/**
 * @brief Utility class for Agent execution helpers
 * 
 * This is NOT a base class to inherit from.
 * It provides static helper functions for common Agent implementation tasks.
 * 
 * NOTE: This aligns with Go version where there is no AgentBase inheritance.
 */
class AgentExecutionHelper {
public:
    /**
     * @brief Convert Compose StreamReader to AsyncIterator of AgentEvents
     * 
     * Useful when an Agent internally uses Compose Runnable and needs to
     * convert its output stream to the Agent's AsyncIterator interface.
     * 
     * @tparam OutputType The output type of the Compose Runnable
     * @param stream The stream from Compose execution
     * @param converter Function to convert OutputType to AgentEvent
     * @return AsyncIterator of AgentEvents
     */
    template<typename OutputType>
    static std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>>
    ConvertStreamToIterator(
        std::shared_ptr<compose::StreamReader<OutputType>> stream,
        std::function<std::shared_ptr<AgentEvent>(const OutputType&)> converter) {
        
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        
        // Process stream in background thread
        std::thread([stream, converter, generator = pair.second]() {
            try {
                OutputType output;
                while (stream->Read(output)) {
                    auto event = converter(output);
                    if (generator && event) {
                        generator->Send(event);
                    }
                }
            } catch (const std::exception& e) {
                auto error_event = std::make_shared<AgentEvent>();
                error_event->error_msg = e.what();
                if (generator) {
                    generator->Send(error_event);
                }
            }
            
            if (generator) {
                generator->Close();
            }
        }).detach();
        
        return pair.first;
    }

    /**
     * @brief Create an iterator that immediately returns an error
     * 
     * @param error_msg Error message to include in the event
     * @return AsyncIterator containing a single error event
     */
    static std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>>
    CreateErrorIterator(const std::string& error_msg) {
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        
        auto error_event = std::make_shared<AgentEvent>();
        error_event->error_msg = error_msg;
        
        if (pair.second) {
            pair.second->Send(error_event);
            pair.second->Close();
        }
        
        return pair.first;
    }

    /**
     * @brief Get Compose Context from generic void* context
     * 
     * Helper to convert void* ctx (used in Agent interface) to
     * Compose Context (used in Compose framework).
     * 
     * @param ctx Generic context pointer
     * @return Compose context
     */
    static std::shared_ptr<compose::Context> GetComposeContext(void* ctx) {
        if (ctx == nullptr) {
            return compose::Context::Background();
        }
        // Implement proper context conversion based on your context design
        return std::static_pointer_cast<compose::Context>(
            *reinterpret_cast<std::shared_ptr<compose::Context>*>(ctx));
    }
};

}  // namespace internal
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_AGENT_BASE_H_
