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

#ifndef EINO_CPP_SCHEMA_STREAM_COPY_H_
#define EINO_CPP_SCHEMA_STREAM_COPY_H_

#include "stream.h"
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

namespace eino {
namespace schema {

// ============================================================================
// StreamReader Copy Support - Aligns with eino schema.StreamReader.Copy()
// ============================================================================

// ParentStreamReader manages multiple child StreamReaders that share data
// from a single source stream.
// Aligns with eino schema.parentStreamReader (stream.go:634-674)
template<typename T>
class ParentStreamReader {
public:
    explicit ParentStreamReader(std::shared_ptr<StreamReader<T>> source, int num_copies)
        : source_(source), num_copies_(num_copies), closed_count_(0) {
        // Initialize substream positions - all start at head
        // Each substream has its own read position in the linked list
        auto head = std::make_shared<StreamElement>();
        for (int i = 0; i < num_copies; i++) {
            positions_.push_back(head);
        }
    }

    // Peek the next value for the child at given index
    // Each child maintains its own position in the shared stream
    // Aligns with eino schema.parentStreamReader.peek (stream.go:656-674)
    bool Peek(int index, T& value) {
        if (index < 0 || index >= num_copies_) {
            return false;
        }

        auto& elem = positions_[index];
        if (!elem) {
            // Child has been closed
            return false;
        }

        // Load the element's data if not already loaded
        std::call_once(elem->load_flag, [this, elem]() {
            T val;
            bool success = source_->Recv(val);
            
            elem->has_value = success;
            if (success) {
                elem->value = val;
                // Create next element for the linked list
                elem->next = std::make_shared<StreamElement>();
            }
        });

        // Read the cached value
        if (!elem->has_value) {
            return false;  // EOF or error
        }

        value = elem->value;
        
        // Move to next element for this child
        positions_[index] = elem->next;
        
        return true;
    }

    // Close a child stream
    // When all children are closed, close the source stream
    // Aligns with eino schema.parentStreamReader.close (stream.go:676-686)
    void CloseChild(int index) {
        if (index < 0 || index >= num_copies_) {
            return;
        }

        if (!positions_[index]) {
            return;  // Already closed
        }

        positions_[index] = nullptr;

        int closed = ++closed_count_;
        if (closed == num_copies_) {
            // All children closed, close source
            if (source_) {
                source_->Close();
            }
        }
    }

private:
    // StreamElement represents a node in the linked list of stream values
    // Each element is loaded once and shared by all children
    struct StreamElement {
        std::once_flag load_flag;  // Ensures data is loaded only once
        T value;
        bool has_value = false;
        std::shared_ptr<StreamElement> next;
    };

    std::shared_ptr<StreamReader<T>> source_;
    int num_copies_;
    std::atomic<int> closed_count_;
    
    // Each child's current position in the linked list
    std::vector<std::shared_ptr<StreamElement>> positions_;
};

// ChildStreamReader is a child of a ParentStreamReader
// Aligns with eino schema.childStreamReader (stream.go:688-700)
template<typename T>
class ChildStreamReader : public StreamReader<T> {
public:
    ChildStreamReader(std::shared_ptr<ParentStreamReader<T>> parent, int index)
        : parent_(parent), index_(index) {}

    bool Recv(T& value) override {
        if (!parent_) {
            return false;
        }
        return parent_->Peek(index_, value);
    }

    void Close() override {
        if (parent_) {
            parent_->CloseChild(index_);
            parent_ = nullptr;
        }
    }

private:
    std::shared_ptr<ParentStreamReader<T>> parent_;
    int index_;
};

// CopyStreamReader creates multiple independent copies of a StreamReader
// Each copy can be read independently
// Aligns with eino schema.copyStreamReaders (stream.go:618-632)
template<typename T>
std::vector<std::shared_ptr<StreamReader<T>>> CopyStreamReader(
    std::shared_ptr<StreamReader<T>> source, int n) {
    
    if (n < 2) {
        // No copy needed, return original
        return {source};
    }

    auto parent = std::make_shared<ParentStreamReader<T>>(source, n);
    
    std::vector<std::shared_ptr<StreamReader<T>>> copies;
    for (int i = 0; i < n; i++) {
        copies.push_back(
            std::make_shared<ChildStreamReader<T>>(parent, i)
        );
    }

    return copies;
}

// ArrayStreamReader with copy support
// Aligns with eino schema.arrayReader.copy (stream.go:455-465)
template<typename T>
class CopyableArrayStreamReader : public ArrayStreamReader<T> {
public:
    explicit CopyableArrayStreamReader(const std::vector<T>& items, size_t start_index = 0)
        : ArrayStreamReader<T>(items), current_index_(start_index) {
        this->index_ = start_index;
    }

    // Create n copies of this array reader
    std::vector<std::shared_ptr<StreamReader<T>>> Copy(int n) {
        std::vector<std::shared_ptr<StreamReader<T>>> copies;
        for (int i = 0; i < n; i++) {
            copies.push_back(
                std::make_shared<CopyableArrayStreamReader<T>>(
                    this->items_, current_index_
                )
            );
        }
        return copies;
    }

private:
    size_t current_index_;
};

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_STREAM_COPY_H_
