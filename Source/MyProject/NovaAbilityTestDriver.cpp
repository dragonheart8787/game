// Project Nova — dev-only PIE ability regression driver

#include "NovaAbilityTestDriver.h"

#include "ElementAbilityComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NovaDummyTarget.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaTestDriver, Log, All);

void ANovaAbilityTestDriver::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogNovaTestDriver, Log,
		TEXT("[TestDriver] armed: Slash+recast @1.0s, Edgewall @2.5s, Slash @4.0s"));
	GetWorldTimerManager().SetTimer(Step1Handle, this, &ANovaAbilityTestDriver::StepSlashTwice, 1.0f);
	GetWorldTimerManager().SetTimer(Step2Handle, this, &ANovaAbilityTestDriver::StepEdgewall, 2.5f);
	GetWorldTimerManager().SetTimer(Step3Handle, this, &ANovaAbilityTestDriver::StepSlash, 4.0f);
}

UElementAbilityComponent* ANovaAbilityTestDriver::GetPlayerAbilityComp() const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	return Pawn ? Pawn->FindComponentByClass<UElementAbilityComponent>() : nullptr;
}

void ANovaAbilityTestDriver::AimAtDummy(UElementAbilityComponent& Comp) const
{
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	for (TActorIterator<ANovaDummyTarget> It(GetWorld()); Pawn && It; ++It)
	{
		Comp.SetRuntimeDirection(It->GetActorLocation() - Pawn->GetActorLocation());
		return;
	}
}

void ANovaAbilityTestDriver::StepSlashTwice()
{
	if (UElementAbilityComponent* Comp = GetPlayerAbilityComp())
	{
		AimAtDummy(*Comp);
		UE_LOG(LogNovaTestDriver, Log, TEXT("[TestDriver] step 1: Slash + same-frame re-cast"));
		Comp->CastAbilityById(TEXT("Slash"));
		Comp->CastAbilityById(TEXT("Slash"));
	}
}

void ANovaAbilityTestDriver::StepEdgewall()
{
	if (UElementAbilityComponent* Comp = GetPlayerAbilityComp())
	{
		AimAtDummy(*Comp);
		UE_LOG(LogNovaTestDriver, Log, TEXT("[TestDriver] step 2: Edgewall"));
		Comp->CastAbilityById(TEXT("Edgewall"));
	}
}

void ANovaAbilityTestDriver::StepSlash()
{
	if (UElementAbilityComponent* Comp = GetPlayerAbilityComp())
	{
		AimAtDummy(*Comp);
		UE_LOG(LogNovaTestDriver, Log, TEXT("[TestDriver] step 3: Slash"));
		Comp->CastAbilityById(TEXT("Slash"));
	}
}
