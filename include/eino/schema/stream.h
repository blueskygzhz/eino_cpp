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

#ifndef EINO_CPP_SCHEMA_STREAM_H_
#define EINO_CPP_SCHEMA_STREAM_H_

#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <stdexcept>

namespace eino {
namespace schema {

// StreamItem holds a value and optional error
// Aligns with: eino/schema/stream.go:streamItem
template<typename T>
struct StreamItem {
    T chunk;
    std::string error;  // empty string means no error
    
    StreamItem() = default;
    StreamItem(const T& c, const std::string& e = "") : chunk(c), error(e) {}
};

// StreamWriter is the sender of a stream
// Aligns with: eino/schema/stream.go:StreamWriter
template<typename T>
class StreamWriter {
public:
    explicit StreamWriter(int capacity = 10) 
        : capacity_(capacity), closed_(false) {}
    
    ~StreamWriter() = default;
    
    // Send sends a value to the stream
    // Returns true if stream is closed, false if sent successfully
    // Aligns with: eino/schema/stream.go:StreamWriter.Send
    bool Send(const T& chunk, const std::string& error = "") {
        // Check if closed first (non-blocking)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (closed_) return true;
        }
        
        StreamItem<T> item{chunk, error};
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait if buffer is full
            while (items_.size() >= static_cast<size_t>(capacity_) && !closed_) {
                full_cv_.wait(lock);
            }
            
            // Check closed again after waiting
            if (closed_) return true;
            
            items_.push(item);
        }
        
        empty_cv_.notify_one();
        return false;  // Successfully sent
    }
    
    // Close notifies the receiver that the stream sender has finished
    // Aligns with: eino/schema/stream.go:StreamWriter.Close
    void Close() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (closed_) return;
            closed_ = true;
        }
        empty_cv_.notify_all();
        full_cv_.notify_all();
    }
    
    bool IsClosed() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return closed_;
    }

private:
    template<typename U> friend class StreamReader;
    template<typename U> friend class SimpleStreamReader;
    
    std::queue<StreamItem<T>> items_;
    int capacity_;
    mutable std::mutex mutex_;
    std::condition_variable empty_cv_;
    std::condition_variable full_cv_;
    bool closed_;
};

// StreamReader is the receiver of a stream
// Aligns with: eino/schema/stream.go:StreamReader
template<typename T>
class StreamReader {
public:
    StreamReader() = default;
    virtual ~StreamReader() = default;
    
    // Recv receives a value from the stream
    // Returns true if value was received (check error for error status)
    // Returns false if stream is closed/exhausted (EOF)
    // Aligns with: eino/schema/stream.go:StreamReader.Recv
    virtual bool Recv(T& value, std::string& error) = 0;
    
    // Convenience method without error parameter
    bool Recv(T& value) {
        std::string error;
        return Recv(value, error);
    }
    
    // RecvAll receives all remaining values from stream
    virtual std::vector<T> RecvAll() {
        std::vector<T> result;
        T value;
        std::string error;
        while (Recv(value, error)) {
            if (error.empty()) {
                result.push_back(value);
            }
        }
        return result;
    }
    
    // Close safely closes the StreamReader
    // Aligns with: eino/schema/stream.go:StreamReader.Close
    virtual void Close() = 0;
    
    // Copy creates n independent copies of this StreamReader
    // The original StreamReader becomes unusable after Copy
    // Aligns with: eino/schema/stream.go:StreamReader.Copy
    virtual std::vector<std::shared_ptr<StreamReader<T>>> Copy(int n) {
        // Default implementation returns single reader
        // Subclasses should override for true multi-consumer support
        if (n < 2) {
            return {std::shared_ptr<StreamReader<T>>(this, [](StreamReader<T>*){})};
        }
        throw std::runtime_error("Copy not implemented for this stream type");
    }
    
    // SetAutomaticClose sets the StreamReader to automatically close
    // when it's no longer reachable and ready to be GCed
    // Aligns with: eino/schema/stream.go:StreamReader.SetAutomaticClose
    virtual void SetAutomaticClose() {
        // Default implementation: use shared_ptr deleter for automatic cleanup
        // Subclasses can override for custom behavior
    }
};

// SimpleStreamReader wraps a StreamWriter and provides reading capability
// Aligns with: eino/schema/stream.go:stream (as reader)
template<typename T>
class SimpleStreamReader : public StreamReader<T> {
public:
    explicit SimpleStreamReader(std::shared_ptr<StreamWriter<T>> writer)
        : writer_(writer) {}
    
