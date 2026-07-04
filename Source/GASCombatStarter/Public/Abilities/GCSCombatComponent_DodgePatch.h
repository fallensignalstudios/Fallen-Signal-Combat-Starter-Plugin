// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

/**
 * GCSCombatComponent_DodgePatch.h
 *
 * ============================================================================
 * THIS IS NOT A COMPILABLE HEADER. It is a documentation-only patch describing
 * the additions UGCSAbility_Dodge requires on the EXISTING UGCSCombatComponent
 * class (Components/GCSCombatComponent.h / .cpp). Do not #include this file
 * anywhere or add it to the module's build -- apply the changes below directly
 * to GCSCombatComponent.h/.cpp, then delete this patch file.
 *
 * Rationale for not rewriting the whole class here: UGCSCombatComponent is an
 * existing, working class (blocking/parry/combo/stagger). Reproducing it in
 * full risks accidental regressions if this patch and the real file drift.
 * Instead, apply the following surgical additions.
 * ============================================================================
 *
 * --------------------------------------------------------------------------
 * 1. GCSCombatComponent.h -- add near the other DECLARE_DYNAMIC_MULTICAST_DELEGATE
 *    macros at the top of the file (alongside FGCSOnBlockStart, etc.):
 * --------------------------------------------------------------------------
 *
 *     DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSOnDodgeStart);
 *     DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGCSOnDodgeEnd);
 *
 * --------------------------------------------------------------------------
 * 2. GCSCombatComponent.h -- add a new public section, e.g. directly after the
 *    "Hit timing / stagger" section and before "Delegates":
 * --------------------------------------------------------------------------
 *
 *     // ----------------------------------------------------------------------
 *     // Invincibility (i-frames), driven by UGCSAbility_Dodge
 *     // ----------------------------------------------------------------------
 *
 *     /** True while the owner is invincible (e.g. mid-dodge i-frames). Damage-handling
 *      * code (attribute set / damage execution) should check this and no-op incoming
 *      * damage while true. *\/
 *     UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsInvincible, Category = "GCS|Combat|Dodge")
 *     bool bIsInvincible = false;
 *
 *     /** Begins an invincibility window for the given duration (seconds); auto-clears
 *      * via timer. Server-authoritative. Safe to call while already invincible -- this
 *      * restarts/extends the timer to the new Duration. *\/
 *     UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Dodge")
 *     void BeginInvincibility(float Duration);
 *
 *     /** Ends the invincibility window immediately. Server-authoritative. *\/
 *     UFUNCTION(BlueprintCallable, Category = "GCS|Combat|Dodge")
 *     void EndInvincibility();
 *
 * --------------------------------------------------------------------------
 * 3. GCSCombatComponent.h -- add the two new delegate UPROPERTYs alongside the
 *    existing OnBlockStart / OnStaggerEnd delegates in the "Delegates" section:
 * --------------------------------------------------------------------------
 *
 *     UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
 *     FGCSOnDodgeStart OnDodgeStart;
 *
 *     UPROPERTY(BlueprintAssignable, Category = "GCS|Combat|Events")
 *     FGCSOnDodgeEnd OnDodgeEnd;
 *
 * --------------------------------------------------------------------------
 * 4. GCSCombatComponent.h -- add to the protected section, alongside the other
 *    OnRep_* declarations and timer handles:
 * --------------------------------------------------------------------------
 *
 *     UFUNCTION()
 *     void OnRep_IsInvincible();
 *
 *     /** Timer handle for auto-clearing invincibility. *\/
 *     FTimerHandle InvincibilityTimerHandle;
 *
 * --------------------------------------------------------------------------
 * 5. GCSCombatComponent.cpp -- register the new replicated property in
 *    GetLifetimeReplicatedProps, alongside the existing DOREPLIFETIME calls:
 * --------------------------------------------------------------------------
 *
 *     void UGCSCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
 *     {
 *         Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 *
 *         DOREPLIFETIME(UGCSCombatComponent, bIsBlocking);
 *         DOREPLIFETIME(UGCSCombatComponent, bIsInParryWindow);
 *         DOREPLIFETIME(UGCSCombatComponent, CurrentComboIndex);
 *         DOREPLIFETIME(UGCSCombatComponent, bIsStaggered);
 *         DOREPLIFETIME(UGCSCombatComponent, bIsInvincible); // <-- NEW
 *     }
 *
 * --------------------------------------------------------------------------
 * 6. GCSCombatComponent.cpp -- add a new "Invincibility" section (mirrors the
 *    existing BeginStagger/EndStagger/OnRep_IsStaggered pattern exactly):
 * --------------------------------------------------------------------------
 *
 *     // ----------------------------------------------------------------------
 *     // Invincibility (i-frames)
 *     // ----------------------------------------------------------------------
 *
 *     void UGCSCombatComponent::BeginInvincibility(float Duration)
 *     {
 *         if (!GetOwner() || !GetOwner()->HasAuthority() || Duration <= 0.f)
 *         {
 *             return;
 *         }
 *
 *         bIsInvincible = true;
 *         OnRep_IsInvincible(); // Server doesn't get its own RepNotify; fire manually.
 *
 *         // SetTimer (not just extends) so repeated dodges correctly refresh the window
 *         // rather than letting an earlier, shorter timer end invincibility early.
 *         GetWorld()->GetTimerManager().SetTimer(InvincibilityTimerHandle, this, &UGCSCombatComponent::EndInvincibility, Duration, false);
 *     }
 *
 *     void UGCSCombatComponent::EndInvincibility()
 *     {
 *         if (!GetOwner() || !GetOwner()->HasAuthority())
 *         {
 *             return;
 *         }
 *
 *         if (GetWorld())
 *         {
 *             GetWorld()->GetTimerManager().ClearTimer(InvincibilityTimerHandle);
 *         }
 *
 *         if (bIsInvincible)
 *         {
 *             bIsInvincible = false;
 *             OnRep_IsInvincible();
 *         }
 *     }
 *
 *     void UGCSCombatComponent::OnRep_IsInvincible()
 *     {
 *         if (bIsInvincible)
 *         {
 *             OnDodgeStart.Broadcast();
 *         }
 *         else
 *         {
 *             OnDodgeEnd.Broadcast();
 *         }
 *     }
 *
 * --------------------------------------------------------------------------
 * 7. Damage pipeline integration (informational -- not part of this component,
 *    but required for invincibility to actually matter gameplay-wise):
 * --------------------------------------------------------------------------
 *
 *     In UGCSCombatAttributeSet::PostGameplayEffectExecute (or wherever incoming
 *     damage is finally applied to Health/Shield), add an early-out guard:
 *
 *         if (UGCSCombatComponent* CombatComponent = GetOwningActor()->FindComponentByClass<UGCSCombatComponent>())
 *         {
 *             if (CombatComponent->bIsInvincible)
 *             {
 *                 // Zero out / ignore incoming damage meta-attribute here.
 *                 return;
 *             }
 *         }
 *
 *     This keeps "what does invincible mean" as a damage-pipeline decision
 *     rather than baking it into the combat component itself.
 */
