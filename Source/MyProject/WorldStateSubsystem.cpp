// Project Nova — WorldState subsystem (vertical slice stub)

#include "WorldStateSubsystem.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

DEFINE_LOG_CATEGORY(LogNovaWorldState);

bool UWorldStateSubsystem::LoadWorldStateFromJson(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogNovaWorldState, Warning, TEXT("LoadWorldStateFromJson: could not read '%s'"), *FilePath);
		return false;
	}

	FNovaWorldState LoadedState;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &LoadedState))
	{
		UE_LOG(LogNovaWorldState, Warning, TEXT("LoadWorldStateFromJson: failed to parse '%s'"), *FilePath);
		return false;
	}

	WorldState = MoveTemp(LoadedState);
	UE_LOG(LogNovaWorldState, Log, TEXT("Loaded world state from '%s' (revision %lld)"), *FilePath, WorldState.Revision);
	return true;
}

bool UWorldStateSubsystem::SaveWorldStateToJson(const FString& FilePath) const
{
	const FString JsonString = SerializeStateToJson();
	if (JsonString.IsEmpty())
	{
		UE_LOG(LogNovaWorldState, Warning, TEXT("SaveWorldStateToJson: serialization failed"));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(LogNovaWorldState, Warning, TEXT("SaveWorldStateToJson: could not write '%s'"), *FilePath);
		return false;
	}
	return true;
}

bool UWorldStateSubsystem::ApplyWorldDelta(const FNovaWorldDelta& Delta)
{
	// Transactional: mutate a copy, commit only if every op succeeds.
	FNovaWorldState Staged = WorldState;

	for (const FNovaWorldDeltaOp& Op : Delta.Ops)
	{
		if (!ApplyOpToState(Staged, Op))
		{
			UE_LOG(LogNovaWorldState, Warning,
				TEXT("ApplyWorldDelta: delta '%s' rolled back (op on key '%s' failed)"),
				*Delta.DeltaId.ToString(), *Op.Key.ToString());
			return false;
		}
	}

	Staged.Revision = WorldState.Revision + 1;
	WorldState = MoveTemp(Staged);
	AppendJournalEntry(Delta);
	OnWorldDeltaApplied.Broadcast(Delta);

	UE_LOG(LogNovaWorldState, Log, TEXT("Applied delta '%s' from '%s' (revision %lld, hash %s)"),
		*Delta.DeltaId.ToString(), *Delta.Source.ToString(), WorldState.Revision, *GetWorldHash());
	return true;
}

FString UWorldStateSubsystem::GetWorldHash() const
{
	const FString JsonString = SerializeStateToJson();
	return FMD5::HashAnsiString(*JsonString);
}

void UWorldStateSubsystem::SetEventFlag(FName Flag, bool bValue)
{
	FNovaWorldDelta Delta;
	Delta.DeltaId = FName(*FString::Printf(TEXT("SetFlag_%s"), *Flag.ToString()));
	Delta.Source = TEXT("Direct");

	FNovaWorldDeltaOp& Op = Delta.Ops.AddDefaulted_GetRef();
	Op.Op = bValue ? ENovaWorldDeltaOp::SetFlag : ENovaWorldDeltaOp::ClearFlag;
	Op.Key = Flag;

	ApplyWorldDelta(Delta);
}

bool UWorldStateSubsystem::GetEventFlag(FName Flag) const
{
	const bool* Found = WorldState.EventFlags.Find(Flag);
	return Found ? *Found : false;
}

FString UWorldStateSubsystem::GetDefaultWorldStatePath()
{
	return FPaths::ProjectSavedDir() / TEXT("Nova") / TEXT("WorldState.json");
}

bool UWorldStateSubsystem::ApplyOpToState(FNovaWorldState& State, const FNovaWorldDeltaOp& Op)
{
	if (Op.Key.IsNone())
	{
		return false;
	}

	switch (Op.Op)
	{
	case ENovaWorldDeltaOp::SetFlag:
		State.EventFlags.Add(Op.Key, true);
		return true;
	case ENovaWorldDeltaOp::ClearFlag:
		State.EventFlags.Add(Op.Key, false);
		return true;
	case ENovaWorldDeltaOp::SetNumeric:
		State.NumericValues.Add(Op.Key, Op.NumericValue);
		return true;
	case ENovaWorldDeltaOp::AddNumeric:
		State.NumericValues.FindOrAdd(Op.Key) += Op.NumericValue;
		return true;
	case ENovaWorldDeltaOp::SetString:
		State.StringValues.Add(Op.Key, Op.StringValue);
		return true;
	default:
		return false;
	}
}

FString UWorldStateSubsystem::SerializeStateToJson() const
{
	FString JsonString;
	FJsonObjectConverter::UStructToJsonObjectString(WorldState, JsonString);
	return JsonString;
}

void UWorldStateSubsystem::AppendJournalEntry(const FNovaWorldDelta& Delta)
{
	FNovaJournalEntry& Entry = Journal.AddDefaulted_GetRef();
	Entry.Revision = WorldState.Revision;
	Entry.DeltaId = Delta.DeltaId;
	Entry.Source = Delta.Source;
	Entry.Timestamp = FDateTime::UtcNow();
	Entry.Description = Delta.Description;
}
