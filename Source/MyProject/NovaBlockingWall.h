// Project Nova — Edgewall blocking actor (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaBlockingWall.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * Physical wall left behind by the Edgewall ability's Spawn node.
 * Blocking box collision + placeholder cube visual; destroys itself after
 * LifeSeconds. Spawn deferred and call InitWall before FinishSpawning.
 */
UCLASS()
class ANovaBlockingWall : public AActor
{
	GENERATED_BODY()

public:

	ANovaBlockingWall();

	/** Sizes collision and visual, and sets the auto-destroy lifetime. */
	void InitWall(const FVector& InHalfExtent, float InLifeSeconds);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wall", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wall", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> MeshComp;

	FVector HalfExtent = FVector(30.f, 200.f, 150.f);

	float LifeSeconds = 5.f;
};
