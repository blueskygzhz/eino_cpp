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

// Tool JSONSchema Test - Validates ToJSONSchema implementation
// Aligns with: eino/schema/tool_test.go TestParamsOneOfToJSONSchema

#include "eino/schema/tool.h"
#include <cassert>
#include <iostream>

using namespace eino::schema;
using json = nlohmann::json;

// Test 1: Simple parameters
// Aligns with: eino/schema/tool_test.go TestParamsOneOfToJSONSchema basic test
void test_simple_params() {
    std::cout << "Test 1: Simple parameters..." << std::endl;
    
    auto name_param = std::make_shared<ParameterInfo>(
        DataType::kString, 
        "User name", 
        true  // required
    );
    
    auto age_param = std::make_shared<ParameterInfo>(
        DataType::kInteger,
        "User age",
        false  // optional
    );
    
    std::map<std::string, std::shared_ptr<ParameterInfo>> params = {
        {"name", name_param},
        {"age", age_param}
    };
    
    auto params_oneof = ParamsOneOf::FromParams(params);
    auto schema = params_oneof->ToJSONSchema();
    
    // Verify structure
    assert(schema["type"] == "object");
    assert(schema["properties"].contains("name"));
    assert(schema["properties"].contains("age"));
    
    // Verify name field
    assert(schema["properties"]["name"]["type"] == "string");
    assert(schema["properties"]["name"]["description"] == "User name");
    
    // Verify age field
    assert(schema["properties"]["age"]["type"] == "integer");
    assert(schema["properties"]["age"]["description"] == "User age");
    
    // Verify required
    assert(schema["required"].is_array());
    assert(schema["required"].size() == 1);
    assert(schema["required"][0] == "name");
    
    std::cout << "Generated schema:\n" << schema.dump(2) << std::endl;
    std::cout << "✅ PASS" << std::endl;
}

// Test 2: Nested object parameters
// Aligns with: eino/schema/tool_test.go TestParamsOneOfToJSONSchema nested object
void test_nested_object() {
    std::cout << "\nTest 2: Nested object parameters..." << std::endl;
    
    // Create address sub-parameters
    auto street_param = std::make_shared<ParameterInfo>(
        DataType::kString, "Street name", true
    );
    auto city_param = std::make_shared<ParameterInfo>(
        DataType::kString, "City name", true
    );
    auto zip_param = std::make_shared<ParameterInfo>(
        DataType::kString, "ZIP code", false
    );
    
    auto address_param = std::make_shared<ParameterInfo>(
        DataType::kObject, "User address", true
    );
    address_param->sub_params = {
        {"street", street_param},
        {"city", city_param},
        {"zip", zip_param}
    };
    
    // Create root parameters
    auto name_param = std::make_shared<ParameterInfo>(
        DataType::kString, "User name", true
    );
    
    std::map<std::string, std::shared_ptr<ParameterInfo>> params = {
        {"name", name_param},
        {"address", address_param}
    };
    
    auto params_oneof = ParamsOneOf::FromParams(params);
    auto schema = params_oneof->ToJSONSchema();
    
    // Verify root level
    assert(schema["type"] == "object");
    assert(schema["required"].size() == 2);
    
    // Verify address nested object
    assert(schema["properties"]["address"]["type"] == "object");
    assert(schema["properties"]["address"]["properties"].contains("street"));
    assert(schema["properties"]["address"]["properties"].contains("city"));
    assert(schema["properties"]["address"]["properties"].contains("zip"));
    
    // Verify address required fields
    assert(schema["properties"]["address"]["required"].size() == 2);
    
    std::cout << "Generated schema:\n" << schema.dump(2) << std::endl;
    std::cout << "✅ PASS" << std::endl;
}

// Test 3: Array parameters
// Aligns with: eino/schema/tool_test.go TestParamsOneOfToJSONSchema array
void test_array_params() {
    std::cout << "\nTest 3: Array parameters..." << std::endl;
    
    // Create array element type
    auto elem_param = std::make_shared<ParameterInfo>(
        DataType::kString, "Tag value", false
    );
    
    // Create array parameter
    auto tags_param = std::make_shared<ParameterInfo>(
        DataType::kArray, "User tags", true
    );
    tags_param->elem_info = elem_param;
    
    std::map<std::string, std::shared_ptr<ParameterInfo>> params = {
        {"tags", tags_param}
    };
    
    auto params_oneof = ParamsOneOf::FromParams(params);
    auto schema = params_oneof->ToJSONSchema();
    
    // Verify array structure
    assert(schema["properties"]["tags"]["type"] == "array");
    assert(schema["properties"]["tags"]["items"]["type"] == "string");
    assert(schema["properties"]["tags"]["description"] == "User tags");
    assert(schema["required"][0] == "tags");
    
    std::cout << "Generated schema:\n" << schema.dump(2) << std::endl;
    std::cout << "✅ PASS" << std::endl;
}

