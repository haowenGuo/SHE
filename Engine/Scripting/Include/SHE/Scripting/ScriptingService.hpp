#pragma once

#include "SHE/Core/RuntimeServices.hpp"

#include <memory>
#include <string>
#include <vector>

namespace she
{
struct ScriptInvocationRecord
{
    std::string moduleName;
    std::string functionName;
    std::string payload;
    bool accepted = false;
    bool queuedGameplayCommand = false;
    std::string queuedCommandName;
    std::string statusMessage;
};

// ScriptingService is the stable host boundary that future Lua integration will
// plug into. The important design choice is that game code depends on module
// contracts, binding names, and function invocation rules rather than on a
// specific scripting middleware API.
class ScriptingService final : public IScriptingService
{
public:
    explicit ScriptingService(std::shared_ptr<IGameplayService> gameplay = nullptr);

    void Initialize() override;
    void Shutdown() override;
    void RegisterBinding(ScriptHostBindingDescriptor descriptor) override;
    void RegisterScriptModule(ScriptModuleContract module) override;
    [[nodiscard]] bool HasBinding(std::string_view bindingName) const override;
    [[nodiscard]] bool HasScriptModule(std::string_view moduleName) const override;
    [[nodiscard]] ScriptModuleLoadResult LoadScriptModule(std::string_view moduleName) override;
    [[nodiscard]] ScriptInvocationResult InvokeScriptFunction(
        std::string_view moduleName,
        std::string_view functionName,
        std::string payload) override;
    void Update(double deltaSeconds) override;
    [[nodiscard]] std::size_t GetBindingCount() const override;
    [[nodiscard]] std::size_t GetModuleCount() const override;
    [[nodiscard]] std::size_t GetLoadedModuleCount() const override;
    [[nodiscard]] std::vector<ScriptHostBindingDescriptor> ListBindings() const override;
    [[nodiscard]] std::vector<ScriptModuleCatalogEntry> ListModules() const override;
    [[nodiscard]] std::string BuildScriptCatalog() const override;

private:
    [[nodiscard]] ScriptModuleCatalogEntry* FindModule(std::string_view moduleName);
    [[nodiscard]] const ScriptModuleCatalogEntry* FindModule(std::string_view moduleName) const;
    [[nodiscard]] ScriptHostBindingDescriptor* FindBinding(std::string_view bindingName);
    [[nodiscard]] const ScriptHostBindingDescriptor* FindBinding(std::string_view bindingName) const;
    [[nodiscard]] ScriptModuleLoadResult ValidateModule(ScriptModuleCatalogEntry& module);
    [[nodiscard]] static const ScriptFunctionContract* FindFunction(
        const ScriptModuleCatalogEntry& module,
        std::string_view functionName);
    void RecordInvocation(const ScriptInvocationResult& result);

    std::shared_ptr<IGameplayService> m_gameplay;
    std::vector<ScriptHostBindingDescriptor> m_bindings;
    std::vector<ScriptModuleCatalogEntry> m_modules;
    std::vector<ScriptInvocationRecord> m_recentInvocations;
    double m_lastDeltaSeconds = 0.0;
    std::size_t m_updateCount = 0;
};
} // namespace she

