// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConnectIt_Structs.h"
#include "Board/Shift/GridShiftTypes.h"
#include "ConnectIt_BoardShiftComponent.generated.h"

class AGridTileBase;
class UGridTileRegistryComponent;

// Owns shift end-to-end for ConnectIt: computing the position remap,
// applying it to FConnectItBoardState, animating it, and self-registering
// as the ConnectIt_Event_Shift gated-sequence listener. Absorbed what used
// to be a separate, generic UBoardShiftComponent (UnrealGridMechanics
// plugin) -- that abstraction only ever had one consumer, so the extra
// indirection wasn't earning its keep. Still reuses the plugin's stateless
// math/interface types (UGridMechanics_GridShiftLibrary, IGridShiftInterface)
// directly, just without a separate live component in between.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_BoardShiftComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UConnectIt_BoardShiftComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // Duration of the shift animation in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConnectIt|Shift")
    float ShiftDuration = 0.5f;

    // Computes the position remap for a shift operation -- pure and
    // synchronous, no tile actor calls, no animation. Called by
    // AConnectIt_BoardManager::HandleShiftRequest before committing state.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Shift")
    bool ComputeShift(
        FConnectItBoardState& MutableState,
        FShiftResult& InShiftResult,
        const FShiftOperation& Operation) const;

    // Applies Result's position remap onto MutableState. Snapshots old data
    // before applying so overlapping remaps (e.g. a full row rotating
    // through itself) don't read data already overwritten by an earlier
    // iteration of the same loop.
    void ApplyShiftToState(FConnectItBoardState& MutableState, const FShiftResult& Result) const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Shift")
    bool IsShifting() const { return bIsShifting; }

protected:

    virtual void BeginPlay() override;

private:

    // Bound as ConnectIt_Event_Shift's registered UGameEventTask_Async
    // (persistent -- see BeginPlay). Reads the just-committed ChangeEvent's
    // shift payload and starts the animation. The board manager has no
    // involvement in or knowledge of this; it only decided that
    // ConnectIt_Event_Shift should fire, same as every other event tag.
    UFUNCTION()
    void HandleShiftAnimationExecute();

    // Starts animating an already-computed shift result -- builds
    // instructions, sends PrepareShift to tiles, starts ticking. Returns
    // false (caller should complete the task immediately) if a shift is
    // already animating or required components are missing.
    bool StartShiftAnimation(const FShiftOperation& Operation, const FShiftResult& Result);

    // Called from TickComponent once ShiftAlpha reaches 1 -- sends
    // OnShiftComplete to tiles, resets animation state, and completes
    // ShiftAnimationTask directly. No delegate hop needed here since
    // compute, animate, and task-completion all live on this one class.
    void FinaliseShift();

    TMap<FGridPosition, FVector> BuildWorldPositionMap(const TArray<FGridPosition>& Positions) const;
    TMap<FGridPosition, AGridTileBase*> BuildGridPositionTileMap(const TArray<FGridPosition>& Positions) const;
    void BroadcastAlphaToTiles(float Alpha) const;

    // Resolved once in BeginPlay via GetOwner() -- this component only ever
    // sits on AConnectIt_BoardManager.
    UPROPERTY()
    TObjectPtr<class UConnectIt_BoardStateComponent> BoardStateComponentRef = nullptr;

    UPROPERTY()
    TObjectPtr<UGridTileRegistryComponent> TileRegistryComponentRef = nullptr;

    // Persistent (bIsPersistentTask = true) -- registered once here in
    // BeginPlay, auto-re-added to ConnectIt_Event_Shift's manager on every
    // future trigger, so this component never needs to know when a new
    // shift happens.
    UPROPERTY()
    TObjectPtr<class UGameEventTask_Async> ShiftAnimationTask = nullptr;

    // Active animation state -- ported directly from the plugin's
    // UBoardShiftComponent, unchanged
    bool bIsShifting = false;
    float ShiftAlpha = 0.f;
    FShiftOperation ActiveOperation;
    FShiftResult ActiveResult;
    TArray<FTileShiftInstruction> ActiveInstructions;
};
