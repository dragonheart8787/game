// Project Nova — ability graph definition as an editable Content asset (Patch 4)

#include "NovaAbilityDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaAsset, Log, All);

void UNovaAbilityDataAsset::PostLoad()
{
	Super::PostLoad();

	UE_LOG(LogNovaAsset, Verbose,
		TEXT("PostLoad '%s' (legacy affect=%d, legacy extras=%d, legacy cost=%.0f, legacy cd=%.1f, affects=%d)"),
		*GetPathName(), static_cast<int32>(Definition.Affect.AffectType),
		Definition.ExtraAffects.Num(), Definition.EnergyCost, Definition.CooldownSeconds,
		Definition.Affects.Num());

	// Pre-Patch-8 assets serialized single-slot Affect/ExtraAffects and the
	// top-level EnergyCost/CooldownSeconds floats (and, until they are re-saved,
	// keep doing so). Fold them into the endgame shape here; idempotent, so a
	// migrated asset pays nothing.
	if (Definition.MigrateLegacyFields())
	{
		UE_LOG(LogNovaAsset, Log,
			TEXT("'%s': migrated pre-Patch-8 fields (affects=%d, cost=%.0f, cd=%.1fs) (re-save to persist)"),
			*GetPathName(), Definition.Affects.Num(),
			Definition.Cost.Amount, Definition.Cooldown.CooldownSeconds);
	}
}
