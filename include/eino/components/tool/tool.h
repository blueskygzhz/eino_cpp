#ifndef EINO_CPP_COMPONENTS_TOOL_TOOL_H_
#define EINO_CPP_COMPONENTS_TOOL_TOOL_H_

#include <string>
#include <memory>
#include <map>

namespace eino {
namespace tool {

// Simplified tool interface for compilation
class BaseTool {
public:
    virtual ~BaseTool() = default;
    virtual std::string Name() const = 0;
    virtual std::string Description() const = 0;
    virtual void* Call(void* ctx, const std::map<std::string, void*>& params) = 0;
};

}  // namespace tool
}  // namespace eino

#endif  // EINO_CPP_COMPONENTS_TOOL_TOOL_H_
