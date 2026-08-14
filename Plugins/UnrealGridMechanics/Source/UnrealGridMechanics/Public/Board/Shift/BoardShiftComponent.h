// Fill out your copyright notice in the Description page of Project Settings.
// BoardShiftComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnrealGridMechanics/Public/Board/Shift/GridShiftTypes.h"
#include "BoardShiftComponent.generated.h"

class UBoardStateComponentBase;
class UGridTileRegistryComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShiftStarted, FShiftOperation, Operation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShiftCompleted, FShiftOperation, Operation);

UCLASS(ClassGroup=(Grid), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UBoardShiftComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UBoardShiftComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // Duration of the shift animation in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Shift")
    float ShiftDuration = 0.5f;

    // Computes the position remap for a shift operation -- pure and
    // synchronous, no tile actor calls, no animation. Callers that need an
    // authoritative result before committing state (e.g. AConnectIt_BoardManager)
    // call this directly instead of waiting on a delegate.
    UFUNCTION(BlueprintCallable, Category = "Grid|Shift")
    FShiftResult ComputeShift(const FShiftOperation& Operation) const;

    // Plays the visual shift animation for an already-computed result.
    // Purely presentational -- does not recompute or mutate any board state.
    // Returns false if an animation is already in progress.
    UFUNCTION(BlueprintCallable, Category = "Grid|Shift")
    bool PlayShiftAnimation(const FShiftOperation& Operation, const FShiftResult& Result);

    UFUNCTION(BlueprintPure, Category = "Grid|Shift")
    bool IsShifting() const { return bIsShifting; }

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Grid|Shift")
    FOnShiftStarted OnShiftStarted;

    UPROPERTY(BlueprintAssignable, Category = "Grid|Shift")
    FOnShiftCompleted OnShiftCompleted;

protected:

    virtual void BeginPlay() override;

private:

    // Cached component references — resolved on BeginPlay
    UPROPERTY()
    UGridTileRegistryComponent* RegistryComponent = nullptr;

    UPROPERTY()
    UBoardStateComponentBase* StateComponent = nullptr;

    // Active shift state
    bool bIsShifting = false;
    float ShiftAlpha = 0.f;
    FShiftOperation ActiveOperation;
    FShiftResult ActiveResult;

    // Instructions sent to tile actors
    TArray<FTileShiftInstruction> ActiveInstructions;

    // Resolves component dependencies from owner
    bool ResolveComponents();

    // Builds world position map from registry for all affected positions
    TMap<FGridPosition, FVector> BuildWorldPositionMap(
        const TArray<FGridPosition>& Positions) const;

    // Builds tile actor map from registry for all affected positions
    TMap<FGridPosition, AGridTileBase*> BuildGridPositionTileMap(
        const TArray<FGridPosition>& Positions) const;

    // Broadcasts alpha to all active tile instructions
    void BroadcastAlphaToTiles(float Alpha);

    // Finalises the shift — updates state and broadcasts completion
    void FinaliseShift();
};