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

#ifndef EINO_CPP_COMPOSE_BRANCH_NODE_H_
#define EINO_CPP_COMPOSE_BRANCH_NODE_H_

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <any>
#include <functional>
#include <typeinfo>
#include <stdexcept>
#include "runnable.h"

namespace eino {
namespace compose {

// Forward declarations
template<typename I, typename O>
class BranchNode;

// ============================================================================
// Operator Types
// ============================================================================
// Aligns with: coze-studio/backend/domain/workflow/internal/nodes/selector/operator.go

enum class BranchOperator {
    Equal,                  // "="
    NotEqual,              // "!="
    Empty,                 // "empty"
    NotEmpty,              // "not_empty"
    Greater,               // ">"
    GreaterOrEqual,        // ">="
    Lesser,                // "<"
    LesserOrEqual,         // "<="
    IsTrue,                // "true"
    IsFalse,               // "false"
    LengthGreater,         // "len >"
    LengthGreaterOrEqual,  // "len >="
    LengthLesser,          // "len <"
    LengthLesserOrEqual,   // "len <="
    Contain,               // "contain"
    NotContain,            // "not_contain"
    ContainKey,            // "contain_key"
    NotContainKey,         // "not_contain_key"
};

// Convert operator enum to string (for debugging)
inline std::string OperatorToString(BranchOperator op) {
    switch (op) {
        case BranchOperator::Equal: return "=";
        case BranchOperator::NotEqual: return "!=";
        case BranchOperator::Empty: return "empty";
        case BranchOperator::NotEmpty: return "not_empty";
        case BranchOperator::Greater: return ">";
        case BranchOperator::GreaterOrEqual: return ">=";
        case BranchOperator::Lesser: return "<";
        case BranchOperator::LesserOrEqual: return "<=";
        case BranchOperator::IsTrue: return "true";
        case BranchOperator::IsFalse: return "false";
        case BranchOperator::LengthGreater: return "len >";
        case BranchOperator::LengthGreaterOrEqual: return "len >=";
        case BranchOperator::LengthLesser: return "len <";
        case BranchOperator::LengthLesserOrEqual: return "len <=";
        case BranchOperator::Contain: return "contain";
        case BranchOperator::NotContain: return "not_contain";
        case BranchOperator::ContainKey: return "contain_key";
        case BranchOperator::NotContainKey: return "not_contain_key";
        default: return "unknown";
    }
}

// ============================================================================
// Node Reference Types - 节点输出引用能力
// ============================================================================
// Aligns with: coze-studio/backend/domain/workflow/internal/schema/stream.go

// NodeReference represents a reference to another node's output
// Aligns with: FieldInfo.Source.Ref (stream.go:40-43)
struct NodeReference {
    std::string from_node_key;              // Source node key (e.g., "node_a", "node_b")
    std::vector<std::string> from_path;     // Path within node output (e.g., ["age"], ["result", "score"])
    
    NodeReference() = default;
    NodeReference(const std::string& node_key, const std::vector<std::string>& path = {})
        : from_node_key(node_key), from_path(path) {}
};

// ValueSource represents the source of a value (either a node reference or literal value)
// Aligns with: FieldInfo.Source (combines Ref and Literal)
struct ValueSource {
    enum class Type {
        Literal,    // Static literal value
        Reference   // Reference to node output
    };
    
    Type type;
    std::any literal_value;                       // Used when type == Literal
    std::shared_ptr<NodeReference> node_ref;      // Used when type == Reference
    
    // Create literal value source
    static ValueSource Literal(const std::any& value) {
        ValueSource src;
        src.type = Type::Literal;
        src.literal_value = value;
        return src;
    }
    
    // Create node reference source
    static ValueSource Reference(const std::string& node_key, const std::vector<std::string>& path = {}) {
        ValueSource src;
        src.type = Type::Reference;
        src.node_ref = std::make_shared<NodeReference>(node_key, path);
        return src;
    }
};

// ============================================================================
// Clause Types
// ============================================================================
// Aligns with: coze-studio/backend/domain/workflow/internal/nodes/selector/clause.go

enum class ClauseRelation {
    AND,  // All clauses must be true
    OR    // At least one clause must be true
};

// Operants holds the left and right operands for a condition
// Aligns with: selector.Operants (selector.go:83-87)
struct Operants {
    std::any left;   // Left operand value
    std::any right;  // Right operand value (optional for unary operators)
    
