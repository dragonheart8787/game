// Project Nova — Identity Override component (Patch 5: minimal verifiable core)

#include "IdentityOverrideComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "ElementAbilityComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NovaAbilityDataAsset.h"
#include "NovaDummyTarget.h"
#include "StoryDirectorSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogNovaIdentity);

UIdentityOverrideComponent::UIdentityOverrideComponent()
{
	// Slice default: the test identity replaces Slash with the TestOverride asset
	// (deliberately different numbers) and knows no other abilities.
	FullOverrideLoadout.Add(TEXT("Slash"), TSoftObjectPtr<UNovaAbilityDataAsset>(
		FSoftObjectPath(TEXT("/Game/Data/Abilities/DA_Ability_TestOverride.DA_Ability_TestOverride"))));
}

void UIdentityOverrideComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UElementAbilityComponent* Abilities = GetSiblingAbilities())
	{
		Abilities->OnAbilityEvent.AddDynamic(this, &UIdentityOverrideComponent::HandleAbilityEvent);
	}
}

void UIdentityOverrideComponent::ApplyFullOverride(FName IdentityId)
{
	if (OverrideMode != ENovaIdentityOverrideMode::None)
	{
		RevertOverride();
	}

	// Loadout and constraints land before SetOverride so OnIdentityChanged
	// listeners observe the new identity's state, not the old one's.
	bDashLocked = bFullOverrideLocksDash;

	if (UElementAbilityComponent* Abilities = GetSiblingAbilities())
	{
		Abilities->SetAbilityLoadout(FullOverrideLoadout);
	}

	SetOverride(ENovaIdentityOverrideMode::Full, IdentityId, /*bRestrictAbilities=*/false);

	UE_LOG(LogNovaIdentity, Log, TEXT("[Identity] Full override '%s': loadout swapped (%d abilities), dash %s"),
		*IdentityId.ToString(), FullOverrideLoadout.Num(), bDashLocked ? TEXT("LOCKED") : TEXT("free"));
}

void UIdentityOverrideComponent::ApplyPartialOverride(FName IdentityId)
{
	if (OverrideMode != ENovaIdentityOverrideMode::None)
	{
		RevertOverride();
	}

	// Partial keeps the player's own abilities usable — the cost is exposure,
	// not a hard lock, so casts must go through for the meter to mean anything.
	SetOverride(ENovaIdentityOverrideMode::Partial, IdentityId, /*bRestrictAbilities=*/false);
	Exposure = 0.f;
	bExposureMaxedFired = false;

	UE_LOG(LogNovaIdentity, Log, TEXT("[Identity] Partial override '%s': exposure meter armed (+%.0f per cast, threshold %.0f)"),
		*IdentityId.ToString(), ExposurePerCast, ExposureThreshold);
}

void UIdentityOverrideComponent::ApplyLensOverride(FName IdentityId)
{
	if (OverrideMode != ENovaIdentityOverrideMode::None)
	{
		RevertOverride();
	}

	SetOverride(ENovaIdentityOverrideMode::Lens, IdentityId, /*bRestrictAbilities=*/true);

	// Control lock goes through the Director — it owns the mask.
	if (UStoryDirectorSubsystem* Director = GetStoryDirector())
	{
		Director->SetControlMask(ENovaControlMask::Hold);
	}

	AActor* ViewTarget = FindLensViewTarget();
	APlayerController* PC = GetOwnerPlayerController();
	if (PC && ViewTarget)
	{
		PC->SetViewTargetWithBlend(ViewTarget, LensBlendSeconds);
	}
	UE_LOG(LogNovaIdentity, Log, TEXT("[Identity] Lens override '%s': view -> %s for %.1fs"),
		*IdentityId.ToString(), ViewTarget ? *ViewTarget->GetName() : TEXT("<no view target>"), LensDurationSeconds);

	GetWorld()->GetTimerManager().SetTimer(LensRevertTimerHandle, this,
		&UIdentityOverrideComponent::RevertOverride, LensDurationSeconds, false);
}

