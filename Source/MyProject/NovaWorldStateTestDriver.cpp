// Project Nova — dev-only PIE world-state save/load regression driver

#include "NovaWorldStateTestDriver.h"

#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "WorldStateSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogNovaWorldStateTest, Log, All);

namespace
{
	void LogCheck(bool bPass, const FString& What)
	{
		UE_LOG(LogNovaWorldStateTest, Log, TEXT("[WorldStateTest] %s %s"),
			bPass ? TEXT("[PASS]") : TEXT("[FAIL]"), *What);
	}

	// Deterministic seed shared by both phases: two deltas covering all five
	// op kinds. PHASE2 asserts against these exact values after the restart.
	constexpr int64 ExpectedRevision = 2;
	const TCHAR* FlagDoor = TEXT("Nova.E2E.DoorOpened");
	const TCHAR* FlagAlarm = TEXT("Nova.E2E.AlarmRaised");
	const TCHAR* KeyCharge = TEXT("Nova.E2E.Charge");
	constexpr float ExpectedCharge = 50.f; // 42.5 set + 7.5 added
	const TCHAR* KeyLastStory = TEXT("Nova.E2E.LastStory");
	const TCHAR* ExpectedLastStory = TEXT("StoryB_Demo");
}

void ANovaWorldStateTestDriver::BeginPlay()
{
	Super::BeginPlay();

	// Phase is picked by save-file existence so the same driver runs on both
	// sides of the editor restart with zero configuration.
	const bool bPhase2 = FPaths::FileExists(GetE2ESavePath());
	UE_LOG(LogNovaWorldStateTest, Log, TEXT("[WorldStateTest] armed: %s ('%s' %s)"),
		bPhase2 ? TEXT("PHASE2 load/verify/cleanup") : TEXT("PHASE1 seed/save/schema"),
		*GetE2ESavePath(), bPhase2 ? TEXT("exists") : TEXT("absent"));

	if (!bPhase2)
	{
		Schedule(0.5f, &ANovaWorldStateTestDriver::StepSeedAndVerify);
		Schedule(1.0f, &ANovaWorldStateTestDriver::StepSaveAndHash);
		Schedule(1.5f, &ANovaWorldStateTestDriver::StepLegacyAndFutureSchema);
		Schedule(2.0f, &ANovaWorldStateTestDriver::StepPhase1Done);
	}
	else
	{
		Schedule(0.5f, &ANovaWorldStateTestDriver::StepLoadAfterRestart);
		Schedule(1.0f, &ANovaWorldStateTestDriver::StepVerifyRestoredContent);
		Schedule(1.5f, &ANovaWorldStateTestDriver::StepVerifyRestoredHash);
		Schedule(2.0f, &ANovaWorldStateTestDriver::StepCleanupAndDone);
	}
}

UWorldStateSubsystem* ANovaWorldStateTestDriver::GetWorldStateSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UWorldStateSubsystem>() : nullptr;
}

FString ANovaWorldStateTestDriver::GetE2ESavePath()
{
	return FPaths::ProjectSavedDir() / TEXT("Nova") / TEXT("WorldStateE2E.json");
}

FString ANovaWorldStateTestDriver::GetE2EHashPath()
{
	return FPaths::ProjectSavedDir() / TEXT("Nova") / TEXT("WorldStateE2E.hash");
}

FString ANovaWorldStateTestDriver::GetE2ELegacyV1Path()
{
	return FPaths::ProjectSavedDir() / TEXT("Nova") / TEXT("WorldStateE2E_v1.json");
}

FString ANovaWorldStateTestDriver::GetE2EFuturePath()
{
	return FPaths::ProjectSavedDir() / TEXT("Nova") / TEXT("WorldStateE2E_future.json");
}

void ANovaWorldStateTestDriver::Schedule(float Delay, void (ANovaWorldStateTestDriver::*Step)())
{
	FTimerHandle& Handle = StepHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, this, Step, Delay);
}

// --- PHASE1 ---

