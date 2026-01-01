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

#ifndef EINO_CPP_COMPOSE_CHECKPOINT_H_
#define EINO_CPP_COMPOSE_CHECKPOINT_H_

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

#include "eino/context.h"

// Forward declaration for GraphCompileOption
namespace eino {
namespace compose {
class GraphCompileOptions;
using GraphCompileOption = std::function<void(GraphCompileOptions&)>;
}
}

namespace eino {
namespace compose {

using json = nlohmann::json;

/**
 * @brief CheckPointStore interface for persisting and retrieving checkpoints
 * 
 * Aligned with: eino/compose/checkpoint.go CheckPointStore
 */
class CheckPointStore {
public:
    virtual ~CheckPointStore() = default;
    
    /**
     * @brief Get retrieves a checkpoint by ID
     * @param ctx Context
     * @param checkpoint_id Unique identifier for the checkpoint
     * @param data Output parameter for checkpoint data
     * @return tuple<bool, error> - (exists, error)
     */
    virtual std::tuple<bool, std::string> Get(
        std::shared_ptr<Context> ctx,
        const std::string& checkpoint_id,
        std::vector<uint8_t>& data) = 0;
    
    /**
     * @brief Set stores a checkpoint
     * @param ctx Context
     * @param checkpoint_id Unique identifier for the checkpoint
     * @param checkpoint Checkpoint data to store
     * @return error message if any
     */
    virtual std::string Set(
        std::shared_ptr<Context> ctx,
        const std::string& checkpoint_id,
        const std::vector<uint8_t>& checkpoint) = 0;
};

/**
 * @brief Serializer interface for marshaling and unmarshaling checkpoint data
 * 
 * Aligned with: eino/compose/checkpoint.go Serializer
 */
class Serializer {
public:
    virtual ~Serializer() = default;
    
    /**
     * @brief Marshal serializes a value to bytes
     * @param value The value to serialize
     * @param data Output parameter for serialized data
     * @return error message if any
     */
    virtual std::string Marshal(const json& value, std::vector<uint8_t>& data) = 0;
    
    /**
     * @brief Unmarshal deserializes bytes to a value
     * @param data The serialized data
     * @param value Output parameter for deserialized value
     * @return error message if any
     */
    virtual std::string Unmarshal(const std::vector<uint8_t>& data, json& value) = 0;
};

/**
 * @brief Default JSON serializer implementation
 */
class JSONSerializer : public Serializer {
public:
    std::string Marshal(const json& value, std::vector<uint8_t>& data) override;
    std::string Unmarshal(const std::vector<uint8_t>& data, json& value) override;
};

/**
 * @brief StateModifier is a function type for modifying state at specific node paths
 * 
 * Aligned with: eino/compose/checkpoint.go StateModifier
 */
using StateModifier = std::function<std::string(
    std::shared_ptr<Context> ctx,
    const std::vector<std::string>& path,
    json& state)>;

/**
 * @brief WithCheckPointStore sets the checkpoint store for graph compilation
 */
GraphCompileOption WithCheckPointStore(std::shared_ptr<CheckPointStore> store);

/**
 * @brief WithSerializer sets the serializer for checkpoint data
 */
GraphCompileOption WithSerializer(std::shared_ptr<Serializer> serializer);

/**
 * @brief Option for graph/chain invocation with checkpoint support
 */
struct Option {
    std::string checkpoint_id;
    std::string write_to_checkpoint_id;
    bool force_new_run = false;
    StateModifier state_modifier;
    std::vector<std::string> paths;
    std::map<std::string, json> options;
    
