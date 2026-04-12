#include "SHE/Scripting/ScriptingService.hpp"

#include "SHE/Core/Logger.hpp"

#include <sstream>
#include <utility>

namespace she
{
void ScriptingService::Initialize()
{
    m_modules.clear();
    m_lastDeltaSeconds = 0.0;
    SHE_LOG_INFO("Scripting", "Scripting service initialized.");
}

void ScriptingService::Shutdown()
{
    SHE_LOG_INFO("Scripting", "Scripting service shutdown complete.");
    m_modules.clear();
}

void ScriptingService::RegisterScriptModule(std::string moduleName, std::string description)
{
    m_modules.push_back(ScriptModuleDescriptor{std::move(moduleName), std::move(description)});
}

void ScriptingService::Update(const double deltaSeconds)
{
    m_lastDeltaSeconds = deltaSeconds;
}

std::size_t ScriptingService::GetModuleCount() const
{
    return m_modules.size();
}

std::string ScriptingService::BuildScriptCatalog() const
{
    std::ostringstream stream;
    stream << "script_modules=" << m_modules.size() << ", last_delta=" << m_lastDeltaSeconds << '\n';
    for (const auto& module : m_modules)
    {
        stream << "- " << module.moduleName << ": " << module.description << '\n';
    }

    return stream.str();
}
} // namespace she

