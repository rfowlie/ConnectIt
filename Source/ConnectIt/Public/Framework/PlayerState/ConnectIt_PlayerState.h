// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "ConnectIt_PlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSwapUsesRemainingChanged, int32, NewCount);

// ConnectIt-specific player state. Holds per-player, match-lifetime action
// budgets (currently just SWAP) -- distinct from UTurnBasedAction's own
// CompletionsThisTurn, which resets every turn and isn't replicated (each
// controller's UTurnBasedActionsComponent is independently DuplicateObject'd
// -- server/client action instances are never in sync and were never meant
// to be trusted, see UConnectIt_BoardRequestMediator::HandleSwapPiecesRequest).
// This is the authoritative, replicated home for state like that instead.
UCLASS()
class CONNECTIT_API AConnectIt_PlayerState : public ATurnBasedPlayerState
{
    GENERATED_BODY()

public:

    // --- SWAP Action Budget ---
    // Granted at match start (see AConnectIt_GameMode::HandleMatchHasStarted),
    // consumed only by the server on a confirmed successful swap (see
    // UConnectIt_BoardRequestMediator::HandleSwapPiecesRequest). Never
    // replenished once exhausted.
    UFUNCTION(BlueprintPure, BlueprintCallable, Category = "ConnectIt|PlayerState")
    int32 GetSwapActionUsesRemaining() const { return SwapUsesRemaining; }
    
    // Fires on all clients (and immediately, locally, on the server) when
    // SwapUsesRemaining changes -- UI binds this directly.
    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Actions")
    FOnSwapUsesRemainingChanged OnSwapUsesRemainingChanged;

    // Server-only. No-ops and returns false if SwapUsesRemaining is already
    // 0 -- callers must not decrement on their own. Broadcasts
    // OnSwapUsesRemainingChanged locally on the server (OnRep_ handles the
    // same broadcast on clients once the change replicates).
    void ConsumeSwapUse();

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SwapUsesRemaining, Category = "ConnectIt|Actions")
    int32 SwapUsesRemaining = 3;

    
private:

    UFUNCTION()
    void OnRep_SwapUsesRemaining();
    void BroadcastSwapUsesRemaining();
};
