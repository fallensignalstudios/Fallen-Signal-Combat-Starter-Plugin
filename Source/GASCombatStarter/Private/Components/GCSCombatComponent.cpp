// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Components/GCSCombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

UGCSCombatComponent::UGCSCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGCSCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGCSCombatComponent, bIsBlocking);
	DOREPLIFETIME(UGCSCombatComponent, bIsInParryWindow);
	DOREPLIFETIME(UGCSCombatComponent, CurrentComboIndex);
	DOREPLIFETIME(UGCSCombatComponent, bIsStaggered);
}

void UGCSCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

// ----------------------------------------------------------------------
// Blocking
// ----------------------------------------------------------------------

void UGCSCombatComponent::StartBlocking()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!bIsBlocking)
	{
		bIsBlocking = true;
		OnRep_IsBlocking(); // Server doesn't get its own RepNotify; fire manually for local delegate/UI updates.
	}
}

void UGCSCombatComponent::StopBlocking()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (bIsBlocking)
	{
		bIsBlocking = false;
		OnRep_IsBlocking();
	}
}

void UGCSCombatComponent::OnRep_IsBlocking()
{
	if (bIsBlocking)
	{
		OnBlockStart.Broadcast();
	}
	else
	{
		OnBlockEnd.Broadcast();
	}
}

// ----------------------------------------------------------------------
// Parry window
// ----------------------------------------------------------------------

void UGCSCombatComponent::BeginParryWindow(float Duration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Duration <= 0.f)
	{
		return;
	}

	bIsInParryWindow = true;
	OnRep_IsInParryWindow();

	GetWorld()->GetTimerManager().SetTimer(ParryWindowTimerHandle, this, &UGCSCombatComponent::EndParryWindow, Duration, false);
}

void UGCSCombatComponent::EndParryWindow()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
	}

	if (bIsInParryWindow)
	{
		bIsInParryWindow = false;
		OnRep_IsInParryWindow();
	}
}

bool UGCSCombatComponent::TryConsumeParry(AActor* Attacker)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!bIsInParryWindow)
	{
		return false;
	}

	EndParryWindow();
	OnParrySuccess.Broadcast(Attacker);
	return true;
}

void UGCSCombatComponent::OnRep_IsInParryWindow()
{
	// Intentionally no delegate here beyond state change; OnParrySuccess is the
	// meaningful gameplay event and is broadcast explicitly from TryConsumeParry
	// (which only ever runs on the server, then implicitly reflected via bIsInParryWindow
	// replication). If you need client-side "window opened" feedback, bind to this
	// via a Blueprint OnRep-driven event on bIsInParryWindow instead.
}

// ----------------------------------------------------------------------
// Combo tracking
// ----------------------------------------------------------------------

void UGCSCombatComponent::AdvanceCombo()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	++CurrentComboIndex;
	OnRep_CurrentComboIndex();
}

void UGCSCombatComponent::ResetCombo()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (CurrentComboIndex != 0)
	{
		CurrentComboIndex = 0;
		OnComboReset.Broadcast();
	}
}

void UGCSCombatComponent::OnRep_CurrentComboIndex()
{
	OnComboAdvance.Broadcast(CurrentComboIndex);
}

// ----------------------------------------------------------------------
// Hit timing / stagger
// ----------------------------------------------------------------------

void UGCSCombatComponent::RecordHitTaken()
{
	if (GetWorld())
	{
		LastHitTime = GetWorld()->GetTimeSeconds();
	}
}

void UGCSCombatComponent::BeginStagger(float Duration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Duration <= 0.f)
	{
		return;
	}

	bIsStaggered = true;
	OnRep_IsStaggered();

	GetWorld()->GetTimerManager().SetTimer(StaggerTimerHandle, this, &UGCSCombatComponent::EndStagger, Duration, false);
}

void UGCSCombatComponent::EndStagger()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(StaggerTimerHandle);
	}

	if (bIsStaggered)
	{
		bIsStaggered = false;
		OnRep_IsStaggered();
	}
}

void UGCSCombatComponent::OnRep_IsStaggered()
{
	if (bIsStaggered)
	{
		OnStaggerBegin.Broadcast(0.f); // Duration not tracked post-hoc; bind to BeginStagger call site if exact duration is needed client-side.
	}
	else
	{
		OnStaggerEnd.Broadcast();
	}
}
