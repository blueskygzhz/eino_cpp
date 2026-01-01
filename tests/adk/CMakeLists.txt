# Copyright 2025 CloudWeGo Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# ADK unit tests

# TaskTool tests
add_executable(task_tool_test task_tool_test.cpp)
target_link_libraries(task_tool_test
    PRIVATE eino_adk
    PRIVATE GTest::gtest
    PRIVATE GTest::gtest_main
)
gtest_discover_tests(task_tool_test)

# Supervisor tests
add_executable(supervisor_test supervisor_test.cpp)
target_link_libraries(supervisor_test
    PRIVATE eino_adk
    PRIVATE eino_adk_prebuilt
    PRIVATE GTest::gtest
    PRIVATE GTest::gtest_main
)
gtest_discover_tests(supervisor_test)

# ReAct tests
add_executable(react_test react_test.cpp)
target_link_libraries(react_test
    PRIVATE eino_adk
    PRIVATE GTest::gtest
    PRIVATE GTest::gtest_main
)
gtest_discover_tests(react_test)
