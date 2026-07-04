// Copyright Fallen Signal Studios. All Rights Reserved.
//
// ============================================================================
// PATCH DOCUMENT -- NOT A COMPILED HEADER
// ============================================================================
//
// This file is NOT included anywhere and is NOT compiled as part of the
// module. It documents the exact edits required in the existing
// AGCSCharacterBase class (GCSCharacterBase.h / GCSCharacterBase.cpp) to
// integrate UGCSDeathComponent and IGCSDeathInterface. Apply the snippets
// below directly to the real files.
//
// ============================================================================
// 1) GCSCharacterBase.h -- CHANGES
// ============================================================================
//
// (a) Add includes near the top, alongside the existing ASC/AttributeSet/
//     CombatComponent includes:
//
//     #include "Interfaces/IGCSDeathInterface.h"
//
//     class UGCSDeathComponent; // forward declaration is sufficient in the header
//
// (b) Change the class declaration to also implement IGCSDeathInterface.
//     BEFORE:
//
//         UCLASS()
//         class GASCOMBATSTARTER_API AGCSCharacterBase : public ACharacter, public IAbilitySystemInterface
//         {
//             GENERATED_BODY()
//             ...
//         };
//
//     AFTER:
//
//         UCLASS()
//         class GASCOMBATSTARTER_API AGCSCharacterBase : public ACharacter, public IAbilitySystemInterface, public IGCSDeathInterface
//         {
//             GENERATED_BODY()
//             ...
//         };
//
// (c) Add the DeathComponent subobject next to the existing
//     AbilitySystemComponent / CombatAttributeSet / CombatComponent members.
//     Mirror the visibility/category style already used for CombatComponent:
//
//         /** Drives death/respawn state for this character. Works alongside CombatComponent. */
//         UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GCS|Death")
//         TObjectPtr<UGCSDeathComponent> DeathComponent;
//
// (d) Declare the IGCSDeathInterface overrides. Because the interface
//     functions are BlueprintNativeEvent, the native override uses the
//     "_Implementation" suffix and is NOT marked UFUNCTION itself (the
//     UFUNCTION macro lives on the interface declaration only):
//
//         //~ Begin IGCSDeathInterface Interface
//         virtual void OnDeath_Implementation(AActor* Killer) override;
//         virtual void OnRespawn_Implementation() override;
//         virtual bool IsDead_Implementation() const override;
//         //~ End IGCSDeathInterface Interface
//
// ============================================================================
// 2) GCSCharacterBase.cpp -- CHANGES
// ============================================================================
//
// (a) Add the include:
//
//         #include "Components/GCSDeathComponent.h"
//
// (b) In the constructor, create the DeathComponent the same way
//     CombatComponent is created (CreateDefaultSubobject), e.g.:
//
//         AGCSCharacterBase::AGCSCharacterBase()
//         {
//             ...
//             CombatComponent = CreateDefaultSubobject<UGCSCombatComponent>(TEXT("CombatComponent"));
//
//             // Death handling is a distinct concern from combat resolution
//             // (staggering, etc.) and lives in its own component so it can be
//             // reused by non-combat-component actors (e.g. destructible props)
//             // if needed later.
//             DeathComponent = CreateDefaultSubobject<UGCSDeathComponent>(TEXT("DeathComponent"));
//             ...
//         }
//
// (c) Implement the IGCSDeathInterface functions. These are the character's
//     own cosmetic/gameplay reactions to death and respawn -- the *decision*
//     to die/respawn is still owned by UGCSDeathComponent; this is purely the
//     "what happens to me as a character" reaction:
//
//         void AGCSCharacterBase::OnDeath_Implementation(AActor* Killer)
//         {
//             // Disable input so a dying/dead character cannot keep issuing
//             // movement or ability commands.
//             if (AController* MyController = GetController())
//             {
//                 DisableInput(Cast<APlayerController>(MyController));
//             }
//
//             // Stop movement immediately; DeathComponent::EnableRagdoll (if
//             // bAutoRagdoll is set) will fully hand off physics shortly after.
//             if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
//             {
//                 MovementComponent->DisableMovement();
//             }
//
//             // Project-specific: play a death montage, broadcast to a kill-feed
//             // widget, notify an AI controller, etc.
//         }
//
//         void AGCSCharacterBase::OnRespawn_Implementation()
//         {
//             // Re-enable input and movement now that DeathComponent has already
//             // restored Health/Shield and cleared the Dead tag/GE.
//             if (AController* MyController = GetController())
//             {
//                 EnableInput(Cast<APlayerController>(MyController));
//             }
//
//             if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
//             {
//                 MovementComponent->SetMovementMode(MOVE_Walking);
//             }
//
//             // Project-specific: teleport to a spawn point, play a respawn VFX, etc.
//         }
//
//         bool AGCSCharacterBase::IsDead_Implementation() const
//         {
//             // Delegate to DeathComponent, which is the single source of truth
//             // (it checks the ASC for DeadStateTag). Falls back to false if the
//             // component hasn't been created yet (e.g. called extremely early).
//             return DeathComponent ? DeathComponent->IsDead() : false;
//         }
//
// ============================================================================
// 3) Wiring the AttributeSet's death trigger to DeathComponent
// ============================================================================
//
// See GCSCombatAttributeSet_DeathPatch.h for the exact PostGameplayEffectExecute
// snippet. In short, once Health is clamped to <= 0 in the AttributeSet, it
// resolves the owning actor's AGCSCharacterBase (or, more generally, any actor
// exposing a UGCSDeathComponent) and calls:
//
//         if (UGCSDeathComponent* DeathComp = OwnerActor->FindComponentByClass<UGCSDeathComponent>())
//         {
//             DeathComp->HandleDeath(Killer);
//         }
//
// This keeps the AttributeSet decoupled from AGCSCharacterBase specifically --
// it only needs to know about UGCSDeathComponent, so the same AttributeSet
// class can be reused on any actor that has that component, not just characters.
//
// ============================================================================