    Option() = default;
};

/**
 * @brief WithCheckPointID specifies the checkpoint ID to load from
 */
inline Option WithCheckPointID(const std::string& checkpoint_id) {
    Option opt;
    opt.checkpoint_id = checkpoint_id;
    return opt;
}

/**
 * @brief WithWriteToCheckPointID specifies a different checkpoint ID to write to
 * 
 * This is useful when you want to load from an existing checkpoint
 * but save progress to a new, separate checkpoint
 */
inline Option WithWriteToCheckPointID(const std::string& checkpoint_id) {
    Option opt;
    opt.write_to_checkpoint_id = checkpoint_id;
    return opt;
}

/**
 * @brief WithForceNewRun forces the graph to run from the beginning
 * 
 * Ignores any existing checkpoints
 */
inline Option WithForceNewRun() {
    Option opt;
    opt.force_new_run = true;
    return opt;
}

/**
 * @brief WithStateModifier sets a state modifier for the execution
 */
inline Option WithStateModifier(StateModifier sm) {
    Option opt;
    opt.state_modifier = sm;
    return opt;
}

// Forward declare Channel (from graph_manager.h or dag.h/pregel.h)
class Channel;

/**
 * @brief Internal checkpoint structure
 * 
 * Aligned with: eino/compose/checkpoint.go checkpoint
 */
struct CheckPoint {
    // ⭐ CRITICAL FIX: Channels state: channel_name -> Channel object
    // Aligned with: eino/compose/checkpoint.go:52 Channels map[string]channel
    // 
    // ❌ WRONG (before): std::map<std::string, json> channels;
    // ✅ CORRECT (now):  std::map<std::string, std::shared_ptr<Channel>> channels;
    // 
    // Reason: Must be Channel objects to support convertValues() method
    // which is called in ConvertCheckPoint() and RestoreCheckPoint()
    std::map<std::string, std::shared_ptr<Channel>> channels;
    
    // Inputs for each node: node_key -> input_data
    std::map<std::string, json> inputs;
    
    // Graph state
    json state;
    
    // Skip pre-handler flags: node_key -> should_skip
    std::map<std::string, bool> skip_pre_handler;
    
    // Nodes to rerun
    std::vector<std::string> rerun_nodes;
    
    // Tools node executed tools: tool_node_key -> (tool_call_id -> tool_name)
    std::map<std::string, std::map<std::string, std::string>> tools_node_executed_tools;
    
    // Nested subgraph checkpoints: subgraph_key -> checkpoint
    std::map<std::string, std::shared_ptr<CheckPoint>> sub_graphs;
    
    // Serialize to JSON
    json ToJSON() const;
    
    // Deserialize from JSON
    static std::shared_ptr<CheckPoint> FromJSON(const json& j);
};

// Forward declare StreamConvertPair (defined in generic_helper.h)
struct StreamConvertPair;

/**
 * @brief StreamConverter manages stream conversion for checkpoint
 * 
 * Aligned with: eino/compose/checkpoint.go:283-288 streamConverter
 */
class StreamConverter {
public:
    StreamConverter(
        const std::map<std::string, StreamConvertPair>& input_pairs,
        const std::map<std::string, StreamConvertPair>& output_pairs);
    
    /**
     * @brief Convert inputs from stream to value
     * Aligned with: eino/compose/checkpoint.go:294-296
     */
    std::string ConvertInputs(bool is_stream, std::map<std::string, json>& values);
    
    /**
     * @brief Restore inputs from value to stream
     * Aligned with: eino/compose/checkpoint.go:298-300
     */
    std::string RestoreInputs(bool is_stream, std::map<std::string, json>& values);
    
    /**
     * @brief Convert outputs from stream to value
     * Aligned with: eino/compose/checkpoint.go:302-304
     */
    std::string ConvertOutputs(bool is_stream, std::map<std::string, json>& values);
    
    /**
     * @brief Restore outputs from value to stream
     * Aligned with: eino/compose/checkpoint.go:306-308
     */
    std::string RestoreOutputs(bool is_stream, std::map<std::string, json>& values);
    
private:
    std::map<std::string, StreamConvertPair> input_pairs_;
    std::map<std::string, StreamConvertPair> output_pairs_;
};

/**
 * @brief CheckPointer manages checkpoint operations
 * 
 * Aligned with: eino/compose/checkpoint.go checkPointer
 */
class CheckPointer {
public:
    CheckPointer(
        std::shared_ptr<CheckPointStore> store,
        std::shared_ptr<Serializer> serializer = nullptr);
    
