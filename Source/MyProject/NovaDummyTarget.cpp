// Project Nova — Dummy training target (vertical slice stub)

#include "NovaDummyTarget.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogNovaDummy);

ANovaDummyTarget::ANovaDummyTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(45.f, 90.f);
	Capsule->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Capsule);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(Capsule);
	MeshComp->SetCollisionProfileName(TEXT("NoCollision"));

	// Placeholder visual: engine basic cylinder, stretched to dummy proportions.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(CylinderMesh.Object);
		MeshComp->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.8f));
	}
}

void ANovaDummyTarget::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

float ANovaDummyTarget::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Health = FMath::Clamp(Health - DamageAmount, 0.f, MaxHealth);

	const FString Message = FString::Printf(TEXT("[Dummy %s] took %.0f damage, health %.0f/%.0f"),
		*GetName(), DamageAmount, Health, MaxHealth);
	UE_LOG(LogNovaDummy, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		// Keyed on the actor id so repeated hits update in place.
		GEngine->AddOnScreenDebugMessage(static_cast<int32>(GetUniqueID()), 3.f, FColor::Orange, Message);
	}

	if (Health <= 0.f)
	{
		UE_LOG(LogNovaDummy, Log, TEXT("[Dummy %s] destroyed"), *GetName());
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}

	return Applied;
}

void ANovaDummyTarget::ApplySlow(float SpeedMultiplier, float DurationSeconds)
{
	CurrentSpeedMultiplier = FMath::Clamp(SpeedMultiplier, 0.f, 1.f);

	// Re-applying refreshes the timer rather than stacking; a single restore wins.
	GetWorldTimerManager().SetTimer(SlowTimerHandle, this, &ANovaDummyTarget::RestoreSpeed,
		FMath::Max(DurationSeconds, 0.01f), false);

	const FString Message = FString::Printf(TEXT("[Dummy %s] slowed to %.0f%% (%.0f -> %.0f) for %.1fs"),
		*GetName(), CurrentSpeedMultiplier * 100.f, BaseMoveSpeed, GetCurrentMoveSpeed(), DurationSeconds);
	UE_LOG(LogNovaDummy, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<int32>(GetUniqueID()) + 1, DurationSeconds,
			FColor::Purple, Message);
	}
}

void ANovaDummyTarget::RestoreSpeed()
{
	CurrentSpeedMultiplier = 1.f;

	const FString Message = FString::Printf(TEXT("[Dummy %s] slow expired, speed restored to %.0f"),
		*GetName(), GetCurrentMoveSpeed());
	UE_LOG(LogNovaDummy, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<int32>(GetUniqueID()) + 1, 3.f, FColor::Green, Message);
	}
}