void ANovaWorldStateTestDriver::StepSeedAndVerify()
{
	UWorldStateSubsystem* WorldStateSys = GetWorldStateSubsystem();
	if (!WorldStateSys)
	{
		LogCheck(false, TEXT("world state subsystem resolved"));
		return;
	}

	FNovaWorldDelta DeltaA;
	DeltaA.DeltaId = TEXT("E2E_DeltaA");
	DeltaA.Source = TEXT("WorldStateTestDriver");
	{
		FNovaWorldDeltaOp& Op = DeltaA.Ops.AddDefaulted_GetRef();
		Op.Op = ENovaWorldDeltaOp::SetFlag;
		Op.Key = FlagDoor;
	}
	{
		FNovaWorldDeltaOp& Op = DeltaA.Ops.AddDefaulted_GetRef();
		Op.Op = ENovaWorldDeltaOp::SetNumeric;
		Op.Key = KeyCharge;
		Op.NumericValue = 42.5f;
	}
	{
		FNovaWorldDeltaOp& Op = DeltaA.Ops.AddDefaulted_GetRef();
		Op.Op = ENovaWorldDeltaOp::SetString;
		Op.Key = KeyLastStory;
		Op.StringValue = ExpectedLastStory;
	}

	FNovaWorldDelta DeltaB;
	DeltaB.DeltaId = TEXT("E2E_DeltaB");
	DeltaB.Source = TEXT("WorldStateTestDriver");
	{
		FNovaWorldDeltaOp& Op = DeltaB.Ops.AddDefaulted_GetRef();
		Op.Op = ENovaWorldDeltaOp::AddNumeric;
		Op.Key = KeyCharge;
		Op.NumericValue = 7.5f;
	}
	{
		FNovaWorldDeltaOp& Op = DeltaB.Ops.AddDefaulted_GetRef();
		Op.Op = ENovaWorldDeltaOp::ClearFlag;
		Op.Key = FlagAlarm;
	}

	const bool bAppliedA = WorldStateSys->ApplyWorldDelta(DeltaA);
	const bool bAppliedB = WorldStateSys->ApplyWorldDelta(DeltaB);
	LogCheck(bAppliedA && bAppliedB, TEXT("both seed deltas applied (all five op kinds)"));

	const FNovaWorldState& State = WorldStateSys->GetWorldState();
	LogCheck(State.Revision == ExpectedRevision,
		FString::Printf(TEXT("revision %lld == %lld after two deltas"), State.Revision, ExpectedRevision));
	LogCheck(WorldStateSys->GetEventFlag(FlagDoor) && !WorldStateSys->GetEventFlag(FlagAlarm),
		TEXT("flags: DoorOpened set, AlarmRaised cleared"));

	const float* Charge = State.NumericValues.Find(KeyCharge);
	const FString* LastStory = State.StringValues.Find(KeyLastStory);
	LogCheck(Charge && FMath::IsNearlyEqual(*Charge, ExpectedCharge),
		FString::Printf(TEXT("numeric Charge %.1f == %.1f (42.5 set + 7.5 added)"),
			Charge ? *Charge : -1.f, ExpectedCharge));
	LogCheck(LastStory && *LastStory == ExpectedLastStory,
		FString::Printf(TEXT("string LastStory '%s' == '%s'"),
			LastStory ? **LastStory : TEXT("<missing>"), ExpectedLastStory));
}

void ANovaWorldStateTestDriver::StepSaveAndHash()
{
	UWorldStateSubsystem* WorldStateSys = GetWorldStateSubsystem();
	if (!WorldStateSys)
	{
		LogCheck(false, TEXT("world state subsystem resolved"));
		return;
	}

	LogCheck(WorldStateSys->SaveWorldStateToJson(GetE2ESavePath()),
		FString::Printf(TEXT("saved world state to '%s'"), *GetE2ESavePath()));

	const FString Hash = WorldStateSys->GetWorldHash();
	LogCheck(!Hash.IsEmpty() && FFileHelper::SaveStringToFile(Hash, *GetE2EHashPath()),
		FString::Printf(TEXT("hash sidecar written (%s)"), *Hash));
}

void ANovaWorldStateTestDriver::StepLegacyAndFutureSchema()
{
	UWorldStateSubsystem* WorldStateSys = GetWorldStateSubsystem();
	if (!WorldStateSys)
	{
		LogCheck(false, TEXT("world state subsystem resolved"));
		return;
	}

	// A pre-Task-C file: same shape, but no schemaVersion field at all.
	// Loading it must succeed and stamp the state to the current version.
	const FString LegacyV1Json = TEXT(
		"{\"revision\":7,"
		"\"eventFlags\":{\"Nova.E2E.LegacyFlag\":true},"
		"\"numericValues\":{},"
		"\"stringValues\":{}}");
	FFileHelper::SaveStringToFile(LegacyV1Json, *GetE2ELegacyV1Path());

	const bool bLegacyLoaded = WorldStateSys->LoadWorldStateFromJson(GetE2ELegacyV1Path());
	const FNovaWorldState& AfterLegacy = WorldStateSys->GetWorldState();
	LogCheck(bLegacyLoaded
			&& AfterLegacy.SchemaVersion == NovaWorldState::CurrentSchemaVersion
			&& AfterLegacy.Revision == 7
			&& WorldStateSys->GetEventFlag(TEXT("Nova.E2E.LegacyFlag")),
		FString::Printf(TEXT("v1 file (no schemaVersion) loaded and migrated to v%d, content intact"),
			NovaWorldState::CurrentSchemaVersion));

	// A file from the future must be refused, leaving the current state untouched.
	const FString FutureJson = TEXT("{\"schemaVersion\":999,\"revision\":123}");
	FFileHelper::SaveStringToFile(FutureJson, *GetE2EFuturePath());

	const bool bFutureLoaded = WorldStateSys->LoadWorldStateFromJson(GetE2EFuturePath());
	LogCheck(!bFutureLoaded && WorldStateSys->GetWorldState().Revision == 7,
		TEXT("schemaVersion=999 file refused, state left untouched"));

	// Restore the seeded state so PHASE2 verifies the real save, not leftovers.
	LogCheck(WorldStateSys->LoadWorldStateFromJson(GetE2ESavePath()),
		TEXT("seeded E2E save re-loaded after schema checks"));
}

