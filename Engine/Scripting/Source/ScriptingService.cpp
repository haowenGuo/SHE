#include "SHE/Scripting/ScriptingService.hpp"

#include "SHE/Core/Logger.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>
#include <utility>

namespace she
{
namespace
{
constexpr std::size_t kMaxRecentInvocations = 8;

bool EndsWith(const std::string_view text, const std::string_view suffix)
{
    return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
}

bool ContainsScriptDirectoryMarker(const std::string_view path)
{
    return path.find("/Scripts/") != std::string_view::npos || path.find("\\Scripts\\") != std::string_view::npos ||
           path.starts_with("Scripts/") || path.starts_with("Scripts\\");
}

std::string JoinValues(const std::vector<std::string>& values)
{
    if (values.empty())
    {
        return "<none>";
    }

    std::ostringstream stream;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        stream << values[index];
        if (index + 1 < values.size())
        {
            stream << ", ";
        }
    }

    return stream.str();
}

std::string DescribeLoadState(const ScriptModuleLoadState state)
{
    switch (state)
    {
    case ScriptModuleLoadState::Registered:
        return "registered";
    case ScriptModuleLoadState::Loaded:
        return "loaded";
    case ScriptModuleLoadState::Blocked:
        return "blocked";
    }

    return "unknown";
}
} // namespace

ScriptingService::ScriptingService(std::shared_ptr<IGameplayService> gameplay) : m_gameplay(std::move(gameplay))
{
}

void ScriptingService::Initialize()
{
    m_bindings.clear();
    m_modules.clear();
    m_recentInvocations.clear();
    m_lastDeltaSeconds = 0.0;
    m_updateCount = 0;
    SHE_LOG_INFO("Scripting", "Scripting service initialized.");
}

void ScriptingService::Shutdown()
{
    SHE_LOG_INFO("Scripting", "Scripting service shutdown complete.");
    m_recentInvocations.clear();
    m_modules.clear();
    m_bindings.clear();
}

void ScriptingService::RegisterBinding(ScriptHostBindingDescriptor descriptor)
{
    if (descriptor.bindingName.empty())
    {
        SHE_LOG_WARNING("Scripting", "Ignored script binding registration because the binding name was empty.");
        return;
    }

    if (descriptor.owningService.empty())
    {
        descriptor.owningService = "Unknown";
    }

    if (ScriptHostBindingDescriptor* existing = FindBinding(descriptor.bindingName))
    {
        *existing = std::move(descriptor);
        return;
    }

    m_bindings.push_back(std::move(descriptor));
}

void ScriptingService::RegisterScriptModule(ScriptModuleContract module)
{
    if (module.moduleName.empty())
    {
        SHE_LOG_WARNING("Scripting", "Ignored script module registration because the module name was empty.");
        return;
    }

    if (module.language.empty())
    {
        module.language = "lua";
    }

    if (module.owningFeature.empty())
    {
        module.owningFeature = "Unknown";
    }

    ScriptModuleCatalogEntry entry;
    entry.contract = std::move(module);
    entry.loadState = ScriptModuleLoadState::Registered;
    entry.lastStatusMessage = "Module contract registered.";

    if (ScriptModuleCatalogEntry* existing = FindModule(entry.contract.moduleName))
    {
        *existing = std::move(entry);
        return;
    }

    m_modules.push_back(std::move(entry));
}

bool ScriptingService::HasBinding(const std::string_view bindingName) const
{
    return FindBinding(bindingName) != nullptr;
}

bool ScriptingService::HasScriptModule(const std::string_view moduleName) const
{
    return FindModule(moduleName) != nullptr;
}

ScriptModuleLoadResult ScriptingService::LoadScriptModule(const std::string_view moduleName)
{
    ScriptModuleCatalogEntry* module = FindModule(moduleName);
    if (module == nullptr)
    {
        return ScriptModuleLoadResult{
            std::string(moduleName),
            ScriptModuleLoadState::Blocked,
            false,
            {},
            "Script module is not registered."};
    }

    return ValidateModule(*module);
}

ScriptInvocationResult ScriptingService::InvokeScriptFunction(
    const std::string_view moduleName,
    const std::string_view functionName,
    std::string payload)
{
    ScriptInvocationResult result;
    result.moduleName = std::string(moduleName);
    result.functionName = std::string(functionName);
    result.payload = payload;

    ScriptModuleCatalogEntry* module = FindModule(moduleName);
    if (module == nullptr)
    {
        result.message = "Script module is not registered.";
        RecordInvocation(result);
        return result;
    }

    if (module->loadState != ScriptModuleLoadState::Loaded)
    {
        const ScriptModuleLoadResult loadResult = ValidateModule(*module);
        if (!loadResult.loaded)
        {
            result.message = loadResult.message;
            RecordInvocation(result);
            return result;
        }
    }

    const ScriptFunctionContract* function = FindFunction(*module, functionName);
    if (function == nullptr)
    {
        result.message = "Requested script function was not declared by the module contract.";
        module->lastStatusMessage = result.message;
        RecordInvocation(result);
        return result;
    }

    if (!function->routedCommandName.empty())
    {
        if (!m_gameplay)
        {
            result.message = "Scripting host cannot route the call because no gameplay service is attached.";
            module->lastStatusMessage = result.message;
            RecordInvocation(result);
            return result;
        }

        m_gameplay->QueueCommand(function->routedCommandName, result.payload);
        result.queuedGameplayCommand = true;
        result.queuedCommandName = function->routedCommandName;
        result.message = "Queued gameplay command via the scripting host.";
    }
    else
    {
        result.message = "Recorded script function call without gameplay command dispatch.";
    }

    result.accepted = true;
    ++module->invocationCount;
    module->lastStatusMessage = result.message;
    RecordInvocation(result);
    return result;
}

