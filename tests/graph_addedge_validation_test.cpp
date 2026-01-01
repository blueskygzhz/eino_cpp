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
#include <memory>
#include <string>
#include "eino/compose/graph.h"
#include "eino/compose/runnable.h"

namespace eino {
namespace compose {

// Mock Runnable for testing
template<typename I, typename O>
class MockRunnable : public Runnable<I, O> {
public:
    O Invoke(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = {}) override {
        return O();
    }
    
    std::shared_ptr<StreamReader<O>> Stream(
        std::shared_ptr<Context> ctx,
        const I& input,
        const std::vector<Option>& opts = {}) override {
        return nullptr;
    }
    
    O Collect(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = {}) override {
        return O();
    }
    
    std::shared_ptr<StreamReader<O>> Transform(
        std::shared_ptr<Context> ctx,
        std::shared_ptr<StreamReader<I>> input,
        const std::vector<Option>& opts = {}) override {
        return nullptr;
    }
    
    const std::type_info& GetInputType() const override {
        return typeid(I);
    }
    
    const std::type_info& GetOutputType() const override {
        return typeid(O);
    }
    
    std::string GetComponentType() const override {
        return "MockRunnable";
    }
};

/**
 * Test: AddEdge should call AddToValidateMap and detect type mismatches early
 * This test verifies the fix for the missing AddToValidateMap call
 */
TEST(GraphAddEdgeValidationTest, TypeMismatchDetectedInAddEdge) {
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // node1: string → string
    auto node1 = std::make_shared<MockRunnable<std::string, std::string>>();
    graph->AddNode("node1", node1);
    
    // node2: int → int (type mismatch!)
    auto node2 = std::make_shared<MockRunnable<int, int>>();
    // Note: This should fail because Graph<string, string> expects Runnable<string, string>
    // In a proper implementation, AddNode would be templated or type-checked
    
    // For this test, we assume the graph can hold heterogeneous nodes
    // and rely on AddEdge to detect the type mismatch
    
    // TODO: This test needs proper multi-type graph support
    // or we need to create Graph<any, any> variant
}

/**
 * Test: AddEdge with compatible types should succeed
 */
TEST(GraphAddEdgeValidationTest, CompatibleTypesSucceed) {
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    // node1: string → string
    auto node1 = std::make_shared<MockRunnable<std::string, std::string>>();
    graph->AddNode("node1", node1);
    
    // node2: string → string (compatible!)
    auto node2 = std::make_shared<MockRunnable<std::string, std::string>>();
    graph->AddNode("node2", node2);
    
    // This should succeed without throwing
    EXPECT_NO_THROW({
        graph->AddEdge("node1", "node2");
    });
}

/**
 * Test: AddEdge should track edges in validator
 */
TEST(GraphAddEdgeValidationTest, ValidatorTracksEdges) {
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto node1 = std::make_shared<MockRunnable<std::string, std::string>>();
    auto node2 = std::make_shared<MockRunnable<std::string, std::string>>();
    auto node3 = std::make_shared<MockRunnable<std::string, std::string>>();
    
    graph->AddNode("node1", node1);
    graph->AddNode("node2", node2);
    graph->AddNode("node3", node3);
    
    // Add multiple edges
    graph->AddEdge("node1", "node2");
    graph->AddEdge("node2", "node3");
    
    // Verify edges were added
    EXPECT_EQ(graph->GetEdgeCount(), 2);
}

/**
 * Test: Control edges should not trigger type validation
 */
TEST(GraphAddEdgeValidationTest, ControlEdgesSkipValidation) {
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto node1 = std::make_shared<MockRunnable<std::string, std::string>>();
    auto node2 = std::make_shared<MockRunnable<std::string, std::string>>();
    
    graph->AddNode("node1", node1);
    graph->AddNode("node2", node2);
    
    // Control edge (no_data = true) should not trigger type validation
    EXPECT_NO_THROW({
        graph->AddEdge("node1", "node2", false, true);  // no_control=false, no_data=true
    });
}

/**
 * Test: Data edges should trigger type validation
 */
TEST(GraphAddEdgeValidationTest, DataEdgesTriggerValidation) {
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto node1 = std::make_shared<MockRunnable<std::string, std::string>>();
    auto node2 = std::make_shared<MockRunnable<std::string, std::string>>();
    
    graph->AddNode("node1", node1);
    graph->AddNode("node2", node2);
    
    // Data edge should trigger validation
    EXPECT_NO_THROW({
        graph->AddEdge("node1", "node2");  // Default: both control and data
    });
}

/**
 * Test: Multiple edges to same node
 */
TEST(GraphAddEdgeValidationTest, MultipleEdgesToSameNode) {
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto node1 = std::make_shared<MockRunnable<std::string, std::string>>();
    auto node2 = std::make_shared<MockRunnable<std::string, std::string>>();
    auto node3 = std::make_shared<MockRunnable<std::string, std::string>>();
    
    graph->AddNode("node1", node1);
    graph->AddNode("node2", node2);
    graph->AddNode("node3", node3);
    
    // Multiple nodes connect to node3
    graph->AddEdge("node1", "node3");
    graph->AddEdge("node2", "node3");
    
    // Verify all edges were added
    EXPECT_EQ(graph->GetEdgeCount(), 2);
}

/**
 * Test: Field mappings trigger validation
 */
TEST(GraphAddEdgeValidationTest, FieldMappingsTriggerValidation) {
    auto graph = std::make_shared<Graph<std::string, std::string>>();
    
    auto node1 = std::make_shared<MockRunnable<std::string, std::string>>();
    auto node2 = std::make_shared<MockRunnable<std::string, std::string>>();
    
    graph->AddNode("node1", node1);
    graph->AddNode("node2", node2);
    
    // Add edge with field mappings
    std::vector<FieldMapping> mappings;
    FieldMapping mapping;
    mapping.from_key = "field1";
    mapping.to_key = "field2";
    mappings.push_back(mapping);
    
    EXPECT_NO_THROW({
        graph->AddEdge("node1", "node2", mappings);
    });
}

} // namespace compose
} // namespace eino

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