    bool Recv(T& value, std::string& error) override {
        if (!writer_) {
            error = "EOF";
            return false;
        }
        
        StreamItem<T> item;
        {
            std::unique_lock<std::mutex> lock(writer_->mutex_);
            
            // Wait for data or close
            while (writer_->items_.empty() && !writer_->closed_) {
                writer_->empty_cv_.wait(lock);
            }
            
            // Stream is closed and empty - EOF
            if (writer_->items_.empty()) {
                error = "EOF";
                return false;
            }
            
            item = writer_->items_.front();
            writer_->items_.pop();
        }
        
        writer_->full_cv_.notify_one();
        
        value = item.chunk;
        error = item.error;
        
        // Return false only on EOF, true even if error present
        return error != "EOF";
    }
    
    void Close() override {
        if (writer_) {
            writer_->Close();
            writer_ = nullptr;
        }
    }

private:
    std::shared_ptr<StreamWriter<T>> writer_;
};

// ArrayStreamReader provides streaming interface for arrays
// Aligns with: eino/schema/stream.go:arrayReader
template<typename T>
class ArrayStreamReader : public StreamReader<T> {
public:
    explicit ArrayStreamReader(const std::vector<T>& items)
        : items_(items), index_(0) {}
    
    bool Recv(T& value, std::string& error) override {
        if (index_ >= items_.size()) {
            error = "EOF";
            return false;
        }
        value = items_[index_++];
        error = "";
        return true;
    }
    
    void Close() override {
        // No cleanup needed for array
    }
    
    // Copy creates n independent copies
    // Aligns with: eino/schema/stream.go:arrayReader.copy
    std::vector<std::shared_ptr<StreamReader<T>>> Copy(int n) override {
        if (n < 2) {
            return {std::make_shared<ArrayStreamReader<T>>(items_)};
        }
        
        std::vector<std::shared_ptr<StreamReader<T>>> result;
        for (int i = 0; i < n; ++i) {
            auto reader = std::make_shared<ArrayStreamReader<T>>(items_);
            reader->index_ = this->index_;  // Copy current position
            result.push_back(reader);
        }
        return result;
    }

private:
    std::vector<T> items_;
    size_t index_;
};

// Create a pipe with capacity
// Aligns with: eino/schema/stream.go:Pipe
template<typename T>
std::pair<std::shared_ptr<StreamReader<T>>, std::shared_ptr<StreamWriter<T>>> 
Pipe(int capacity = 10) {
    auto writer = std::make_shared<StreamWriter<T>>(capacity);
    auto reader = std::make_shared<SimpleStreamReader<T>>(writer);
    return {reader, writer};
}

// Create a stream reader from array
// Aligns with: eino/schema/stream.go:StreamReaderFromArray
template<typename T>
std::shared_ptr<StreamReader<T>> StreamReaderFromArray(const std::vector<T>& items) {
    return std::make_shared<ArrayStreamReader<T>>(items);
}

// Convert stream using a conversion function
// Aligns with: eino/schema/stream.go:streamReaderWithConvert
template<typename T, typename U>
class ConvertStreamReader : public StreamReader<U> {
public:
    ConvertStreamReader(std::shared_ptr<StreamReader<T>> reader,
                       std::function<bool(const T&, U&, std::string&)> converter)
        : reader_(reader), converter_(converter) {}
    
    bool Recv(U& value, std::string& error) override {
        T src_value;
        std::string src_error;
        
        while (reader_->Recv(src_value, src_error)) {
            if (!src_error.empty()) {
                error = src_error;
                return true;  // Error propagates
            }
            
            // Try to convert
            if (converter_(src_value, value, error)) {
                return true;  // Successfully converted
            }
            
            // If error is "no_value", skip this item and continue
            if (error != "no_value") {
                return true;  // Actual error, propagate it
            }
            
            // Continue to next item (no_value means skip)
        }
        
        error = "EOF";
        return false;
    }
    
    void Close() override {
        if (reader_) {
            reader_->Close();
        }
    }

private:
    std::shared_ptr<StreamReader<T>> reader_;
    std::function<bool(const T&, U&, std::string&)> converter_;
};

// Create a converted stream reader
// Aligns with: eino/schema/stream.go:StreamReaderWithConvert
template<typename T, typename U>
std::shared_ptr<StreamReader<U>> StreamReaderWithConvert(
    std::shared_ptr<StreamReader<T>> reader,
    std::function<bool(const T&, U&, std::string&)> converter) {
    return std::make_shared<ConvertStreamReader<T, U>>(reader, converter);
}

