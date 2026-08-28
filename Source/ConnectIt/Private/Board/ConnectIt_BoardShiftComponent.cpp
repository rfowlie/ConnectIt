// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_BoardShiftComponent.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Shift/GridMechanics_GridShiftLibrary.h"
#include "ConnectIt_GameplayTags.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTask_Async.h"
#include "Interface/GridShiftInterface.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryComponent.h"


UConnectIt_BoardShiftComponent::UConnectIt_BoardShiftComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UConnectIt_BoardShiftComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!IsValid(GetOwner()))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardShiftComponent: BeginPlay — no owner"));
        return;
    }

    // Resolved via the owner -- this component only assumes it's a sibling
    // of these two component types on whatever actor owns it, same as the
    // plugin's own (now-unused-by-ConnectIt) UBoardShiftComponent used to.
    BoardStateComponentRef   = GetOwner()->FindComponentByClass<UConnectIt_BoardStateComponent>();
    TileRegistryComponentRef = GetOwner()->FindComponentByClass<UGridTileRegistryComponent>();

    if (!IsValid(BoardStateComponentRef))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardShiftComponent: BeginPlay — "
                 "no UConnectIt_BoardStateComponent found on owner"));
    }

    if (!IsValid(TileRegistryComponentRef))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardShiftComponent: BeginPlay — "
                 "no UGridTileRegistryComponent found on owner"));
    }

    UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;

    if (!IsValid(GameEventSubsystem))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardShiftComponent: BeginPlay — "
                 "no UGameEventTaskSubsystem in world"));
        return;
    }

    // Self-register once, persistently -- no external caller ever needs to
    // know this component exists, same as any other ConnectIt_Event_Shift
    // listener would register itself (see GameEventSubsystem_Workflow.txt).
    ShiftAnimationTask = NewObject<UGameEventTask_Async>(this);
    ShiftAnimationTask->bIsPersistentTask = true;
    ShiftAnimationTask->OnExecuteDelegate.BindDynamic(
        this, &UConnectIt_BoardShiftComponent::HandleShiftAnimationExecute);

    GameEventSubsystem->RegisterAsyncTask(ConnectIt_Event_Shift, ShiftAnimationTask, 0);
}

bool UConnectIt_BoardShiftComponent::ComputeShift(
    FConnectItBoardState& MutableState,
    FShiftResult& InShiftResult,
    const FShiftOperation& Operation) const
{
    if (!IsValid(TileRegistryComponentRef))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardShiftComponent: ComputeShift — "
                 "missing TileRegistryComponentRef"));
        return false;
    }

    // Get all positions in the affected row or column
    const TArray<FGridPosition> AffectedPositions = Operation.Axis == EShiftAxis::Row
        ? TileRegistryComponentRef->GetRowPositions(Operation.Index)
        : TileRegistryComponentRef->GetColumnPositions(Operation.Index);

    if (AffectedPositions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardShiftComponent: No positions found for %s %d"),
            Operation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
            Operation.Index);
        return false;
    }

    InShiftResult = UGridMechanics_GridShiftLibrary::ComputeShiftResult(AffectedPositions, Operation);
    ApplyShiftToState(MutableState, InShiftResult);
    return true;
}

void UConnectIt_BoardShiftComponent::ApplyShiftToState(
    FConnectItBoardState& MutableState,
    const FShiftResult& Result) const
{
    // Snapshot positions before remap to prevent mid-remap overwrites
    TMap<FGridPosition, FConnectItTileData> Snapshot;
    for (const auto& [OldPos, NewPos] : Result.PositionRemap)
    {
        const FConnectItTileData* Data = MutableState.GetTileData(OldPos);
        if (Data) Snapshot.Add(OldPos, *Data);
    }

    // Apply to remap atomically from snapshot
    for (const auto& [OldPos, NewPos] : Result.PositionRemap)
    {
        const FConnectItTileData* Data = Snapshot.Find(OldPos);
        if (Data)
        {
            MutableState.SetTileData(NewPos, *Data);
        }
        else
        {
            MutableState.SetTileData(NewPos, FConnectItTileData());
        }
    }
}

void UConnectIt_BoardShiftComponent::HandleShiftAnimationExecute()
{
    if (!IsValid(BoardStateComponentRef))
    {
        if (IsValid(ShiftAnimationTask)) { ShiftAnimationTask->OnComplete.Broadcast(ShiftAnimationTask); }
        return;
    }

    const FConnectItBoardChangeEvent& ChangeEvent = BoardStateComponentRef->GetChangeEvent();

    // Reconstruct FShiftResult from the replicated parallel arrays -- one
    // code path for both server and client, same as every other visual step
    FShiftResult Result;
    for (int32 i = 0; i < ChangeEvent.ShiftFromPositions.Num(); i++)
    {
        Result.PositionRemap.Add(
            ChangeEvent.ShiftFromPositions[i], ChangeEvent.ShiftToPositions[i]);
    }
    Result.WrappingPositions.Append(ChangeEvent.ShiftWrappingPositions);

    if (!StartShiftAnimation(ChangeEvent.ShiftOperation, Result))
    {
        // Animation failed to start -- finish the task now rather than
        // leaving the sequence waiting on a completion that will never fire
        if (IsValid(ShiftAnimationTask)) { ShiftAnimationTask->OnComplete.Broadcast(ShiftAnimationTask); }
    }
}

