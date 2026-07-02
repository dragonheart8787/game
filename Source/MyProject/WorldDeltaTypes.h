// Project Nova — WorldDelta core types (vertical slice stubs)

#pragma once

#include "CoreMinimal.h"
#include "WorldDeltaTypes.generated.h"

/** Operation kinds a WorldDelta can carry. */
UENUM(BlueprintType)
enum class ENovaWorldDeltaOp : uint8
{
	SetFlag,
	ClearFlag,
	SetNumeric,
	AddNumeric,
	SetString
};

/** One atomic operation inside a WorldDelta. */
USTRUCT(BlueprintType)
struct FNovaWorldDeltaOp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	ENovaWorldDeltaOp Op = ENovaWorldDeltaOp::SetFlag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	bool BoolValue = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	float NumericValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	FString StringValue;
};

/** A transactional batch of world state changes. Applied all-or-nothing. */
USTRUCT(BlueprintType)
struct FNovaWorldDelta
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	FName DeltaId = NAME_None;

	/** Where the delta came from (story id, ability id, debug, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	FName Source = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldDelta")
	TArray<FNovaWorldDeltaOp> Ops;
};