    // For multi-clause support (AND/OR)
    std::vector<Operants> multi;
    
    Operants() = default;
    Operants(const std::any& l, const std::any& r = std::any()) : left(l), right(r) {}
};

// Predicate interface - evaluates to bool
// Aligns with: selector.Predicate (clause.go:27)
class Predicate {
public:
    virtual ~Predicate() = default;
    
    // Resolve evaluates the predicate and returns the result
    virtual bool Resolve() = 0;
};

// Clause represents a single condition (left op right)
// Aligns with: selector.Clause (clause.go:29-33)
class Clause : public Predicate {
public:
    std::any left_operand;
    BranchOperator op;
    std::any right_operand;
    
    Clause(const std::any& left, BranchOperator op_val, const std::any& right = std::any())
        : left_operand(left), op(op_val), right_operand(right) {}
    
    // Resolve implements Predicate interface
    // Aligns with: selector.Clause.Resolve (clause.go:35-286)
    bool Resolve() override;
    
private:
    // Type alignment for numeric comparisons (int64 <-> float64)
    // Aligns with: alignNumberTypes (clause.go:289-300)
    static void AlignNumberTypes(std::any& left, std::any& right);
    
    // Helper methods for specific operator types
    bool ResolveEqual();
    bool ResolveNotEqual();
    bool ResolveEmpty();
    bool ResolveNotEmpty();
    bool ResolveComparison();  // >, >=, <, <=
    bool ResolveBoolean();     // true, false
    bool ResolveLength();      // len >, len >=, len <, len <=
    bool ResolveContain();     // contain, not_contain
    bool ResolveContainKey();  // contain_key, not_contain_key
};

// MultiClause represents multiple conditions combined with AND/OR
// Aligns with: selector.MultiClause (clause.go:35-38)
class MultiClause : public Predicate {
public:
    std::vector<std::shared_ptr<Clause>> clauses;
    ClauseRelation relation;
    
    MultiClause(ClauseRelation rel) : relation(rel) {}
    
    void AddClause(std::shared_ptr<Clause> clause) {
        clauses.push_back(clause);
    }
    
    // Resolve implements Predicate interface
    // Aligns with: selector.MultiClause.Resolve (clause.go:288-312)
    bool Resolve() override {
        if (relation == ClauseRelation::AND) {
            // All clauses must be true
            for (const auto& clause : clauses) {
                if (!clause->Resolve()) {
                    return false;
                }
            }
            return true;
        } else {  // OR
            // At least one clause must be true
            for (const auto& clause : clauses) {
                if (clause->Resolve()) {
                    return true;
                }
            }
            return false;
        }
    }
};

// ============================================================================
// BranchNode Configuration
// ============================================================================

// OperandConfig represents configuration for a single operand (left or right)
struct OperandConfig {
    ValueSource source;
    
    OperandConfig() = default;
    explicit OperandConfig(const ValueSource& src) : source(src) {}
    
    // Helper: Create from literal value
    static OperandConfig FromLiteral(const std::any& value) {
        return OperandConfig(ValueSource::Literal(value));
    }
    
    // Helper: Create from node reference
    static OperandConfig FromNode(const std::string& node_key, const std::vector<std::string>& path = {}) {
        return OperandConfig(ValueSource::Reference(node_key, path));
    }
};

// SingleClauseConfig represents one condition (left op right)
struct SingleClauseConfig {
    BranchOperator op;
    OperandConfig left;
    OperandConfig right;  // Optional for unary operators
    
    SingleClauseConfig() = default;
    SingleClauseConfig(BranchOperator op_val, const OperandConfig& l, const OperandConfig& r = OperandConfig())
        : op(op_val), left(l), right(r) {}
};

// OneClauseConfig represents a single branch condition (single or multi-clause)
// Aligns with: selector.OneClauseSchema (schema.go:23-26)
struct OneClauseConfig {
    // Single condition
    std::shared_ptr<SingleClauseConfig> single;
    
    // Multi-clause condition (AND/OR)
    struct MultiClauseConfig {
        std::vector<SingleClauseConfig> clauses;
        ClauseRelation relation;
    };
    std::shared_ptr<MultiClauseConfig> multi;
    
    OneClauseConfig() = default;
    
