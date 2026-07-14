// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConnectIt_GameUtilityLibrary.generated.h"

class AConnectIt_BoardManager;
class AConnectIt_GameState;
class UConnectIt_BoardStateComponent;
class UConnectIt_BoardManagerComponent;
class UConnectIt_BlackboardSubsystem;
class UTurnBasedParticipantManagerComponent;
class UGridWorldSubsystem;

UCLASS()
class CONNECTIT_API UConnectIt_GameUtilityLibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    // --- Board Manager ---

    // Finds the ConnectIt board manager actor in the world
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static AConnectIt_BoardManager* GetBoardManager(
        const UObject* WorldContextObject);

    // Returns the board state component from the board manager
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UConnectIt_BoardStateComponent* GetBoardStateComponent(
        const UObject* WorldContextObject);

    // Returns the board manager component from the board manager
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UConnectIt_BoardManagerComponent* GetBoardManagerComponent(
        const UObject* WorldContextObject);

    // --- Game State ---

    // Returns the ConnectIt game state
    // Use this instead of GetWorld()->GetGameState<AConnectIt_GameState>()
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static AConnectIt_GameState* GetConnectItGameState(
        const UObject* WorldContextObject);

    // Returns the participant manager from game state
    // Shortcut for UI and systems that need turn phase or participant list
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UTurnBasedParticipantManagerComponent* GetParticipantManager(
        const UObject* WorldContextObject);

    // Returns the current turn phase
    // Convenience for UI -- avoids two step game state lookup
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static ETurnPhase GetCurrentTurnPhase(
        const UObject* WorldContextObject);

    // --- Local Player ---

    // Returns the slot index of the local player
    // Reads CachedSlotIndex from the participant component
    // on the first local player controller
    // Returns -1 if not found
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static int32 GetLocalPlayerSlotIndex(
        const UObject* WorldContextObject);

    // Returns true if it is currently the local player's turn
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static bool IsLocalPlayerTurn(
        const UObject* WorldContextObject);

    // --- Subsystems ---

    // Returns the ConnectIt blackboard subsystem
    // Use this in shard actions instead of raw GetSubsystem call
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UConnectIt_BlackboardSubsystem* GetBlackboardSubsystem(
        const UObject* WorldContextObject);

    // Returns the grid world subsystem
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Utility",
        meta = (WorldContext = "WorldContextObject"))
    static UGridWorldSubsystem* GetGridSubsystem(
        const UObject* WorldContextObject);
};