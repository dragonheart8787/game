// Project Nova — Bind tether-link actor (Constraint node: TetherToActor)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaTetherLink.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNovaTether, Log, All);

/**
 * Visual "chain" left behind by a Constraint(TetherToActor) node. Ticks every
 * frame and redraws a DrawDebugLine from the instigator to the tethered target
 * so the link follows a moving target, then destroys itself after Duration.
 * Purely a debug-draw stand-in for a real chain VFX (mirrors ANovaBlockingWall's
 * persistent-actor pattern, but tick-driven so it can track the target).
 */
UCLASS(NotBlueprintable)
class ANovaTetherLink : public AActor
{
	GENERATED_BODY()

public:

	ANovaTetherLink();

	/** Bind the link to its endpoints and arm its lifetime. Call right after spawn. */
	void InitTether(AActor* InInstigator, AActor* InTarget, float InDurationSeconds);

	virtual void Tick(float DeltaSeconds) override;

	/** The actor this link is anchored to (null once it dies / target is gone). */
	AActor* GetTetherTarget() const { return TetherTarget.Get(); }

	/** Target location as of the last redraw — the regression driver reads this
	 *  to confirm the tether followed a moved target. */
	FVector GetLastTargetLocation() const { return LastTargetLocation; }

protected:

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	void DrawLink() const;

	TWeakObjectPtr<AActor> TetherInstigator;
	TWeakObjectPtr<AActor> TetherTarget;

	float DurationSeconds = 3.f;

	FVector LastTargetLocation = FVector::ZeroVector;
};
