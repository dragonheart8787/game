// Project Nova — Element ability component (vertical slice stub)

#include "ElementAbilityComponent.h"

#include "AbilityGraphRuntime.h"
#include "IdentityOverrideComponent.h"
#include "NovaAbilityDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogNovaAbility);

UElementAbilityComponent::UElementAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Patch 4: which abilities exist is still C++'s call, but every tunable
	// value now lives in these Content assets (edit + save, no recompile).
	AbilityAssets.Add(TEXT("Slash"), TSoftObjectPtr<UNovaAbilityDataAsset>(
		FSoftObjectPath(TEXT("/Game/Data/Abilities/DA_Ability_Slash.DA_Ability_Slash"))));
	AbilityAssets.Add(TEXT("Edgewall"), TSoftObjectPtr<UNovaAbilityDataAsset>(
		FSoftObjectPath(TEXT("/Game/Data/Abilities/DA_Ability_Edgewall.DA_Ability_Edgewall"))));
}

void UElementAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	Energy = MaxEnergy;

	// Snapshot the configured loadout (C++ defaults or BP overrides) so a Full
	// Identity Override can always be reverted to it. Skip if SetAbilityLoadout
	// already captured it — an override applied before BeginPlay must not
	// become the revert baseline.
	if (DefaultAbilityAssets.IsEmpty())
	{
		DefaultAbilityAssets = AbilityAssets;
	}

	for (const TPair<FName, TSoftObjectPtr<UNovaAbilityDataAsset>>& Pair : AbilityAssets)
	{
		if (!Abilities.Contains(Pair.Key))
		{
			LoadAndRegisterAbilityAsset(Pair.Key);
		}
	}
}

void UElementAbilityComponent::SetAbilityLoadout(const TMap<FName, TSoftObjectPtr<UNovaAbilityDataAsset>>& NewAbilityAssets)
{
	// First swap before BeginPlay's snapshot must still record the real default.
	if (DefaultAbilityAssets.IsEmpty())
	{
		DefaultAbilityAssets = AbilityAssets;
	}
	AbilityAssets = NewAbilityAssets;
	Abilities.Empty();
	Runtimes.Empty();
	CooldownEndTimes.Empty();

	FString Ids;
	for (const TPair<FName, TSoftObjectPtr<UNovaAbilityDataAsset>>& Pair : AbilityAssets)
	{
		if (LoadAndRegisterAbilityAsset(Pair.Key))
		{
			Ids += FString::Printf(TEXT("%s "), *Pair.Key.ToString());
		}
	}
	UE_LOG(LogNovaAbility, Log, TEXT("Ability loadout replaced: %d abilities (%s)"),
		Abilities.Num(), Ids.TrimEnd().IsEmpty() ? TEXT("<none>") : *Ids.TrimEnd());
}

void UElementAbilityComponent::ResetAbilityLoadoutToDefault()
{
	if (DefaultAbilityAssets.IsEmpty())
	{
		UE_LOG(LogNovaAbility, Warning, TEXT("ResetAbilityLoadoutToDefault: no default captured (BeginPlay not run yet?)"));
		return;
	}
	UE_LOG(LogNovaAbility, Log, TEXT("Ability loadout reset to default"));
	SetAbilityLoadout(DefaultAbilityAssets);
}

bool UElementAbilityComponent::LoadAndRegisterAbilityAsset(FName AbilityId)
{
	const TSoftObjectPtr<UNovaAbilityDataAsset>* AssetPtr = AbilityAssets.Find(AbilityId);
	if (!AssetPtr)
	{
		return false;
	}

	const UNovaAbilityDataAsset* Asset = AssetPtr->LoadSynchronous();
	if (!Asset)
	{
		UE_LOG(LogNovaAbility, Warning, TEXT("LoadAndRegisterAbilityAsset: '%s' failed to load (%s)"),
			*AbilityId.ToString(), *AssetPtr->ToSoftObjectPath().ToString());
		return false;
	}

	if (Asset->Definition.AbilityId != AbilityId)
	{
		UE_LOG(LogNovaAbility, Warning,
			TEXT("Ability asset '%s' declares AbilityId '%s' but is mapped as '%s'; the map key wins"),
			*Asset->GetName(), *Asset->Definition.AbilityId.ToString(), *AbilityId.ToString());
	}

	// The map key is authoritative so CastAbilityById(key) always matches.
	FNovaAbilityGraphDef Definition = Asset->Definition;
	Definition.AbilityId = AbilityId;
	RegisterAbility(Definition);

	UE_LOG(LogNovaAbility, Log, TEXT("Loaded ability '%s' from %s"),
		*AbilityId.ToString(), *Asset->GetPathName());
	return true;
}

void UElementAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Energy = FMath::Min(MaxEnergy, Energy + EnergyRegenPerSecond * DeltaTime);
}

