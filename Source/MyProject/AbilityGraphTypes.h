// Project Nova — Ability Graph core types (vertical slice stubs)

#pragma once

#include "CoreMinimal.h"
#include "AbilityGraphTypes.generated.h"

/** Node categories that make up an ability graph. */
UENUM(BlueprintType)
enum class ENovaAbilityNodeType : uint8
{
	Shape,
	Path,
	Constraint,
	Spawn,
	Affect,
	Interact,
	Cost,
	Cooldown
};

/** Runtime-adjustable parameters shared by every ability instance. */
USTRUCT(BlueprintType)
struct FNovaAbilityRuntimeParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float Width = 100.f;

	/** Arc in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float Arc = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float Range = 600.f;

	/** 0..1 curvature of the path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float Curve = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	int32 LinkPoints = 0;

	/** 0..1 charge amount at release. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float Charge = 0.f;

	/** Seconds between press and release. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float ReleaseTiming = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FName TargetSurface = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FName AttachPoint = NAME_None;
};

/** A single node inside an ability graph definition. */
USTRUCT(BlueprintType)
struct FNovaAbilityNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ENovaAbilityNodeType NodeType = ENovaAbilityNodeType::Shape;

	/** Loose key/value payload until node-specific structs exist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TMap<FName, float> ScalarParams;

	/** Downstream node ids this node feeds into. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TArray<FName> LinkedNodes;
};

/** Full definition of one composed ability. */
USTRUCT(BlueprintType)
struct FNovaAbilityGraphDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FName AbilityId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TArray<FNovaAbilityNode> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float CooldownSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float EnergyCost = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilityRuntimeParams DefaultParams;
};
