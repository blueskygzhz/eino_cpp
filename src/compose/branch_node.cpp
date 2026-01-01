/*
 * Copyright 2025 CloudWeGo Authors
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

#include "../../include/eino/compose/branch_node.h"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace eino {
namespace compose {

// ============================================================================
// Clause::AlignNumberTypes - Type alignment for numeric comparisons
// ============================================================================
// Aligns with: alignNumberTypes (clause.go:289-300)

void Clause::AlignNumberTypes(std::any& left, std::any& right) {
    // Check if left is int64 and right is double (float64)
    if (left.type() == typeid(int64_t) && right.type() == typeid(double)) {
        int64_t left_val = std::any_cast<int64_t>(left);
        left = static_cast<double>(left_val);
    }
    // Check if left is double and right is int64
    else if (left.type() == typeid(double) && right.type() == typeid(int64_t)) {
        int64_t right_val = std::any_cast<int64_t>(right);
        right = static_cast<double>(right_val);
    }
}

// ============================================================================
// Clause Resolve Methods
// ============================================================================

bool Clause::ResolveEqual() {
    // Aligns with: clause.go:47-54
    if (!left_operand.has_value() && !right_operand.has_value()) {
        return true;
    }
    
    if (!left_operand.has_value() || !right_operand.has_value()) {
        return false;
    }
    
    std::any left = left_operand;
    std::any right = right_operand;
    AlignNumberTypes(left, right);
    
    // Compare by type
    if (left.type() == typeid(int64_t)) {
        return std::any_cast<int64_t>(left) == std::any_cast<int64_t>(right);
    } else if (left.type() == typeid(double)) {
        return std::any_cast<double>(left) == std::any_cast<double>(right);
    } else if (left.type() == typeid(bool)) {
        return std::any_cast<bool>(left) == std::any_cast<bool>(right);
    } else if (left.type() == typeid(std::string)) {
        return std::any_cast<std::string>(left) == std::any_cast<std::string>(right);
    }
    
    return false;
}

bool Clause::ResolveNotEqual() {
    // Aligns with: clause.go:55-65
    return !ResolveEqual();
}

bool Clause::ResolveEmpty() {
    // Aligns with: clause.go:66-93
    if (!left_operand.has_value()) {
        return true;
    }
    
    // Check array/vector
    if (left_operand.type() == typeid(std::vector<std::any>)) {
        auto vec = std::any_cast<std::vector<std::any>>(left_operand);
        return vec.empty();
    }
    
    // Check map
    if (left_operand.type() == typeid(std::map<std::string, std::any>)) {
        auto map = std::any_cast<std::map<std::string, std::any>>(left_operand);
        return map.empty();
    }
    
    // Check string
    if (left_operand.type() == typeid(std::string)) {
        auto str = std::any_cast<std::string>(left_operand);
        return str.empty() || str == "None";
    }
    
    // Check int64
    if (left_operand.type() == typeid(int64_t)) {
        return std::any_cast<int64_t>(left_operand) == 0;
    }
    
    // Check double
    if (left_operand.type() == typeid(double)) {
        return std::any_cast<double>(left_operand) == 0.0;
    }
    
    // Check bool
    if (left_operand.type() == typeid(bool)) {
        return !std::any_cast<bool>(left_operand);
    }
    
    return false;
}

bool Clause::ResolveNotEmpty() {
    // Aligns with: clause.go:94-96
    return !ResolveEmpty();
}

bool Clause::ResolveComparison() {
    // Aligns with: clause.go:97-163
    if (!left_operand.has_value()) {
        if (op == BranchOperator::GreaterOrEqual || op == BranchOperator::LesserOrEqual) {
            return !right_operand.has_value();
        }
        return op == BranchOperator::Lesser;
    }
    
    if (!right_operand.has_value()) {
        if (op == BranchOperator::GreaterOrEqual) {
            return true;
        }
        return false;
    }
    
    std::any left = left_operand;
    std::any right = right_operand;
    AlignNumberTypes(left, right);
    
    // Perform comparison based on type
    if (left.type() == typeid(double)) {
        double left_val = std::any_cast<double>(left);
        double right_val = std::any_cast<double>(right);
        
        switch (op) {
            case BranchOperator::Greater:
                return left_val > right_val;
            case BranchOperator::GreaterOrEqual:
                return left_val >= right_val;
            case BranchOperator::Lesser:
                return left_val < right_val;
            case BranchOperator::LesserOrEqual:
                return left_val <= right_val;
            default:
                return false;
        }
    } else if (left.type() == typeid(int64_t)) {
        int64_t left_val = std::any_cast<int64_t>(left);
        int64_t right_val = std::any_cast<int64_t>(right);
        
        switch (op) {
            case BranchOperator::Greater:
                return left_val > right_val;
            case BranchOperator::GreaterOrEqual:
                return left_val >= right_val;
            case BranchOperator::Lesser:
                return left_val < right_val;
            case BranchOperator::LesserOrEqual:
                return left_val <= right_val;
            default:
                return false;
        }
    }
    
    return false;
}

bool Clause::ResolveBoolean() {
    // Aligns with: clause.go:164-174
    if (!left_operand.has_value()) {
        return op == BranchOperator::IsFalse;
    }
    
    if (left_operand.type() != typeid(bool)) {
        return false;
    }
    
    bool val = std::any_cast<bool>(left_operand);
    
    if (op == BranchOperator::IsTrue) {
        return val;
    } else {  // IsFalse
        return !val;
    }
}

bool Clause::ResolveLength() {
    // Aligns with: clause.go:175-217
    if (!left_operand.has_value()) {
        int64_t threshold = std::any_cast<int64_t>(right_operand);
        switch (op) {
            case BranchOperator::LengthGreaterOrEqual:
            case BranchOperator::LengthLesserOrEqual:
                return threshold == 0;
            case BranchOperator::LengthGreater:
                return false;
            case BranchOperator::LengthLesser:
                return threshold > 0;
            default:
                return false;
        }
    }
    
    int64_t length = 0;
    
    // Get length based on type
    if (left_operand.type() == typeid(std::string)) {
        length = std::any_cast<std::string>(left_operand).length();
    } else if (left_operand.type() == typeid(std::vector<std::any>)) {
        length = std::any_cast<std::vector<std::any>>(left_operand).size();
    } else {
        return false;
    }
    
    int64_t threshold = std::any_cast<int64_t>(right_operand);
    
    switch (op) {
        case BranchOperator::LengthGreater:
            return length > threshold;
        case BranchOperator::LengthGreaterOrEqual:
            return length >= threshold;
        case BranchOperator::LengthLesser:
            return length < threshold;
        case BranchOperator::LengthLesserOrEqual:
            return length <= threshold;
        default:
            return false;
    }
}

bool Clause::ResolveContain() {
    // Aligns with: clause.go:218-254
    if (!left_operand.has_value()) {
        return false;
    }
    
    bool should_contain = (op == BranchOperator::Contain);
    
    // String contains
    if (left_operand.type() == typeid(std::string)) {
        std::string left_str = std::any_cast<std::string>(left_operand);
        std::string right_str = std::any_cast<std::string>(right_operand);
        bool contains = left_str.find(right_str) != std::string::npos;
        return should_contain ? contains : !contains;
    }
    
    // Array contains
    if (left_operand.type() == typeid(std::vector<std::any>)) {
        auto vec = std::any_cast<std::vector<std::any>>(left_operand);
        
        // Simple equality check (for basic types)
        for (const auto& elem : vec) {
            // This is simplified - proper implementation needs deep comparison
            if (elem.type() == right_operand.type()) {
                if (elem.type() == typeid(int64_t)) {
                    if (std::any_cast<int64_t>(elem) == std::any_cast<int64_t>(right_operand)) {
                        return should_contain;
                    }
                } else if (elem.type() == typeid(std::string)) {
                    if (std::any_cast<std::string>(elem) == std::any_cast<std::string>(right_operand)) {
                        return should_contain;
                    }
                }
            }
        }
        
        return !should_contain;
    }
    
    return false;
}

bool Clause::ResolveContainKey() {
    // Aligns with: clause.go:255-286
    if (!left_operand.has_value()) {
        return false;
    }
    
    bool should_contain = (op == BranchOperator::ContainKey);
    
    if (left_operand.type() == typeid(std::map<std::string, std::any>)) {
        auto map = std::any_cast<std::map<std::string, std::any>>(left_operand);
        std::string key = std::any_cast<std::string>(right_operand);
        
        bool contains = map.find(key) != map.end();
        return should_contain ? contains : !contains;
    }
    
    return false;
}

// ============================================================================
// Clause::Resolve - Main resolution logic
// ============================================================================
// Aligns with: clause.go:35-286

bool Clause::Resolve() {
    switch (op) {
        case BranchOperator::Equal:
            return ResolveEqual();
        
        case BranchOperator::NotEqual:
            return ResolveNotEqual();
        
        case BranchOperator::Empty:
            return ResolveEmpty();
        
        case BranchOperator::NotEmpty:
            return ResolveNotEmpty();
        
        case BranchOperator::Greater:
        case BranchOperator::GreaterOrEqual:
        case BranchOperator::Lesser:
        case BranchOperator::LesserOrEqual:
            return ResolveComparison();
        
        case BranchOperator::IsTrue:
        case BranchOperator::IsFalse:
            return ResolveBoolean();
        
        case BranchOperator::LengthGreater:
        case BranchOperator::LengthGreaterOrEqual:
        case BranchOperator::LengthLesser:
        case BranchOperator::LengthLesserOrEqual:
            return ResolveLength();
        
        case BranchOperator::Contain:
        case BranchOperator::NotContain:
            return ResolveContain();
        
        case BranchOperator::ContainKey:
        case BranchOperator::NotContainKey:
            return ResolveContainKey();
        
        default:
            throw std::runtime_error("Unknown operator: " + OperatorToString(op));
    }
}

// ============================================================================
// BranchNode::ResolveValueSource - Resolve ValueSource to actual value
// ============================================================================

template<typename I, typename O>
bool BranchNode<I, O>::ResolveValueSource(
    const ValueSource& source,
    const std::map<std::string, std::any>& input,
    std::any& out) {
    
    if (source.type == ValueSource::Type::Literal) {
        // Direct literal value
        out = source.literal_value;
        return true;
    } else {
        // Node reference - extract value from input
        if (!source.node_ref) {
            return false;
        }
        
        // Build path: [node_key, ...from_path]
        std::vector<std::string> full_path;
        full_path.push_back(source.node_ref->from_node_key);
        full_path.insert(full_path.end(), 
                        source.node_ref->from_path.begin(), 
                        source.node_ref->from_path.end());
        
        return TakeMapValue(input, full_path, out);
    }
}

// ============================================================================
// BranchNode::ConvertInputLegacy - Convert input (backward compatible)
// ============================================================================
// Aligns with: selector.Selector.selectorInputConverter (selector.go:132-175)

template<typename I, typename O>
std::vector<Operants> BranchNode<I, O>::ConvertInputLegacy(const std::map<std::string, std::any>& input) {
    std::vector<Operants> result;
    result.reserve(config_.clauses.size());
    
    for (size_t i = 0; i < config_.clauses.size(); ++i) {
        const auto& clause_config = config_.clauses[i];
        
        if (clause_config.single) {
            // Single condition
            std::any left, right;
            
            bool has_left = TakeMapValue(input, {std::to_string(i), "left"}, left);
            if (!has_left) {
                throw std::runtime_error(
                    "Failed to take left operand from input map, clause index=" + std::to_string(i));
            }
            
            bool has_right = TakeMapValue(input, {std::to_string(i), "right"}, right);
            
            if (has_right) {
                result.push_back(Operants(left, right));
            } else {
                result.push_back(Operants(left));
            }
            
        } else if (clause_config.multi) {
            // Multi-clause condition
            Operants operants;
            
            for (size_t j = 0; j < clause_config.multi->clauses.size(); ++j) {
                std::any left, right;
                
                bool has_left = TakeMapValue(input, {std::to_string(i), std::to_string(j), "left"}, left);
                if (!has_left) {
                    throw std::runtime_error(
                        "Failed to take left operand from input map, clause index=" + 
                        std::to_string(i) + ", single clause index=" + std::to_string(j));
                }
                
                bool has_right = TakeMapValue(input, {std::to_string(i), std::to_string(j), "right"}, right);
                
                if (has_right) {
                    operants.multi.push_back(Operants(left, right));
                } else {
                    operants.multi.push_back(Operants(left));
                }
            }
            
            result.push_back(operants);
        } else {
            throw std::runtime_error(
                "Invalid clause config, both single and multi are null at index " + std::to_string(i));
        }
    }
    
    return result;
}

// ============================================================================
// BranchNode::ConvertInputWithReferences - Convert input with node references
// ============================================================================

template<typename I, typename O>
std::vector<Operants> BranchNode<I, O>::ConvertInputWithReferences(
    const std::map<std::string, std::any>& input) {
    
    std::vector<Operants> result;
    result.reserve(config_.clauses.size());
    
    for (size_t i = 0; i < config_.clauses.size(); ++i) {
        const auto& clause_config = config_.clauses[i];
        
        if (clause_config.single) {
            // Single condition with node references
            std::any left, right;
            
            // Resolve left operand
            bool has_left = ResolveValueSource(clause_config.single->left.source, input, left);
            if (!has_left) {
                throw std::runtime_error(
                    "Failed to resolve left operand for clause index=" + std::to_string(i));
            }
            
            // Resolve right operand (optional for unary operators)
            bool has_right = false;
            if (clause_config.single->right.source.type != ValueSource::Type::Literal ||
                clause_config.single->right.source.literal_value.has_value()) {
                has_right = ResolveValueSource(clause_config.single->right.source, input, right);
            }
            
            if (has_right) {
                result.push_back(Operants(left, right));
            } else {
                result.push_back(Operants(left));
            }
            
        } else if (clause_config.multi) {
            // Multi-clause condition with node references
            Operants operants;
            
            for (size_t j = 0; j < clause_config.multi->clauses.size(); ++j) {
                const auto& single_clause = clause_config.multi->clauses[j];
                std::any left, right;
                
                // Resolve left operand
                bool has_left = ResolveValueSource(single_clause.left.source, input, left);
                if (!has_left) {
                    throw std::runtime_error(
                        "Failed to resolve left operand for clause index=" + 
                        std::to_string(i) + ", single clause index=" + std::to_string(j));
                }
                
                // Resolve right operand
                bool has_right = false;
                if (single_clause.right.source.type != ValueSource::Type::Literal ||
                    single_clause.right.source.literal_value.has_value()) {
                    has_right = ResolveValueSource(single_clause.right.source, input, right);
                }
                
                if (has_right) {
                    operants.multi.push_back(Operants(left, right));
                } else {
                    operants.multi.push_back(Operants(left));
                }
            }
            
            result.push_back(operants);
        } else {
            throw std::runtime_error(
                "Invalid clause config, both single and multi are null at index " + std::to_string(i));
        }
    }
    
    return result;
}

// ============================================================================
// BranchNode::TakeMapValue - Extract value from nested map
// ============================================================================
// Aligns with: nodes.TakeMapValue

template<typename I, typename O>
bool BranchNode<I, O>::TakeMapValue(
    const std::map<std::string, std::any>& map,
    const std::vector<std::string>& path,
    std::any& out) {
    
    if (path.empty()) {
        return false;
    }
    
    const std::map<std::string, std::any>* current_map = &map;
    
    for (size_t i = 0; i < path.size(); ++i) {
        const std::string& key = path[i];
        
        auto it = current_map->find(key);
        if (it == current_map->end()) {
            return false;
        }
        
        if (i == path.size() - 1) {
            // Last element - return value
            out = it->second;
            return true;
        } else {
            // Intermediate element - must be a map
            if (it->second.type() != typeid(std::map<std::string, std::any>)) {
                return false;
            }
            current_map = &std::any_cast<const std::map<std::string, std::any>&>(it->second);
        }
    }
    
    return false;
}

// ============================================================================
// BranchNode::New - Create new BranchNode
// ============================================================================
// Aligns with: selector.Config.Build (selector.go:38-70)

template<typename I, typename O>
std::shared_ptr<BranchNode<I, O>> BranchNode<I, O>::New(
    std::shared_ptr<Context> ctx,
    const BranchNodeConfig& config) {
    
    if (config.clauses.empty()) {
        throw std::runtime_error("BranchNode: config clauses are empty");
    }
    
    // Detect if any clause uses node references
    bool uses_refs = false;
    
    // Validate clauses and detect reference usage
    for (const auto& clause : config.clauses) {
        if (!clause.single && !clause.multi) {
            throw std::runtime_error("BranchNode: single clause and multi clause are both null");
        }
        
        if (clause.single && clause.multi) {
            throw std::runtime_error("BranchNode: multi clause and single clause are both non-null");
        }
        
        // Check if single clause has node references
        if (clause.single) {
            if (clause.single->left.source.type == ValueSource::Type::Reference ||
                clause.single->right.source.type == ValueSource::Type::Reference) {
                uses_refs = true;
            }
        }
        
        if (clause.multi) {
            if (clause.multi->clauses.empty()) {
                throw std::runtime_error("BranchNode: multi clause's single clauses are empty");
            }
            
            if (clause.multi->relation != ClauseRelation::AND && 
                clause.multi->relation != ClauseRelation::OR) {
                throw std::runtime_error("BranchNode: multi clause relation must be AND or OR");
            }
            
            // Check if multi clause has node references
            for (const auto& single_clause : clause.multi->clauses) {
                if (single_clause.left.source.type == ValueSource::Type::Reference ||
                    single_clause.right.source.type == ValueSource::Type::Reference) {
                    uses_refs = true;
                }
            }
        }
    }
    
    auto node = std::shared_ptr<BranchNode>(new BranchNode());
    node->config_ = config;
    node->uses_node_references_ = uses_refs;
    
    return node;
}

// ============================================================================
// BranchNode::Invoke - Evaluate conditions and return branch index
// ============================================================================
// Aligns with: selector.Selector.Invoke (selector.go:91-130)

template<typename I, typename O>
O BranchNode<I, O>::Invoke(std::shared_ptr<Context> ctx, const I& input, 
                           const std::vector<Option>& opts) {
    // Convert input to map (must be std::map<std::string, std::any>)
    if constexpr (!std::is_same_v<I, std::map<std::string, std::any>>) {
        throw std::runtime_error("BranchNode: Input type must be std::map<std::string, std::any>");
    }
    
    const auto& input_map = *reinterpret_cast<const std::map<std::string, std::any>*>(&input);
    
    // Convert input to operands (choose mode based on config)
    std::vector<Operants> operands;
    if (uses_node_references_) {
        operands = ConvertInputWithReferences(input_map);
    } else {
        operands = ConvertInputLegacy(input_map);
    }
    
    // Build predicates
    std::vector<std::shared_ptr<Predicate>> predicates;
    predicates.reserve(config_.clauses.size());
    
    for (size_t i = 0; i < config_.clauses.size(); ++i) {
        const auto& clause_config = config_.clauses[i];
        const auto& operand = operands[i];
        
        if (clause_config.single) {
            // Single condition
            BranchOperator op;
            
            // Get operator (handle both old and new config styles)
            if (clause_config.single->op != BranchOperator::Equal) {
                // New style: operator is in SingleClauseConfig
                op = clause_config.single->op;
            } else {
                // This shouldn't happen in new code, but handle gracefully
                op = clause_config.single->op;
            }
            
            auto clause = std::make_shared<Clause>(
                operand.left,
                op,
                operand.right
            );
            predicates.push_back(clause);
            
        } else if (clause_config.multi) {
            // Multi-clause condition
            auto multi_clause = std::make_shared<MultiClause>(clause_config.multi->relation);
            
            for (size_t j = 0; j < clause_config.multi->clauses.size(); ++j) {
                const auto& op = clause_config.multi->clauses[j].op;
                const auto& operand_j = operand.multi[j];
                
                auto clause = std::make_shared<Clause>(
                    operand_j.left,
                    op,
                    operand_j.right
                );
                multi_clause->AddClause(clause);
            }
            
            predicates.push_back(multi_clause);
        } else {
            throw std::runtime_error(
                "Invalid clause config, both single and multi are null: " + std::to_string(i));
        }
    }
    
    // Evaluate predicates (short-circuit evaluation)
    for (size_t i = 0; i < predicates.size(); ++i) {
        bool is_true = predicates[i]->Resolve();
        
        if (is_true) {
            // Return matched branch index
            if constexpr (std::is_same_v<O, std::map<std::string, std::any>>) {
                std::map<std::string, std::any> result;
                result["selected"] = static_cast<int64_t>(i);
                return *reinterpret_cast<O*>(&result);
            } else {
                throw std::runtime_error("BranchNode: Output type must be std::map<std::string, std::any>");
            }
        }
    }
    
    // No match - return default branch index (number of clauses)
    if constexpr (std::is_same_v<O, std::map<std::string, std::any>>) {
        std::map<std::string, std::any> result;
        result["selected"] = static_cast<int64_t>(operands.size());
        return *reinterpret_cast<O*>(&result);
    } else {
        throw std::runtime_error("BranchNode: Output type must be std::map<std::string, std::any>");
    }
}

// ============================================================================
// Template Instantiations
// ============================================================================

// Instantiate for common types
template class BranchNode<std::map<std::string, std::any>, std::map<std::string, std::any>>;

} // namespace compose
} // namespace eino
