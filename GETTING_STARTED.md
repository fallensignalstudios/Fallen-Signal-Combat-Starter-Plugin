# GASCombatStarter – Getting Started Guide

**Version:** 0.1.0  
**Engine:** Unreal Engine 5.4+  
**License:** Private — Fallen Signal Studios

---

## Overview

GASCombatStarter is a clean, focused baseline plugin for building GAS-driven combat in Unreal Engine 5. It is not a full framework — it is a set of well-structured, heavily commented starting points designed to be read, understood, and extended by your team.

The plugin gives you:

- A combat `AttributeSet` with a Shield→Health damage absorption pipeline
- A base `AbilitySystemComponent` with convenience wrappers and delegates
- Three ready-to-extend ability classes: melee, projectile, and block/parry
- A `CombatComponent` that tracks blocking, parry windows, combos, and stagger
- A `CharacterBase` that wires everything together correctly on both server and client
- A data-driven `CombatConfig` asset for initializing characters from data
- A complete `GCS.*` GameplayTag hierarchy

Everything is Blueprint-exposed. All combat logic is replicated from the start.

---

## Prerequisites

Before installing, confirm your project has the following plugins enabled in your `.uproject`:

```json
{ "Name": "GameplayAbilities", "Enabled": true },
{ "Name": "EnhancedInput",     "Enabled": true }
```

If you haven't used GAS before, Epic's [GAS documentation](https://docs.unrealengine.com/5.0/en-US/gameplay-ability-system-for-unreal-engine/) is the required pre-reading. This plugin assumes working GAS knowledge.

---

## Installation

### Step 1 – Copy the plugin

Copy the `GASCombatStarter/` folder into your project's `Plugins/` directory:

```
YourProject/
  Plugins/
    GASCombatStarter/       ← place it here
      GASCombatStarter.uplugin
      Source/
      Config/
      Content/
```

### Step 2 – Enable the plugin

Open your project in Unreal Editor. Go to **Edit → Plugins**, search for **GASCombatStarter**, and enable it. Restart when prompted.

Alternatively, add it to your `.uproject` manually:

```json
{
  "Name": "GASCombatStarter",
  "Enabled": true
}
```

### Step 3 – Add the module dependency

In your game module's `Build.cs`, add:

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "GASCombatStarter",
    "GameplayAbilities",
    "GameplayTags",
    "EnhancedInput"
});
```

### Step 4 – Initialize AbilitySystemGlobals

In your project's `DefaultGame.ini`, ensure this is present (the plugin's runtime module handles `InitGlobalData()`, but the ini entry is required):

```ini
[/Script/GameplayAbilities.AbilitySystemGlobals]
AbilitySystemGlobalsClassName="/Script/GameplayAbilities.AbilitySystemGlobals"
```

### Step 5 – Register the tag config

In `DefaultGame.ini`, add the plugin's tag config so the `GCS.*` tags are loaded:

```ini
[/Script/GameplayTags.GameplayTagsManager]
+GameplayTagTableList=(FilePath="/GASCombatStarter/Config/DefaultGASCombatStarterTags.ini")
```

Rebuild the project. You should see all `GCS.*` tags appear in the Gameplay Tag editor.

---

## Project Setup

### Option A – Subclass in C++ (recommended)

Subclass `AGCSCharacterBase` for your player and enemy characters:

```cpp
// MyPlayerCharacter.h
#include "Characters/GCSCharacterBase.h"

UCLASS()
class MYGAME_API AMyPlayerCharacter : public AGCSCharacterBase
{
    GENERATED_BODY()

public:
    AMyPlayerCharacter();

protected:
    virtual void BeginPlay() override;
};
```

```cpp
// MyPlayerCharacter.cpp
#include "MyPlayerCharacter.h"

AMyPlayerCharacter::AMyPlayerCharacter()
{
    // Your character-specific setup here.
    // ASC, AttributeSet, and CombatComponent are already initialized in the base class.
}

void AMyPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Apply your CombatConfig data asset here:
    // if (CombatConfig) { InitializeFromConfig(CombatConfig); }
}
```

### Option B – Subclass in Blueprint

Create a Blueprint subclass of `GCSCharacterBase`. All components and the ASC are already attached. Assign your `GCSCombatConfig` data asset in the Details panel.

---

## Creating Your First CombatConfig

`UGCSCombatConfig` is a `PrimaryDataAsset` that describes how a character is initialized.

1. In the Content Browser, right-click → **Miscellaneous → Data Asset**.
2. Select **GCSCombatConfig** as the class.
3. Name it (e.g. `DA_PlayerCombatConfig`).
4. Fill in:
   - **DefaultAbilities** — ability classes to grant on spawn (e.g. `BP_MeleeAttack_Player`)
   - **DefaultEffects** — GameplayEffects to apply on spawn (e.g. your `GE_InitStats` effect)
   - **ParryWindowDuration** — seconds the parry window stays open (start with `0.25`)
   - **StaggerDuration** — seconds a stagger state lasts (start with `0.6`)
   - **CriticalHitThreshold** — CritChance value at which a hit is always critical (e.g. `1.0`)
5. Assign this asset to your character's `CombatConfig` property.

---

## Creating Abilities

### Melee ability

1. Create a Blueprint subclass of `GCSAbility_MeleeAttack`.
2. In the Class Defaults, configure:
   - **SweepShape** — Sphere or Box
   - **SweepRadius** / **SweepExtent** — size of the hit detection volume
   - **DamageEffectClass** — your `GE_MeleeDamage` GameplayEffect (see below)
   - **ActivationPolicy** — `OnInput` for player abilities
3. Override `OnHitConfirmed` in the Blueprint Event Graph to add VFX, sound, and camera shake.
4. Add this class to your `CombatConfig → DefaultAbilities`.

### Block ability

1. Create a Blueprint subclass of `GCSAbility_Block`.
2. Set **ParryWindowDuration** (or leave it driven by `CombatConfig`).
3. Set **BlockDamageReductionEffect** — a `GE` that applies a damage modifier while blocking.
4. Bind the ability to your block input tag (`GCS.Input.Block`).

### Projectile ability

1. Create a Blueprint subclass of `GCSAbility_Projectile`.
2. Set **ProjectileClass** to your `AGCSProjectile` subclass (or the base class directly).
3. Set **MuzzleSocketName** to match your weapon mesh socket.
4. Set **LaunchSpeed**.

---

## Creating Damage GameplayEffects

The plugin does not ship example GameplayEffect assets — these must live in your project. Here is the standard setup for a melee damage effect:

1. Create a Blueprint subclass of `UGameplayEffect`. Name it `GE_MeleeDamage`.
2. **Duration Policy** → `Instant`.
3. **Modifiers** → Add one modifier:
   - **Attribute** → `GCSCombatAttributeSet.IncomingDamage`
   - **Modifier Op** → `Add`
   - **Magnitude Calculation** → `Scalable Float` or `Set by Caller` (recommended for variable damage)
4. Add the tag `GCS.Effect.Damage` to **Granted Tags**.
5. Assign this GE class to your ability's `DamageEffectClass`.

The `IncomingDamage` meta-attribute in `UGCSCombatAttributeSet` automatically routes damage through the Shield→Health pipeline in `PostGameplayEffectExecute`.

---

## Input Binding

`AGCSCharacterBase` includes Enhanced Input stubs. Wire your Input Actions to ability activation using the `GCS.Input.*` tags:

```cpp
// In your character's SetupPlayerInputComponent override:
// (example — adapt to your Input Action assets)

EnhancedInputComponent->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this,
    &AGCSCharacterBase::OnPrimaryAttackInput);

EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this,
    &AGCSCharacterBase::OnBlockInputStart);

EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this,
    &AGCSCharacterBase::OnBlockInputEnd);
