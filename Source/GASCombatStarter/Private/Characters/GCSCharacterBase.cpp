// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Characters/GCSCharacterBase.h"
#include "AbilitySystem/GCSAbilitySystemComponent.h"
#include "AttributeSets/GCSCombatAttributeSet.h"
#include "Components/GCSCombatComponent.h"
#include "Data/GCSCombatConfig.h"
#include "GCSNativeGameplayTags.h"
#include "GASCombatStarterModule.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

AGCSCharacterBase::AGCSCharacterBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UGCSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	CombatAttributeSet = CreateDefaultSubobject<UGCSCombatAttributeSet>(TEXT("CombatAttributeSet"));

	CombatComponent = CreateDefaultSubobject<UGCSCombatComponent>(TEXT("CombatComponent"));

	bReplicates = true;
}

UAbilitySystemComponent* AGCSCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGCSCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server-authoritative path. This always runs on the server the moment a
	// controller (AI or player) possesses this pawn, regardless of whether the
	// controller belongs to a local or remote client.
	InitAbilityActorInfo();
}

void AGCSCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client-side path for player-controlled characters. OnRep_PlayerState fires
	// on owning clients once PlayerState has replicated, which is a reliable
	// signal that this character is "ready" from the local client's point of
	// view. Since the ASC lives on the character itself (not PlayerState) in
	// this starter, re-running InitAbilityActorInfo here mainly re-establishes
	// the AvatarActor/OwnerActor pointers on the client's local ASC proxy.
	InitAbilityActorInfo();
}

void AGCSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Enhanced Input mapping context setup for locally-controlled players.
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController() && DefaultMappingContext)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(DefaultMappingContext, InputMappingPriority);
			}
		}
	}
}

void AGCSCharacterBase::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogGASCombatStarter, Error, TEXT("%s has no AbilitySystemComponent to initialize."), *GetName());
		return;
	}

	// Standard GAS pattern: OwnerActor is typically the Controller/PlayerState in
	// frameworks that put the ASC on PlayerState, but since this starter places
	// the ASC on the character itself, Owner == Avatar == this. If you move the
	// ASC to PlayerState in your project, update the OwnerActor argument to
	// GetPlayerState() instead.
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		InitializeFromCombatConfig();
	}
}

void AGCSCharacterBase::InitializeFromCombatConfig()
{
	if (!DefaultCombatConfig)
	{
		return;
	}

	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitializeDefaultAbilities(DefaultCombatConfig->DefaultAbilities);

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : DefaultCombatConfig->DefaultEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	if (CombatComponent)
	{
		// Tuning values on UGCSCombatComponent (like parry duration) are passed
		// explicitly per-call (see UGCSAbility_Block::ParryWindowDuration) rather
		// than cached here, so DefaultCombatConfig's ParryWindowDuration/
		// StaggerDuration are meant to be read by abilities/Blueprint logic that
		// need them (e.g. wire DefaultCombatConfig->ParryWindowDuration into your
		// Block ability instance's ParryWindowDuration when granting it, or read
		// it directly via GetOwningCharacter()->DefaultCombatConfig in Blueprint).
	}
}

void AGCSCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogGASCombatStarter, Warning, TEXT("%s: PlayerInputComponent is not an UEnhancedInputComponent. Ensure Enhanced Input is enabled as the default input system."), *GetName());
		return;
	}

	if (PrimaryAttackInputAction)
	{
		EnhancedInputComponent->BindAction(PrimaryAttackInputAction, ETriggerEvent::Started, this, &AGCSCharacterBase::OnPrimaryAttackInput);
	}

	if (BlockInputAction)
	{
		EnhancedInputComponent->BindAction(BlockInputAction, ETriggerEvent::Started, this, &AGCSCharacterBase::OnBlockInputStarted);
		EnhancedInputComponent->BindAction(BlockInputAction, ETriggerEvent::Completed, this, &AGCSCharacterBase::OnBlockInputCompleted);
	}

	if (DodgeInputAction)
	{
		EnhancedInputComponent->BindAction(DodgeInputAction, ETriggerEvent::Started, this, &AGCSCharacterBase::OnDodgeInput);
	}
}

void AGCSCharacterBase::OnPrimaryAttackInput_Implementation(const FInputActionValue& Value)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(GCSGameplayTags::Input_PrimaryAttack));
	}
}

void AGCSCharacterBase::OnBlockInputStarted_Implementation(const FInputActionValue& Value)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(GCSGameplayTags::Input_Block));
	}
}

void AGCSCharacterBase::OnBlockInputCompleted_Implementation(const FInputActionValue& Value)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Ending block on input release: cancel any active ability tagged
	// GCS.Ability.Block. UGCSAbility_Block::InputReleased handles a clean,
	// non-cancelled end when input is routed through the ASC's input-release
	// path instead; this explicit cancel is a robust fallback for input setups
	// that don't route release events through AbilitySpecInputReleased.
	const FGameplayTagContainer BlockTag(GCSGameplayTags::Ability_Block);
	AbilitySystemComponent->CancelAbilities(&BlockTag);
}

void AGCSCharacterBase::OnDodgeInput_Implementation(const FInputActionValue& Value)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(GCSGameplayTags::Input_Dodge));
	}
}