    // Create single condition (backward compatible - uses literal values)
    static OneClauseConfig Single(BranchOperator op) {
        OneClauseConfig config;
        config.single = std::make_shared<SingleClauseConfig>();
        config.single->op = op;
        return config;
    }
    
    // Create single condition with full operand configuration
    static OneClauseConfig SingleWithOperands(const SingleClauseConfig& clause) {
        OneClauseConfig config;
        config.single = std::make_shared<SingleClauseConfig>(clause);
        return config;
    }
    
    // Create multi-clause condition (backward compatible)
    static OneClauseConfig Multi(const std::vector<BranchOperator>& ops, ClauseRelation rel) {
        OneClauseConfig config;
        config.multi = std::make_shared<MultiClauseConfig>();
        config.multi->relation = rel;
        for (const auto& op : ops) {
            SingleClauseConfig clause;
            clause.op = op;
            config.multi->clauses.push_back(clause);
        }
        return config;
    }
    
    // Create multi-clause condition with full operand configuration
    static OneClauseConfig MultiWithOperands(const std::vector<SingleClauseConfig>& clauses, ClauseRelation rel) {
        OneClauseConfig config;
        config.multi = std::make_shared<MultiClauseConfig>();
        config.multi->clauses = clauses;
        config.multi->relation = rel;
        return config;
    }
};

// BranchNodeConfig configuration for BranchNode
// Aligns with: selector.Config (selector.go:34-36)
struct BranchNodeConfig {
    std::vector<OneClauseConfig> clauses;
    
    BranchNodeConfig() = default;
    
    // ========== Backward Compatible API (使用literal values) ==========
    
    // Add a single condition branch (backward compatible)
    void AddSingleCondition(BranchOperator op) {
        clauses.push_back(OneClauseConfig::Single(op));
    }
    
    // Add a multi-clause condition branch (backward compatible)
    void AddMultiCondition(const std::vector<BranchOperator>& ops, ClauseRelation rel) {
        clauses.push_back(OneClauseConfig::Multi(ops, rel));
    }
    
    // ========== New API with Node Reference Support ==========
    
    // Add single condition with full operand configuration
    // Example:
    //   config.AddConditionWithOperands(
    //       BranchOperator::GreaterOrEqual,
    //       OperandConfig::FromNode("node_a", {"age"}),      // Reference node A's output.age
    //       OperandConfig::FromLiteral(18)                    // Compare with literal 18
    //   );
    void AddConditionWithOperands(BranchOperator op, 
                                   const OperandConfig& left,
                                   const OperandConfig& right = OperandConfig()) {
        SingleClauseConfig clause(op, left, right);
        clauses.push_back(OneClauseConfig::SingleWithOperands(clause));
    }
    