```

Or use the ASC's `AbilityLocalInputPressed`/`Released` methods if you prefer the GAS input binding approach.

---

## Tag Reference

All plugin tags live under the `GCS.*` namespace.

| Namespace | Tags |
|---|---|
| `GCS.Ability.*` | `Melee`, `Projectile`, `Block` |
| `GCS.State.*` | `Blocking`, `Staggered`, `Dead`, `InParryWindow` |
| `GCS.Event.*` | `HitReact`, `ParrySuccess`, `ComboAdvance`, `Death` |
| `GCS.Effect.*` | `Damage`, `Heal`, `Stagger` |
| `GCS.Input.*` | `PrimaryAttack`, `Block`, `Dodge` |
| `GCS.Cooldown.*` | `Melee`, `Projectile`, `Block` |

Native C++ tag references are declared in `GCSNativeGameplayTags.h` — use these in C++ code instead of string literals:

```cpp
#include "GCSNativeGameplayTags.h"

// Example:
AbilitySystemComponent->AddLooseGameplayTag(GCSGameplayTags::State_Blocking);
```

---

## CombatComponent Reference

`UGCSCombatComponent` is automatically attached to `AGCSCharacterBase`. Access it anywhere with:

```cpp
UGCSCombatComponent* Combat = GetComponentByClass<UGCSCombatComponent>();
```

Key state queries (all Blueprint callable):

| Function | Returns |
|---|---|
| `IsBlocking()` | `bool` |
| `IsInParryWindow()` | `bool` |
| `GetCurrentComboIndex()` | `int32` |
| `IsStaggered()` | `bool` |

Key delegates (bind in C++ or Blueprint):

| Delegate | Fires when |
|---|---|
| `OnBlockStart` | Character begins blocking |
| `OnParrySuccess` | Attack landed during parry window |
| `OnComboAdvance` | Combo index increments |
| `OnStaggerBegin` | Stagger state activates |
| `OnStaggerEnd` | Stagger state clears |

---

## Multiplayer Considerations

All plugin classes are replication-ready out of the box:

- `UGCSAbilitySystemComponent` uses standard GAS replication (Mixed mode for player-controlled, Full for AI).
- `UGCSCombatComponent` replicates `bIsBlocking`, `bIsInParryWindow`, `CurrentComboIndex`, and stagger state.
- `AGCSCharacterBase` calls `InitAbilityActorInfo` on both the server (`PossessedBy`) and the client (`OnRep_PlayerState`/`AcknowledgePossession`) — this is the correct pattern for player-state-owned ASCs.
- For AI characters (no PlayerState), `InitAbilityActorInfo` is called with the character as both Owner and Avatar in `BeginPlay`.

If you need a dedicated server build, no additional configuration is required — GAS handles ability prediction automatically.

---

## Common Issues

**Tags not showing in the editor**  
Confirm `DefaultGASCombatStarterTags.ini` is registered in `DefaultGame.ini` and that you have rebuilt the project after enabling the plugin.

**Abilities not activating**  
Check that `InitializeDefaultAbilities()` is being called (it is called automatically in `AGCSCharacterBase::BeginPlay` on the server). Verify the ability is in your `CombatConfig → DefaultAbilities` array and the `ActivationPolicy` matches how you are triggering it.

**Damage not applying**  
Confirm your `GE_*` damage effect targets `GCSCombatAttributeSet.IncomingDamage`, not `Health` directly. The attribute set's `PostGameplayEffectExecute` handles the routing.

**Parry window not triggering**  
The block ability opens the parry window via the `CombatComponent`. Confirm `UGCSCombatComponent` is attached to your character and that the block ability's `CombatComponentClass` is set (or is the default `UGCSCombatComponent`).

---

## Support

Licensed clients receive direct support from Fallen Signal Studios.

For bug reports or implementation questions, contact your studio liaison or reach out directly at:  
**studio@fallensignal.com**

Please include your UE version, plugin version, a description of the issue, and any relevant log output.

---

_Copyright Fallen Signal Studios. All Rights Reserved. Unauthorized distribution prohibited._
