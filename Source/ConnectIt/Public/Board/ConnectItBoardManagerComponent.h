// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "Components/ActorComponent.h"
#include "ConnectItBoardManagerComponent.generated.h"

class UGridTileRegistryComponent;
class UConnectItBoardStateComponent;
class UBoardShiftComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPiecePlaced, FGridPosition, Position);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLineScored, int32, FactionSlot, float, PointsScored);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameOver, int32, WinningFactionSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShiftApplied, const FShiftOperation&, Operation, const FShiftResult&, Result);

UCLASS(ClassGroup=(ConnectIt), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectItBoardManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UConnectItBoardManagerComponent();

    // Win score threshold -- set from UConnectItConfigComponent
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules")
    float WinScoreThreshold = 100.f;

    // Number of tiles in a line required to score
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules")
    int32 ConnectLength = 4;

    // --- Initialisation ---

    // Called by AConnectItBoardActor after all tiles register
    // Reads tile positions from registry and initialises board state
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    void InitialiseBoard(int32 NumFactions);

    // --- Request Processing ---

    // Entry point for all board change requests
    // Called by UTurnBasedActionComponent::OnBoardChangeRequested on server
    // Routes to specific handler based on RequestType tag
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    void ProcessRequest(const FTurnActionRequest& Request);

    // Called by UBoardShiftComponent::OnShiftResultReady
    // Applies position remap to board state
    UFUNCTION()
    void HandleShiftResult(
        const FShiftOperation& Operation,
        const FShiftResult& Result);

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
    FOnPiecePlaced OnPiecePlaced;

    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
    FOnLineScored OnLineScored;

    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
    FOnGameOver OnGameOver;

    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
    FOnShiftApplied OnShiftApplied;

protected:

    virtual void BeginPlay() override;

private:

    // Cached component references -- resolved on BeginPlay
    UPROPERTY()
    TObjectPtr<UConnectItBoardStateComponent> BoardStateComponent = nullptr;

    UPROPERTY()
    TObjectPtr<UGridTileRegistryComponent> RegistryComponent = nullptr;

    UPROPERTY()
    TObjectPtr<UBoardShiftComponent> ShiftComponent = nullptr;

    bool ResolveComponents();

    // --- Request Handlers ---
    // Each handles one RequestType tag

    // ConnectIt.Board.PlacePiece
    void HandlePlacePieceRequest(const FTurnActionRequest& Request);

    // --- Game Logic ---

    // Checks all directions from Position for scoring lines
    // Updates tile data and scoreboard if lines found
    // Returns total points scored
    float CheckAndApplyScoring(
        FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot);

    // Finds all complete lines of ConnectLength through Position
    // owned by FactionSlot
    TArray<TArray<FGridPosition>> FindScoringLines(
        const FConnectItBoardState& State,
        FGridPosition Position,
        int32 FactionSlot) const;

    // Applies scoring line rules to MutableState
    // Removes pieces in line, increments multipliers
    // Returns points scored from this line
    float ApplyScoringLine(
    FConnectItBoardState& MutableState,
    const TArray<FGridPosition>& Line,
    FGridPosition CompletingPosition,
    int32 FactionSlot) const;

    // Checks whether any faction has reached WinScoreThreshold
    void CheckWinCondition(FConnectItBoardState& MutableState) const;

    // --- Direction helpers ---

    static const TArray<FGridDirectionVector>& GetScoringDirections();
};