void UElementAbilityComponent::RegisterAbility(const FNovaAbilityGraphDef& Definition)
{
	if (Definition.AbilityId.IsNone())
	{
		UE_LOG(LogNovaAbility, Warning, TEXT("RegisterAbility: rejected definition with no AbilityId"));
		return;
	}

	Abilities.Add(Definition.AbilityId, Definition);

	UNovaAbilityGraphRuntime* Runtime = NewObject<UNovaAbilityGraphRuntime>(this);
	Runtime->Initialize(Definition);
	Runtimes.Add(Definition.AbilityId, Runtime);
}

bool UElementAbilityComponent::CastAbilityById(FName AbilityId)
{
	const FNovaAbilityGraphDef* Definition = Abilities.Find(AbilityId);
	if (!Definition && LoadAndRegisterAbilityAsset(AbilityId))
	{
		Definition = Abilities.Find(AbilityId);
	}
	if (!Definition)
	{
		UE_LOG(LogNovaAbility, Warning, TEXT("CastAbilityById: unknown ability '%s'"), *AbilityId.ToString());
		return false;
	}

	// Identity Override can forbid ability use (e.g. Partial Override in public).
	if (const AActor* Owner = GetOwner())
	{
		if (const UIdentityOverrideComponent* Identity = Owner->FindComponentByClass<UIdentityOverrideComponent>())
		{
			if (Identity->AreAbilitiesRestricted())
			{
				BroadcastAbilityEvent(AbilityId, TEXT("CastBlocked_Identity"),
					TEXT("abilities restricted by identity override"));
				return false;
			}
		}
	}

	const float Remaining = GetRemainingCooldown(AbilityId);
	if (Remaining > 0.f)
	{
		BroadcastAbilityEvent(AbilityId, TEXT("CastBlocked_Cooldown"),
			FString::Printf(TEXT("%.1fs remaining"), Remaining));
		return false;
	}

	if (Energy < Definition->EnergyCost)
	{
		BroadcastAbilityEvent(AbilityId, TEXT("CastBlocked_Cost"),
			FString::Printf(TEXT("need %.0f, have %.0f"), Definition->EnergyCost, Energy));
		return false;
	}

	// Single execution path: Shape -> Path -> Spawn -> Affect all run inside the
	// graph runtime. Debug draws stand in for VFX until CastVFX is hooked up.
	UNovaAbilityGraphRuntime* Runtime = Runtimes.FindRef(AbilityId);
	if (!Runtime || !Runtime->Execute(GetOwner(), RuntimeParams, DebugDrawSeconds))
	{
		BroadcastAbilityEvent(AbilityId, TEXT("CastFailed"), TEXT("runtime execute failed"));
		return false;
	}

	Energy -= Definition->EnergyCost;
	CooldownEndTimes.Add(AbilityId, GetWorld()->GetTimeSeconds() + Definition->CooldownSeconds);

	BroadcastAbilityEvent(AbilityId, TEXT("CastStarted"),
		FString::Printf(TEXT("W=%.0f Arc=%.0f R=%.0f Dmg=%.0f"),
			RuntimeParams.Width, RuntimeParams.Arc, RuntimeParams.Range, Definition->Affect.Damage));
	return true;
}

void UElementAbilityComponent::BroadcastAbilityEvent(FName AbilityId, FName EventType, const FString& Detail)
{
	LastAbilityEventText = FString::Printf(TEXT("%s: %s (%s)"),
		*AbilityId.ToString(), *EventType.ToString(), *Detail);
	UE_LOG(LogNovaAbility, Log, TEXT("%s"), *LastAbilityEventText);
	OnAbilityEvent.Broadcast(AbilityId, EventType);
}

void UElementAbilityComponent::SetRuntimeDirection(const FVector& Direction)
{
	RuntimeParams.Direction = Direction.GetSafeNormal();
}

void UElementAbilityComponent::SetRuntimeWidth(float Width)
{
	RuntimeParams.Width = FMath::Max(0.f, Width);
}

void UElementAbilityComponent::SetRuntimeArc(float Arc)
{
	RuntimeParams.Arc = FMath::Clamp(Arc, 0.f, 360.f);
}

void UElementAbilityComponent::SetRuntimeRange(float Range)
{
	RuntimeParams.Range = FMath::Max(0.f, Range);
}

float UElementAbilityComponent::GetRemainingCooldown(FName AbilityId) const
{
	const double* EndTime = CooldownEndTimes.Find(AbilityId);
	if (!EndTime || !GetWorld())
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(*EndTime - GetWorld()->GetTimeSeconds()));
}

TArray<FName> UElementAbilityComponent::GetRegisteredAbilityIds() const
{
	TArray<FName> Ids;
	Abilities.GenerateKeyArray(Ids);
	return Ids;
}
