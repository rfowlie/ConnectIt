// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "TurnBasedMechanicsStructs.h"
#include "ConnectIt_GameState.generated.h"

class AConnectIt_BoardManager;
class UConnectIt_BoardStateComponent;


// Why the match ended
UENUM(BlueprintType)
enum class EMatchEndReason : uint8
{
    ScoreThresholdReached   UMETA(DisplayName = "Score Threshold Reached"),
    OpponentForfeited       UMETA(DisplayName = "Opponent Forfeited"),
    OpponentDisconnected    UMETA(DisplayName = "Opponent Disconnected"),
    Unknown                 UMETA(DisplayName = "Unknown")
};

// Replicated match result -- written by server on game over
// Read by clients for results screen
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItMatchResult
{
    GENERATED_BODY()

    // Slot index of the winning faction -- -1 if no winner yet
    UPROPERTY(BlueprintReadOnly)
    int32 WinningFactionSlot = -1;

    // Final scores per faction at match end
    UPROPERTY(BlueprintReadOnly)
    TArray<float> FinalScores;

    // How many turns were played total
    UPROPERTY(BlueprintReadOnly)
    int32 TotalTurnsPlayed = 0;

    // Why the match ended
    UPROPERTY(BlueprintReadOnly)
    EMatchEndReason EndReason = EMatchEndReason::Unknown;

    // Whether the match has ended
    UPROPERTY(BlueprintReadOnly)
    bool bMatchOver = false;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchResultUpdated, const FConnectItMatchResult&, Result);

// Everything a debug widget needs to know about this game state's current
// values in one call -- used to seed initial state once, right after
// binding, through the same events used for later reactive updates (see
// UDWidgetBase's own class comment for the convention this follows).
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItGameStateInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EMatchPhase MatchPhase = EMatchPhase::WaitingForParticipants;

    UPROPERTY(BlueprintReadOnly)
    FConnectItMatchResult MatchResult;
};

UCLASS()
class CONNECTIT_API AConnectIt_GameState : public ATurnBasedGameState
{
    GENERATED_BODY()

public:

    AConnectIt_GameState();

    // --- Replicated Match Result ---
    // Written by server via SetMatchResult
    // All clients read this for results screen

    UPROPERTY(BlueprintReadOnly,
        ReplicatedUsing = OnRep_MatchResult,
        Category = "ConnectIt|Match")
    FConnectItMatchResult MatchResult;

    // Fires on all clients when match result is updated
    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Match")
    FOnMatchResultUpdated OnMatchResultUpdated;
    
    // --- Server API ---
    // Called by AConnectIt_GameMode when game over fires

    void SetMatchResult(
        int32 WinningFactionSlot,
        EMatchEndReason EndReason,
        int32 TotalTurns);

    // --- Board State Accessors ---
    // Convenience API for UI -- hides board actor lookup
    // All read from the locally replicated board state snapshot

    // Returns current score for a faction
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    float GetFactionScore(int32 FactionSlot) const;

    // Returns all current scores
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    TArray<float> GetAllScores() const;

    // Returns true if a tile is currently occupied
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    bool IsTileOccupied(FGridPosition Position) const;

    // Returns true if a tile is valid for placement
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    bool IsTileValidForPlacement(FGridPosition Position) const;

    // Returns the current board state snapshot
    // UI can read previous and current state for animations
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    FConnectItBoardStateSnapshot GetBoardSnapshot() const;

    // Score needed to win, published by whichever win condition is active.
    // 0 means the active win condition isn't score-based. See
    // FConnectItBoardState::TargetScore.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    float GetTargetScore() const;

    // FactionSlot's score over the target, 0..1. Returns 0 when the target
    // is 0 (non-score-based win condition) rather than dividing.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    float GetFactionScoreProgress(int32 FactionSlot) const;

    // --- Turn Accessors ---
    // Convenience wrappers on top of ATurnBasedGameState

    // Returns the display name of the currently active participant
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Turn")
    FString GetActiveParticipantName() const;

    // Returns true if the local player is the active participant
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Turn")
    bool IsLocalPlayerTurn() const;

    // Everything a debug widget needs, in one call -- see
    // FConnectItGameStateInfo's own comment.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    FConnectItGameStateInfo GetInfo() const { return { GetMatchPhase(), MatchResult }; }

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

    // Finds and caches board state component
    // Searches for AConnectItBoardActor in world on first call
    UConnectIt_BoardStateComponent* GetBoardStateComponent() const;

    // Cached component reference -- found once, reused
    mutable TObjectPtr<UConnectIt_BoardStateComponent> CachedBoardStateComponent = nullptr;

    UFUNCTION()
    void OnRep_MatchResult();
};