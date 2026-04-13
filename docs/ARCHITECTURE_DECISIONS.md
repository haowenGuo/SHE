# Architecture Decisions

This file records architecture choices that should not live only in chat
history.

## ADR-001: The engine is AI-native, not AI-added

Decision:

- the engine should expose authoring context through first-class runtime
  services, diagnostics, metadata, and data schemas

Reason:

- Codex needs stable, discoverable context to modify gameplay reliably

## ADR-002: Gameplay is organized by feature, not by one large source folder

Decision:

- gameplay modules live in `Game/Features/<FeatureName>/`

Reason:

- parallel development is safer
- AI-assisted editing works better with small, explicit boundaries

## ADR-003: Gameplay uses commands, events, and timers as shared primitives

Decision:

- gameplay infrastructure belongs in `Engine/Gameplay`

Reason:

- future features should compose on common primitives rather than invent their
  own flow systems

## ADR-004: Gameplay data is schema-first

Decision:

- feature data should be defined through schema registration and later YAML
  validation

Reason:

- this keeps gameplay data explicit and machine-readable

## ADR-005: AI context is read-only with respect to simulation

Decision:

- `IAIService` exports context, but does not directly mutate the deterministic
  simulation path

Reason:

- deterministic runtime behavior should remain under explicit engine/gameplay
  control

## ADR-006: Multi-Codex development should prefer workstream ownership and
separate worktrees

Decision:

- parallel work should be sliced by workstream and tracked through shared
  coordination documents

Reason:

- this reduces merge conflict risk and improves handoff quality

## ADR-007: Gameplay public contracts are the shared runtime gameplay baseline

Decision:

- downstream systems should treat the `IGameplayService` public surface in
  `RuntimeServices.hpp` as the stable gameplay entry point
- gameplay-triggering integrations should route through the shared
  command/event/timer path instead of inventing private dispatch channels
- diagnostics and AI context may summarize gameplay activity, but they should
  observe the gameplay contract rather than bypass it

Reason:

- scripting, physics, audio, and diagnostics all need one visible story for how
  gameplay activity enters and moves through the engine
- a shared gameplay baseline keeps runtime behavior explainable to future Codex
  sessions and reduces hidden coupling between workstreams

