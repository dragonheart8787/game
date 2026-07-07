// Project Nova — Story Director subsystem (vertical slice stub)

#include "StoryDirectorSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "IdentityOverrideComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WorldStateSubsystem.h"

DEFINE_LOG_CATEGORY(LogNovaStory);

void UStoryDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// WorldState must exist before stories can emit deltas.
	Collection.InitializeDependency<UWorldStateSubsystem>();

	// Vertical slice: two hardcoded demo stories. Real ones load from /Content/Data/Story JSON later.
	{
		FNovaStoryDef StoryA;
		StoryA.StoryId = TEXT("StoryA_Demo");
		StoryA.StoryType = ENovaStoryType::TypeA;
		StoryA.Beats = {
			{ TEXT("Establish"), 1.f, ENovaControlMask::Hold },
			{ TEXT("Approach"), 4.f, ENovaControlMask::Guide },
			{ TEXT("Impact"), 1.f, ENovaControlMask::Punch },
			{ TEXT("Resolve"), 2.f, ENovaControlMask::None },
		};
		RegisterStory(StoryA);
	}
	{
		// Type B vignette = the doc's canonical Lens Override use case, so its
		// opening beat carries the Identity hookup (Patch 5).
		FNovaStoryDef StoryB;
		StoryB.StoryId = TEXT("StoryB_Demo");
		StoryB.StoryType = ENovaStoryType::TypeB;

		FNovaStoryBeat LensIn;
		LensIn.BeatName = TEXT("LensIn");
		LensIn.DurationSeconds = 0.5f;
		LensIn.ControlMask = ENovaControlMask::Hold;
		LensIn.IdentityAction = ENovaBeatIdentityAction::ApplyLens;
		LensIn.IdentityId = TEXT("TestLens");

		FNovaStoryBeat Observe;
		Observe.BeatName = TEXT("Observe");
		Observe.DurationSeconds = 3.f;
		Observe.ControlMask = ENovaControlMask::Guide;

		StoryB.Beats = { LensIn, Observe };
		RegisterStory(StoryB);
	}
}

bool UStoryDirectorSubsystem::TriggerStory(FName StoryId)
{
	if (IsStoryActive())
	{
		UE_LOG(LogNovaStory, Warning, TEXT("TriggerStory: '%s' rejected, '%s' is already running"),
			*StoryId.ToString(), *CurrentStoryId.ToString());
		return false;
	}

	const FNovaStoryDef* Story = Stories.Find(StoryId);
	if (!Story || Story->Beats.IsEmpty())
	{
		UE_LOG(LogNovaStory, Warning, TEXT("TriggerStory: unknown or empty story '%s'"), *StoryId.ToString());
		return false;
	}

	CurrentStoryId = StoryId;
	UE_LOG(LogNovaStory, Log, TEXT("Story '%s' started (%d beats)"), *StoryId.ToString(), Story->Beats.Num());
	EnterBeat(0);
	return true;
}

void UStoryDirectorSubsystem::StartStoryA()
{
	TriggerStory(TEXT("StoryA_Demo"));
}

void UStoryDirectorSubsystem::StartStoryB()
{
	TriggerStory(TEXT("StoryB_Demo"));
}

void UStoryDirectorSubsystem::AdvanceBeat()
{
	if (!IsStoryActive())
	{
		return;
	}

	const FNovaStoryDef& Story = Stories.FindChecked(CurrentStoryId);
	const int32 NextIndex = CurrentBeatIndex + 1;
	if (NextIndex >= Story.Beats.Num())
	{
		EndCurrentStory();
	}
	else
	{
		EnterBeat(NextIndex);
	}
}

void UStoryDirectorSubsystem::SetControlMask(ENovaControlMask NewMask)
{
	if (ControlMask == NewMask)
	{
		return;
	}
	const ENovaControlMask OldMask = ControlMask;
	ControlMask = NewMask;
	OnControlMaskChanged.Broadcast(OldMask, NewMask);
}

bool UStoryDirectorSubsystem::GetCurrentBeat(FNovaStoryBeat& OutBeat) const
{
	if (!IsStoryActive() || CurrentBeatIndex == INDEX_NONE)
	{
		return false;
	}
	const FNovaStoryDef* Story = Stories.Find(CurrentStoryId);
	if (!Story || !Story->Beats.IsValidIndex(CurrentBeatIndex))
	{
		return false;
	}
	OutBeat = Story->Beats[CurrentBeatIndex];
	return true;
}

bool UStoryDirectorSubsystem::EmitWorldDeltaForStory(FName StoryId, const FNovaWorldDelta& Delta)
{
	UWorldStateSubsystem* WorldState = GetGameInstance()->GetSubsystem<UWorldStateSubsystem>();
	if (!WorldState)
	{
		return false;
	}

	FNovaWorldDelta Tagged = Delta;
	Tagged.Source = StoryId;
	return WorldState->ApplyWorldDelta(Tagged);
}

