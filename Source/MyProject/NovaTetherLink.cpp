// Project Nova — Bind tether-link actor (Constraint node: TetherToActor)

#include "NovaTetherLink.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogNovaTether);

ANovaTetherLink::ANovaTetherLink()
{
	// Must tick so the link redraws to the target's *current* location each frame.
	PrimaryActorTick.bCanEverTick = true;

	// A bare scene root keeps the actor well-formed; the link itself never moves.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ANovaTetherLink::InitTether(AActor* InInstigator, AActor* InTarget, float InDurationSeconds)
{
	TetherInstigator = InInstigator;
	TetherTarget = InTarget;
	DurationSeconds = FMath::Max(InDurationSeconds, 0.1f);

	if (InTarget)
	{
		LastTargetLocation = InTarget->GetActorLocation();
	}

	// Auto-release: matches the Slow duration so chain and slow expire together.
	SetLifeSpan(DurationSeconds);

	// Draw once immediately so a zero-follow (stationary target) still shows a link.
	DrawLink();

	UE_LOG(LogNovaTether, Log, TEXT("[Bind] Tether '%s' -> '%s' for %.1fs"),
		InInstigator ? *InInstigator->GetName() : TEXT("<none>"),
		InTarget ? *InTarget->GetName() : TEXT("<none>"),
		DurationSeconds);
}

void ANovaTetherLink::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// If the target vanished (destroyed / hidden-and-gone), the tether has nothing
	// to anchor to — end it early rather than draw to a stale location.
	if (!TetherTarget.IsValid())
	{
		Destroy();
		return;
	}

	LastTargetLocation = TetherTarget->GetActorLocation();
	DrawLink();
}

void ANovaTetherLink::DrawLink() const
{
	UWorld* World = GetWorld();
	if (!World || !TetherInstigator.IsValid() || !TetherTarget.IsValid())
	{
		return;
	}

	// Purple neon = Bind family (quantum-fracture energy chains). Redrawn every
	// frame with a single-frame lifetime so it tracks the moving target.
	DrawDebugLine(World, TetherInstigator->GetActorLocation(), TetherTarget->GetActorLocation(),
		FColor::Purple, false, 0.f, 0, 4.f);
}

void ANovaTetherLink::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Final endpoint proves the per-frame follow: it should equal the target's
	// last position, not the cast-time position, if Tick tracked a moved target.
	UE_LOG(LogNovaTether, Log, TEXT("[Bind] Tether '%s' released (final target endpoint %s)"),
		*GetName(), *LastTargetLocation.ToCompactString());
	Super::EndPlay(EndPlayReason);
}