// Test 4: Enum parameters
// Aligns with: eino/schema/tool_test.go TestParamsOneOfToJSONSchema enum
void test_enum_params() {
    std::cout << "\nTest 4: Enum parameters..." << std::endl;
    
    auto status_param = std::make_shared<ParameterInfo>(
        DataType::kString, "User status", true
    );
    status_param->enum_values = {"active", "inactive", "suspended"};
    
    std::map<std::string, std::shared_ptr<ParameterInfo>> params = {
        {"status", status_param}
    };
    
    auto params_oneof = ParamsOneOf::FromParams(params);
    auto schema = params_oneof->ToJSONSchema();
    
    // Verify enum
    assert(schema["properties"]["status"]["type"] == "string");
    assert(schema["properties"]["status"]["enum"].is_array());
    assert(schema["properties"]["status"]["enum"].size() == 3);
    assert(schema["properties"]["status"]["enum"][0] == "active");
    assert(schema["properties"]["status"]["enum"][1] == "inactive");
    assert(schema["properties"]["status"]["enum"][2] == "suspended");
    
    std::cout << "Generated schema:\n" << schema.dump(2) << std::endl;
    std::cout << "✅ PASS" << std::endl;
}

// Test 5: Complex nested structure
void test_complex_nested() {
    std::cout << "\nTest 5: Complex nested structure..." << std::endl;
    
    // Create array of objects
    auto item_name = std::make_shared<ParameterInfo>(
        DataType::kString, "Item name", true
    );
    auto item_price = std::make_shared<ParameterInfo>(
        DataType::kNumber, "Item price", true
    );
    
    auto item_object = std::make_shared<ParameterInfo>(
        DataType::kObject, "Order item", false
    );
    item_object->sub_params = {
        {"name", item_name},
        {"price", item_price}
    };
    
    auto items_array = std::make_shared<ParameterInfo>(
        DataType::kArray, "Order items", true
    );
    items_array->elem_info = item_object;
    
    // Root parameters
    auto order_id = std::make_shared<ParameterInfo>(
        DataType::kString, "Order ID", true
    );
    
    std::map<std::string, std::shared_ptr<ParameterInfo>> params = {
        {"order_id", order_id},
        {"items", items_array}
    };
    
    auto params_oneof = ParamsOneOf::FromParams(params);
    auto schema = params_oneof->ToJSONSchema();
    
    // Verify structure
    assert(schema["type"] == "object");
    assert(schema["properties"]["items"]["type"] == "array");
    assert(schema["properties"]["items"]["items"]["type"] == "object");
    assert(schema["properties"]["items"]["items"]["properties"].contains("name"));
    assert(schema["properties"]["items"]["items"]["properties"].contains("price"));
    assert(schema["properties"]["items"]["items"]["required"].size() == 2);
    
    std::cout << "Generated schema:\n" << schema.dump(2) << std::endl;
    std::cout << "✅ PASS" << std::endl;
}

// Test 6: Direct JSONSchema
void test_direct_jsonschema() {
    std::cout << "\nTest 6: Direct JSONSchema..." << std::endl;
    
    json direct_schema = {
        {"type", "object"},
        {"properties", {
            {"custom_field", {
                {"type", "string"},
                {"description", "Custom field"}
            }}
        }},
        {"required", {"custom_field"}}
    };
    
    auto params_oneof = ParamsOneOf::FromJSONSchema(direct_schema);
    auto schema = params_oneof->ToJSONSchema();
    
    // Should return the same schema
    assert(schema == direct_schema);
    
    std::cout << "Generated schema:\n" << schema.dump(2) << std::endl;
    std::cout << "✅ PASS" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Tool JSONSchema Test Suite" << std::endl;
    std::cout << "Validates ToJSONSchema aligns with eino" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    test_simple_params();
    test_nested_object();
    test_array_params();
    test_enum_params();
    test_complex_nested();
    test_direct_jsonschema();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "✅ All tests passed!" << std::endl;
    std::cout << "ToJSONSchema aligns with eino implementation" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