// Convenience overload with simpler converter (no error handling)
template<typename T, typename U>
std::shared_ptr<StreamReader<U>> StreamReaderWithConvert(
    std::shared_ptr<StreamReader<T>> reader,
    std::function<bool(const T&, U&)> converter) {
    auto wrapped_converter = [converter](const T& src, U& dst, std::string& error) -> bool {
        return converter(src, dst);
    };
    return std::make_shared<ConvertStreamReader<T, U>>(reader, wrapped_converter);
}

// Merge multiple stream readers
// Aligns with: eino/schema/stream.go:multiStreamReader
template<typename T>
class MergeStreamReader : public StreamReader<T> {
public:
    explicit MergeStreamReader(const std::vector<std::shared_ptr<StreamReader<T>>>& readers)
        : readers_(readers), current_reader_(0) {}
    
    bool Recv(T& value, std::string& error) override {
        while (current_reader_ < readers_.size()) {
            if (readers_[current_reader_]->Recv(value, error)) {
                return true;
            }
            // Current reader exhausted, move to next
            current_reader_++;
        }
        
        error = "EOF";
        return false;
    }
    
    void Close() override {
        for (auto& reader : readers_) {
            if (reader) {
                reader->Close();
            }
        }
    }

private:
    std::vector<std::shared_ptr<StreamReader<T>>> readers_;
    size_t current_reader_;
};

// Merge multiple streams
// Aligns with: eino/schema/stream.go:MergeStreamReaders
template<typename T>
std::shared_ptr<StreamReader<T>> MergeStreamReaders(
    const std::vector<std::shared_ptr<StreamReader<T>>>& readers) {
    if (readers.empty()) {
        return std::make_shared<ArrayStreamReader<T>>(std::vector<T>());
    }
    if (readers.size() == 1) {
        return readers[0];
    }
    return std::make_shared<MergeStreamReader<T>>(readers);
}

// Named stream merge reader
// Aligns with: eino/schema/stream.go:multiStreamReader with sourceReaderNames
template<typename T>
class NamedMergeStreamReader : public StreamReader<T> {
public:
    explicit NamedMergeStreamReader(
        const std::map<std::string, std::shared_ptr<StreamReader<T>>>& named_readers)
        : current_index_(0) {
        for (const auto& pair : named_readers) {
            readers_.push_back(pair.second);
            names_.push_back(pair.first);
        }
    }
    
    bool Recv(T& value, std::string& error) override {
        while (current_index_ < readers_.size()) {
            std::string inner_error;
            if (readers_[current_index_]->Recv(value, inner_error)) {
                error = inner_error;
                return true;
            }
            
            // This stream ended, report source EOF
            error = "source_eof:" + names_[current_index_];
            current_index_++;
            return false;  // Signal this source ended
        }
        
        error = "EOF";
        return false;
    }
    
    void Close() override {
        for (auto& reader : readers_) {
            if (reader) {
                reader->Close();
            }
        }
    }

private:
    std::vector<std::shared_ptr<StreamReader<T>>> readers_;
    std::vector<std::string> names_;
    size_t current_index_;
};

// Merge named streams
// Aligns with: eino/schema/stream.go:MergeNamedStreamReaders
template<typename T>
std::shared_ptr<StreamReader<T>> MergeNamedStreamReaders(
    const std::map<std::string, std::shared_ptr<StreamReader<T>>>& named_readers) {
    if (named_readers.empty()) {
        return std::make_shared<ArrayStreamReader<T>>(std::vector<T>());
    }
    if (named_readers.size() == 1) {
        return named_readers.begin()->second;
    }
    return std::make_shared<NamedMergeStreamReader<T>>(named_readers);
}

// Error constants
// Aligns with: eino/schema/stream.go:ErrNoValue, ErrRecvAfterClosed
const std::string kErrNoValue = "no_value";
const std::string kErrRecvAfterClosed = "recv_after_closed";
const std::string kErrEOF = "EOF";

// Helper to check if error is source EOF
// Aligns with: eino/schema/stream.go:GetSourceName
inline bool GetSourceName(const std::string& error, std::string& source_name) {
    const std::string prefix = "source_eof:";
    if (error.substr(0, prefix.length()) == prefix) {
        source_name = error.substr(prefix.length());
        return true;
    }
    return false;
}

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_STREAM_H_
