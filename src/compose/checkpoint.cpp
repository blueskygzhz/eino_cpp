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

#include "eino/compose/checkpoint.h"
#include "eino/compose/channel.h"
#include "eino/compose/stream_convert.h"
#include "eino/compose/stream_reader.h"
#include "eino/compose/graph_compile_options.h"
#include "eino/context.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace eino {
namespace compose {

using json = nlohmann::json;

// =============================================================================
// Context Key Definitions
// Aligns with: eino/compose/checkpoint.go
// =============================================================================

struct StateModifierKey {};
struct CheckPointKey {};
struct NodePathKey {};

// =============================================================================
// State Modifier Context Functions
// Aligns with: eino/compose/checkpoint.go:154-162
// =============================================================================

StateModifier GetStateModifier(std::shared_ptr<Context> ctx) {
    if (!ctx) {
        return nullptr;
    }
    
    try {
        return context::GetContextValue<StateModifier>(ctx.get(), StateModifierKey{});
    } catch (...) {
        return nullptr;
    }
}

std::shared_ptr<Context> SetStateModifier(
    std::shared_ptr<Context> ctx,
    StateModifier modifier) {
    
    if (!ctx) {
        return ctx;
    }
    
    return context::SetContextValue(ctx, StateModifierKey{}, modifier);
}

// =============================================================================
// CheckPoint Context Functions
// Aligns with: eino/compose/checkpoint.go:165-196
// =============================================================================

std::shared_ptr<CheckPoint> GetCheckPointFromCtx(std::shared_ptr<Context> ctx) {
    if (!ctx) {
        return nullptr;
    }
    
    try {
        return context::GetContextValue<std::shared_ptr<CheckPoint>>(ctx.get(), CheckPointKey{});
    } catch (...) {
        return nullptr;
    }
}

std::shared_ptr<Context> SetCheckPointToCtx(
    std::shared_ptr<Context> ctx,
    std::shared_ptr<CheckPoint> cp) {
    
    if (!ctx) {
        return ctx;
    }
    
    return context::SetContextValue(ctx, CheckPointKey{}, cp);
}

std::tuple<std::shared_ptr<CheckPoint>, std::string> GetCheckPointFromStore(
    std::shared_ptr<Context> ctx,
    const std::string& id,
    std::shared_ptr<CheckPointer> cpr) {
    
    if (!cpr) {
        return {nullptr, "checkpointer is null"};
    }
    
    auto [cp, existed, err] = cpr->Get(ctx, id);
    if (!err.empty()) {
        return {nullptr, err};
    }
    
    if (!existed) {
        return {nullptr, ""};
    }
    
    return {cp, ""};
}

std::tuple<std::shared_ptr<CheckPoint>, std::string> GetCheckPointFromStore(
    std::shared_ptr<Context> ctx,
    const std::string& id,
    CheckPointStore* store) {
    
    if (!store) {
        return {nullptr, "checkpoint store is null"};
    }
    
    // Create a temporary CheckPointer to use existing logic
    auto cpr = std::make_shared<CheckPointer>(
        std::shared_ptr<CheckPointStore>(store, [](CheckPointStore*){}));
    
    return GetCheckPointFromStore(ctx, id, cpr);
}

std::shared_ptr<Context> ForwardCheckPoint(
    std::shared_ptr<Context> ctx,
    const std::string& node_key) {
    
    auto cp = GetCheckPointFromCtx(ctx);
    if (!cp) {
        return ctx;
    }
    
    auto it = cp->sub_graphs.find(node_key);
    if (it != cp->sub_graphs.end()) {
        auto sub_cp = it->second;
        cp->sub_graphs.erase(it); // only forward once
        return SetCheckPointToCtx(ctx, sub_cp);
    }
    
    return SetCheckPointToCtx(ctx, nullptr);
}

// =============================================================================
// NodePath Context Functions
// Aligns with: eino/compose/checkpoint.go:136-151
// =============================================================================

NodePath GetNodeKey(std::shared_ptr<Context> ctx) {
    if (!ctx) {
        return {};
    }
    
    try {
        return context::GetContextValue<NodePath>(ctx.get(), NodePathKey{});
    } catch (...) {
        return {};
    }
}

std::shared_ptr<Context> SetNodeKey(std::shared_ptr<Context> ctx, const std::string& key) {
    if (!ctx) {
        return ctx;
    }
    
    auto path = GetNodeKey(ctx);
    path.push_back(key);
    return context::SetContextValue(ctx, NodePathKey{}, path);
}

std::shared_ptr<Context> ClearNodeKey(std::shared_ptr<Context> ctx) {
    if (!ctx) {
        return ctx;
    }
    
    return context::SetContextValue(ctx, NodePathKey{}, NodePath{});
}

// =============================================================================
// StreamConverter Implementation
// Aligns with: eino/compose/checkpoint.go:283-338
// =============================================================================

StreamConverter::StreamConverter(
    const std::map<std::string, StreamConvertPair>& input_pairs,
    const std::map<std::string, StreamConvertPair>& output_pairs)
    : input_pairs_(input_pairs)
    , output_pairs_(output_pairs) {
}

// Helper function: convert values using convertPairs
// Aligns with: eino/compose/checkpoint.go:310-326 (convert function)
static std::string Convert(
    std::map<std::string, json>& values,
    const std::map<std::string, StreamConvertPair>& conv_pairs,
    bool is_stream) {
    
    // Aligned with: Go:311-312
    if (!is_stream) {
        return "";
    }
    
    // Aligned with: Go:313-327
    for (auto& [key, value] : values) {
        // 1. Check if this node has a registered convert pair
        // Aligned with: Go:314-316
        auto it = conv_pairs.find(key);
        if (it == conv_pairs.end()) {
            return "checkpoint conv stream fail, node[" + key + "] have not been registered";
        }
        
        const auto& conv_pair = it->second;
        
        // 2. Check if value is a StreamReader (type assertion in Go)
        // Aligned with: Go:317-320
        // sr, ok := v.(streamReader)
        // if !ok {
        //     return fmt.Errorf("checkpoint conv stream fail, value of [%s] isn't stream", key)
        // }
        if (!StreamSerializationHelper::IsStreamReaderJSON(value)) {
            return "checkpoint conv stream fail, value of [" + key + "] isn't stream";
        }
        
        // 3. Deserialize StreamReader from JSON
        auto [sr, err] = StreamSerializationHelper::DeserializeStreamReader(
            value, 
            conv_pair.restore_stream
        );
        if (!err.empty()) {
            return "failed to deserialize stream for node[" + key + "]: " + err;
        }
        
        // 4. Call concat_stream to convert StreamReader → Value
        // Aligned with: Go:321-324
        if (!conv_pair.concat_stream) {
            return "concat_stream function not set for node[" + key + "]";
        }
        
        auto [new_value, concat_err] = conv_pair.concat_stream(sr);
        if (!concat_err.empty()) {
            return "failed to concat stream for node[" + key + "]: " + concat_err;
        }
        
        // 5. Replace with converted value
        values[key] = new_value;
    }
    
    return "";
}

// Helper function: restore values using convertPairs
// Aligns with: eino/compose/checkpoint.go:328-338 (restore function)
static std::string Restore(
    std::map<std::string, json>& values,
    const std::map<std::string, StreamConvertPair>& conv_pairs,
    bool is_stream) {
    
    // Aligned with: Go:329
    if (!is_stream) {
        return "";
    }
    
    // Aligned with: Go:330-339
    for (auto& [key, value] : values) {
        // 1. Check if this node has a registered convert pair
        // Aligned with: Go:331-333
        auto it = conv_pairs.find(key);
        if (it == conv_pairs.end()) {
            return "checkpoint restore stream fail, node[" + key + "] have not been registered";
        }
        
        const auto& conv_pair = it->second;
        
        // 2. Check if restore_stream function is available
        if (!conv_pair.restore_stream) {
            return "restore_stream function not set for node[" + key + "]";
        }
        
        // 3. Call restore_stream to create StreamReader from value
        // Aligned with: Go:334-337
        // sr, err := convPair.restoreStream(v)
        // if err != nil { return err }
        // values[key] = sr
        auto [sr, err] = conv_pair.restore_stream(value);
        if (!err.empty()) {
            return "failed to restore stream for node[" + key + "]: " + err;
        }
        
        // 4. Serialize StreamReader back to JSON with marker
        auto [sr_json, ser_err] = StreamSerializationHelper::SerializeStreamReader(
            sr,
            conv_pair.concat_stream
        );
        if (!ser_err.empty()) {
            return "failed to serialize restored stream for node[" + key + "]: " + ser_err;
        }
        
        // 5. Replace with StreamReader JSON
        values[key] = sr_json;
    }
    
    return "";
}

std::string StreamConverter::ConvertInputs(bool is_stream, std::map<std::string, json>& values) {
    return Convert(values, input_pairs_, is_stream);
}

std::string StreamConverter::RestoreInputs(bool is_stream, std::map<std::string, json>& values) {
    return Restore(values, input_pairs_, is_stream);
}

std::string StreamConverter::ConvertOutputs(bool is_stream, std::map<std::string, json>& values) {
    return Convert(values, output_pairs_, is_stream);
}

std::string StreamConverter::RestoreOutputs(bool is_stream, std::map<std::string, json>& values) {
    return Restore(values, output_pairs_, is_stream);
}

// =============================================================================
// CheckPointer Implementation
// Aligns with: eino/compose/checkpoint.go:198-296
// =============================================================================

CheckPointer::CheckPointer(
    std::shared_ptr<CheckPointStore> store,
    std::shared_ptr<Serializer> serializer)
    : store_(store)
    , serializer_(serializer ? serializer : std::make_shared<JSONSerializer>())
    , sc_(nullptr) {
}

CheckPointer::CheckPointer(
    const std::map<std::string, StreamConvertPair>& input_pairs,
    const std::map<std::string, StreamConvertPair>& output_pairs,
    std::shared_ptr<CheckPointStore> store,
    std::shared_ptr<Serializer> serializer)
    : store_(store)
    , serializer_(serializer ? serializer : std::make_shared<JSONSerializer>())
    , sc_(std::make_shared<StreamConverter>(input_pairs, output_pairs)) {
}

std::tuple<std::shared_ptr<CheckPoint>, bool, std::string> CheckPointer::Get(
    std::shared_ptr<Context> ctx,
    const std::string& id) {
    
    if (!store_) {
        return {nullptr, false, "checkpoint store is null"};
    }
    
    std::vector<uint8_t> data;
    auto [existed, err] = store_->Get(ctx, id, data);
    
    if (!err.empty()) {
        return {nullptr, false, err};
    }
    
    if (!existed) {
        return {nullptr, false, ""};
    }
    
    json j;
    err = serializer_->Unmarshal(data, j);
    if (!err.empty()) {
        return {nullptr, false, err};
    }
    
    auto cp = CheckPoint::FromJSON(j);
    return {cp, true, ""};
}

std::string CheckPointer::Set(
    std::shared_ptr<Context> ctx,
    const std::string& id,
    std::shared_ptr<CheckPoint> cp) {
    
    if (!store_) {
        return "checkpoint store is null";
    }
    
    if (!cp) {
        return "checkpoint is null";
    }
    
    json j = cp->ToJSON();
    
    std::vector<uint8_t> data;
    auto err = serializer_->Marshal(j, data);
    if (!err.empty()) {
        return err;
    }
    
    return store_->Set(ctx, id, data);
}

std::string CheckPointer::ConvertCheckPoint(std::shared_ptr<CheckPoint> cp, bool is_stream) {
    // 对齐 Go: eino/compose/checkpoint.go:246-261
    if (!is_stream) {
        return "";  // 非 stream 模式不需要转换
    }
    
    if (!cp) {
        return "checkpoint is null";
    }
    
    // ✅ FIX 1: Convert channels (严格对齐 Go:247-254)
    // Go: for _, ch := range cp.Channels {
    //         err = ch.convertValues(func(m map[string]any) error {
    //             return c.sc.convertOutputs(isStream, m)
    //         })
    //     }
    // 
    // 关键：ch是Channel对象，调用其convertValues()方法
    for (auto& [ch_name, channel] : cp->channels) {
        if (!channel) {
            continue;
        }
        
        // 调用Channel的ConvertValues方法
        // 该方法会传递一个函数，用于转换channel内部的Values map
        auto convert_fn = [this, is_stream](std::map<std::string, json>& values) -> std::string {
            if (!sc_) {
                return "";
            }
            return sc_->ConvertOutputs(is_stream, values);
        };
        
        auto err = channel->ConvertValues(convert_fn);
        if (!err.empty()) {
            return "failed to convert channel[" + ch_name + "]: " + err;
        }
    }
    
    // ✅ FIX 2: Convert inputs (对齐 Go:256-259)
    // err = c.sc.convertInputs(isStream, cp.Inputs)
    if (sc_) {
        auto err = sc_->ConvertInputs(is_stream, cp->inputs);
        if (!err.empty()) {
            return "failed to convert inputs: " + err;
        }
    }
    
    return "";
}

std::string CheckPointer::RestoreCheckPoint(std::shared_ptr<CheckPoint> cp, bool is_stream) {
    // 对齐 Go: eino/compose/checkpoint.go:265-280
    if (!is_stream) {
        return "";  // 非 stream 模式不需要转换
    }
    
    if (!cp) {
        return "checkpoint is null";
    }
    
    // ✅ FIX 1: Restore channels (严格对齐 Go:266-273)
    // Go: for _, ch := range cp.Channels {
    //         err = ch.convertValues(func(m map[string]any) error {
    //             return c.sc.restoreOutputs(isStream, m)
    //         })
    //     }
    // 
    // 关键：ch是Channel对象，调用其convertValues()方法
    for (auto& [ch_name, channel] : cp->channels) {
        if (!channel) {
            continue;
        }
        
        // 调用Channel的ConvertValues方法
        // 该方法会传递一个函数，用于转换channel内部的Values map
        auto restore_fn = [this, is_stream](std::map<std::string, json>& values) -> std::string {
            if (!sc_) {
                return "";
            }
            return sc_->RestoreOutputs(is_stream, values);
        };
        
        auto err = channel->ConvertValues(restore_fn);
        if (!err.empty()) {
            return "failed to restore channel[" + ch_name + "]: " + err;
        }
    }
    
    // ✅ FIX 2: Restore inputs (对齐 Go:275-278)
    // err = c.sc.restoreInputs(isStream, cp.Inputs)
    if (sc_) {
        auto err = sc_->RestoreInputs(is_stream, cp->inputs);
        if (!err.empty()) {
            return "failed to restore inputs: " + err;
        }
    }
    
    return "";
}

// =============================================================================
// CheckPoint Serialization
// Aligns with: eino/compose/checkpoint.go checkpoint structure
// =============================================================================

json CheckPoint::ToJSON() const {
    json j;
    
    j["channels"] = channels;
    j["inputs"] = inputs;
    j["state"] = state;
    j["skip_pre_handler"] = skip_pre_handler;
    j["rerun_nodes"] = rerun_nodes;
    j["tools_node_executed_tools"] = tools_node_executed_tools;
    
    // Serialize subgraphs
    json sub_graphs_json = json::object();
    for (const auto& [key, cp] : sub_graphs) {
        if (cp) {
            sub_graphs_json[key] = cp->ToJSON();
        }
    }
    j["sub_graphs"] = sub_graphs_json;
    
    return j;
}

std::shared_ptr<CheckPoint> CheckPoint::FromJSON(const json& j) {
    auto cp = std::make_shared<CheckPoint>();
    
    if (j.contains("channels")) {
        cp->channels = j["channels"].get<std::map<std::string, json>>();
    }
    
    if (j.contains("inputs")) {
        cp->inputs = j["inputs"].get<std::map<std::string, json>>();
    }
    
    if (j.contains("state")) {
        cp->state = j["state"];
    }
    
    if (j.contains("skip_pre_handler")) {
        cp->skip_pre_handler = j["skip_pre_handler"].get<std::map<std::string, bool>>();
    }
    
    if (j.contains("rerun_nodes")) {
        cp->rerun_nodes = j["rerun_nodes"].get<std::vector<std::string>>();
    }
    
    if (j.contains("tools_node_executed_tools")) {
        cp->tools_node_executed_tools = j["tools_node_executed_tools"]
            .get<std::map<std::string, std::map<std::string, std::string>>>();
    }
    
    if (j.contains("sub_graphs")) {
        for (const auto& [key, sub_json] : j["sub_graphs"].items()) {
            cp->sub_graphs[key] = FromJSON(sub_json);
        }
    }
    
    return cp;
}

// =============================================================================
// JSONSerializer Implementation
// =============================================================================

std::string JSONSerializer::Marshal(const json& value, std::vector<uint8_t>& data) {
    try {
        std::string str = value.dump();
        data.assign(str.begin(), str.end());
        return "";
    } catch (const std::exception& e) {
        return std::string("JSON marshal error: ") + e.what();
    }
}

std::string JSONSerializer::Unmarshal(const std::vector<uint8_t>& data, json& value) {
    try {
        std::string str(data.begin(), data.end());
        value = json::parse(str);
        return "";
    } catch (const std::exception& e) {
        return std::string("JSON unmarshal error: ") + e.what();
    }
}

// =============================================================================
// GraphCompileOption Functions
// Aligns with: eino/compose/checkpoint.go:59-71
// =============================================================================

GraphCompileOption WithCheckPointStore(std::shared_ptr<CheckPointStore> store) {
    return [store](GraphCompileOptions& opts) {
        if (!store) {
            return;
        }
        // Create a CheckPointer with the store and serializer (if available)
        auto serializer = opts.serializer ? opts.serializer : std::make_shared<JSONSerializer>();
        auto checkpointer = std::make_shared<CheckPointer>(store, serializer);
        opts.checkpointer = checkpointer;
    };
}

GraphCompileOption WithSerializer(std::shared_ptr<Serializer> serializer) {
    return [serializer](GraphCompileOptions& opts) {
        if (!serializer) {
            return;
        }
        // Store serializer for later use by WithCheckPointStore
        opts.serializer = serializer;
        
        // If checkpointer already exists, recreate it with new serializer
        if (opts.checkpointer) {
            // Note: This is a simplified approach
            // In practice, CheckPointer should support dynamic serializer updates
            // For now, WithSerializer should be called before WithCheckPointStore
        }
    };
}

} // namespace compose
} // namespace eino
