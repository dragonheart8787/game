// Project Nova — dev-only PIE world-state save/load regression driver (never saved into a level)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NovaWorldStateTestDriver.generated.h"

class UWorldStateSubsystem;

/**
 * Dev-only end-to-end driver for WorldState persistence (Patch 8 Task C):
 * proves flags/revision/hash survive a full process restart, and that the
 * SchemaVersion migration framework folds legacy v1 files and refuses
 * newer-than-current ones. Spawn in memory (never save), start PIE, compare
 * LogNovaWorldStateTest PASS/FAIL lines.
 *
 * Two phases, auto-selected by whether the E2E save file exists:
 *   PHASE1 (no file): apply two deterministic deltas (covers all five op
 *     kinds), assert revision/values, save to Saved/Nova/WorldStateE2E.json,
 *     write the world hash to a sidecar; then prove a handcrafted v1 file
 *     (no schemaVersion field) loads + migrates to current, and a
 *     schemaVersion=999 file is refused with state left untouched.
 *     Ends with "restart editor and re-run for PHASE2".
 *   PHASE2 (file exists): load the file into a FRESH process, assert
 *     schema==current, revision==2, every flag/numeric/string value, and
 *     GetWorldHash() == the sidecar hash from before the restart; then
 *     delete the E2E files so the next run starts at PHASE1 again.
 */
UCLASS(NotBlueprintable)
class ANovaWorldStateTestDriver : public AActor
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

private:

	UWorldStateSubsystem* GetWorldStateSubsystem() const;

	static FString GetE2ESavePath();
	static FString GetE2EHashPath();
	static FString GetE2ELegacyV1Path();
	static FString GetE2EFuturePath();

	void Schedule(float Delay, void (ANovaWorldStateTestDriver::*Step)());

	// --- PHASE1 steps ---
	void StepSeedAndVerify();
	void StepSaveAndHash();
	void StepLegacyAndFutureSchema();
	void StepPhase1Done();

	// --- PHASE2 steps ---
	void StepLoadAfterRestart();
	void StepVerifyRestoredContent();
	void StepVerifyRestoredHash();
	void StepCleanupAndDone();

	TArray<FTimerHandle> StepHandles;
};