void UStoryDirectorSubsystem::RegisterStory(const FNovaStoryDef& StoryDef)
{
	if (StoryDef.StoryId.IsNone())
	{
		return;
	}
	Stories.Add(StoryDef.StoryId, StoryDef);
}

void UStoryDirectorSubsystem::EnterBeat(int32 BeatIndex)
{
	const FNovaStoryDef& Story = Stories.FindChecked(CurrentStoryId);
	CurrentBeatIndex = BeatIndex;

	const FNovaStoryBeat& Beat = Story.Beats[BeatIndex];
	SetControlMask(Beat.ControlMask);
	OnStoryBeat.Broadcast(CurrentStoryId, BeatIndex);
	ApplyBeatIdentityAction(Beat);

	// Beats auto-advance on a timer for the slice; sequencer/dialogue pacing comes later.
	if (Beat.DurationSeconds > 0.f)
	{
		GetGameInstance()->GetTimerManager().SetTimer(BeatTimerHandle, this,
			&UStoryDirectorSubsystem::AdvanceBeat, Beat.DurationSeconds, false);
	}

	UE_LOG(LogNovaStory, Log, TEXT("Story '%s' beat %d ('%s'), mask %d"),
		*CurrentStoryId.ToString(), BeatIndex, *Beat.BeatName.ToString(), static_cast<int32>(Beat.ControlMask));
}

void UStoryDirectorSubsystem::ApplyBeatIdentityAction(const FNovaStoryBeat& Beat)
{
	if (Beat.IdentityAction == ENovaBeatIdentityAction::None)
	{
		return;
	}

	UIdentityOverrideComponent* Identity = GetPlayerIdentityComponent();
	if (!Identity)
	{
		UE_LOG(LogNovaStory, Warning, TEXT("[Director] beat '%s' identity action skipped: no IdentityOverrideComponent on player pawn"),
			*Beat.BeatName.ToString());
		return;
	}

	switch (Beat.IdentityAction)
	{
	case ENovaBeatIdentityAction::ApplyFull:
		Identity->ApplyFullOverride(Beat.IdentityId);
		bStoryDroveIdentityOverride = true;
		break;
	case ENovaBeatIdentityAction::ApplyPartial:
		Identity->ApplyPartialOverride(Beat.IdentityId);
		bStoryDroveIdentityOverride = true;
		break;
	case ENovaBeatIdentityAction::ApplyLens:
		Identity->ApplyLensOverride(Beat.IdentityId);
		bStoryDroveIdentityOverride = true;
		break;
	case ENovaBeatIdentityAction::Revert:
		Identity->RevertOverride();
		break;
	default:
		break;
	}

	UE_LOG(LogNovaStory, Log, TEXT("[Director] story '%s' beat '%s' -> identity action %s ('%s')"),
		*CurrentStoryId.ToString(), *Beat.BeatName.ToString(),
		*UEnum::GetValueAsString(Beat.IdentityAction), *Beat.IdentityId.ToString());
}

UIdentityOverrideComponent* UStoryDirectorSubsystem::GetPlayerIdentityComponent() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetGameInstance()->GetWorld(), 0);
	return Pawn ? Pawn->FindComponentByClass<UIdentityOverrideComponent>() : nullptr;
}

void UStoryDirectorSubsystem::EndCurrentStory()
{
	GetGameInstance()->GetTimerManager().ClearTimer(BeatTimerHandle);

	// A story that drove an identity override must not leak it past its end.
	if (bStoryDroveIdentityOverride)
	{
		bStoryDroveIdentityOverride = false;
		if (UIdentityOverrideComponent* Identity = GetPlayerIdentityComponent())
		{
			if (Identity->GetOverrideMode() != ENovaIdentityOverrideMode::None)
			{
				UE_LOG(LogNovaStory, Log, TEXT("[Director] story end: reverting lingering identity override"));
				Identity->RevertOverride();
			}
		}
	}

	const FName FinishedStoryId = CurrentStoryId;

	// Story outcomes must land in WorldState as a delta.
	FNovaWorldDelta Delta;
	Delta.DeltaId = FName(*FString::Printf(TEXT("StoryCompleted_%s"), *FinishedStoryId.ToString()));
	Delta.Description = FString::Printf(TEXT("Story '%s' completed"), *FinishedStoryId.ToString());
	FNovaWorldDeltaOp& Op = Delta.Ops.AddDefaulted_GetRef();
	Op.Op = ENovaWorldDeltaOp::SetFlag;
	Op.Key = FName(*FString::Printf(TEXT("Story.%s.Completed"), *FinishedStoryId.ToString()));

	CurrentStoryId = NAME_None;
	CurrentBeatIndex = INDEX_NONE;
	SetControlMask(ENovaControlMask::None);

	EmitWorldDeltaForStory(FinishedStoryId, Delta);
	UE_LOG(LogNovaStory, Log, TEXT("Story '%s' finished"), *FinishedStoryId.ToString());
}
