// Project Nova — Ability Graph core types (vertical slice stubs)

#pragma once

#include "CoreMinimal.h"
#include "AbilityGraphTypes.generated.h"

/**
 * Node categories that make up an ability graph.
 * Shape/Path/Constraint/Spawn/Affect/Cost/Cooldown have typed node structs
 * below; Interact is reserved so adding it later doesn't break the enum.
 */
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

/** Geometric footprint the ability's effect covers. */
UENUM(BlueprintType)
enum class ENovaAbilityShapeType : uint8
{
	/** Direction ray (magenta debug line). */
	Line,
	/** Arc/Range cone (purple debug cone) — Slash. */
	Cone,
	/** Radius sphere (cyan debug sphere). */
	Sphere
};

/** How the shape travels before it resolves. */
UENUM(BlueprintType)
enum class ENovaAbilityPathType : uint8
{
	/** Resolves in place at the caster — Slash. */
	Instant,
	/** Resolves TravelDistance along Direction — Edgewall. */
	Linear
};

/** What (if anything) the graph leaves behind in the world. */
UENUM(BlueprintType)
enum class ENovaAbilitySpawnType : uint8
{
	None,
	/** A collision box that physically blocks movement for LifeSeconds — Edgewall. */
	BlockingWall
};

/** Effect applied to actors caught in the resolved shape. */
UENUM(BlueprintType)
enum class ENovaAbilityAffectType : uint8
{
	None,
	/** ApplyDamage to every actor in the shape — Slash. */
	Damage,
	/** Pure physical blocking; the spawned actor's collision does the work. */
	Block,
	/** Scale the target's move speed down for a duration, then auto-restore — Bind.
	 *  (Appended last on purpose: keeps Damage=1/Block=2 stable for existing assets.) */
	Slow
};

/** How the ability's effect is fixed or pulled toward an object / location. */
UENUM(BlueprintType)
enum class ENovaAbilityConstraintType : uint8
{
	None,
	/** Anchor the effect to the hit actor and follow it for a duration — Bind. */
	TetherToActor
};

/** What resource a cast spends. Append only — serialized by value (P6 lesson). */
UENUM(BlueprintType)
enum class ENovaAbilityCostType : uint8
{
	/** Spend from the caster's energy pool (the only pool that exists today). */
	Energy
	// Reserved next: Health, Material — append here, never insert.
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

/**
 * Shape node: which geometry the ability covers.
 * Overrides <= 0 mean "inherit the matching runtime param" so player shaping
 * (SetRuntimeArc/Range/Width) keeps working; a positive value pins the shape.
 */
USTRUCT(BlueprintType)
struct FNovaAbilityShapeNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ENovaAbilityShapeType ShapeType = ENovaAbilityShapeType::Cone;

	/** Cone arc in degrees; <= 0 uses runtime Arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float ArcDegreesOverride = 0.f;

	/** Line/Cone length; <= 0 uses runtime Range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float RangeOverride = 0.f;

	/** Sphere radius; <= 0 uses runtime Width * 0.5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float RadiusOverride = 0.f;
};

/** Path node: where the shape resolves relative to the caster. */
USTRUCT(BlueprintType)
struct FNovaAbilityPathNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ENovaAbilityPathType PathType = ENovaAbilityPathType::Instant;

	/** Linear only: how far along Direction the effect origin travels (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float TravelDistance = 300.f;
};

/** Spawn node: persistent actor left behind at the resolved origin. */
USTRUCT(BlueprintType)
struct FNovaAbilitySpawnNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ENovaAbilitySpawnType SpawnType = ENovaAbilitySpawnType::None;

	/** BlockingWall: box half extents — X thin (along Direction), Y wide, Z tall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FVector WallHalfExtent = FVector(30.f, 200.f, 150.f);

	/** Seconds before the spawned actor destroys itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.1"))
	float LifeSeconds = 5.f;
};

/** Affect node: effect applied to actors detected inside the shape. */
USTRUCT(BlueprintType)
struct FNovaAbilityAffectNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ENovaAbilityAffectType AffectType = ENovaAbilityAffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	float Damage = 20.f;

	/** Slow: fraction of base move speed the target keeps while slowed (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SlowSpeedMultiplier = 0.5f;

	/** Slow: seconds the slow lasts before the target auto-restores. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.0"))
	float SlowDurationSeconds = 3.f;
};

/**
 * Constraint node: how the resolved effect is anchored/pulled to a target.
 * TetherToActor spawns a follow-link on the hit actor for TetherDurationSeconds
 * (the "chain" visual). Pure serializable data, mirroring the other nodes.
 */
USTRUCT(BlueprintType)
struct FNovaAbilityConstraintNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ENovaAbilityConstraintType ConstraintType = ENovaAbilityConstraintType::None;

