// Project Nova — Edgewall blocking actor (vertical slice stub)

#include "NovaBlockingWall.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaWall, Log, All);

ANovaBlockingWall::ANovaBlockingWall()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(HalfExtent);
	Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Box);

	// Placeholder visual: engine cube scaled to the box; collision lives on Box only.
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(Box);
	MeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(CubeMesh.Object);
	}
}

void ANovaBlockingWall::InitWall(const FVector& InHalfExtent, float InLifeSeconds)
{
	HalfExtent = InHalfExtent;
	LifeSeconds = FMath::Max(InLifeSeconds, 0.1f);

	Box->SetBoxExtent(HalfExtent);
	if (MeshComp->GetStaticMesh())
	{
		// Engine cube is 100cm; scale it to fill the collision box.
		MeshComp->SetRelativeScale3D(HalfExtent * 2.f / 100.f);
	}
}

void ANovaBlockingWall::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSeconds);

	// Debug visualization for the wall's full lifetime (green = Sculpt-family).
	DrawDebugBox(GetWorld(), GetActorLocation(), HalfExtent, GetActorQuat(),
		FColor::Green, false, LifeSeconds, 0, 2.f);

	UE_LOG(LogNovaWall, Log, TEXT("[Edgewall] Wall '%s' up at %s (extent %s) for %.1fs"),
		*GetName(), *GetActorLocation().ToCompactString(), *HalfExtent.ToCompactString(), LifeSeconds);
}

void ANovaBlockingWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogNovaWall, Log, TEXT("[Edgewall] Wall '%s' down"), *GetName());
	Super::EndPlay(EndPlayReason);
}
