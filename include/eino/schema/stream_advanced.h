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

#ifndef EINO_CPP_SCHEMA_STREAM_ADVANCED_H_
#define EINO_CPP_SCHEMA_STREAM_ADVANCED_H_

#include <memory>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <string>

#include "eino/schema/stream.h"

namespace eino {
namespace schema {

// ============================================================================
// StreamWriter and Pipe
// ============================================================================

// StreamWriter is the sender side of a bidirectional stream
// Created by Pipe() function
template<typename T>
class StreamWriter {
public:
    // Send a value to the stream
    // Returns true if stream is closed (receiver stopped)
    bool Send(const T& chunk, const std::string& error = "") {
        return stream_->Send(chunk, error);
    }
    
    // Close the stream (signal EOF to receiver)
    void Close() {
        stream_->CloseSend();
    }

private:
    friend std::pair<std::shared_ptr<StreamReader<T>>, std::shared_ptr<StreamWriter<T>>>
        Pipe(size_t capacity);
    
    explicit StreamWriter(std::shared_ptr<Stream<T>> stream) 
        : stream_(std::move(stream)) {}
    
    std::shared_ptr<Stream<T>> stream_;
};

// Pipe creates a new bidirectional stream with given capacity
// Returns (reader, writer) pair
//
// Example:
//   auto [reader, writer] = Pipe<std::string>(10);
//   
//   // In producer thread:
//   for (auto item : items) {
//       if (writer->Send(item)) break;  // Closed
//   }
//   writer->Close();
//   
//   // In consumer thread:
//   T item;
//   while (reader->Recv(item)) {
//       process(item);
//   }
template<typename T>
std::pair<std::shared_ptr<StreamReader<T>>, std::shared_ptr<StreamWriter<T>>>
Pipe(size_t capacity = 0) {
    auto stream = std::make_shared<Stream<T>>(capacity);
    auto reader = std::make_shared<StreamReader<T>>(stream);
    auto writer = std::make_shared<StreamWriter<T>>(stream);
    return {reader, writer};
}

// ============================================================================
// StreamReaderWithConvert
// ============================================================================

// ConvertedStreamReader wraps a stream and converts each item
template<typename TOut, typename TIn>
class ConvertedStreamReader : public IStreamReader {
public:
    ConvertedStreamReader(
        std::shared_ptr<StreamReader<TIn>> source,
        std::function<bool(const TIn&, TOut&, std::string&)> converter)
        : source_(std::move(source))
        , converter_(std::move(converter)) {}
    
    bool Recv(TOut& item, std::string& error) {
        while (true) {
            TIn source_item;
            std::string source_error;
            
            if (!source_->Recv(source_item, source_error)) {
                error = source_error;
                return false;
            }
            
            if (!converter_(source_item, item, error)) {
                if (error.empty()) {
                    // ErrNoValue - skip this item
                    continue;
                }
                return false;
            }
            
            return true;
        }
    }
    
    void Close() override {
        source_->Close();
    }

private:
    std::shared_ptr<StreamReader<TIn>> source_;
    std::function<bool(const TIn&, TOut&, std::string&)> converter_;
};

// StreamReaderWithConvert creates a new stream that converts each item
//
// Example:
//   auto int_reader = StreamReaderFromArray<int>({1, 2, 3});
//   auto str_reader = StreamReaderWithConvert<std::string, int>(
//       int_reader,
//       [](const int& i, std::string& out, std::string& err) {
//           out = std::to_string(i);
//           return true;
//       }
//   );
template<typename TOut, typename TIn>
std::shared_ptr<StreamReader<TOut>> StreamReaderWithConvert(
    std::shared_ptr<StreamReader<TIn>> source,
    std::function<bool(const TIn&, TOut&, std::string&)> converter) {
    
    auto converted = std::make_shared<ConvertedStreamReader<TOut, TIn>>(
        std::move(source), std::move(converter));
    
    // Wrap in StreamReader interface
    auto stream = std::make_shared<Stream<TOut>>(0);
    
    // Start conversion thread
    std::thread([converted, stream]() {
        TOut item;
        std::string error;
        while (converted->Recv(item, error)) {
            if (stream->Send(item, "")) {
                break;  // Receiver closed
            }
        }
        stream->CloseSend();
        converted->Close();
    }).detach();
    
    return std::make_shared<StreamReader<TOut>>(stream);
}

// ============================================================================
// MergeStreamReaders
// ============================================================================

// MergedStreamReader combines multiple streams into one
// Items are interleaved based on availability
template<typename T>
class MergedStreamReader {
public:
    explicit MergedStreamReader(std::vector<std::shared_ptr<StreamReader<T>>> sources)
        : sources_(std::move(sources))
        , active_count_(sources_.size()) {}
    
