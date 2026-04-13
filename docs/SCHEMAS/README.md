# Schemas

Gameplay schemas should live close to the features that own them, but this
folder can hold cross-feature schema notes or generated documentation later.

Current feature-local examples:

- [bootstrap_enemy.schema.yml](../../Game/Features/Bootstrap/Data/bootstrap_enemy.schema.yml)
- [bootstrap_encounter.schema.yml](../../Game/Features/Bootstrap/Data/bootstrap_encounter.schema.yml)

Current Data Core contract expectations:

- schema contracts should declare `schema`, `description`, `owner`, and
  `fields`
- each field contract should declare `name`, `kind`, `required`, and an
  optional `description`
- the first YAML loader validates top-level mapping fields against the declared
  contract and currently supports `scalar`, `list`, and `map` field kinds
