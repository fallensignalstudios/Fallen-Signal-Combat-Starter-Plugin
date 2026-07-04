GAS Combat Starter -- Content Folder
=====================================

This folder is intentionally empty in the source scaffold.

The GAS Combat Starter plugin ships as C++ only -- it deliberately contains no
pre-built Blueprint assets (no example GameplayEffect Blueprints, no example
Character Blueprint, no example Input Actions/Mapping Contexts, no example
UGCSCombatConfig data assets).

Why no example content?
------------------------
This plugin is sold as a clean baseline, not a demo project. Client teams
consistently prefer a C++-only starting point they extend with their own art,
animation, and Blueprint conventions, rather than having to strip out or
rework placeholder content that doesn't match their project's structure.

What you need to create in YOUR project (not this plugin):
------------------------------------------------------------
1. GameplayEffect Blueprints:
   - A damage GE reparented to UGCSEffect_DamageBase (see the extensive
     comment block in Source/GASCombatStarter/Public/Effects/GCSEffect_DamageBase.h
     for the exact setup steps, including how to write the accompanying
     UGameplayEffectExecutionCalculation).
   - A "Blocking" state GE (Duration/Infinite) that grants GCS.State.Blocking
     and applies your incoming-damage mitigation.
   - Starting-attributes GEs if you want per-archetype stat variation beyond
     the AttributeSet's C++ defaults.

2. GameplayAbility Blueprints:
   - Child Blueprints of UGCSAbility_MeleeAttack, UGCSAbility_Projectile, and
     UGCSAbility_Block with your trace/projectile/damage parameters set, and
     AbilityTags/ActivationOwnedTags configured to match
     Config/DefaultGASCombatStarterTags.ini (e.g. GCS.Ability.Melee,
     GCS.Input.PrimaryAttack as an Ability Trigger via Input Tag).

3. Enhanced Input assets:
   - An Input Mapping Context and Input Actions for PrimaryAttack/Block/Dodge,
     assigned to the corresponding properties on your AGCSCharacterBase
     Blueprint subclass.

4. A Character Blueprint:
   - Subclass AGCSCharacterBase, assign a mesh/animation blueprint, and set
     DefaultCombatConfig to a UGCSCombatConfig data asset listing your ability
     Blueprints.

5. A UGCSCombatConfig data asset (or several, one per archetype):
   - Lists DefaultAbilities/DefaultEffects and tuning values (parry window,
     stagger duration, critical hit threshold).

This keeps the plugin's compiled footprint minimal and avoids shipping
opinions about art/animation pipelines that vary per client project.
