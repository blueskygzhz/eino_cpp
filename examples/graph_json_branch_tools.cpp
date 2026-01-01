/*
 * Copyright 2025 CloudWeGo Authors
 *
 * Branch 和 ToolsNode 的序列化/反序列化完整示例
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>

// 简化的数据结构（零依赖版本）
namespace eino {
namespace compose {

// =============================================================================
// Branch 信息结构
// =============================================================================

struct BranchNodeInfo {
    std::string name;
    std::string branch_type;  // "single" | "multi" | "stream_single" | "stream_multi"
    std::set<std::string> end_nodes;
    std::string condition_key;
    std::map<std::string, std::string> metadata;
    std::string condition_description;
    std::vector<std::map<std::string, std::string>> example_routes;
};

// =============================================================================
// Tool 信息结构
// =============================================================================

struct ToolDefinitionInfo {
    std::string name;
    std::string description;
    std::string parameters_schema;
    std::string type;  // "invokable" | "streamable" | "both"
    std::map<std::string, std::string> metadata;
};

struct ToolsNodeInfo {
    std::string name;
    std::vector<ToolDefinitionInfo> tools;
    bool execute_sequentially = false;
    bool has_unknown_tools_handler = false;
    bool has_arguments_handler = false;
    int middleware_count = 0;
    std::map<std::string, std::string> metadata;
};

// =============================================================================
// JSON 序列化（手动实现）
// =============================================================================

std::string EscapeJson(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

std::string SetToJson(const std::set<std::string>& s) {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& item : s) {
        if (!first) oss << ", ";
        oss << "\"" << EscapeJson(item) << "\"";
        first = false;
    }
    oss << "]";
    return oss.str();
}

std::string MapToJson(const std::map<std::string, std::string>& m) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& kv : m) {
        if (!first) oss << ", ";
        oss << "\"" << EscapeJson(kv.first) << "\": \"" << EscapeJson(kv.second) << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string BranchNodeInfoToJson(const BranchNodeInfo& info, const std::string& indent = "    ") {
    std::ostringstream oss;
    oss << indent << "{\n";
    oss << indent << "  \"name\": \"" << EscapeJson(info.name) << "\",\n";
    oss << indent << "  \"branch_type\": \"" << EscapeJson(info.branch_type) << "\",\n";
    oss << indent << "  \"end_nodes\": " << SetToJson(info.end_nodes) << ",\n";
    oss << indent << "  \"condition_key\": \"" << EscapeJson(info.condition_key) << "\",\n";
    oss << indent << "  \"condition_description\": \"" << EscapeJson(info.condition_description) << "\",\n";
    oss << indent << "  \"metadata\": " << MapToJson(info.metadata);
    
    if (!info.example_routes.empty()) {
        oss << ",\n" << indent << "  \"example_routes\": [\n";
        for (size_t i = 0; i < info.example_routes.size(); ++i) {
            if (i > 0) oss << ",\n";
            oss << indent << "    " << MapToJson(info.example_routes[i]);
        }
        oss << "\n" << indent << "  ]";
    }
    
    oss << "\n" << indent << "}";
    return oss.str();
}

std::string ToolDefinitionInfoToJson(const ToolDefinitionInfo& info, const std::string& indent = "      ") {
    std::ostringstream oss;
    oss << indent << "{\n";
    oss << indent << "  \"name\": \"" << EscapeJson(info.name) << "\",\n";
    oss << indent << "  \"description\": \"" << EscapeJson(info.description) << "\",\n";
    oss << indent << "  \"type\": \"" << EscapeJson(info.type) << "\",\n";
    oss << indent << "  \"parameters_schema\": \"" << EscapeJson(info.parameters_schema) << "\",\n";
    oss << indent << "  \"metadata\": " << MapToJson(info.metadata) << "\n";
    oss << indent << "}";
    return oss.str();
}

std::string ToolsNodeInfoToJson(const ToolsNodeInfo& info, const std::string& indent = "    ") {
    std::ostringstream oss;
    oss << indent << "{\n";
    oss << indent << "  \"name\": \"" << EscapeJson(info.name) << "\",\n";
    oss << indent << "  \"execute_sequentially\": " << (info.execute_sequentially ? "true" : "false") << ",\n";
    oss << indent << "  \"has_unknown_tools_handler\": " << (info.has_unknown_tools_handler ? "true" : "false") << ",\n";
    oss << indent << "  \"has_arguments_handler\": " << (info.has_arguments_handler ? "true" : "false") << ",\n";
    oss << indent << "  \"middleware_count\": " << info.middleware_count << ",\n";
    oss << indent << "  \"metadata\": " << MapToJson(info.metadata) << ",\n";
    oss << indent << "  \"tools\": [\n";
    
    for (size_t i = 0; i < info.tools.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << ToolDefinitionInfoToJson(info.tools[i]);
    }
    
    oss << "\n" << indent << "  ]\n";
    oss << indent << "}";
    return oss.str();
}

} // namespace compose
} // namespace eino

using namespace eino::compose;

// =============================================================================
// 示例 1: Branch 节点序列化
// =============================================================================

void Example1_BranchSerialization() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 1: Branch 节点序列化\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "📝 场景：根据用户意图路由到不同的处理节点\n\n";
    
    BranchNodeInfo branch;
    branch.name = "intent_router";
    branch.branch_type = "single";
    branch.end_nodes = {"weather_handler", "news_handler", "default_handler"};
    branch.condition_key = "route_by_intent";
    branch.condition_description = "根据用户输入的关键词判断意图并路由";
    branch.metadata["version"] = "1.0.0";
    branch.metadata["author"] = "CloudWeGo";
    
    // 示例路由规则
    branch.example_routes = {
        {{"input", "今天天气怎么样"}, {"target", "weather_handler"}},
        {{"input", "最新新闻"}, {"target", "news_handler"}},
        {{"input", "其他请求"}, {"target", "default_handler"}}
    };
    
    std::string json = BranchNodeInfoToJson(branch);
    
    std::cout << "Branch JSON:\n" << json << "\n\n";
    
    std::cout << "✅ 序列化完成\n";
    std::cout << "   类型: " << branch.branch_type << "\n";
    std::cout << "   可能路由: " << branch.end_nodes.size() << " 个目标节点\n";
    std::cout << "   条件标识: " << branch.condition_key << "\n\n";
    
    std::cout << "💡 反序列化时的处理：\n";
    std::cout << "   1. 解析 JSON 获取 branch.condition_key = \"route_by_intent\"\n";
    std::cout << "   2. 从注册表查找条件函数: GetCondition(\"route_by_intent\")\n";
    std::cout << "   3. 使用条件函数和 end_nodes 重建 Branch 对象\n";
    std::cout << "   4. 将 Branch 添加到 Graph\n";
}

// =============================================================================
// 示例 2: 多路 Branch 序列化
// =============================================================================

void Example2_MultiBranchSerialization() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 2: 多路 Branch 序列化\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "📝 场景：同时路由到多个处理节点（并行处理）\n\n";
    
    BranchNodeInfo branch;
    branch.name = "multi_processor_router";
    branch.branch_type = "multi";
    branch.end_nodes = {"sentiment_analyzer", "entity_extractor", "keyword_extractor"};
    branch.condition_key = "route_to_all_processors";
    branch.condition_description = "将输入同时路由到所有文本处理器";
    branch.metadata["mode"] = "parallel";
    branch.metadata["timeout"] = "5000ms";
    
    // 多路路由示例
    branch.example_routes = {
        {{"input", "任意文本"}, 
         {"targets", "sentiment_analyzer,entity_extractor,keyword_extractor"}}
    };
    
    std::string json = BranchNodeInfoToJson(branch);
    
    std::cout << "Multi-Branch JSON:\n" << json << "\n\n";
    
    std::cout << "✅ 序列化完成\n";
    std::cout << "   类型: multi（多路分发）\n";
    std::cout << "   目标节点: " << branch.end_nodes.size() << " 个\n";
    std::cout << "   执行模式: 并行\n\n";
    
    std::cout << "💡 使用场景：\n";
    std::cout << "   - NLP 文本多维度分析\n";
    std::cout << "   - 数据多种格式转换\n";
    std::cout << "   - 多模型推理对比\n";
}

// =============================================================================
// 示例 3: ToolsNode 序列化
// =============================================================================

void Example3_ToolsNodeSerialization() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 3: ToolsNode 序列化\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "📝 场景：Agent 工具调用节点\n\n";
    
    ToolsNodeInfo tools_node;
    tools_node.name = "agent_tools";
    tools_node.execute_sequentially = false;
    tools_node.has_unknown_tools_handler = true;
    tools_node.has_arguments_handler = true;
    tools_node.middleware_count = 2;
    tools_node.metadata["agent_type"] = "assistant";
    tools_node.metadata["max_iterations"] = "10";
    
    // Tool 1: 天气查询
    ToolDefinitionInfo weather_tool;
    weather_tool.name = "get_weather";
    weather_tool.description = "查询指定城市的天气信息";
    weather_tool.type = "invokable";
    weather_tool.parameters_schema = R"({
        "type": "object",
        "properties": {
            "city": {"type": "string", "description": "城市名称"},
            "unit": {"type": "string", "enum": ["celsius", "fahrenheit"]}
        },
        "required": ["city"]
    })";
    weather_tool.metadata["api_endpoint"] = "https://api.weather.com/v1";
    tools_node.tools.push_back(weather_tool);
    
    // Tool 2: 搜索
    ToolDefinitionInfo search_tool;
    search_tool.name = "web_search";
    search_tool.description = "在互联网上搜索信息";
    search_tool.type = "streamable";
    search_tool.parameters_schema = R"({
        "type": "object",
        "properties": {
            "query": {"type": "string", "description": "搜索关键词"},
            "max_results": {"type": "integer", "default": 10}
        },
        "required": ["query"]
    })";
    search_tool.metadata["search_engine"] = "google";
    tools_node.tools.push_back(search_tool);
    
    // Tool 3: 计算器
    ToolDefinitionInfo calc_tool;
    calc_tool.name = "calculator";
    calc_tool.description = "执行数学计算";
    calc_tool.type = "invokable";
    calc_tool.parameters_schema = R"({
        "type": "object",
        "properties": {
            "expression": {"type": "string", "description": "数学表达式"}
        },
        "required": ["expression"]
    })";
    calc_tool.metadata["precision"] = "double";
    tools_node.tools.push_back(calc_tool);
    
    std::string json = ToolsNodeInfoToJson(tools_node);
    
    std::cout << "ToolsNode JSON:\n" << json << "\n\n";
    
    std::cout << "✅ 序列化完成\n";
    std::cout << "   节点名称: " << tools_node.name << "\n";
    std::cout << "   工具数量: " << tools_node.tools.size() << "\n";
    std::cout << "   执行模式: " << (tools_node.execute_sequentially ? "顺序" : "并行") << "\n";
    std::cout << "   中间件数: " << tools_node.middleware_count << "\n\n";
    
    std::cout << "📋 工具列表:\n";
    for (size_t i = 0; i < tools_node.tools.size(); ++i) {
        const auto& tool = tools_node.tools[i];
        std::cout << "   " << (i+1) << ". " << tool.name 
                  << " [" << tool.type << "]\n";
        std::cout << "      " << tool.description << "\n";
    }
    
    std::cout << "\n💡 反序列化时的处理：\n";
    std::cout << "   1. 解析 JSON 获取 tools 列表\n";
    std::cout << "   2. 对每个 tool，从工厂注册表创建实例：\n";
    std::cout << "      CreateTool(\"get_weather\") -> WeatherTool\n";
    std::cout << "      CreateTool(\"web_search\") -> SearchTool\n";
    std::cout << "      CreateTool(\"calculator\") -> CalculatorTool\n";
    std::cout << "   3. 创建 ToolsNodeConfig，设置配置参数\n";
    std::cout << "   4. 使用 ToolsNode::New() 创建节点\n";
    std::cout << "   5. 将 ToolsNode 添加到 Graph\n";
}

// =============================================================================
// 示例 4: 完整 Graph（含 Branch 和 ToolsNode）
// =============================================================================

void Example4_CompleteGraphSerialization() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 4: 完整 Graph 序列化（含 Branch 和 ToolsNode）\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "📝 场景：智能助手 Graph\n";
    std::cout << "   流程：输入 → 意图识别 → Branch 路由 → 工具调用 → 输出\n\n";
    
    std::ostringstream graph_json;
    graph_json << "{\n";
    graph_json << "  \"type\": \"Graph\",\n";
    graph_json << "  \"name\": \"IntelligentAssistant\",\n";
    graph_json << "  \"is_compiled\": true,\n";
    graph_json << "  \"max_steps\": 20,\n\n";
    
    // 普通节点
    graph_json << "  \"nodes\": [\n";
    graph_json << "    {\n";
    graph_json << "      \"name\": \"intent_classifier\",\n";
    graph_json << "      \"type\": \"Classifier\",\n";
    graph_json << "      \"has_runnable\": true\n";
    graph_json << "    },\n";
    graph_json << "    {\n";
    graph_json << "      \"name\": \"response_generator\",\n";
    graph_json << "      \"type\": \"Generator\",\n";
    graph_json << "      \"has_runnable\": true\n";
    graph_json << "    }\n";
    graph_json << "  ],\n\n";
    
    // Branch 节点
    BranchNodeInfo branch;
    branch.name = "intent_router";
    branch.branch_type = "single";
    branch.end_nodes = {"simple_qa", "tool_calling", "chitchat"};
    branch.condition_key = "route_by_intent";
    branch.condition_description = "根据意图分类结果路由";
    
    graph_json << "  \"branches\": [\n";
    graph_json << BranchNodeInfoToJson(branch, "    ");
    graph_json << "\n  ],\n\n";
    
    // ToolsNode
    ToolsNodeInfo tools_node;
    tools_node.name = "tool_calling";
    tools_node.execute_sequentially = false;
    
    ToolDefinitionInfo tool;
    tool.name = "knowledge_search";
    tool.description = "搜索知识库";
    tool.type = "invokable";
    tool.parameters_schema = "{\"query\": \"string\"}";
    tools_node.tools.push_back(tool);
    
    graph_json << "  \"tools_nodes\": [\n";
    graph_json << ToolsNodeInfoToJson(tools_node, "    ");
    graph_json << "\n  ],\n\n";
    
    // 边
    graph_json << "  \"edges\": [\n";
    graph_json << "    {\"from\": \"__START__\", \"to\": \"intent_classifier\"},\n";
    graph_json << "    {\"from\": \"intent_classifier\", \"to\": \"intent_router\"},\n";
    graph_json << "    {\"from\": \"intent_router\", \"to\": \"simple_qa\"},\n";
    graph_json << "    {\"from\": \"intent_router\", \"to\": \"tool_calling\"},\n";
    graph_json << "    {\"from\": \"intent_router\", \"to\": \"chitchat\"},\n";
    graph_json << "    {\"from\": \"simple_qa\", \"to\": \"response_generator\"},\n";
    graph_json << "    {\"from\": \"tool_calling\", \"to\": \"response_generator\"},\n";
    graph_json << "    {\"from\": \"chitchat\", \"to\": \"response_generator\"},\n";
    graph_json << "    {\"from\": \"response_generator\", \"to\": \"__END__\"}\n";
    graph_json << "  ]\n";
    graph_json << "}\n";
    
    std::string json = graph_json.str();
    std::cout << "Complete Graph JSON:\n" << json << "\n\n";
    
    // 保存到文件
    std::string filepath = "/tmp/intelligent_assistant_graph.json";
    std::ofstream file(filepath);
    file << json;
    file.close();
    
    std::cout << "✅ 保存到: " << filepath << "\n\n";
    
    std::cout << "📊 Graph 结构:\n";
    std::cout << "   - 2 个普通节点\n";
    std::cout << "   - 1 个 Branch 节点（3 路分支）\n";
    std::cout << "   - 1 个 ToolsNode（1 个工具）\n";
    std::cout << "   - 9 条边\n\n";
    
    std::cout << "🔄 执行流程:\n";
    std::cout << "   输入\n";
    std::cout << "     ↓\n";
    std::cout << "   意图分类\n";
    std::cout << "     ↓\n";
    std::cout << "   Branch 路由 ─→ simple_qa ──┐\n";
    std::cout << "            ├─→ tool_calling ─┤\n";
    std::cout << "            └─→ chitchat ─────┘\n";
    std::cout << "                               ↓\n";
    std::cout << "                          响应生成\n";
    std::cout << "                               ↓\n";
    std::cout << "                             输出\n";
}

// =============================================================================
// 示例 5: 反序列化流程说明
// =============================================================================

void Example5_DeserializationWorkflow() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Example 5: 反序列化工作流程\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "📋 完整的反序列化步骤：\n\n";
    
    std::cout << "1️⃣  加载 JSON 文件\n";
    std::cout << "   auto json = LoadJsonFile(\"graph.json\");\n\n";
    
    std::cout << "2️⃣  解析基础信息\n";
    std::cout << "   - 节点列表\n";
    std::cout << "   - 边列表\n";
    std::cout << "   - 编译选项\n\n";
    
    std::cout << "3️⃣  解析 Branch 节点\n";
    std::cout << "   for (auto& branch_info : json[\"branches\"]) {\n";
    std::cout << "       // 获取条件函数\n";
    std::cout << "       auto condition = BranchConditionRegistry::Instance()\n";
    std::cout << "           .GetSingleCondition(branch_info.condition_key);\n";
    std::cout << "       \n";
    std::cout << "       // 创建 Branch\n";
    std::cout << "       auto branch = NewGraphBranch(condition, branch_info.end_nodes);\n";
    std::cout << "       \n";
    std::cout << "       // 存储 Branch\n";
    std::cout << "       branches[branch_info.name] = branch;\n";
    std::cout << "   }\n\n";
    
    std::cout << "4️⃣  解析 ToolsNode\n";
    std::cout << "   for (auto& tools_node_info : json[\"tools_nodes\"]) {\n";
    std::cout << "       // 创建工具列表\n";
    std::cout << "       std::vector<BaseTool> tools;\n";
    std::cout << "       for (auto& tool_info : tools_node_info.tools) {\n";
    std::cout << "           auto tool = ToolFactoryRegistry::Instance()\n";
    std::cout << "               .CreateTool(tool_info);\n";
    std::cout << "           tools.push_back(tool);\n";
    std::cout << "       }\n";
    std::cout << "       \n";
    std::cout << "       // 创建配置\n";
    std::cout << "       ToolsNodeConfig config;\n";
    std::cout << "       config.tools = tools;\n";
    std::cout << "       config.execute_sequentially = tools_node_info.execute_sequentially;\n";
    std::cout << "       \n";
    std::cout << "       // 创建 ToolsNode\n";
    std::cout << "       auto tools_node = ToolsNode::New(ctx, config);\n";
    std::cout << "       nodes[tools_node_info.name] = tools_node;\n";
    std::cout << "   }\n\n";
    
    std::cout << "5️⃣  重建 Graph\n";
    std::cout << "   auto graph = std::make_shared<Graph>();\n";
    std::cout << "   \n";
    std::cout << "   // 添加普通节点\n";
    std::cout << "   for (auto& [name, runnable] : nodes) {\n";
    std::cout << "       graph->AddNode(name, runnable);\n";
    std::cout << "   }\n";
    std::cout << "   \n";
    std::cout << "   // 添加 Branch\n";
    std::cout << "   for (auto& [name, branch] : branches) {\n";
    std::cout << "       graph->AddBranch(name, branch);\n";
    std::cout << "   }\n";
    std::cout << "   \n";
    std::cout << "   // 添加边\n";
    std::cout << "   for (auto& edge : json[\"edges\"]) {\n";
    std::cout << "       graph->AddEdge(edge.from, edge.to);\n";
    std::cout << "   }\n";
    std::cout << "   \n";
    std::cout << "   // 编译\n";
    std::cout << "   graph->Compile(compile_options);\n\n";
    
    std::cout << "6️⃣  验证和测试\n";
    std::cout << "   auto result = graph->Invoke(ctx, input);\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Branch 和 ToolsNode 序列化/反序列化完整指南                  ║\n";
    std::cout << "║   包含条件逻辑、工具定义的完整处理方案                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    
    try {
        Example1_BranchSerialization();
        Example2_MultiBranchSerialization();
        Example3_ToolsNodeSerialization();
        Example4_CompleteGraphSerialization();
        Example5_DeserializationWorkflow();
        
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "✅ 所有示例运行完成！\n";
        std::cout << std::string(70, '=') << "\n\n";
        
        std::cout << "📚 核心要点总结:\n\n";
        
        std::cout << "1️⃣  Branch 序列化策略:\n";
        std::cout << "   ❌ 函数指针无法序列化\n";
        std::cout << "   ✅ 保存 condition_key + end_nodes\n";
        std::cout << "   ✅ 使用注册表模式重建条件函数\n\n";
        
        std::cout << "2️⃣  ToolsNode 序列化策略:\n";
        std::cout << "   ❌ Tool 对象无法序列化\n";
        std::cout << "   ✅ 保存 Tool 定义（name, schema, metadata）\n";
        std::cout << "   ✅ 使用工厂模式重建 Tool 对象\n\n";
        
        std::cout << "3️⃣  注册机制:\n";
        std::cout << "   - BranchConditionRegistry: 管理条件函数\n";
        std::cout << "   - ToolFactoryRegistry: 管理 Tool 工厂\n";
        std::cout << "   - 使用宏简化注册过程\n\n";
        
        std::cout << "4️⃣  元数据的重要性:\n";
        std::cout << "   - condition_key: 唯一标识条件逻辑\n";
        std::cout << "   - example_routes: 辅助理解路由规则\n";
        std::cout << "   - parameters_schema: Tool 参数定义\n";
        std::cout << "   - metadata: 扩展配置信息\n\n";
        
        std::cout << "💡 最佳实践:\n";
        std::cout << "   ✅ 为每个条件函数分配唯一的 key\n";
        std::cout << "   ✅ 在应用启动时注册所有条件和工厂\n";
        std::cout << "   ✅ 添加详细的 condition_description\n";
        std::cout << "   ✅ 提供 example_routes 作为文档\n";
        std::cout << "   ✅ 使用 JSON Schema 定义 Tool 参数\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}
