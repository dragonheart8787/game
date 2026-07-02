// Project Nova — WorldState subsystem (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldStateTypes.h"
#include "WorldDeltaTypes.h"
#include "WorldStateSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNovaWorldState, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNovaOnWorldDeltaApplied, const FNovaWorldDelta&, Delta);

/**
 * Owns the canonical world state for the session.
 * All mutations go through ApplyWorldDelta (transactional: all ops apply or none do).
 */
UCLASS()
class UWorldStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** Load state from a JSON file. Returns false and leaves state untouched on failure. */
	UFUNCTION(BlueprintCallable, Category="WorldState")
	bool LoadWorldStateFromJson(const FString& FilePath);

	/** Save current state to a JSON file. */
	UFUNCTION(BlueprintCallable, Category="WorldState")
	bool SaveWorldStateToJson(const FString& FilePath) const;

	/** Apply a delta transactionally. On any op failure the whole delta is rolled back. */
	UFUNCTION(BlueprintCallable, Category="WorldState")
	bool ApplyWorldDelta(const FNovaWorldDelta& Delta);

	/** Hash of the current canonical state (MD5 of the JSON form). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WorldState")
	FString GetWorldHash() const;

	UFUNCTION(BlueprintCallable, Category="WorldState")
	void SetEventFlag(FName Flag, bool bValue);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WorldState")
	bool GetEventFlag(FName Flag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WorldState")
	const TArray<FNovaJournalEntry>& GetJournal() const { return Journal; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WorldState")
	const FNovaWorldState& GetWorldState() const { return WorldState; }

	/** Default save location under the project's Saved dir. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WorldState")
	static FString GetDefaultWorldStatePath();

	UPROPERTY(BlueprintAssignable, Category="WorldState")
	FNovaOnWorldDeltaApplied OnWorldDeltaApplied;

private:

	/** Applies one op to the given state. Returns false if the op is invalid. */
	static bool ApplyOpToState(FNovaWorldState& State, const FNovaWorldDeltaOp& Op);

	FString SerializeStateToJson() const;

	void AppendJournalEntry(const FNovaWorldDelta& Delta);

	FNovaWorldState WorldState;

	TArray<FNovaJournalEntry> Journal;
};
