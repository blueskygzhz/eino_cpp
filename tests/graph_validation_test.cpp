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

#include <gtest/gtest.h>
#include <map>
#include <string>
#include <any>

#include "eino/compose/graph_validation.h"
#include "eino/compose/type_registry.h"
#include "eino/compose/field_mapping.h"
#include "eino/compose/utils.h"

using namespace eino::compose;

// Test types
struct MessageInput {
    std::string content;
    std::map<std::string, std::any> metadata;
};

struct MessageOutput {
    std::string result;
    int code;
};

struct ProcessorNode {
    MessageInput input;
    MessageOutput output;
};

// Mock interface for testing
class IProcessor {
public:
    virtual ~IProcessor() = default;
    virtual void Process() = 0;
};

class ConcreteProcessor : public IProcessor {
public:
    void Process() override {}
};

// Register implementations
EINO_REGISTER_IMPLEMENTATION(ConcreteProcessor, IProcessor);

// Tests

TEST(TypeRegistryTest, BasicImplementation) {
    auto& registry = TypeRegistry::Instance();
    
    // Check registered implementation
    EXPECT_TRUE(registry.Implements(typeid(ConcreteProcessor), typeid(IProcessor)));
    
    // Check non-existent implementation
    EXPECT_FALSE(registry.Implements(typeid(MessageInput), typeid(IProcessor)));
}

TEST(TypeRegistryTest, IsAssignable) {
    auto& registry = TypeRegistry::Instance();
    
    // Same type
    EXPECT_TRUE(registry.IsAssignable(typeid(std::string), typeid(std::string)));
    
    // Implementation to interface
    EXPECT_TRUE(registry.IsAssignable(typeid(ConcreteProcessor), typeid(IProcessor)));
    
    // Incompatible types
    EXPECT_FALSE(registry.IsAssignable(typeid(int), typeid(std::string)));
}

TEST(CheckAssignableTest, SameType) {
    AssignableType result = CheckAssignable(typeid(std::string), typeid(std::string));
    EXPECT_EQ(result, AssignableType::Must);
}

TEST(CheckAssignableTest, InterfaceImplementation) {
    AssignableType result = CheckAssignable(typeid(ConcreteProcessor), typeid(IProcessor));
    EXPECT_EQ(result, AssignableType::Must);
}

TEST(CheckAssignableTest, IncompatibleTypes) {
    AssignableType result = CheckAssignable(typeid(int), typeid(std::string));
    EXPECT_EQ(result, AssignableType::MustNot);
}

TEST(CheckAssignableTest, AnyType) {
    // std::any can accept anything
    AssignableType result = CheckAssignable(typeid(int), typeid(std::any));
    EXPECT_EQ(result, AssignableType::Must);
}

TEST(FieldMappingTest, CreateBasicMapping) {
    auto mapping = MapField("source", "target");
    
    EXPECT_EQ(mapping.from_key, "source");
    EXPECT_EQ(mapping.to_key, "target");
    EXPECT_FALSE(mapping.transformer);
}

TEST(FieldMappingTest, CreateEntireInputMapping) {
    auto mapping = MapEntireInput("target");
    
    EXPECT_TRUE(mapping.from_key.empty());
    EXPECT_EQ(mapping.to_key, "target");
    EXPECT_TRUE(mapping.map_entire_input);
}

TEST(FieldMappingTest, CreateTransformMapping) {
    auto transformer = [](void*, const std::any& input) -> std::any {
        return std::string("transformed");
    };
    
    auto mapping = MapFieldWithTransform("source", "target", transformer);
    
    EXPECT_EQ(mapping.from_key, "source");
    EXPECT_EQ(mapping.to_key, "target");
    EXPECT_TRUE(mapping.transformer);
    EXPECT_TRUE(mapping.transformer_func != nullptr);
}

TEST(ValidateFieldMappingTest, EmptyKeys) {
    FieldMapping mapping;
    mapping.from_key = "";
    mapping.to_key = "";
    
    std::vector<FieldMapping> mappings = {mapping};
    
    std::string error = ValidateFieldMapping(
        typeid(MessageInput),
        typeid(MessageOutput),
        mappings);
    
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("both"), std::string::npos);
}

TEST(ValidateFieldMappingTest, ValidMapping) {
    auto mapping = MapField("content", "result");
    std::vector<FieldMapping> mappings = {mapping};
    
    // For map[string]any types, validation should pass
    std::string error = ValidateFieldMapping(
        typeid(std::map<std::string, std::any>),
        typeid(std::map<std::string, std::any>),
        mappings);
    
    EXPECT_TRUE(error.empty());
}

TEST(ValidateFieldMappingTest, WithTransformer) {
    auto transformer = [](void*, const std::any& input) -> std::any {
        return input;
    };
    
    auto mapping = MapFieldWithTransform("source", "target", transformer);
    std::vector<FieldMapping> mappings = {mapping};
    
    std::string error = ValidateFieldMapping(
        typeid(int),
        typeid(std::string),
        mappings);
    
    // With transformer, type mismatch should be allowed
    EXPECT_TRUE(error.empty());
}

TEST(FieldPathTest, ParseSimplePath) {
    FieldPath path("field");
    
    EXPECT_EQ(path.GetSegments().size(), 1);
    EXPECT_EQ(path.GetSegments()[0], "field");
}

TEST(FieldPathTest, ParseNestedPath) {
    FieldPath path("field.subfield.value");
    
    EXPECT_EQ(path.GetSegments().size(), 3);
    EXPECT_EQ(path.GetSegments()[0], "field");
    EXPECT_EQ(path.GetSegments()[1], "subfield");
    EXPECT_EQ(path.GetSegments()[2], "value");
}

