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

# Prebuilt agents implementation

set(ADK_PREBUILT_SOURCES
    deep.cpp
    plan_execute.cpp
    supervisor.cpp
)

add_library(eino_adk_prebuilt STATIC ${ADK_PREBUILT_SOURCES})

target_link_libraries(eino_adk_prebuilt
    PUBLIC eino_adk
    PUBLIC nlohmann_json::nlohmann_json
)

target_include_directories(eino_adk_prebuilt
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