    bool Recv(T& item, std::string& error) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        while (active_count_ > 0) {
            // Try each active source in round-robin
            for (size_t i = 0; i < sources_.size(); ++i) {
                if (sources_[i] == nullptr) continue;
                
                std::string source_error;
                if (sources_[i]->Recv(item, source_error)) {
                    return true;
                }
                
                // This source is done
                sources_[i]->Close();
                sources_[i] = nullptr;
                --active_count_;
            }
        }
        
        error = "EOF";
        return false;
    }
    
    void Close() {
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto& source : sources_) {
            if (source != nullptr) {
                source->Close();
                source = nullptr;
            }
        }
        active_count_ = 0;
    }

private:
    std::vector<std::shared_ptr<StreamReader<T>>> sources_;
    size_t active_count_;
    std::mutex mutex_;
};

// MergeStreamReaders combines multiple StreamReaders into one
// Items from all streams are interleaved
//
// Example:
//   auto s1 = StreamReaderFromArray<int>({1, 2});
//   auto s2 = StreamReaderFromArray<int>({3, 4});
//   auto merged = MergeStreamReaders<int>({s1, s2});
template<typename T>
std::shared_ptr<StreamReader<T>> MergeStreamReaders(
    std::vector<std::shared_ptr<StreamReader<T>>> sources) {
    
    if (sources.empty()) {
        return nullptr;
    }
    
    if (sources.size() == 1) {
        return sources[0];
    }
    
    auto merged = std::make_shared<MergedStreamReader<T>>(std::move(sources));
    auto stream = std::make_shared<Stream<T>>(0);
    
    // Start merge thread
    std::thread([merged, stream]() {
        T item;
        std::string error;
        while (merged->Recv(item, error)) {
            if (stream->Send(item, "")) {
                break;  // Receiver closed
            }
        }
        stream->CloseSend();
        merged->Close();
    }).detach();
    
    return std::make_shared<StreamReader<T>>(stream);
}

// ============================================================================
// MergeNamedStreamReaders
// ============================================================================

// SourceEOF represents EOF from a specific named source
class SourceEOF : public std::exception {
public:
    explicit SourceEOF(const std::string& name) : source_name_(name) {}
    
    const char* what() const noexcept override {
        return ("EOF from source: " + source_name_).c_str();
    }
    
    const std::string& GetSourceName() const { return source_name_; }

private:
    std::string source_name_;
};

// MergeNamedStreamReaders combines multiple named streams
// When a source reaches EOF, returns SourceEOF error with the source name
//
// Example:
//   std::map<std::string, std::shared_ptr<StreamReader<int>>> sources = {
//       {"stream1", s1},
//       {"stream2", s2}
//   };
//   auto merged = MergeNamedStreamReaders<int>(sources);
//   
//   int item;
//   std::string error;
//   while (merged->Recv(item, error)) {
//       process(item);
//   }
//   // Check if error is SourceEOF to know which stream ended
template<typename T>
std::shared_ptr<StreamReader<T>> MergeNamedStreamReaders(
    const std::map<std::string, std::shared_ptr<StreamReader<T>>>& sources) {
    
    if (sources.empty()) {
        return nullptr;
    }
    
    // Convert map to vectors
    std::vector<std::shared_ptr<StreamReader<T>>> readers;
    std::vector<std::string> names;
    
    for (const auto& [name, reader] : sources) {
        names.push_back(name);
        readers.push_back(reader);
    }
    
    auto stream = std::make_shared<Stream<T>>(0);
    
    // Start merge thread
    std::thread([readers = std::move(readers), names = std::move(names), stream]() {
        std::vector<bool> active(readers.size(), true);
        size_t active_count = readers.size();
        
        while (active_count > 0) {
            for (size_t i = 0; i < readers.size(); ++i) {
                if (!active[i]) continue;
                
                T item;
                std::string error;
                if (readers[i]->Recv(item, error)) {
                    if (stream->Send(item, "")) {
                        goto cleanup;  // Receiver closed
                    }
                } else {
                    // This source ended - send SourceEOF error
                    stream->Send(T{}, "SourceEOF:" + names[i]);
                    active[i] = false;
                    --active_count;
                }
            }
        }
        
    cleanup:
        stream->CloseSend();
        for (auto& reader : readers) {
            reader->Close();
        }
    }).detach();
    
    return std::make_shared<StreamReader<T>>(stream);
}

// GetSourceName extracts source name from SourceEOF error string
inline bool GetSourceName(const std::string& error, std::string& source_name) {
    const std::string prefix = "SourceEOF:";
    if (error.compare(0, prefix.length(), prefix) == 0) {
        source_name = error.substr(prefix.length());
        return true;
    }
    return false;
}

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_STREAM_ADVANCED_H_