void ScriptingService::Update(const double deltaSeconds)
{
    m_lastDeltaSeconds = deltaSeconds;
    ++m_updateCount;

    for (auto& module : m_modules)
    {
        if (module.contract.autoLoad && module.loadState != ScriptModuleLoadState::Loaded)
        {
            ValidateModule(module);
        }
    }
}

std::size_t ScriptingService::GetBindingCount() const
{
    return m_bindings.size();
}

std::size_t ScriptingService::GetModuleCount() const
{
    return m_modules.size();
}

std::size_t ScriptingService::GetLoadedModuleCount() const
{
    return static_cast<std::size_t>(std::count_if(
        m_modules.begin(),
        m_modules.end(),
        [](const ScriptModuleCatalogEntry& module)
        {
            return module.loadState == ScriptModuleLoadState::Loaded;
        }));
}

std::vector<ScriptHostBindingDescriptor> ScriptingService::ListBindings() const
{
    return m_bindings;
}

std::vector<ScriptModuleCatalogEntry> ScriptingService::ListModules() const
{
    return m_modules;
}

std::string ScriptingService::BuildScriptCatalog() const
{
    std::ostringstream stream;
    stream << "script_host_contract_version: 1\n";
    stream << "binding_count: " << m_bindings.size() << '\n';
    stream << "registered_modules: " << m_modules.size() << '\n';
    stream << "loaded_modules: " << GetLoadedModuleCount() << '\n';
    stream << "update_count: " << m_updateCount << '\n';
    stream << "recent_invocation_count: " << m_recentInvocations.size() << '\n';
    stream << "last_delta: " << m_lastDeltaSeconds << '\n';

    for (std::size_t index = 0; index < m_bindings.size(); ++index)
    {
        const auto& binding = m_bindings[index];
        stream << "\n[binding_" << index << "]\n";
        stream << "binding_name: " << binding.bindingName << '\n';
        stream << "owning_service: " << binding.owningService << '\n';
        stream << "description: " << (binding.description.empty() ? "<none>" : binding.description) << '\n';
    }

    for (std::size_t index = 0; index < m_modules.size(); ++index)
    {
        const auto& module = m_modules[index];
        stream << "\n[module_" << index << "]\n";
        stream << "module_name: " << module.contract.moduleName << '\n';
        stream << "source_path: " << module.contract.sourcePath << '\n';
        stream << "language: " << module.contract.language << '\n';
        stream << "owning_feature: " << module.contract.owningFeature << '\n';
        stream << "description: " << (module.contract.description.empty() ? "<none>" : module.contract.description) << '\n';
        stream << "auto_load: " << (module.contract.autoLoad ? "true" : "false") << '\n';
        stream << "load_state: " << DescribeLoadState(module.loadState) << '\n';
        stream << "required_bindings: " << JoinValues(module.contract.requiredBindings) << '\n';
        stream << "exported_function_count: " << module.contract.exportedFunctions.size() << '\n';
        stream << "invocation_count: " << module.invocationCount << '\n';
        stream << "last_status: " << (module.lastStatusMessage.empty() ? "<none>" : module.lastStatusMessage) << '\n';

        for (std::size_t functionIndex = 0; functionIndex < module.contract.exportedFunctions.size(); ++functionIndex)
        {
            const auto& function = module.contract.exportedFunctions[functionIndex];
            stream << "\n[module_" << index << "_function_" << functionIndex << "]\n";
            stream << "function_name: " << function.functionName << '\n';
            stream << "description: " << (function.description.empty() ? "<none>" : function.description) << '\n';
            stream << "routed_command_name: "
                   << (function.routedCommandName.empty() ? "<none>" : function.routedCommandName) << '\n';
        }
    }

    for (std::size_t index = 0; index < m_recentInvocations.size(); ++index)
    {
        const auto& invocation = m_recentInvocations[index];
        stream << "\n[invocation_" << index << "]\n";
        stream << "module_name: " << invocation.moduleName << '\n';
        stream << "function_name: " << invocation.functionName << '\n';
        stream << "accepted: " << (invocation.accepted ? "true" : "false") << '\n';
        stream << "queued_gameplay_command: " << (invocation.queuedGameplayCommand ? "true" : "false") << '\n';
        stream << "queued_command_name: "
               << (invocation.queuedCommandName.empty() ? "<none>" : invocation.queuedCommandName) << '\n';
        stream << "payload: " << (invocation.payload.empty() ? "<empty>" : invocation.payload) << '\n';
        stream << "status: " << (invocation.statusMessage.empty() ? "<none>" : invocation.statusMessage) << '\n';
    }

    return stream.str();
}

