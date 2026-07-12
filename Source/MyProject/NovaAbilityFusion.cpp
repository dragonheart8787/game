// Project Nova — Ability fusion (Patch 7)

#include "NovaAbilityFusion.h"

namespace
{
	/** Append every active affect node of Def (primary slot + extras) to OutChain. */
	void AppendAffectChain(const FNovaAbilityGraphDef& Def, TArray<FNovaAbilityAffectNode>& OutChain)
	{
		if (Def.Affect.AffectType != ENovaAbilityAffectType::None)
		{
			OutChain.Add(Def.Affect);
		}
		for (const FNovaAbilityAffectNode& Extra : Def.ExtraAffects)
		{
			if (Extra.AffectType != ENovaAbilityAffectType::None)
			{
				OutChain.Add(Extra);
			}
		}
	}
}

FNovaAbilityGraphDef UNovaAbilityFusionLibrary::FuseAbilityGraphDefs(const FNovaAbilityGraphDef& Primary,
	const FNovaAbilityGraphDef& Secondary, FName FusedAbilityId, FText FusedDisplayName)
{
	FNovaAbilityGraphDef Fused;
	Fused.AbilityId = FusedAbilityId;
	Fused.DisplayName = FusedDisplayName;

	// Delivery comes from the primary: its geometry, its path, and its runtime
	// param defaults, so player shaping (SetRuntimeArc/Range/...) behaves exactly
	// like it does on the primary ability.
	Fused.Shape = Primary.Shape;
	Fused.Path = Primary.Path;
	Fused.DefaultParams = Primary.DefaultParams;

	// Single-slot nodes: primary wins when it has a value, otherwise the
	// secondary's contribution fills the empty slot.
	Fused.Spawn = Primary.Spawn.SpawnType != ENovaAbilitySpawnType::None ? Primary.Spawn : Secondary.Spawn;
	Fused.Constraint = Primary.Constraint.ConstraintType != ENovaAbilityConstraintType::None
		? Primary.Constraint
		: Secondary.Constraint;

	// Affects stack: every target the fused shape detects receives both
	// abilities' effect chains, in primary-then-secondary order.
	TArray<FNovaAbilityAffectNode> Chain;
	AppendAffectChain(Primary, Chain);
	AppendAffectChain(Secondary, Chain);
	if (Chain.Num() > 0)
	{
		Fused.Affect = Chain[0];
		Chain.RemoveAt(0);
	}
	Fused.ExtraAffects = MoveTemp(Chain);

	// Costs are derived, not authored: a fusion spends both parts' energy and is
	// on cooldown at least as long as its heaviest part.
	Fused.EnergyCost = Primary.EnergyCost + Secondary.EnergyCost;
	Fused.CooldownSeconds = FMath::Max(Primary.CooldownSeconds, Secondary.CooldownSeconds);

	UE_LOG(LogTemp, Log,
		TEXT("[AbilityFusion] '%s' = '%s' x '%s' (affects %d, constraint %d, cost %.0f, cd %.1fs)"),
		*FusedAbilityId.ToString(), *Primary.AbilityId.ToString(), *Secondary.AbilityId.ToString(),
		1 + Fused.ExtraAffects.Num(), static_cast<int32>(Fused.Constraint.ConstraintType),
		Fused.EnergyCost, Fused.CooldownSeconds);

	return Fused;
}
