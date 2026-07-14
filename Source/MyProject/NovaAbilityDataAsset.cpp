// Project Nova — ability graph definition as an editable Content asset (Patch 4)

#include "NovaAbilityDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaAsset, Log, All);

void UNovaAbilityDataAsset::PostLoad()
{
	Super::PostLoad();

	UE_LOG(LogNovaAsset, Verbose, TEXT("PostLoad '%s' (legacy affect=%d, legacy extras=%d, affects=%d)"),
		*GetPathName(), static_cast<int32>(Definition.Affect.AffectType),
		Definition.ExtraAffects.Num(), Definition.Affects.Num());

	// Pre-Patch-8 assets serialized single-slot Affect/ExtraAffects (and, until
	// they are re-saved, keep doing so). Fold them into the endgame shape here;
	// idempotent, so a migrated asset pays nothing.
	if (Definition.MigrateLegacyFields())
	{
		UE_LOG(LogNovaAsset, Log, TEXT("'%s': migrated pre-Patch-8 fields into Affects[%d] (re-save to persist)"),
			*GetPathName(), Definition.Affects.Num());
	}
}
