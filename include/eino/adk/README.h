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

#ifndef EINO_CPP_ADK_README_H_
#define EINO_CPP_ADK_README_H_

/*
 * ADK (Application Development Kit) Module Overview
 * ================================================
 *
 * The ADK module provides high-level abstractions for building AI applications
 * with intelligent agents. It's the main interface for end-users building
 * agent-based systems.
 *
 * Core Components:
 * ----------------
 *
 * 1. Agent Interface (agent.h)
 *    - Base Agent class defining the interface for all agents
 *    - ResumableAgent for agents supporting interruption/resumption
 *    - OnSubAgents interface for agent hierarchy management
 *    - AgentMiddleware for customizing agent behavior
 *
 * 2. AsyncIterator (async_iterator.h)
 *    - AsyncGenerator<T>: produces values to be consumed
 *    - AsyncIterator<T>: consumes values produced by generator
 *    - Thread-safe communication pattern for async execution results
 *
 * 3. ChatModelAgent (chat_model_agent.h)
 *    - Main agent implementation using a chat model as the core
 *    - Integrates tools/functions for agent decision-making
 *    - Supports ReAct pattern (Reasoning + Action)
 *    - ChatModelAgentConfig for configuring the agent
 *
 * 4. Flow Management (flow.h)
 *    - FlowAgent wraps agents with flow management
 *    - Manages agent hierarchies and sub-agents
 *    - HistoryRewriter for custom message transformation
 *    - SetSubAgents for setting up agent relationships
 *
 * 5. Runner (runner.h)
 *    - High-level interface for executing agents
 *    - RunnerConfig for configuration
 *    - Supports checkpoint-based resumption
 *    - Methods: Run(), Query(), Resume()
 *
 * 6. Agent as Tool (agent_tool.h)
 *    - Wraps an agent as a tool for use by other agents
 *    - AgentToolOptions for customization
 *    - Enables agent composition and delegation
 *
 * 7. Type Definitions (types.h)
 *    - MessageVariant: union of message or message stream
 *    - AgentInput/AgentOutput/AgentEvent: data structures
 *    - State: internal state for ReAct pattern
 *    - RunContext: execution context with run path and session
 *    - InterruptInfo/ResumeInfo: interruption and resumption data
 *
 * 8. Call Options (call_options.h)
 *    - AgentRunOption: configurable options for agent execution
 *    - WithSessionValues: set session variables
 *    - WithSkipTransferMessages: skip transfer message logging
 *    - WithCheckPointID: specify checkpoint ID
 *
 * 9. Context Management (context.h)
 *    - RunSession: manages session data during execution
 *    - ContextManager: singleton for context operations
 *    - Session value storage and retrieval
 *
 * 10. Checkpoint/Persistence (checkpoint.h)
 *     - CheckPointStore interface for persistence
 *     - InMemoryCheckPointStore: simple in-memory implementation
 *     - CheckPointData: serialization helper
 *
 * Key Features:
 * ============
 *
 * 1. Agent Execution:
 *    - Run agents with input messages
 *    - Stream results via AsyncIterator
 *    - Support for streaming and non-streaming modes
 *
 * 2. Multi-Agent Systems:
 *    - Set sub-agents and parent relationships
 *    - Transfer control between agents
 *    - Hierarchical agent organization
 *
 * 3. Tool Integration:
 *    - Agents can use tools/functions for decision-making
 *    - Tools configured via ToolsConfig
 *    - Return-immediately tools for early termination
 *
 * 4. Interruption & Resumption:
 *    - Save execution state at interruption points
 *    - Resume from saved state with CheckPointStore
 *    - Preserve session context across resumptions
 *
 * 5. Customization:
 *    - Middleware hooks: before_chat_model, after_chat_model
 *    - Custom instruction and tool sets
 *    - History rewriting for custom message transformation
 *
 * 6. Session Management:
 *    - Store and retrieve arbitrary data during execution
 *    - Session values accessible throughout the execution
 *    - Supports f-string placeholders in instructions
 *
 * Usage Example:
 * ==============
 *
 *    // Create agent configuration
 *    auto config = std::make_shared<ChatModelAgentConfig>();
 *    config->name = "MyAgent";
 *    config->description = "My first agent";
 *    config->model = my_chat_model;  // ToolCallingChatModel
 *
 *    // Create agent
 *    auto agent = NewChatModelAgent(ctx, config);
 *
 *    // Create runner
 *    RunnerConfig runner_config;
 *    runner_config.agent = agent;
 *    auto runner = NewRunner(ctx, runner_config);
 *
 *    // Execute with input
 *    auto input = std::make_shared<AgentInput>();
 *    input->messages.push_back(schema::UserMessage("Hello!"));
 *    
 *    auto iterator = runner->Run(ctx, input->messages, {});
 *    
 *    auto result = std::make_shared<AgentEvent>();
 *    while (iterator->Next(result)) {
 *        // Process result
 *    }
 *
 * Integration Points:
 * ===================
 *
 * - schema module: Message, StreamReader, RoleType, ToolInfo
 * - components module: ChatModel, Tool, ToolCallingChatModel
 * - compose module: Graph, Chain, Runnable, ToolsNode
 * - callbacks module: Handler, RunInfo
 *
 * Architecture Note:
 * ==================
 *
 * The ADK layer is built on top of the compose layer (Graph, Chain) and
 * components layer (ChatModel, Tool). It provides a simpler, more specialized
 * interface for building agent applications while leveraging the core
 * composition and component infrastructure.
 */

#endif  // EINO_CPP_ADK_README_H_