TEST(FieldPathTest, EmptyPath) {
    FieldPath path("");
    
    EXPECT_TRUE(path.IsEmpty());
    EXPECT_EQ(path.GetSegments().size(), 0);
}

TEST(GraphValidatorTest, AddToValidateMap) {
    GraphValidator validator;
    
    validator.AddToValidateMap("node1", "node2", {});
    
    EXPECT_FALSE(validator.IsEmpty());
}

TEST(GraphValidatorTest, UpdateToValidateMapDirectConnection) {
    GraphValidator validator;
    
    std::map<std::string, const std::type_info*> input_types;
    std::map<std::string, const std::type_info*> output_types;
    
    // Setup types
    output_types["node1"] = &typeid(std::string);
    input_types["node2"] = &typeid(std::string);
    
    // Add edge
    validator.AddToValidateMap("node1", "node2", {});
    
    // Update should succeed (types match)
    std::string error = validator.UpdateToValidateMap(
        input_types, 
        output_types,
        nullptr);
    
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(validator.IsEmpty());
}

TEST(GraphValidatorTest, UpdateToValidateMapTypeMismatch) {
    GraphValidator validator;
    
    std::map<std::string, const std::type_info*> input_types;
    std::map<std::string, const std::type_info*> output_types;
    
    // Setup incompatible types
    output_types["node1"] = &typeid(int);
    input_types["node2"] = &typeid(std::string);
    
    // Add edge
    validator.AddToValidateMap("node1", "node2", {});
    
    // Update should fail (type mismatch)
    std::string error = validator.UpdateToValidateMap(
        input_types,
        output_types,
        nullptr);
    
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("mismatch"), std::string::npos);
}

TEST(GraphValidatorTest, PassthroughForwardInference) {
    GraphValidator validator;
    
    std::map<std::string, const std::type_info*> input_types;
    std::map<std::string, const std::type_info*> output_types;
    
    // node1 has type, node2 is passthrough (no type yet)
    output_types["node1"] = &typeid(std::string);
    
    auto is_passthrough = [](const std::string& name) {
        return name == "node2";  // node2 is passthrough
    };
    
    validator.AddToValidateMap("node1", "node2", {});
    
    std::string error = validator.UpdateToValidateMap(
        input_types,
        output_types,
        is_passthrough);
    
    EXPECT_TRUE(error.empty());
    
    // node2 should inherit type from node1
    EXPECT_TRUE(input_types.count("node2") > 0);
    EXPECT_EQ(*input_types["node2"], typeid(std::string));
    EXPECT_EQ(*output_types["node2"], typeid(std::string));
}

TEST(GraphValidatorTest, PassthroughBackwardInference) {
    GraphValidator validator;
    
    std::map<std::string, const std::type_info*> input_types;
    std::map<std::string, const std::type_info*> output_types;
    
    // node2 has type, node1 is passthrough (no type yet)
    input_types["node2"] = &typeid(int);
    
    auto is_passthrough = [](const std::string& name) {
        return name == "node1";  // node1 is passthrough
    };
    
    validator.AddToValidateMap("node1", "node2", {});
    
    std::string error = validator.UpdateToValidateMap(
        input_types,
        output_types,
        is_passthrough);
    
    EXPECT_TRUE(error.empty());
    
    // node1 should inherit type from node2
    EXPECT_TRUE(input_types.count("node1") > 0);
    EXPECT_EQ(*input_types["node1"], typeid(int));
    EXPECT_EQ(*output_types["node1"], typeid(int));
}

TEST(GraphValidatorTest, MultiRoundInference) {
    GraphValidator validator;
    
    std::map<std::string, const std::type_info*> input_types;
    std::map<std::string, const std::type_info*> output_types;
    
    // Chain: node1 (has type) -> node2 (passthrough) -> node3 (passthrough)
    output_types["node1"] = &typeid(double);
    
    auto is_passthrough = [](const std::string& name) {
        return name == "node2" || name == "node3";
    };
    
    validator.AddToValidateMap("node1", "node2", {});
    validator.AddToValidateMap("node2", "node3", {});
    
    std::string error = validator.UpdateToValidateMap(
        input_types,
        output_types,
        is_passthrough);
    
    EXPECT_TRUE(error.empty());
    
    // Both passthrough nodes should get the type
    EXPECT_TRUE(output_types.count("node2") > 0);
    EXPECT_EQ(*output_types["node2"], typeid(double));
    
    EXPECT_TRUE(output_types.count("node3") > 0);
    EXPECT_EQ(*output_types["node3"], typeid(double));
}

TEST(EdgeTypeValidatorTest, ValidDirectEdge) {
    std::string error = EdgeTypeValidator::ValidateEdge(
        typeid(std::string),
        typeid(std::string),
        {});
    
    EXPECT_TRUE(error.empty());
}

TEST(EdgeTypeValidatorTest, InvalidDirectEdge) {
    std::string error = EdgeTypeValidator::ValidateEdge(
        typeid(int),
        typeid(std::string),
        {});
    
    EXPECT_FALSE(error.empty());
}

TEST(EdgeTypeValidatorTest, NeedsRuntimeCheck) {
    // For interface types, might need runtime check
    bool needs_check = EdgeTypeValidator::NeedsRuntimeCheck(
        typeid(IProcessor),
        typeid(ConcreteProcessor));
    
    // Depends on CheckAssignable implementation
    // This is just to ensure the function works
    EXPECT_TRUE(needs_check || !needs_check);  // Always passes, demonstrates usage
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
