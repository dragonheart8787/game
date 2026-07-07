// Project Nova — Debug overlay subsystem (vertical slice stub)

#include "NovaDebugSubsystem.h"

#include "ElementAbilityComponent.h"
#include "IdentityOverrideComponent.h"
#include "StoryDirectorSubsystem.h"
#include "WorldStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// Stable keys so each line updates in place instead of stacking.
	constexpr int32 DebugKeyBase = 74100;

	void DrawLine(int32 LineIndex, const FString& Text, const FColor& Color = FColor::Cyan)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(DebugKeyBase + LineIndex, 0.f, Color, Text);
		}
	}
}

void UNovaDebugSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDisplayEnabled)
	{
		DrawOverlay(DeltaTime);
	}
}

TStatId UNovaDebugSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UNovaDebugSubsystem, STATGROUP_Tickables);
}

bool UNovaDebugSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UNovaDebugSubsystem::DrawOverlay(float DeltaTime) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 Line = 0;

	const float Fps = (DeltaTime > 0.f) ? (1.f / DeltaTime) : 0.f;
	DrawLine(Line++, FString::Printf(TEXT("[Nova] FPS: %.1f"), Fps), FColor::Green);

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	if (const UStoryDirectorSubsystem* Story = GameInstance->GetSubsystem<UStoryDirectorSubsystem>())
	{
		FNovaStoryBeat Beat;
		const bool bHasBeat = Story->GetCurrentBeat(Beat);
		DrawLine(Line++, FString::Printf(TEXT("[Nova] Story: %s | Beat: %d (%s) | Mask: %s"),
			Story->IsStoryActive() ? *Story->GetCurrentStoryId().ToString() : TEXT("<none>"),
			Story->GetCurrentBeatIndex(),
			bHasBeat ? *Beat.BeatName.ToString() : TEXT("-"),
			*UEnum::GetValueAsString(Story->GetControlMask())));
	}

	if (const UWorldStateSubsystem* WorldState = GameInstance->GetSubsystem<UWorldStateSubsystem>())
	{
		DrawLine(Line++, FString::Printf(TEXT("[Nova] WorldHash: %s | Rev: %lld"),
			*WorldState->GetWorldHash(), WorldState->GetWorldState().Revision));

		FString Flags;
		for (const TPair<FName, bool>& Flag : WorldState->GetWorldState().EventFlags)
		{
			Flags += FString::Printf(TEXT("%s=%d "), *Flag.Key.ToString(), Flag.Value ? 1 : 0);
		}
		DrawLine(Line++, FString::Printf(TEXT("[Nova] Flags: %s"), Flags.IsEmpty() ? TEXT("<none>") : *Flags));
	}

	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (const UElementAbilityComponent* Abilities = Pawn ? Pawn->FindComponentByClass<UElementAbilityComponent>() : nullptr)
	{
		const FNovaAbilityRuntimeParams& Params = Abilities->GetRuntimeParams();
		DrawLine(Line++, FString::Printf(TEXT("[Nova] Params: W=%.0f Arc=%.0f R=%.0f | Energy: %.0f"),
			Params.Width, Params.Arc, Params.Range, Abilities->GetEnergy()));

		FString Cooldowns;
		for (const FName AbilityId : Abilities->GetRegisteredAbilityIds())
		{
			Cooldowns += FString::Printf(TEXT("%s=%.1fs "), *AbilityId.ToString(), Abilities->GetRemainingCooldown(AbilityId));
		}
		DrawLine(Line++, FString::Printf(TEXT("[Nova] Cooldowns: %s"), *Cooldowns));

		const FString& LastEvent = Abilities->GetLastAbilityEventText();
		DrawLine(Line++, FString::Printf(TEXT("[Nova] LastAbility: %s"),
			LastEvent.IsEmpty() ? TEXT("<none>") : *LastEvent), FColor::Yellow);
	}

	if (const UIdentityOverrideComponent* Identity = Pawn ? Pawn->FindComponentByClass<UIdentityOverrideComponent>() : nullptr)
	{
		DrawLine(Line++, FString::Printf(TEXT("[Nova] Identity: %s | Mode: %s | Exposure: %.0f/%.0f | Dash: %s"),
			Identity->GetActiveIdentityId().IsNone() ? TEXT("<none>") : *Identity->GetActiveIdentityId().ToString(),
			*UEnum::GetValueAsString(Identity->GetOverrideMode()),
			Identity->GetExposure(), Identity->GetExposureThreshold(),
			Identity->IsDashLocked() ? TEXT("LOCKED") : TEXT("free")), FColor::Orange);
	}
}