    CheckPointer(
        const std::map<std::string, StreamConvertPair>& input_pairs,
        const std::map<std::string, StreamConvertPair>& output_pairs,
        std::shared_ptr<CheckPointStore> store,
        std::shared_ptr<Serializer> serializer = nullptr);
    
    /**
     * @brief Get retrieves a checkpoint from store
     */
    std::tuple<std::shared_ptr<CheckPoint>, bool, std::string> Get(
        std::shared_ptr<Context> ctx,
        const std::string& id);
    
    /**
     * @brief Set stores a checkpoint
     */
    std::string Set(
        std::shared_ptr<Context> ctx,
        const std::string& id,
        std::shared_ptr<CheckPoint> cp);
    
    /**
     * @brief ConvertCheckPoint converts stream values in checkpoint if needed
     * 
     * Aligned with: eino/compose/checkpoint.go:246-261
     * 
     * In stream mode, converts StreamReader values to concrete values for serialization.
     * This is called before saving checkpoint to ensure data can be properly serialized.
     */
    std::string ConvertCheckPoint(std::shared_ptr<CheckPoint> cp, bool is_stream);
    
    /**
     * @brief RestoreCheckPoint restores stream values in checkpoint if needed
     * 
     * Aligned with: eino/compose/checkpoint.go:265-280
     * 
     * In stream mode, converts concrete values back to StreamReader values.
     * This is called after loading checkpoint to restore the original stream semantics.
     */
    std::string RestoreCheckPoint(std::shared_ptr<CheckPoint> cp, bool is_stream);
    
private:
    std::shared_ptr<CheckPointStore> store_;
    std::shared_ptr<Serializer> serializer_;
    std::shared_ptr<StreamConverter> sc_;
};

/**
 * @brief Context helpers for checkpoint management
 */

// Get checkpoint from context
std::shared_ptr<CheckPoint> GetCheckPointFromCtx(std::shared_ptr<Context> ctx);

// Set checkpoint to context
std::shared_ptr<Context> SetCheckPointToCtx(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<CheckPoint> cp);

// Get checkpoint from store
std::tuple<std::shared_ptr<CheckPoint>, std::string> GetCheckPointFromStore(
    std::shared_ptr<Context> ctx,
    const std::string& id,
    std::shared_ptr<CheckPointer> cpr);

// Get checkpoint from store using CheckPointStore directly
std::tuple<std::shared_ptr<CheckPoint>, std::string> GetCheckPointFromStore(
    std::shared_ptr<Context> ctx,
    const std::string& id,
    CheckPointStore* store);

// Forward checkpoint to subgraph
std::shared_ptr<Context> ForwardCheckPoint(
    std::shared_ptr<Context> ctx,
    const std::string& node_key);

// Node path helpers
using NodePath = std::vector<std::string>;

NodePath GetNodeKey(std::shared_ptr<Context> ctx);
std::shared_ptr<Context> SetNodeKey(std::shared_ptr<Context> ctx, const std::string& key);
std::shared_ptr<Context> ClearNodeKey(std::shared_ptr<Context> ctx);

// State modifier helpers
StateModifier GetStateModifier(std::shared_ptr<Context> ctx);
std::shared_ptr<Context> SetStateModifier(std::shared_ptr<Context> ctx, StateModifier modifier);

/**
 * @brief RegisterSerializableType registers a custom type for serialization
 * 
 * Note: In C++, type registration is handled differently than in Go
 * This is a placeholder for compatibility
 */
template<typename T>
std::string RegisterSerializableType(const std::string& name) {
    // In C++, we rely on JSON serialization library's type system
    // This function exists for API compatibility with Go version
    return "";
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_CHECKPOINT_H_
