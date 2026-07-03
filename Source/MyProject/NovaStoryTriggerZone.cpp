// Project Nova — Story trigger zone (vertical slice stub)

#include "NovaStoryTriggerZone.h"

#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "NovaPlayerCharacter.h"
#include "StoryDirectorSubsystem.h"

ANovaStoryTriggerZone::ANovaStoryTriggerZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->InitBoxExtent(FVector(150.f, 150.f, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);
	SetRootComponent(TriggerBox);
}

void ANovaStoryTriggerZone::BeginPlay()
{
	Super::BeginPlay();

	// Zones have no mesh yet — outline them so they're findable in PIE.
	const FColor ZoneColor = (StoryToTrigger == ENovaStoryType::TypeA) ? FColor::Green : FColor::Orange;
	DrawDebugBox(GetWorld(), GetActorLocation(), TriggerBox->GetScaledBoxExtent(), GetActorQuat(),
		ZoneColor, /*bPersistentLines=*/true, -1.f, 0, 4.f);
}

void ANovaStoryTriggerZone::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!Cast<ANovaPlayerCharacter>(OtherActor))
	{
		return;
	}

	UStoryDirectorSubsystem* Story = GetGameInstance()->GetSubsystem<UStoryDirectorSubsystem>();
	if (!Story)
	{
		return;
	}

	if (StoryToTrigger == ENovaStoryType::TypeA)
	{
		Story->StartStoryA();
	}
	else
	{
		Story->StartStoryB();
	}
}
