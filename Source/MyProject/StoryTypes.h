// Project Nova — Story / Director core types (vertical slice stubs)

#pragma once

#include "CoreMinimal.h"
#include "StoryTypes.generated.h"

/** Director control-mask states. */
UENUM(BlueprintType)
enum class ENovaControlMask : uint8
{
	/** Player has full control. */
	None,
	/** Brief full lock for framing (0.5–2s max). */
	Hold,
	/** Player keeps control, is nudged by light/NPC/audio/geometry. */
	Guide,
	/** Impact beat: slow-mo, close-up, shake, stinger. */
	Punch
};

/** Story tiers. */
UENUM(BlueprintType)
enum class ENovaStoryType : uint8
{
	/** Chapter-level: main/fixed side quests, can restrict abilities, changes world state. */
	TypeA,
	/** Vignette-level: WorldState-triggered short world events. */
	TypeB
};

/**
 * Identity Override hookup a beat can run when it starts (Patch 5).
 * Kept as its own enum (not ENovaIdentityOverrideMode) so StoryTypes stays
 * decoupled from the component header and "no action" is distinct from
 * "apply mode None".
 */
UENUM(BlueprintType)
enum class ENovaBeatIdentityAction : uint8
{
	/** Beat does not touch identity state. */
	None,
	ApplyFull,
	ApplyPartial,
	ApplyLens,
	Revert
};

/** One beat inside a story timeline. */
USTRUCT(BlueprintType)
struct FNovaStoryBeat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	FName BeatName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	float DurationSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	ENovaControlMask ControlMask = ENovaControlMask::None;

	/** Identity Override action fired when the beat starts (Director hookup). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	ENovaBeatIdentityAction IdentityAction = ENovaBeatIdentityAction::None;

	/** Identity handed to the action (ignored for None/Revert). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	FName IdentityId;
};

/** Definition of one story (Type A or Type B). */
USTRUCT(BlueprintType)
struct FNovaStoryDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	FName StoryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	ENovaStoryType StoryType = ENovaStoryType::TypeA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	TArray<FNovaStoryBeat> Beats;
};
