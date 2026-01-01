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

// Stream Alignment Test - Validates eino_cpp aligns with eino Go implementation
// Aligns with: eino/schema/stream_test.go

#include "eino/schema/stream.h"
#include <cassert>
#include <thread>
#include <iostream>

using namespace eino::schema;

// Test 1: Basic Pipe Send/Recv
// Aligns with: eino/schema/stream_test.go TestStreamReader
void test_basic_pipe() {
    std::cout << "Test 1: Basic Pipe Send/Recv..." << std::endl;
    
    auto [reader, writer] = Pipe<int>(3);
    
    // Send data in background thread
    std::thread sender([writer]() {
        for (int i = 0; i < 5; ++i) {
            bool closed = writer->Send(i);
            assert(!closed && "Writer should not be closed");
        }
        writer->Close();
    });
    
    // Receive data
    int value;
    std::string error;
    int count = 0;
    
    while (reader->Recv(value, error)) {
        assert(error.empty() && "Should not have error");
        assert(value == count && "Value should match");
        count++;
    }
    
    assert(error == "EOF" && "Should get EOF");
    assert(count == 5 && "Should receive 5 items");
    
    sender.join();
    reader->Close();
    
    std::cout << "✅ PASS" << std::endl;
}

// Test 2: Stream with errors
// Aligns with: eino/schema/stream_test.go TestStreamReaderWithError
void test_stream_with_errors() {
    std::cout << "Test 2: Stream with errors..." << std::endl;
    
    auto [reader, writer] = Pipe<int>(3);
    
    std::thread sender([writer]() {
        writer->Send(1);
        writer->Send(2, "test error");
        writer->Send(3);
        writer->Close();
    });
    
    int value;
    std::string error;
    
    // First value - no error
    assert(reader->Recv(value, error));
    assert(value == 1 && error.empty());
    
    // Second value - has error
    assert(reader->Recv(value, error));
    assert(value == 2 && error == "test error");
    
    // Third value - no error
    assert(reader->Recv(value, error));
    assert(value == 3 && error.empty());
    
    // EOF
    assert(!reader->Recv(value, error));
    assert(error == "EOF");
    
    sender.join();
    reader->Close();
    
    std::cout << "✅ PASS" << std::endl;
}

// Test 3: StreamReaderFromArray
// Aligns with: eino/schema/stream_test.go TestStreamReaderFromArray
void test_array_reader() {
    std::cout << "Test 3: StreamReaderFromArray..." << std::endl;
    
    std::vector<int> arr = {1, 2, 3, 4, 5};
    auto reader = StreamReaderFromArray(arr);
    
    int value;
    std::string error;
    int count = 1;
    
    while (reader->Recv(value, error)) {
        assert(error.empty());
        assert(value == count);
        count++;
    }
    
    assert(error == "EOF");
    assert(count == 6);
    
    reader->Close();
    
    std::cout << "✅ PASS" << std::endl;
}

// Test 4: StreamReaderWithConvert
// Aligns with: eino/schema/stream_test.go TestStreamReaderWithConvert
void test_convert_stream() {
    std::cout << "Test 4: StreamReaderWithConvert..." << std::endl;
    
    auto int_reader = StreamReaderFromArray<int>({1, 2, 3, 4, 5});
    
    // Convert int to string
    auto str_reader = StreamReaderWithConvert<int, std::string>(
        int_reader,
        [](const int& val, std::string& out, std::string& error) -> bool {
            out = "val_" + std::to_string(val);
            return true;
        }
    );
    
    std::string value;
    std::string error;
    int count = 1;
    
    while (str_reader->Recv(value, error)) {
        assert(error.empty());
        assert(value == "val_" + std::to_string(count));
        count++;
    }
    
    assert(error == "EOF");
    assert(count == 6);
    
    str_reader->Close();
    
    std::cout << "✅ PASS" << std::endl;
}