ScriptModuleCatalogEntry* ScriptingService::FindModule(const std::string_view moduleName)
{
    const auto it = std::find_if(
        m_modules.begin(),
        m_modules.end(),
        [moduleName](const ScriptModuleCatalogEntry& module)
        {
            return module.contract.moduleName == moduleName;
        });
    return it == m_modules.end() ? nullptr : &(*it);
}

const ScriptModuleCatalogEntry* ScriptingService::FindModule(const std::string_view moduleName) const
{
    const auto it = std::find_if(
        m_modules.begin(),
        m_modules.end(),
        [moduleName](const ScriptModuleCatalogEntry& module)
        {
            return module.contract.moduleName == moduleName;
        });
    return it == m_modules.end() ? nullptr : &(*it);
}

ScriptHostBindingDescriptor* ScriptingService::FindBinding(const std::string_view bindingName)
{
    const auto it = std::find_if(
        m_bindings.begin(),
        m_bindings.end(),
        [bindingName](const ScriptHostBindingDescriptor& binding)
        {
            return binding.bindingName == bindingName;
        });
    return it == m_bindings.end() ? nullptr : &(*it);
}

const ScriptHostBindingDescriptor* ScriptingService::FindBinding(const std::string_view bindingName) const
{
    const auto it = std::find_if(
        m_bindings.begin(),
        m_bindings.end(),
        [bindingName](const ScriptHostBindingDescriptor& binding)
        {
            return binding.bindingName == bindingName;
        });
    return it == m_bindings.end() ? nullptr : &(*it);
}

ScriptModuleLoadResult ScriptingService::ValidateModule(ScriptModuleCatalogEntry& module)
{
    ScriptModuleLoadResult result;
    result.moduleName = module.contract.moduleName;

    if (module.contract.moduleName.empty())
    {
        result.state = ScriptModuleLoadState::Blocked;
        result.message = "Script module names must not be empty.";
    }
    else if (module.contract.sourcePath.empty())
    {
        result.state = ScriptModuleLoadState::Blocked;
        result.message = "Script modules must declare a stable source path.";
    }
    else if (module.contract.language == "lua" && !EndsWith(module.contract.sourcePath, ".lua"))
    {
        result.state = ScriptModuleLoadState::Blocked;
        result.message = "Lua-facing script modules must point at a .lua source path.";
    }
    else if (!ContainsScriptDirectoryMarker(module.contract.sourcePath))
    {
        result.state = ScriptModuleLoadState::Blocked;
        result.message = "Script source paths should live under a Scripts/ directory.";
    }
    else
    {
        for (const std::string& bindingName : module.contract.requiredBindings)
        {
            if (!HasBinding(bindingName))
            {
                result.missingBindings.push_back(bindingName);
            }
        }

        if (!result.missingBindings.empty())
        {
            result.state = ScriptModuleLoadState::Blocked;
            result.message = "Script module is waiting on required host bindings.";
        }
        else
        {
            for (const auto& function : module.contract.exportedFunctions)
            {
                if (function.functionName.empty())
                {
                    result.state = ScriptModuleLoadState::Blocked;
                    result.message = "Script modules cannot export unnamed functions.";
                    break;
                }
            }
        }
    }

    if (result.state != ScriptModuleLoadState::Blocked)
    {
        result.state = ScriptModuleLoadState::Loaded;
        result.loaded = true;
        result.message = "Script module loaded through the stable host boundary.";
    }

    module.loadState = result.state;
    module.lastStatusMessage = result.message;
    return result;
}

const ScriptFunctionContract* ScriptingService::FindFunction(
    const ScriptModuleCatalogEntry& module,
    const std::string_view functionName)
{
    const auto it = std::find_if(
        module.contract.exportedFunctions.begin(),
        module.contract.exportedFunctions.end(),
        [functionName](const ScriptFunctionContract& function)
        {
            return function.functionName == functionName;
        });
    return it == module.contract.exportedFunctions.end() ? nullptr : &(*it);
}

void ScriptingService::RecordInvocation(const ScriptInvocationResult& result)
{
    m_recentInvocations.push_back(
        ScriptInvocationRecord{
            result.moduleName,
            result.functionName,
            result.payload,
            result.accepted,
            result.queuedGameplayCommand,
            result.queuedCommandName,
            result.message});

    if (m_recentInvocations.size() > kMaxRecentInvocations)
    {
        m_recentInvocations.erase(m_recentInvocations.begin());
    }
}
} // namespace she
