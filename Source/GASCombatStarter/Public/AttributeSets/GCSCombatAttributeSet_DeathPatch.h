// Copyright Fallen Signal Studios. All Rights Reserved.
//
// ============================================================================
// PATCH DOCUMENT -- NOT A COMPILED HEADER
// ============================================================================
//
// This file is NOT included anywhere and is NOT compiled as part of the
// module. It documents the exact edits required in the existing
// UGCSCombatAttributeSet::PostGameplayEffectExecute
// (Private/AttributeSets/GCSCombatAttributeSet.cpp) to trigger death once
// Health reaches 0. The snippet below is written against the actual current
// body of that function so it can be applied as a direct, minimal diff.
//
// ============================================================================
// Required additional include in GCSCombatAttributeSet.cpp
// ============================================================================
//
//     #include "Components/GCSDeathComponent.h"
//
// (GameplayTagContainer.h is already transitively available via
// AbilitySystemComponent.h, already included in GCSCombatAttributeSet.h.)
//
// ============================================================================
// Current function (for reference) -- Private/AttributeSets/GCSCombatAttributeSet.cpp
// ============================================================================
//
//     void UGCSCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
//     {
//         Super::PostGameplayEffectExecute(Data);
//
//         const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
//         AActor* Instigator = Context.GetOriginalInstigator();
//         AActor* Target = Data.Target.GetAvatarActor();
//
//         if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
//         {
//             const float DamageDone = GetIncomingDamage();
//             SetIncomingDamage(0.f);
//
//             if (DamageDone > 0.f)
//             {
//                 const float CurrentShield = GetShield();
//                 const float DamageAfterShield = FMath::Max(0.f, DamageDone - CurrentShield);
//                 const float ShieldAbsorbed = DamageDone - DamageAfterShield;
//
//                 if (ShieldAbsorbed > 0.f)
//                 {
//                     SetShield(FMath::Clamp(CurrentShield - ShieldAbsorbed, 0.f, GetMaxShield()));
//                 }
//
//                 if (DamageAfterShield > 0.f)
//                 {
//                     const float NewHealth = FMath::Clamp(GetHealth() - DamageAfterShield, 0.f, GetMaxHealth());
//                     SetHealth(NewHealth);
//
//                     UE_LOG(LogGASCombatStarter, Verbose, TEXT("%s took %.1f damage from %s. Health: %.1f/%.1f"),
//                         Target ? *Target->GetName() : TEXT("Unknown"),
//                         DamageAfterShield,
//                         Instigator ? *Instigator->GetName() : TEXT("Unknown"),
//                         GetHealth(), GetMaxHealth());
//
//                     // NOTE: Death detection belongs in the owning ability system component
//                     // or character (e.g. bind to Health attribute change and check <= 0),
//                     // so that death handling (ragdoll, GCS.State.Dead tag, ability removal)
//                     // stays game-specific and out of the attribute set itself.
//                 }
//             }
//         }
//     }
//
// ============================================================================
// Patched function -- ADD the highlighted block, replacing the old NOTE comment
// ============================================================================
//
//     void UGCSCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
//     {
//         Super::PostGameplayEffectExecute(Data);
//
//         const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
//         AActor* Instigator = Context.GetOriginalInstigator();
//         AActor* Causer = Context.GetEffectCauser();
//         AActor* Target = Data.Target.GetAvatarActor();
//
//         if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
//         {
//             const float DamageDone = GetIncomingDamage();
//             SetIncomingDamage(0.f);
//
//             if (DamageDone > 0.f)
//             {
//                 const float CurrentShield = GetShield();
//                 const float DamageAfterShield = FMath::Max(0.f, DamageDone - CurrentShield);
//                 const float ShieldAbsorbed = DamageDone - DamageAfterShield;
//
//                 if (ShieldAbsorbed > 0.f)
//                 {
//                     SetShield(FMath::Clamp(CurrentShield - ShieldAbsorbed, 0.f, GetMaxShield()));
//                 }
//
//                 if (DamageAfterShield > 0.f)
//                 {
//                     const float NewHealth = FMath::Clamp(GetHealth() - DamageAfterShield, 0.f, GetMaxHealth());
//                     SetHealth(NewHealth);
//
//                     UE_LOG(LogGASCombatStarter, Verbose, TEXT("%s took %.1f damage from %s. Health: %.1f/%.1f"),
//                         Target ? *Target->GetName() : TEXT("Unknown"),
//                         DamageAfterShield,
//                         Instigator ? *Instigator->GetName() : TEXT("Unknown"),
//                         GetHealth(), GetMaxHealth());
//
//                     // =========================================================
//                     // >>> DEATH TRIGGER -- ADD THIS BLOCK <<<
//                     // =========================================================
//                     // Runs immediately after Health has been clamped to its
//                     // final post-mitigation value for this execution. Still
//                     // inside "if (DamageAfterShield > 0.f)" so it only
//                     // evaluates when Health actually took damage this frame
//                     // (pure heals or shield-only hits never reach here).
//                     if (GetHealth() <= 0.f && Target)
//                     {
//                         // Resolve the tag once. In production, prefer a cached
//                         // FGameplayTag from a central tag manifest (e.g.
//                         // UGCSNativeGameplayTags, already present in this
//                         // module) instead of RequestGameplayTag per call.
//                         static const FGameplayTag DeadStateTag =
//                             FGameplayTag::RequestGameplayTag(FName("GCS.State.Dead"));
//
//                         UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();
//                         const bool bAlreadyDead = TargetASC && TargetASC->HasMatchingGameplayTag(DeadStateTag);
//
//                         if (!bAlreadyDead)
//                         {
//                             // FindComponentByClass keeps this attribute set
//                             // decoupled from AGCSCharacterBase specifically --
//                             // any actor exposing a UGCSDeathComponent works,
//                             // not just characters (e.g. destructible turrets).
//                             if (UGCSDeathComponent* DeathComp = Target->FindComponentByClass<UGCSDeathComponent>())
//                             {
//                                 // Prefer the original instigator (e.g. the
//                                 // player who fired the shot) and fall back to
//                                 // the effect causer (e.g. the projectile actor)
//                                 // if no instigator is available.
//                                 AActor* Killer = Instigator ? Instigator : Causer;
//                                 DeathComp->HandleDeath(Killer);
//                             }
//                         }
//                     }
//                     // =========================================================
//                     // >>> END DEATH TRIGGER BLOCK <<<
//                     // =========================================================
//                 }
//             }
//         }
//     }
//
// ============================================================================
// Notes
// ============================================================================
//
// 1. Idempotency: UGCSDeathComponent::HandleDeath() already early-outs via its
//    own IsDead() check, so the `bAlreadyDead` guard here is a cheap, optional
//    fast-path optimization that avoids the FindComponentByClass lookup on
//    every subsequent damage tick against an already-dead target (e.g. a
//    damage-over-time effect that ticks once more right after death). It is
//    not required for correctness -- HandleDeath is always safe to call again.
//
// 2. Server authority: PostGameplayEffectExecute only runs where the
//    GameplayEffect is applied authoritatively, which aligns with
//    HandleDeath's own ROLE_Authority guard in UGCSDeathComponent. No extra
//    role checks are required in the AttributeSet itself.
//
// 3. Why FindComponentByClass instead of casting to AGCSCharacterBase: keeping
//    the attribute set generic (finding UGCSDeathComponent rather than
//    assuming AGCSCharacterBase) allows the same attribute set to be reused
//    on turrets, destructible objects, or any other actor that wants
//    death/respawn behavior without deriving from AGCSCharacterBase.
//
// 4. Clamping order: this block must run after Health has already been
//    clamped via FMath::Clamp(..., 0.f, GetMaxHealth()) in the existing
//    Shield -> Health spillover logic above, so GetHealth() reliably reflects
//    the final value for this execution when the death check runs.
//
// 5. Causer added: the original function only captured Instigator. This patch
//    also captures Context.GetEffectCauser() as Causer, purely to provide a
//    sensible Killer fallback (e.g. a projectile actor) when no instigator
//    pawn/controller is available on the effect context.
//
// ============================================================================
