// Project Nova — Ability Graph runtime (vertical slice stub)

#include "AbilityGraphRuntime.h"

#include "GameFramework/Actor.h"

void UNovaAbilityGraphRuntime::Initialize(const FNovaAbilityGraphDef& InDefinition)
{
	Definition = InDefinition;
	bInitialized = true;
}

bool UNovaAbilityGraphRuntime::Execute(AActor* Instigator, const FNovaAbilityRuntimeParams& Params)
{
	if (!bInitialized)
	{
		return false;
	}

	// Stub: walk the nodes so the execution order is visible in the log.
	// Real evaluation (Shape/Path/Constraint/Spawn/Affect/Interact) comes in a later patch.
	for (const FNovaAbilityNode& Node : Definition.Nodes)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[AbilityGraph] %s: node '%s' (type %d)"),
			*Definition.AbilityId.ToString(), *Node.NodeId.ToString(), static_cast<int32>(Node.NodeType));
	}

	UE_LOG(LogTemp, Log, TEXT("[AbilityGraph] Executed '%s' by '%s' (range %.0f, width %.0f, arc %.0f)"),
		*Definition.AbilityId.ToString(),
		Instigator ? *Instigator->GetName() : TEXT("None"),
		Params.Range, Params.Width, Params.Arc);
	return true;
}
