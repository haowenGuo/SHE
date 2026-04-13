#include "SHE/Gameplay/GameplayService.hpp"
#include "SHE/Scripting/ScriptingService.hpp"

#include <iostream>
#include <memory>
#include <string_view>

namespace
{
bool Expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }

    return true;
}

bool ContainsText(const std::string& text, const std::string_view needle)
{
    return text.find(needle) != std::string::npos;
}

bool TestScriptHostBlocksUntilRequiredBindingsExist()
{
    auto gameplay = std::make_shared<she::GameplayService>();
    she::ScriptingService scripting(gameplay);

    gameplay->Initialize();
    scripting.Initialize();
    gameplay->RegisterCommand(
        {"SmokeScriptCommand", "Routes a script host command through gameplay.", "script", "SpawnRequested"});

    scripting.RegisterScriptModule(
        she::ScriptModuleContract{
            "tests.contract.spawn_rules",
            "Tests/Source/Scripts/tests_smoke_module.lua",
            "lua",
            "Tests/Source",
            "Contract test module for the scripting host.",
            true,
            {"gameplay.queue_command"},
            {
                she::ScriptFunctionContract{
                    "request_spawn",
                    "Route the contract test spawn request into gameplay.",
                    "SmokeScriptCommand"},
            }});

    const she::ScriptModuleLoadResult blockedLoad = scripting.LoadScriptModule("tests.contract.spawn_rules");

    bool passed = true;
    passed &= Expect(!blockedLoad.loaded, "Script module should stay blocked until required bindings exist.");
    passed &= Expect(
        blockedLoad.state == she::ScriptModuleLoadState::Blocked,
        "Blocked script module should report the Blocked load state.");
    passed &= Expect(
        blockedLoad.missingBindings.size() == 1 && blockedLoad.missingBindings.front() == "gameplay.queue_command",
        "Blocked script module should report the missing gameplay binding.");

    scripting.RegisterBinding(
        {"gameplay.queue_command", "GameplayService", "Allow script entry points to queue gameplay commands."});
    const she::ScriptModuleLoadResult loaded = scripting.LoadScriptModule("tests.contract.spawn_rules");

    passed &= Expect(loaded.loaded, "Script module should load once the required binding is registered.");
    passed &= Expect(
        scripting.GetLoadedModuleCount() == 1,
        "Loaded module count should reflect the successfully loaded contract.");

    scripting.Shutdown();
    gameplay->Shutdown();
    return passed;
}

bool TestScriptInvocationRoutesThroughGameplayCommands()
{
    auto gameplay = std::make_shared<she::GameplayService>();
    she::ScriptingService scripting(gameplay);

    gameplay->Initialize();
    scripting.Initialize();

    gameplay->RegisterCommand(
        {"SmokeScriptCommand", "Routes a script host command through gameplay.", "script", "SpawnRequested"});
    scripting.RegisterBinding(
        {"gameplay.queue_command", "GameplayService", "Allow script entry points to queue gameplay commands."});
    scripting.RegisterBinding(
        {"data.describe_registry", "DataService", "Expose authored data summaries to script contracts."});
    scripting.RegisterScriptModule(
        she::ScriptModuleContract{
            "tests.contract.spawn_rules",
            "Tests/Source/Scripts/tests_smoke_module.lua",
            "lua",
            "Tests/Source",
            "Contract test module for the scripting host.",
            true,
            {"gameplay.queue_command", "data.describe_registry"},
            {
                she::ScriptFunctionContract{
                    "request_spawn",
                    "Route the contract test spawn request into gameplay.",
                    "SmokeScriptCommand"},
            }});

    const she::ScriptModuleLoadResult loadResult = scripting.LoadScriptModule("tests.contract.spawn_rules");

    std::size_t routedEvents = 0;
    std::string lastPayload;
    const std::size_t subscriptionId = gameplay->SubscribeToEvent(
        "script",
        "SpawnRequested",
        [&routedEvents, &lastPayload](const she::GameplayEvent& event)
        {
            ++routedEvents;
            lastPayload = event.payload;
        });

    gameplay->BeginFrame(5);
    const she::ScriptInvocationResult invocationResult = scripting.InvokeScriptFunction(
        "tests.contract.spawn_rules",
        "request_spawn",
        "encounter=alpha; enemy=training_dummy");
    gameplay->FlushCommands();

    const std::string catalog = scripting.BuildScriptCatalog();

    bool passed = true;
    passed &= Expect(loadResult.loaded, "Expected contract test module to load before invocation.");
    passed &= Expect(invocationResult.accepted, "Expected script function invocation to be accepted.");
    passed &= Expect(
        invocationResult.queuedGameplayCommand && invocationResult.queuedCommandName == "SmokeScriptCommand",
        "Expected script function invocation to queue the declared gameplay command.");
    passed &= Expect(routedEvents == 1, "Expected exactly one gameplay event routed from the script call.");
    passed &= Expect(
        lastPayload == "encounter=alpha; enemy=training_dummy",
        "Expected gameplay payload to stay intact when routed through the scripting host.");
    passed &= Expect(ContainsText(catalog, "script_host_contract_version: 1"), "Expected script catalog to report its contract version.");
    passed &= Expect(ContainsText(catalog, "loaded_modules: 1"), "Expected script catalog to report the loaded module count.");
    passed &= Expect(
        ContainsText(catalog, "routed_command_name: SmokeScriptCommand"),
        "Expected script catalog to include the routed gameplay command.");
    passed &= Expect(
        ContainsText(catalog, "queued_gameplay_command: true"),
        "Expected script catalog to record the successful gameplay dispatch.");

    gameplay->UnsubscribeFromEvent(subscriptionId);
    scripting.Shutdown();
    gameplay->Shutdown();
    return passed;
}
} // namespace

int main()
{
    bool success = true;
    success = TestScriptHostBlocksUntilRequiredBindingsExist() && success;
    success = TestScriptInvocationRoutesThroughGameplayCommands() && success;
    return success ? 0 : 1;
}