void UIdentityOverrideComponent::RevertOverride()
{
	if (OverrideMode == ENovaIdentityOverrideMode::None)
	{
		return;
	}

	const ENovaIdentityOverrideMode OldMode = OverrideMode;
	GetWorld()->GetTimerManager().ClearTimer(LensRevertTimerHandle);

	if (OldMode == ENovaIdentityOverrideMode::Full)
	{
		if (UElementAbilityComponent* Abilities = GetSiblingAbilities())
		{
			Abilities->ResetAbilityLoadoutToDefault();
		}
	}

	if (OldMode == ENovaIdentityOverrideMode::Lens)
	{
		if (APlayerController* PC = GetOwnerPlayerController())
		{
			PC->SetViewTargetWithBlend(GetOwner(), LensBlendSeconds);
		}
		// Release the Hold only if it is still ours; story beats may have moved on.
		if (UStoryDirectorSubsystem* Director = GetStoryDirector())
		{
			if (Director->GetControlMask() == ENovaControlMask::Hold)
			{
				Director->SetControlMask(ENovaControlMask::None);
			}
		}
	}

	bDashLocked = false;
	Exposure = 0.f;
	bExposureMaxedFired = false;

	UE_LOG(LogNovaIdentity, Log, TEXT("[Identity] Reverted from %s to baseline"),
		*UEnum::GetValueAsString(OldMode));
	SetOverride(ENovaIdentityOverrideMode::None, NAME_None, /*bRestrictAbilities=*/false);
}

void UIdentityOverrideComponent::AddExposure(float Amount)
{
	if (OverrideMode != ENovaIdentityOverrideMode::Partial || Amount == 0.f)
	{
		return;
	}

	Exposure = FMath::Clamp(Exposure + Amount, 0.f, ExposureThreshold);
	UE_LOG(LogNovaIdentity, Log, TEXT("[Identity] exposure +%.0f -> %.0f/%.0f ('%s')"),
		Amount, Exposure, ExposureThreshold, *ActiveIdentityId.ToString());

	if (!bExposureMaxedFired && Exposure >= ExposureThreshold)
	{
		bExposureMaxedFired = true;
		UE_LOG(LogNovaIdentity, Log, TEXT("[Identity] EXPOSURE THRESHOLD reached (%.0f/%.0f) — cover blown for '%s'"),
			Exposure, ExposureThreshold, *ActiveIdentityId.ToString());
		OnExposureMaxed.Broadcast(ActiveIdentityId);
	}
}

void UIdentityOverrideComponent::HandleAbilityEvent(FName AbilityId, FName EventType)
{
	if (EventType == TEXT("CastStarted"))
	{
		AddExposure(ExposurePerCast);
	}
}

UElementAbilityComponent* UIdentityOverrideComponent::GetSiblingAbilities() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UElementAbilityComponent>() : nullptr;
}

UStoryDirectorSubsystem* UIdentityOverrideComponent::GetStoryDirector() const
{
	const UWorld* World = GetWorld();
	return World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UStoryDirectorSubsystem>()
		: nullptr;
}

APlayerController* UIdentityOverrideComponent::GetOwnerPlayerController() const
{
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			return PC;
		}
	}
	return UGameplayStatics::GetPlayerController(this, 0);
}

AActor* UIdentityOverrideComponent::FindLensViewTarget() const
{
	// Slice stand-in: the dummy target doubles as the CCTV/bystander viewpoint.
	for (TActorIterator<ANovaDummyTarget> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void UIdentityOverrideComponent::SetOverride(ENovaIdentityOverrideMode Mode, FName IdentityId, bool bRestrictAbilities)
{
	OverrideMode = Mode;
	ActiveIdentityId = IdentityId;
	bAbilitiesRestricted = bRestrictAbilities;

	UE_LOG(LogNovaIdentity, Log, TEXT("Identity override: mode %s, identity '%s', abilities %s"),
		*UEnum::GetValueAsString(Mode), *IdentityId.ToString(), bRestrictAbilities ? TEXT("restricted") : TEXT("free"));

	OnIdentityChanged.Broadcast(Mode, IdentityId);
}
