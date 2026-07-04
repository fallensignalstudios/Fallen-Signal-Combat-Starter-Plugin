# Changelog

All notable changes to **GASCombatStarter** are documented here.  
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).  
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

_Changes staged for the next release._

---

## [0.1.0] – 2026-07-04

### Added

#### Core Systems
- `UGCSCombatAttributeSet` — 12 combat attributes (Health, MaxHealth, Shield, MaxShield, Energy, MaxEnergy, Stamina, MaxStamina, AttackPower, DefensePower, CritChance, CritMultiplier) plus `IncomingDamage` meta-attribute with Shield→Health absorption pipeline. Full replication and OnRep support.
- `UGCSAbilitySystemComponent` — extends `UAbilitySystemComponent` with `InitializeDefaultAbilities()`, `GrantAbility()`, `RemoveAbility()`, `GetActiveAbilitiesByTag()`, and `OnAbilityActivated`/`OnAbilityEnded` delegates. Blueprint callable.
- `UGCSGameplayAbility` — base ability class with `EGCSActivationPolicy` enum (OnInput, OnGranted, OnEvent), CooldownTag, CostTag, and Blueprint implementable `ActivateAbility`/`EndAbility` events.

#### Abilities
- `UGCSAbility_MeleeAttack` — configurable sphere or box sweep, damage GameplayEffect application, `GCS.Event.HitReact` GameplayEvent dispatch, `OnHitConfirmed` BlueprintNativeEvent.
- `UGCSAbility_Projectile` — configurable muzzle socket, projectile class, and launch speed. Spawns `AGCSProjectile` and applies damage on impact.
- `UGCSAbility_Block` — activates `GCS.State.Blocking` tag, applies damage-reduction GameplayEffect, opens configurable parry window, dispatches `GCS.Event.ParrySuccess` if attack lands during window.

#### Actors
- `AGCSProjectile` — replication-ready projectile actor with configurable speed and impact damage GameplayEffect.

#### Components
- `UGCSCombatComponent` — tracks blocking, parry window, combo index, last hit time, and stagger state. Full replication. Delegates: `OnBlockStart`, `OnParrySuccess`, `OnComboAdvance`, `OnStaggerBegin`, `OnStaggerEnd`. Blueprint callable.

#### Characters
- `AGCSCharacterBase` — `ACharacter` subclass integrating `UGCSAbilitySystemComponent`, `UGCSCombatAttributeSet`, and `UGCSCombatComponent`. Implements `IAbilitySystemInterface`. Correct `InitAbilityActorInfo` calls on both server (`PossessedBy`) and client (`OnRep_PlayerState` / `AcknowledgePossession`) paths. Enhanced Input binding stubs included.

#### Data
- `UGCSCombatConfig` — `UPrimaryDataAsset` for data-driven character initialization: default abilities, default effects, parry window duration, stagger duration, critical hit threshold.
- `GCSNativeGameplayTags` — compile-time safe native tag declarations mirroring the ini hierarchy.

#### Configuration
- `Config/DefaultGASCombatStarterTags.ini` — full `GCS.*` tag hierarchy across Ability, State, Event, Effect, Input, and Cooldown namespaces.

#### Build Infrastructure
- Runtime module (`GASCombatStarter`) with `AbilitySystemGlobals` initialization.
- Editor module (`GASCombatStarterEditor`) — minimal, reserved for future editor utilities.
- Both `Build.cs` files with correct public dependencies (Core, CoreUObject, Engine, GameplayAbilities, GameplayTags, EnhancedInput).

---

## Version Policy

| Segment | When it increments |
|---|---|
| **Major** (1.x.x) | Breaking API changes — existing client projects require migration work. |
| **Minor** (x.1.x) | New systems or classes added in a backward-compatible way. |
| **Patch** (x.x.1) | Bug fixes, documentation updates, non-breaking tweaks. |

Licensed clients are notified of all Minor and Major releases. Patch notes are available on request.

---

_Copyright Fallen Signal Studios. All Rights Reserved._