bool UConnectIt_BoardShiftComponent::StartShiftAnimation(
    const FShiftOperation& Operation, const FShiftResult& Result)
{
    if (bIsShifting)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardShiftComponent: StartShiftAnimation requested "
                 "while a shift animation is already in progress — ignoring"));
        return false;
    }

    if (!IsValid(TileRegistryComponentRef))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardShiftComponent: Cannot animate shift — "
                 "missing TileRegistryComponentRef"));
        return false;
    }

    if (!Result.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardShiftComponent: StartShiftAnimation given "
                 "an invalid FShiftResult"));
        return false;
    }

    ActiveOperation = Operation;
    ActiveResult    = Result;

    // Get all positions in the affected row or column -- needed to build
    // world position/tile actor maps for the instructions below
    const TArray<FGridPosition> AffectedPositions = Operation.Axis == EShiftAxis::Row
        ? TileRegistryComponentRef->GetRowPositions(Operation.Index)
        : TileRegistryComponentRef->GetColumnPositions(Operation.Index);

    TMap<FGridPosition, FVector>        WorldPositions = BuildWorldPositionMap(AffectedPositions);
    TMap<FGridPosition, AGridTileBase*> TileActors     = BuildGridPositionTileMap(AffectedPositions);

    ActiveInstructions = UGridMechanics_GridShiftLibrary::BuildShiftInstructions(
        ActiveResult, TileActors, WorldPositions);

    // Send PrepareShift to all affected tiles
    for (const FTileShiftInstruction& Instruction : ActiveInstructions)
    {
        if (!IsValid(Instruction.TileActor)) continue;

        if (Instruction.TileActor->Implements<UGridShiftInterface>())
        {
            IGridShiftInterface::Execute_PrepareShift(Instruction.TileActor, Instruction);
        }
    }

    // Start the animation
    bIsShifting = true;
    ShiftAlpha  = 0.f;
    SetComponentTickEnabled(true);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardShiftComponent: Shift animation started — %s %d by %d"),
        ActiveOperation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
        ActiveOperation.Index,
        ActiveOperation.GetSignedAmount());

    return true;
}

void UConnectIt_BoardShiftComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsShifting) return;

    // Advance alpha
    ShiftAlpha = FMath::Clamp(
        ShiftAlpha + (DeltaTime / FMath::Max(ShiftDuration, 0.016f)),
        0.f,
        1.f
    );

    // Broadcast current alpha to all tiles
    BroadcastAlphaToTiles(ShiftAlpha);

    // Check for completion
    if (ShiftAlpha >= 1.0f)
    {
        FinaliseShift();
    }
}

void UConnectIt_BoardShiftComponent::BroadcastAlphaToTiles(float Alpha) const
{
    for (const FTileShiftInstruction& Instruction : ActiveInstructions)
    {
        if (!IsValid(Instruction.TileActor)) continue;

        if (Instruction.TileActor->Implements<UGridShiftInterface>())
        {
            IGridShiftInterface::Execute_ApplyShiftAlpha(Instruction.TileActor, Alpha);
        }
    }
}

void UConnectIt_BoardShiftComponent::FinaliseShift()
{
    // Notify all tiles that animation is complete
    for (const FTileShiftInstruction& Instruction : ActiveInstructions)
    {
        if (!IsValid(Instruction.TileActor)) continue;
        if (Instruction.TileActor->Implements<UGridShiftInterface>())
        {
            IGridShiftInterface::Execute_OnShiftComplete(Instruction.TileActor);
        }
    }

    // Reset animation state
    bIsShifting = false;
    ShiftAlpha  = 0.f;
    ActiveInstructions.Empty();
    SetComponentTickEnabled(false);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardShiftComponent: Shift animation complete — %s %d"),
        ActiveOperation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
        ActiveOperation.Index);

    // Complete the gated task directly -- no delegate hop needed since
    // compute, animate, and task-completion all live on this one class
    if (IsValid(ShiftAnimationTask)) { ShiftAnimationTask->OnComplete.Broadcast(ShiftAnimationTask); }
}

TMap<FGridPosition, FVector> UConnectIt_BoardShiftComponent::BuildWorldPositionMap(
    const TArray<FGridPosition>& Positions) const
{
    TMap<FGridPosition, FVector> WorldPositions;
    WorldPositions.Reserve(Positions.Num());

    for (const FGridPosition& Pos : Positions)
    {
        WorldPositions.Add(Pos, TileRegistryComponentRef->GridPositionToWorld(Pos));
    }

    return WorldPositions;
}

TMap<FGridPosition, AGridTileBase*> UConnectIt_BoardShiftComponent::BuildGridPositionTileMap(
    const TArray<FGridPosition>& Positions) const
{
    TMap<FGridPosition, AGridTileBase*> TileActors;
    TileActors.Reserve(Positions.Num());

    for (const FGridPosition& Position : Positions)
    {
        if (AGridTileBase* Tile = TileRegistryComponentRef->GetTileAtPosition(Position))
        {
            TileActors.Add(Position, Tile);
        }
    }

    return TileActors;
}
