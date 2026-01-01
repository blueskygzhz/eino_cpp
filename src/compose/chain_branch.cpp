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

#include "eino/compose/chain_branch.h"

namespace eino {
namespace compose {

// ChainBranch template instantiation for common types
// This ensures that the template methods are compiled for these types

template class ChainBranch<std::string>;
template class ChainBranch<int>;
template class ChainBranch<double>;
template class ChainBranch<bool>;

// Explicit instantiation of helper functions for common types
template std::shared_ptr<ChainBranch<std::string>> NewChainBranch<std::string>();
template std::shared_ptr<ChainBranch<std::string>> NewChainMultiBranch<std::string>();

template std::shared_ptr<ChainBranch<int>> NewChainBranch<int>();
template std::shared_ptr<ChainBranch<int>> NewChainMultiBranch<int>();

template std::shared_ptr<ChainBranch<double>> NewChainBranch<double>();
template std::shared_ptr<ChainBranch<double>> NewChainMultiBranch<double>();

template std::shared_ptr<ChainBranch<bool>> NewChainBranch<bool>();
template std::shared_ptr<ChainBranch<bool>> NewChainMultiBranch<bool>();

} // namespace compose
} // namespace eino
