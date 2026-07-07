// Project Nova — Story Director subsystem (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryTypes.h"
#include "WorldDeltaTypes.h"
#include "StoryDirectorSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNovaStory, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNovaOnControlMaskChanged, ENovaControlMask, OldMask, ENovaControlMask, NewMask);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNovaOnStoryBeat, FName, StoryId, int32, BeatIndex);

/**
 * Drives story playback and the Director control mask (Hold / Guide / Punch).
 * Every finished story emits a WorldDelta back into UWorldStateSubsystem.
 */
UCLASS()
class UStoryDirectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Start a registered story by id. Returns false if unknown or one is already running. */
	UFUNCTION(BlueprintCallable, Category="Story")
	bool TriggerStory(FName StoryId);

	/** Convenience entry points for the slice's two demo stories. */
	UFUNCTION(BlueprintCallable, Category="Story")
	void StartStoryA();

	UFUNCTION(BlueprintCallable, Category="Story")
	void StartStoryB();

	/** Advance to the next beat; ends the story (and emits its WorldDelta) past the last beat. */
	UFUNCTION(BlueprintCallable, Category="Story")
	void AdvanceBeat();

	UFUNCTION(BlueprintCallable, Category="Story")
	void SetControlMask(ENovaControlMask NewMask);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Story")
	ENovaControlMask GetControlMask() const { return ControlMask; }

	/** Current beat of the running story. Returns false when no story is active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Story")
	bool GetCurrentBeat(FNovaStoryBeat& OutBeat) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Story")
	FName GetCurrentStoryId() const { return CurrentStoryId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Story")
	int32 GetCurrentBeatIndex() const { return CurrentBeatIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Story")
	bool IsStoryActive() const { return !CurrentStoryId.IsNone(); }

	/** Push a story-result delta into the world state, tagged with the story as source. */
	UFUNCTION(BlueprintCallable, Category="Story")
	bool EmitWorldDeltaForStory(FName StoryId, const FNovaWorldDelta& Delta);

	UFUNCTION(BlueprintCallable, Category="Story")
	void RegisterStory(const FNovaStoryDef& StoryDef);

	UPROPERTY(BlueprintAssignable, Category="Story")
	FNovaOnControlMaskChanged OnControlMaskChanged;

	UPROPERTY(BlueprintAssignable, Category="Story")
	FNovaOnStoryBeat OnStoryBeat;

private:

	void EnterBeat(int32 BeatIndex);

	void EndCurrentStory();

	/** Identity Override hookup: run the beat's IdentityAction on the player. */
	void ApplyBeatIdentityAction(const FNovaStoryBeat& Beat);

	class UIdentityOverrideComponent* GetPlayerIdentityComponent() const;

	/** True once the running story applied an identity override; story end reverts leftovers. */
	bool bStoryDroveIdentityOverride = false;

	/** Auto-advances beats after each beat's DurationSeconds. */
	FTimerHandle BeatTimerHandle;

	/** Registered story definitions by id. */
	TMap<FName, FNovaStoryDef> Stories;

	FName CurrentStoryId = NAME_None;

	int32 CurrentBeatIndex = INDEX_NONE;

	ENovaControlMask ControlMask = ENovaControlMask::None;
};