    // Add multi-clause condition with full operand configuration
    // Example:
    //   config.AddMultiConditionWithOperands({
    //       SingleClauseConfig(BranchOperator::GreaterOrEqual, 
    //                          OperandConfig::FromNode("node_a", {"age"}),
    //                          OperandConfig::FromLiteral(18)),
    //       SingleClauseConfig(BranchOperator::Equal,
    //                          OperandConfig::FromNode("node_b", {"vip"}),
    //                          OperandConfig::FromLiteral(true))
    //   }, ClauseRelation::AND);
    void AddMultiConditionWithOperands(const std::vector<SingleClauseConfig>& clauses_list,
                                        ClauseRelation rel) {
        clauses.push_back(OneClauseConfig::MultiWithOperands(clauses_list, rel));
    }
};

// ============================================================================
// BranchNode - Conditional Branch Node
// ============================================================================
// Aligns with: selector.Selector (selector.go:30-33)
//
// BranchNode evaluates conditions and returns the index of the first matching branch.
// 
// **Two Input Modes:**
// 
// 1. **Legacy Mode (Backward Compatible)**: Input contains literal operand values
//    Input: map<string, any> with paths like "0/left", "0/right", etc.
//    Example:
//      {
//        "0": {"left": 25, "right": 18},  // Condition 0: 25 >= 18
//        "1": {"left": 85, "right": 60}   // Condition 1: 85 > 60
//      }
//
// 2. **Node Reference Mode (NEW)**: Input contains outputs from all referenced nodes
//    Input: map<string, any> with node outputs
//    Example:
//      {
//        "node_a": {"age": 25, "name": "Alice"},       // Output from node A (LLM)
//        "node_b": {"score": 85, "vip": true}          // Output from node B (LLM)
//      }
//    Config specifies how to reference these outputs:
//      config.AddConditionWithOperands(
//          BranchOperator::GreaterOrEqual,
//          OperandConfig::FromNode("node_a", {"age"}),   // node_a.age >= 18
//          OperandConfig::FromLiteral(18)
//      );
//
// Output: map<string, any> with key "selected" containing the branch index (int64)
//
// Branch index meaning:
//   - 0, 1, 2, ..., N-1: Matched branch condition index
//   - N (number of conditions): Default branch (no match)
//
// Usage example (Node Reference Mode):
//   BranchNodeConfig config;
//   config.AddConditionWithOperands(
//       BranchOperator::GreaterOrEqual,
//       OperandConfig::FromNode("node_a", {"age"}),    // Reference node_a's output
//       OperandConfig::FromLiteral(18)
//   );
//   config.AddConditionWithOperands(
//       BranchOperator::Greater,
//       OperandConfig::FromNode("node_b", {"score"}),  // Reference node_b's output
//       OperandConfig::FromLiteral(60)
//   );
//   
//   auto node = BranchNode<map<string,any>, map<string,any>>::New(ctx, config);
//   
//   map<string, any> input = {
//       {"node_a", map<string, any>{{"age", 25}, {"name", "Alice"}}},
//       {"node_b", map<string, any>{{"score", 85}, {"vip", true}}}
//   };
//   
//   auto output = node->Invoke(ctx, input);
//   // output = {"selected": 0}  (first condition matched: age >= 18)

template<typename I, typename O>
class BranchNode : public ComposableRunnable<I, O> {
public:
    // Create new BranchNode with configuration
    // Aligns with: selector.Config.Build (selector.go:38-70)
    static std::shared_ptr<BranchNode> New(
        std::shared_ptr<Context> ctx, 
        const BranchNodeConfig& config);
    
    virtual ~BranchNode() = default;
    
    // Invoke evaluates conditions and returns the matched branch index
    // Aligns with: selector.Selector.Invoke (selector.go:91-130)
    O Invoke(std::shared_ptr<Context> ctx, const I& input, 
             const std::vector<Option>& opts = std::vector<Option>()) override;
    
    // Stream not supported for BranchNode (returns single result)
    std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        throw std::runtime_error("BranchNode: Stream not supported");
    }
    
    // Collect not supported for BranchNode
    O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        throw std::runtime_error("BranchNode: Collect not supported");
    }
    
    // Transform not supported for BranchNode
    std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = std::vector<Option>()) override {
        throw std::runtime_error("BranchNode: Transform not supported");
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(I);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(O);
    }
    
    std::string GetComponentType() const override {
        return "BranchNode";
    }
    
protected:
    BranchNode() = default;
    
    // Convert input map to operands (legacy mode - backward compatible)
    // Aligns with: selector.Selector.selectorInputConverter (selector.go:132-175)
    std::vector<Operants> ConvertInputLegacy(const std::map<std::string, std::any>& input);
    
    // Convert input map to operands (node reference mode - NEW)
    // Resolves node references from input and extracts values
    std::vector<Operants> ConvertInputWithReferences(const std::map<std::string, std::any>& input);
    
    // Resolve a ValueSource to actual value
    // - For Literal: return the literal value directly
    // - For Reference: extract value from input map using node_key + path
    bool ResolveValueSource(const ValueSource& source,
                            const std::map<std::string, std::any>& input,
                            std::any& out);
    
    // Extract value from nested map by path
    // Aligns with: nodes.TakeMapValue
    static bool TakeMapValue(
        const std::map<std::string, std::any>& map,
        const std::vector<std::string>& path,
        std::any& out);
    
private:
    BranchNodeConfig config_;
    bool uses_node_references_;  // True if any clause uses node references
};

// ============================================================================
// Helper Functions
// ============================================================================

// WithBranchNodeName sets the node name
inline std::function<void(std::any&)> WithBranchNodeName(const std::string& name) {
    return [name](std::any& opt) {
        // Set node name in options
        // Implementation depends on graph options structure
    };
}

} // namespace compose
} // namespace eino

#endif // EINO_CPP_COMPOSE_BRANCH_NODE_H_
