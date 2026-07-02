// Project Nova — Identity Override component (vertical slice stub)

#include "IdentityOverrideComponent.h"

DEFINE_LOG_CATEGORY(LogNovaIdentity);

void UIdentityOverrideComponent::ApplyFullOverride(FName IdentityId)
{
	// Full Override swaps abilities/gear/relations/objectives — stubbed for the slice.
	SetOverride(ENovaIdentityOverrideMode::Full, IdentityId, /*bRestrictAbilities=*/false);
}

void UIdentityOverrideComponent::ApplyPartialOverride(FName IdentityId)
{
	SetOverride(ENovaIdentityOverrideMode::Partial, IdentityId, /*bRestrictAbilities=*/true);
	Exposure = 0.f;
}

void UIdentityOverrideComponent::ApplyLensOverride(FName IdentityId)
{
	SetOverride(ENovaIdentityOverrideMode::Lens, IdentityId, /*bRestrictAbilities=*/true);
}

void UIdentityOverrideComponent::RevertOverride()
{
	SetOverride(ENovaIdentityOverrideMode::None, NAME_None, /*bRestrictAbilities=*/false);
	Exposure = 0.f;
}

void UIdentityOverrideComponent::AddExposure(float Amount)
{
	if (OverrideMode != ENovaIdentityOverrideMode::Partial)
	{
		return;
	}
	Exposure = FMath::Clamp(Exposure + Amount, 0.f, 1.f);
	if (Exposure >= 1.f)
	{
		UE_LOG(LogNovaIdentity, Log, TEXT("Exposure maxed — cover blown for identity '%s'"), *ActiveIdentityId.ToString());
	}
}

void UIdentityOverrideComponent::SetOverride(ENovaIdentityOverrideMode Mode, FName IdentityId, bool bRestrictAbilities)
{
	OverrideMode = Mode;
	ActiveIdentityId = IdentityId;
	bAbilitiesRestricted = bRestrictAbilities;

	UE_LOG(LogNovaIdentity, Log, TEXT("Identity override: mode %d, identity '%s', abilities %s"),
		static_cast<int32>(Mode), *IdentityId.ToString(), bRestrictAbilities ? TEXT("restricted") : TEXT("free"));

	OnIdentityChanged.Broadcast(Mode, IdentityId);
}
