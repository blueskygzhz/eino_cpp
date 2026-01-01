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

#ifndef EINO_CPP_ADK_CHECKPOINT_H_
#define EINO_CPP_ADK_CHECKPOINT_H_

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <mutex>
#include <map>

namespace eino {
namespace adk {

// CheckPointStore interface for persisting and retrieving checkpoint data
// Aligns with eino compose.CheckPointStore interface
// Used by Graph/Workflow to save and restore execution state for interrupt/resume
class CheckPointStore {
public:
    virtual ~CheckPointStore() = default;

    // Get retrieves checkpoint data by ID
    // Returns (data, exists, error)
    // If checkpoint_id not found, returns (empty_data, false, "")
    // If error occurs, returns (empty_data, false, error_message)
    // Aligns with eino compose.CheckPointStore.Get(ctx, checkPointID)
    virtual std::tuple<std::vector<uint8_t>, bool, std::string> Get(
        void* ctx,
        const std::string& checkpoint_id) = 0;

    // Set stores checkpoint data by ID
    // Returns error message, empty string if successful
    // Aligns with eino compose.CheckPointStore.Set(ctx, checkPointID, checkPoint)
    virtual std::string Set(
        void* ctx,
        const std::string& checkpoint_id,
        const std::vector<uint8_t>& data) = 0;
};

// InMemoryCheckPointStore is a simple in-memory implementation
class InMemoryCheckPointStore : public CheckPointStore {
public:
    InMemoryCheckPointStore() = default;

    std::tuple<std::vector<uint8_t>, bool, std::string> Get(
        void* ctx,
        const std::string& checkpoint_id) override;

    std::string Set(
        void* ctx,
        const std::string& checkpoint_id,
        const std::vector<uint8_t>& data) override;

private:
    std::map<std::string, std::vector<uint8_t>> storage_;
    mutable std::mutex mutex_;
};

// Checkpoint serialization helper
struct CheckPointData {
    std::shared_ptr<RunContext> run_ctx;
    std::shared_ptr<InterruptInfo> interrupt_info;

    std::vector<uint8_t> Serialize() const;
    static std::shared_ptr<CheckPointData> Deserialize(const std::vector<uint8_t>& data);
};

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_CHECKPOINT_H_
