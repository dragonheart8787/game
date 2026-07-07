// Project Nova — dev-only PIE Identity Override regression driver

#include "NovaIdentityTestDriver.h"

#include "Camera/PlayerCameraManager.h"
#include "ElementAbilityComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "IdentityOverrideComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NovaDummyTarget.h"
#include "NovaPlayerCharacter.h"
#include "StoryDirectorSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaIdentityTest, Log, All);

void ANovaIdentityTestDriver::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] armed: Full 1.0-3.8s, Partial 5.5-8.5s, Lens 10-14s, StoryB 15.5-20s"));

	Schedule(1.0f, &ANovaIdentityTestDriver::StepFullApply);
	Schedule(1.5f, &ANovaIdentityTestDriver::StepFullCast);
	Schedule(2.0f, &ANovaIdentityTestDriver::StepFullDash);
	Schedule(2.3f, &ANovaIdentityTestDriver::StepFullEdge);
	Schedule(3.0f, &ANovaIdentityTestDriver::StepFullRevert);
	Schedule(3.5f, &ANovaIdentityTestDriver::StepNormalCast);
	Schedule(3.8f, &ANovaIdentityTestDriver::StepFreeDash);
	Schedule(5.5f, &ANovaIdentityTestDriver::StepPartialApply);
	Schedule(6.0f, &ANovaIdentityTestDriver::StepPartialCast);
	Schedule(7.0f, &ANovaIdentityTestDriver::StepPartialCast);
	Schedule(8.0f, &ANovaIdentityTestDriver::StepPartialCast);
	Schedule(8.5f, &ANovaIdentityTestDriver::StepPartialRevert);
	Schedule(10.0f, &ANovaIdentityTestDriver::StepLensApply);
	Schedule(11.0f, &ANovaIdentityTestDriver::StepLensMid);
	Schedule(11.5f, &ANovaIdentityTestDriver::StepLensCast);
	Schedule(14.0f, &ANovaIdentityTestDriver::StepLensAfter);
	Schedule(15.5f, &ANovaIdentityTestDriver::StepStoryStart);
	Schedule(17.0f, &ANovaIdentityTestDriver::StepStoryMid);
	Schedule(20.0f, &ANovaIdentityTestDriver::StepStoryEnd);
}

void ANovaIdentityTestDriver::Schedule(float Delay, void (ANovaIdentityTestDriver::*Step)())
{
	FTimerHandle& Handle = StepHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, this, Step, Delay, false);
}

ANovaPlayerCharacter* ANovaIdentityTestDriver::GetPlayerCharacter() const
{
	return Cast<ANovaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

UElementAbilityComponent* ANovaIdentityTestDriver::GetAbilities() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return Pawn ? Pawn->FindComponentByClass<UElementAbilityComponent>() : nullptr;
}

UIdentityOverrideComponent* ANovaIdentityTestDriver::GetIdentity() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return Pawn ? Pawn->FindComponentByClass<UIdentityOverrideComponent>() : nullptr;
}

UStoryDirectorSubsystem* ANovaIdentityTestDriver::GetDirector() const
{
	const UWorld* World = GetWorld();
	return World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UStoryDirectorSubsystem>()
		: nullptr;
}

void ANovaIdentityTestDriver::LogState(const TCHAR* Tag) const
{
	const UIdentityOverrideComponent* Identity = GetIdentity();
	const UStoryDirectorSubsystem* Director = GetDirector();
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const AActor* ViewTarget = (PC && PC->PlayerCameraManager) ? PC->PlayerCameraManager->GetViewTarget() : nullptr;

	UE_LOG(LogNovaIdentityTest, Log,
		TEXT("[IdentityTest] %s: mode=%s identity=%s dash=%s exposure=%.0f/%.0f mask=%s viewtarget=%s story=%s"),
		Tag,
		Identity ? *UEnum::GetValueAsString(Identity->GetOverrideMode()) : TEXT("<no comp>"),
		Identity ? *Identity->GetActiveIdentityId().ToString() : TEXT("-"),
		(Identity && Identity->IsDashLocked()) ? TEXT("LOCKED") : TEXT("free"),
		Identity ? Identity->GetExposure() : 0.f,
		Identity ? Identity->GetExposureThreshold() : 0.f,
		Director ? *UEnum::GetValueAsString(Director->GetControlMask()) : TEXT("-"),
		ViewTarget ? *ViewTarget->GetName() : TEXT("<none>"),
		(Director && Director->IsStoryActive()) ? *Director->GetCurrentStoryId().ToString() : TEXT("<none>"));
}

