#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <string>
#include <vector>

namespace she
{
struct ScriptModuleDescriptor
{
    std::string moduleName;
    std::string description;
};

// ScriptingService is the stable host boundary that future Lua integration will
// plug into. The important design choice is that game code depends on this host
// contract, not on a specific scripting middleware API.
class ScriptingService final : public IScriptingService
{
public:
    void Initialize() override;
    void Shutdown() override;
    void RegisterScriptModule(std::string moduleName, std::string description) override;
    void Update(double deltaSeconds) override;
    [[nodiscard]] std::size_t GetModuleCount() const override;
    [[nodiscard]] std::string BuildScriptCatalog() const override;

private:
    std::vector<ScriptModuleDescriptor> m_modules;
    double m_lastDeltaSeconds = 0.0;
};
} // namespace she

