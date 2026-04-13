# W04 Handoff - Scripting Host Checkpoint

## 1. Summary Of What Changed

W04 promoted `Engine/Scripting` from a placeholder module catalog into a first
real script-host contract layer that gameplay features can register against
today without binding themselves to a concrete Lua middleware API.

Delivered in this slice:

- stable script-host contract types in `RuntimeServices.hpp` for bindings,
  module descriptors, load results, invocation results, and catalog entries
- a richer `IScriptingService` that supports:
  - host binding registration
  - script module registration through structured contracts
  - explicit module load validation
  - script function invocation routed into the accepted W01 gameplay command
    path
  - structured script catalog export for AI/context tooling
- a rebuilt `ScriptingService` implementation that:
  - validates stable source-path and binding conventions
  - tracks loaded vs blocked modules
  - records recent invocations
  - routes declared script functions into `IGameplayService`
- bootstrap feature integration that now registers a real script module
  contract, loads it through the host, and invokes it for encounter spawns
  instead of queuing the gameplay command directly
- sandbox placeholder alignment so the sandbox also uses the new script module
  contract shape
- focused scripting-host tests plus bootstrap registration tests
- smoke coverage updates so AI context and runtime validation see the new
  scripting surface

This slice intentionally stops short of embedding a real Lua VM. The accepted
boundary is the host contract and routing behavior. Future Lua integration
should plug into this surface rather than forcing gameplay/features to adopt a
different API.

## 2. Changed Files

- `Engine/AI/Source/AuthoringAiService.cpp`
- `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`
- `Engine/Scripting/Include/SHE/Scripting/ScriptingService.hpp`
- `Engine/Scripting/Source/ScriptingService.cpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.hpp`
- `Game/Features/Bootstrap/BootstrapFeatureLayer.cpp`
- `Game/Features/Bootstrap/Scripts/spawn_rules.lua`
- `Game/Source/Main.cpp`
- `Tools/Sandbox/Source/Main.cpp`
- `Tools/Sandbox/Source/SandboxLayer.cpp`
- `Tools/Sandbox/Scripts/debug_tools.lua`
- `Tests/CMakeLists.txt`
- `Tests/Source/SmokeTests.cpp`
- `Tests/Source/ScriptingHostTests.cpp`
- `Tests/Source/BootstrapScriptRegistrationTests.cpp`
- `Tests/Source/Scripts/tests_smoke_module.lua`

## 3. Changed Interfaces

Shared runtime interface changed:

- `IScriptingService` in `Engine/Core/Include/SHE/Core/RuntimeServices.hpp`

New public scripting contract types:

- `ScriptModuleLoadState`
- `ScriptHostBindingDescriptor`
- `ScriptFunctionContract`
- `ScriptModuleContract`
- `ScriptModuleLoadResult`
- `ScriptInvocationResult`
- `ScriptModuleCatalogEntry`

New or changed `IScriptingService` methods:

- `RegisterBinding(ScriptHostBindingDescriptor)`
- `RegisterScriptModule(ScriptModuleContract)`
- `HasBinding(std::string_view)`
- `HasScriptModule(std::string_view)`
- `LoadScriptModule(std::string_view)`
- `InvokeScriptFunction(std::string_view, std::string_view, std::string payload)`
- `GetBindingCount()`
- `GetLoadedModuleCount()`
- `ListBindings()`
- `ListModules()`
- `BuildScriptCatalog()`

Compatibility note:

- the old `RegisterScriptModule(std::string moduleName, std::string description)`
  placeholder path is intentionally removed so downstream work cannot keep
  depending on an underspecified contract

## 4. Script Host Rules Landed

1. Features register script modules through `ScriptModuleContract`, not by
   inventing ad hoc runtime calls.
2. Lua-facing modules should point at a stable `.lua` source path under a
   `Scripts/` directory.
3. Script-facing engine access is declared through named host bindings such as
   `gameplay.queue_command`; missing bindings block module load instead of
   silently degrading.
4. Script function execution currently means:
   - validate that the module is registered and loadable
   - validate that the function is declared by the contract
   - optionally route the call into a declared gameplay command
5. Gameplay behavior still flows through W01:
   script-driven gameplay must queue accepted gameplay commands rather than
   bypassing `IGameplayService`.
6. AI/context tooling should consume `BuildScriptCatalog()` and the scripting
   counts exposed in `AuthoringAiService`, not scrape feature code for script
   assumptions.

## 5. Bootstrap Flow Now Covered

Bootstrap now demonstrates the intended W04 authoring path:

1. register gameplay/data/reflection metadata as before
2. register host bindings with `IScriptingService`
3. register `bootstrap.spawn_rules` as a structured module contract backed by
   `Game/Features/Bootstrap/Scripts/spawn_rules.lua`
4. load that module through the host boundary
5. invoke `request_spawn`
6. route the result into the accepted `SpawnEncounter` gameplay command/event
   path

That gives W12 and future feature work a real reference example for
feature-owned script modules without hard-wiring a final Lua backend yet.

## 6. Tests Run And Results

Verified in the W04 worktree:

- `cmake --preset default`
- `cmake --build --preset build-default`
- `ctest --preset test-default --output-on-failure`

Result:

- 11/11 tests passed
- `she_smoke_tests` passed
- `she_gameplay_contract_tests` passed
- `she_data_contract_tests` passed
- `she_platform_input_tests` passed
- `she_scene_contract_tests` passed
- `she_asset_contract_tests` passed
- `she_renderer_contract_tests` passed
- `she_physics_contract_tests` passed
- `she_audio_contract_tests` passed
- `she_scripting_host_tests` passed
- `she_bootstrap_script_tests` passed

## 7. Unresolved Risks / Intentional Gaps

1. This is a host-contract slice, not a final Lua runtime integration slice.
   The `.lua` files are stable authored assets and validation targets today,
   not yet executed by an embedded interpreter.
2. The current invocation path routes into gameplay commands only when a
   function contract declares a `routedCommandName`. Richer return values,
   typed bindings, and native Lua call frames are future extensions.
3. Source-path validation is intentionally convention-based right now:
   `.lua` file extension plus `Scripts/` directory ownership. It does not yet
   guarantee file existence on disk at load time.
4. `RuntimeServices.hpp` changed, so W00 should treat this as a shared contract
   integration rather than a purely local module drop-in.

## 8. Exact Next Steps For The Next Codex

1. Report this handoff to W00 and mark W04 as `ready_for_integration`.
2. Integrate this slice on top of main so downstream feature work can depend on
   `ScriptModuleContract` and `InvokeScriptFunction(...)` instead of inventing
   temporary scripting shims.
3. If a future workstream adds a real Lua backend, keep gameplay/features on
   the current `IScriptingService` contract and swap implementation details
   underneath it.
4. Let W12 author gameplay script modules by declaring bindings and routed
   commands first; avoid direct feature-side interpreter calls.

## 9. Status Recommendation

`ready_for_integration`
