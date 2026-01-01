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

#include "eino/callbacks/interface.h"

namespace eino {
namespace callbacks {

// Internal implementation of Handler with callback functions
class FunctionHandler : public Handler {
public:
    FunctionHandler(
        HandlerBuilder::OnStartFn on_start,
        HandlerBuilder::OnEndFn on_end,
        HandlerBuilder::OnErrorFn on_error,
        HandlerBuilder::OnStartWithStreamInputFn on_start_with_stream_input,
        HandlerBuilder::OnEndWithStreamOutputFn on_end_with_stream_output)
        : on_start_(on_start),
          on_end_(on_end),
          on_error_(on_error),
          on_start_with_stream_input_(on_start_with_stream_input),
          on_end_with_stream_output_(on_end_with_stream_output) {}
    
    void OnStart(const RunInfo& info, const CallbackInput& input) override {
        if (on_start_) {
            on_start_(info, input);
        }
    }
    
    void OnEnd(const RunInfo& info, const CallbackOutput& output) override {
        if (on_end_) {
            on_end_(info, output);
        }
    }
    
    void OnError(const RunInfo& info, const std::string& error) override {
        if (on_error_) {
            on_error_(info, error);
        }
    }
    
    void OnStartWithStreamInput(const RunInfo& info, const CallbackInput& input) override {
        if (on_start_with_stream_input_) {
            on_start_with_stream_input_(info, input);
        }
    }
    
    void OnEndWithStreamOutput(const RunInfo& info, const CallbackOutput& output) override {
        if (on_end_with_stream_output_) {
            on_end_with_stream_output_(info, output);
        }
    }
    
private:
    HandlerBuilder::OnStartFn on_start_;
    HandlerBuilder::OnEndFn on_end_;
    HandlerBuilder::OnErrorFn on_error_;
    HandlerBuilder::OnStartWithStreamInputFn on_start_with_stream_input_;
    HandlerBuilder::OnEndWithStreamOutputFn on_end_with_stream_output_;
};

std::shared_ptr<Handler> HandlerBuilder::Build() {
    return std::make_shared<FunctionHandler>(
        on_start_,
        on_end_,
        on_error_,
        on_start_with_stream_input_,
        on_end_with_stream_output_);
}

} // namespace callbacks
} // namespace eino