void ANovaWorldStateTestDriver::StepPhase1Done()
{
	UE_LOG(LogNovaWorldStateTest, Log,
		TEXT("[WorldStateTest] PHASE1 done — restart the editor, then re-run this driver for PHASE2"));
}

// --- PHASE2 ---

void ANovaWorldStateTestDriver::StepLoadAfterRestart()
{
	UWorldStateSubsystem* WorldStateSys = GetWorldStateSubsystem();
	if (!WorldStateSys)
	{
		LogCheck(false, TEXT("world state subsystem resolved"));
		return;
	}

	LogCheck(WorldStateSys->LoadWorldStateFromJson(GetE2ESavePath()),
		TEXT("E2E save loaded in fresh process"));
	LogCheck(WorldStateSys->GetWorldState().SchemaVersion == NovaWorldState::CurrentSchemaVersion,
		FString::Printf(TEXT("loaded state is schema v%d"), NovaWorldState::CurrentSchemaVersion));
}

void ANovaWorldStateTestDriver::StepVerifyRestoredContent()
{
	UWorldStateSubsystem* WorldStateSys = GetWorldStateSubsystem();
	if (!WorldStateSys)
	{
		LogCheck(false, TEXT("world state subsystem resolved"));
		return;
	}

	const FNovaWorldState& State = WorldStateSys->GetWorldState();
	LogCheck(State.Revision == ExpectedRevision,
		FString::Printf(TEXT("revision restored (%lld == %lld)"), State.Revision, ExpectedRevision));
	LogCheck(WorldStateSys->GetEventFlag(FlagDoor) && !WorldStateSys->GetEventFlag(FlagAlarm)
			&& State.EventFlags.Contains(FlagAlarm),
		TEXT("flags restored: DoorOpened true, AlarmRaised present and false"));

	const float* Charge = State.NumericValues.Find(KeyCharge);
	const FString* LastStory = State.StringValues.Find(KeyLastStory);
	LogCheck(Charge && FMath::IsNearlyEqual(*Charge, ExpectedCharge),
		FString::Printf(TEXT("numeric restored (Charge %.1f)"), Charge ? *Charge : -1.f));
	LogCheck(LastStory && *LastStory == ExpectedLastStory,
		FString::Printf(TEXT("string restored (LastStory '%s')"), LastStory ? **LastStory : TEXT("<missing>")));
}

void ANovaWorldStateTestDriver::StepVerifyRestoredHash()
{
	UWorldStateSubsystem* WorldStateSys = GetWorldStateSubsystem();
	FString SavedHash;
	if (!WorldStateSys || !FFileHelper::LoadFileToString(SavedHash, *GetE2EHashPath()))
	{
		LogCheck(false, TEXT("hash sidecar read back"));
		return;
	}

	SavedHash.TrimStartAndEndInline();
	const FString LiveHash = WorldStateSys->GetWorldHash();
	LogCheck(!SavedHash.IsEmpty() && LiveHash == SavedHash,
		FString::Printf(TEXT("world hash survived restart (%s)"), *LiveHash));
}

void ANovaWorldStateTestDriver::StepCleanupAndDone()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const bool bCleaned =
		PlatformFile.DeleteFile(*GetE2ESavePath())
		& PlatformFile.DeleteFile(*GetE2EHashPath())
		& PlatformFile.DeleteFile(*GetE2ELegacyV1Path())
		& PlatformFile.DeleteFile(*GetE2EFuturePath());
	LogCheck(bCleaned, TEXT("E2E files cleaned up (next run starts at PHASE1)"));

	UE_LOG(LogNovaWorldStateTest, Log, TEXT("[WorldStateTest] done (PHASE2)"));
}
