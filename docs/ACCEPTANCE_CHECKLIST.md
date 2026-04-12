# Acceptance Checklist

Use this checklist for every workstream before integration.

## 1. Universal Checklist

- [ ] module ownership is clear
- [ ] changed files stay inside the intended workstream boundary
- [ ] comments explain non-obvious boundaries or lifecycle rules
- [ ] architecture docs were updated if contracts changed
- [ ] tests were added or updated for risky behavior
- [ ] build/configure still succeeds

## 2. Engine Module Checklist

- [ ] the module has a clear public interface
- [ ] the module does not create forbidden dependency directions
- [ ] the module has a minimal smoke/integration test path
- [ ] the module documents future replacement points if still a bootstrap implementation

## 3. Gameplay / Feature Checklist

- [ ] feature lives in `Game/Features/<FeatureName>/`
- [ ] feature registers metadata through `IReflectionService`
- [ ] feature registers schemas through `IDataService` when data contracts change
- [ ] feature uses `IGameplayService` for commands/events/timers when applicable
- [ ] feature updates AI-visible context through standard engine contracts rather than hidden side channels

## 4. AI-Native Checklist

- [ ] the change is visible or explainable through authoring context export
- [ ] diagnostics can tell the story of what changed
- [ ] new contracts are discoverable by Codex via docs, metadata, or schemas
- [ ] the implementation does not require Codex to guess hidden conventions

## 5. Handoff Checklist

- [ ] handoff note exists
- [ ] changed files are listed
- [ ] tests run are listed
- [ ] known risks are listed
- [ ] next recommended action is listed
- [ ] task board and status ledger were updated

