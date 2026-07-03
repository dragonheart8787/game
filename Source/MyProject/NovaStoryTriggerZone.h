// Project Nova — Story trigger zone (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryTypes.h"
#include "NovaStoryTriggerZone.generated.h"

class UBoxComponent;

/**
 * Walk-in trigger volume that starts one of the demo stories.
 * TypeA -> StartStoryA, TypeB -> StartStoryB.
 */
UCLASS()
class ANovaStoryTriggerZone : public AActor
{
	GENERATED_BODY()

public:

	ANovaStoryTriggerZone();

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	/** Which demo story this zone starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story")
	ENovaStoryType StoryToTrigger = ENovaStoryType::TypeA;

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> TriggerBox;
};
