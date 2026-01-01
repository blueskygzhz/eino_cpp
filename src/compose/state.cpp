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

#include "eino/compose/state.h"

namespace eino {
namespace compose {

// Template instantiation for common state types
template class InternalStateContainer<std::string>;
template class InternalStateContainer<int>;
template class InternalStateContainer<double>;
template class InternalStateContainer<bool>;

template class StateGraph<std::string, std::string, std::string>;
template class StateGraph<int, int, int>;
template class StateGraph<std::string, int, std::string>;

template class StateManager<std::string>;
template class StateManager<int>;
template class StateManager<double>;
template class StateManager<bool>;

} // namespace compose
} // namespace eino