// Test 5: StreamReaderWithConvert with filter (ErrNoValue)
// Aligns with: eino/schema/stream_test.go TestStreamReaderWithConvertFilter
void test_convert_with_filter() {
    std::cout << "Test 5: Convert with filter (ErrNoValue)..." << std::endl;
    
    auto int_reader = StreamReaderFromArray<int>({1, 2, 3, 4, 5, 6, 7, 8});
    
    // Only convert even numbers, skip odd numbers
    auto str_reader = StreamReaderWithConvert<int, std::string>(
        int_reader,
        [](const int& val, std::string& out, std::string& error) -> bool {
            if (val % 2 == 0) {
                out = "even_" + std::to_string(val);
                return true;
            }
            // Skip odd numbers using ErrNoValue
            error = kErrNoValue;
            return false;
        }
    );
    
    std::string value;
    std::string error;
    std::vector<std::string> results;
    
    while (str_reader->Recv(value, error)) {
        assert(error.empty());
        results.push_back(value);
    }
    
    assert(error == "EOF");
    assert(results.size() == 4);  // 2, 4, 6, 8
    assert(results[0] == "even_2");
    assert(results[1] == "even_4");
    assert(results[2] == "even_6");
    assert(results[3] == "even_8");
    
    str_reader->Close();
    
    std::cout << "✅ PASS" << std::endl;
}

// Test 6: MergeStreamReaders
// Aligns with: eino/schema/stream_test.go TestMergeStreamReaders
void test_merge_streams() {
    std::cout << "Test 6: MergeStreamReaders..." << std::endl;
    
    auto reader1 = StreamReaderFromArray<int>({1, 2, 3});
    auto reader2 = StreamReaderFromArray<int>({4, 5});
    auto reader3 = StreamReaderFromArray<int>({6, 7, 8});
    
    auto merged = MergeStreamReaders<int>({reader1, reader2, reader3});
    
    int value;
    std::string error;
    std::vector<int> results;
    
    while (merged->Recv(value, error)) {
        assert(error.empty());
        results.push_back(value);
    }
    
    assert(error == "EOF");
    assert(results.size() == 8);
    
    // Verify order (sequential merge)
    for (int i = 0; i < 8; ++i) {
        assert(results[i] == i + 1);
    }
    
    merged->Close();
    
    std::cout << "✅ PASS" << std::endl;
}

// Test 7: MergeNamedStreamReaders with source tracking
// Aligns with: eino/schema/stream_test.go TestMergeNamedStreamReaders
void test_named_merge_streams() {
    std::cout << "Test 7: MergeNamedStreamReaders..." << std::endl;
    
    auto reader1 = StreamReaderFromArray<int>({1, 2});
    auto reader2 = StreamReaderFromArray<int>({3, 4});
    
    std::map<std::string, std::shared_ptr<StreamReader<int>>> named_readers = {
        {"stream1", reader1},
        {"stream2", reader2}
    };
    
    auto merged = MergeNamedStreamReaders<int>(named_readers);
    
    int value;
    std::string error;
    std::vector<int> results;
    
    // Read from first stream
    while (merged->Recv(value, error)) {
        if (error.empty()) {
            results.push_back(value);
        } else {
            // Check if it's a source EOF
            std::string source_name;
            if (GetSourceName(error, source_name)) {
                std::cout << "  Source '" << source_name << "' ended" << std::endl;
                assert(source_name == "stream1");
                break;
            }
        }
    }
    
    // Continue reading from second stream
    while (merged->Recv(value, error)) {
        if (error.empty()) {
            results.push_back(value);
        } else {
            std::string source_name;
            if (GetSourceName(error, source_name)) {
                std::cout << "  Source '" << source_name << "' ended" << std::endl;
                assert(source_name == "stream2");
                break;
            }
        }
    }
    
    assert(results.size() == 4);
    
    merged->Close();
    
    std::cout << "✅ PASS" << std::endl;
}

// Test 8: Array Copy
// Aligns with: eino/schema/stream_test.go TestStreamReaderCopy
void test_array_copy() {
    std::cout << "Test 8: Array Copy..." << std::endl;
    
    auto reader = StreamReaderFromArray<int>({1, 2, 3, 4, 5});
    
    // Create 3 copies
    auto copies = reader->Copy(3);
    assert(copies.size() == 3);
    
    // Each copy should read the same data
    for (int i = 0; i < 3; ++i) {
        int value;
        std::string error;
        int count = 1;
        
        while (copies[i]->Recv(value, error)) {
            assert(error.empty());
            assert(value == count);
            count++;
        }
        
        assert(error == "EOF");
        assert(count == 6);
        copies[i]->Close();
    }
    
    std::cout << "✅ PASS" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Stream Alignment Test Suite" << std::endl;
    std::cout << "Validates eino_cpp aligns with eino Go" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    test_basic_pipe();
    test_stream_with_errors();
    test_array_reader();
    test_convert_stream();
    test_convert_with_filter();
    test_merge_streams();
    test_named_merge_streams();
    test_array_copy();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "✅ All tests passed!" << std::endl;
    std::cout << "Stream implementation aligns with eino" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