void ANovaIdentityTestDriver::CastSlash(bool bAimAtDummy) const
{
	UElementAbilityComponent* Abilities = GetAbilities();
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Abilities || !Pawn)
	{
		return;
	}

	FVector Direction = -FVector::ForwardVector; // away from the dummy: exposure without damage spam
	if (bAimAtDummy)
	{
		for (TActorIterator<ANovaDummyTarget> It(GetWorld()); It; ++It)
		{
			Direction = It->GetActorLocation() - Pawn->GetActorLocation();
			break;
		}
	}
	Abilities->SetRuntimeDirection(Direction);
	Abilities->CastAbilityById(TEXT("Slash"));
}

void ANovaIdentityTestDriver::StepFullApply()
{
	if (UIdentityOverrideComponent* Identity = GetIdentity())
	{
		Identity->ApplyFullOverride(TEXT("TestIdentity"));
	}
	LogState(TEXT("F.apply"));
}

void ANovaIdentityTestDriver::StepFullCast()
{
	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] F.cast: Slash + same-frame re-cast (expect Dmg=55, cd 2.0s)"));
	CastSlash(true);
	CastSlash(true);
}

void ANovaIdentityTestDriver::StepFullDash()
{
	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] F.dash: expect identity block"));
	if (ANovaPlayerCharacter* Character = GetPlayerCharacter())
	{
		Character->Dash();
	}
}

void ANovaIdentityTestDriver::StepFullEdge()
{
	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] F.edge: Edgewall should be unknown under TestIdentity"));
	if (UElementAbilityComponent* Abilities = GetAbilities())
	{
		Abilities->CastAbilityById(TEXT("Edgewall"));
	}
}

void ANovaIdentityTestDriver::StepFullRevert()
{
	if (UIdentityOverrideComponent* Identity = GetIdentity())
	{
		Identity->RevertOverride();
	}
	LogState(TEXT("F.revert"));
}

void ANovaIdentityTestDriver::StepNormalCast()
{
	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] F.normalcast: expect Dmg=20 after revert"));
	CastSlash(true);
}

void ANovaIdentityTestDriver::StepFreeDash()
{
	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] F.freedash: expect launch + EndDash cap"));
	if (ANovaPlayerCharacter* Character = GetPlayerCharacter())
	{
		Character->SetActorRotation(FRotator(0.f, FreeDashYawDegrees, 0.f));
		Character->Dash();
	}
}

void ANovaIdentityTestDriver::StepPartialApply()
{
	if (UIdentityOverrideComponent* Identity = GetIdentity())
	{
		Identity->ApplyPartialOverride(TEXT("Disguise"));
	}
	LogState(TEXT("P.apply"));
}

void ANovaIdentityTestDriver::StepPartialCast()
{
	CastSlash(false);
	LogState(TEXT("P.cast"));
}

void ANovaIdentityTestDriver::StepPartialRevert()
{
	if (UIdentityOverrideComponent* Identity = GetIdentity())
	{
		Identity->RevertOverride();
	}
	LogState(TEXT("P.revert"));
}

void ANovaIdentityTestDriver::StepLensApply()
{
	if (UIdentityOverrideComponent* Identity = GetIdentity())
	{
		Identity->ApplyLensOverride(TEXT("TestLens"));
	}
	LogState(TEXT("L.apply"));
}

void ANovaIdentityTestDriver::StepLensMid()
{
	LogState(TEXT("L.mid"));
}

void ANovaIdentityTestDriver::StepLensCast()
{
	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] L.cast: expect CastBlocked_Identity while lens is active"));
	CastSlash(true);
}

void ANovaIdentityTestDriver::StepLensAfter()
{
	LogState(TEXT("L.after"));
}

void ANovaIdentityTestDriver::StepStoryStart()
{
	if (UStoryDirectorSubsystem* Director = GetDirector())
	{
		Director->StartStoryB();
	}
	LogState(TEXT("S.start"));
}

void ANovaIdentityTestDriver::StepStoryMid()
{
	LogState(TEXT("S.mid"));
}

void ANovaIdentityTestDriver::StepStoryEnd()
{
	LogState(TEXT("S.end"));
	UE_LOG(LogNovaIdentityTest, Log, TEXT("[IdentityTest] done"));
}