	/** TetherToActor: seconds the tether follows the target before releasing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.0"))
	float TetherDurationSeconds = 3.f;
};

/** Cost node: what a successful cast spends (Patch 8 endgame shape). */
USTRUCT(BlueprintType)
struct FNovaAbilityCostNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ENovaAbilityCostType CostType = ENovaAbilityCostType::Energy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.0"))
	float Amount = 10.f;
};

/** Cooldown node: when the ability may cast again (Patch 8 endgame shape). */
USTRUCT(BlueprintType)
struct FNovaAbilityCooldownNode
{
	GENERATED_BODY()

	/** Seconds after a successful cast before this ability is ready again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.0"))
	float CooldownSeconds = 1.f;

	/**
	 * Reserved: abilities sharing a non-None group will one day share cooldown
	 * state (fusion vs component abilities live here). Field only for now — the
	 * runtime keys cooldowns by AbilityId and ignores this entirely.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FName SharedCooldownGroup = NAME_None;
};

/**
 * Full definition of one composed ability: Shape -> Path -> Spawn -> Affect,
 * with an optional Constraint that anchors the effect to a hit target.
 * Kept as flat data (no inheritance) so defs can later serialize to
 * DataAssets / JSON for the Ability Graph editor.
 */
USTRUCT(BlueprintType)
struct FNovaAbilityGraphDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FName AbilityId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilityShapeNode Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilityPathNode Path;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilitySpawnNode Spawn;

	/**
	 * Affect chain, run in order on every detected target (Patch 8 endgame shape).
	 * A plain ability has one entry; a fusion stacks its component abilities'
	 * effects — e.g. RendLock = [Slash's Damage, Bind's Slow]. Empty = no effect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TArray<FNovaAbilityAffectNode> Affects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilityConstraintNode Constraint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilityCostNode Cost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilityCooldownNode Cooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FNovaAbilityRuntimeParams DefaultParams;

	// --- Pre-Patch-8 migration shims (LEGACY — never read these directly) -----
	// These keep their ORIGINAL serialized names so pre-P8 packages load straight
	// into them with no CoreRedirects involved. MigrateLegacyFields() folds them
	// into Affects and CLEARS them back to defaults, so a re-saved asset omits
	// the tags entirely (delta serialization) and the fold is idempotent. Hidden
	// from the editor / Blueprint on purpose; delete once all assets are re-saved
	// and a follow-up patch retires the shims.

	/** LEGACY pre-P8 single affect slot; folds to the front of Affects. */
	UPROPERTY()
	FNovaAbilityAffectNode Affect;

	/** LEGACY Patch 7 transitional fused-affect chain; appends to Affects. */
	UPROPERTY()
	TArray<FNovaAbilityAffectNode> ExtraAffects;

	// The float shims default to -1 (impossible: both clamped >= 0 in the old
	// editor UI). Pre-P8 packages only serialized these when the authored value
	// differed from the OLD defaults (10 / 1), and the new nodes' defaults
	// reproduce exactly those values — so "tag absent" (shim stays -1) needs no
	// migration, and "tag present" (shim >= 0) folds into the node and resets.

	/** LEGACY pre-P8 top-level energy cost; folds into Cost.Amount. */
	UPROPERTY()
	float EnergyCost = -1.f;

	/** LEGACY pre-P8 top-level cooldown; folds into Cooldown.CooldownSeconds. */
	UPROPERTY()
	float CooldownSeconds = -1.f;

	/**
	 * Folds pre-Patch-8 fields into their endgame replacements, clearing the
	 * legacy slots (see note above). Called from UNovaAbilityDataAsset::PostLoad;
	 * returns true when anything was actually migrated so the caller can log it.
	 */
	bool MigrateLegacyFields()
	{
		bool bMigrated = false;
		if (Affect.AffectType != ENovaAbilityAffectType::None)
		{
			Affects.Insert(Affect, 0);
			Affect = FNovaAbilityAffectNode();
			bMigrated = true;
		}
		if (ExtraAffects.Num() > 0)
		{
			Affects.Append(ExtraAffects);
			ExtraAffects.Reset();
			bMigrated = true;
		}
		if (EnergyCost >= 0.f)
		{
			Cost.Amount = EnergyCost;
			EnergyCost = -1.f;
			bMigrated = true;
		}
		if (CooldownSeconds >= 0.f)
		{
			Cooldown.CooldownSeconds = CooldownSeconds;
			CooldownSeconds = -1.f;
			bMigrated = true;
		}
		return bMigrated;
	}
};
