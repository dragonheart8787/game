// Project Nova — Debug overlay subsystem (vertical slice stub)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NovaDebugSubsystem.generated.h"

/**
 * On-screen debug overlay: FPS, current story/beat/control mask, world hash,
 * ability params, cooldowns and event flags. Toggled from the character (ToggleDebug).
 * Slice version draws via AddOnScreenDebugMessage; a UMG widget can replace it later.
 */
UCLASS()
class UNovaDebugSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Debug")
	void ToggleDebugDisplay() { bDisplayEnabled = !bDisplayEnabled; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Debug")
	bool IsDebugDisplayEnabled() const { return bDisplayEnabled; }

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	void DrawOverlay(float DeltaTime) const;

	bool bDisplayEnabled = true;
};
