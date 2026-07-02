// Project Nova — WorldState core types (vertical slice stubs)

#pragma once

#include "CoreMinimal.h"
#include "WorldStateTypes.generated.h"

/** Canonical persistent world state. Serialized to/from JSON. */
USTRUCT(BlueprintType)
struct FNovaWorldState
{
	GENERATED_BODY()

	/** Monotonic revision, bumped on every committed delta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldState")
	int64 Revision = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldState")
	TMap<FName, bool> EventFlags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldState")
	TMap<FName, float> NumericValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldState")
	TMap<FName, FString> StringValues;
};

/** One journal entry recording a committed world change. */
USTRUCT(BlueprintType)
struct FNovaJournalEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldState")
	int64 Revision = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldState")
	FName DeltaId = NAME_None;

	/** Where the delta came from (story id, ability id, debug, ...). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldState")
	FName Source = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldState")
	FDateTime Timestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldState")
	FString Description;
};
