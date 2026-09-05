// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Framework/GameMode/TurnBasedGameMode.h"
#include "ConnectIt_GameMode.generated.h"

class AConnectIt_BoardManager;
class AConnectIt_AIController;
class UTurnBasedParticipantManagerComponent;


// Defines the match type -- affects how participants are registered
UENUM(BlueprintType)
enum class EConnectItMatchType : uint8
{
    // Single player -- human vs AI spawned by GameMode
    Adventure   UMETA(DisplayName = "Adventure"),

    // Online -- human vs human via lobby
    Online      UMETA(DisplayName = "Online")
};

UCLASS()
class CONNECTIT_API AConnectIt_GameMode : public ATurnBasedGameMode
{
    GENERATED_BODY()

public:

    AConnectIt_GameMode();

    // --- Configuration ---

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Config")
    EConnectItMatchType MatchType = EConnectItMatchType::Adventure;

    // AI controller class spawned in Adventure mode
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Config")
    TSubclassOf<AConnectIt_AIController> AIControllerClass = nullptr;

    // Display name for the AI participant
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Config")
    FString AIDisplayName = TEXT("Opponent");

    // Number of factions -- drives scoreboard size on board state
    // 2 for all current game modes
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Config")
    int32 NumFactions = 2;

    // --- Overrides ---

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void HandleMatchHasStarted() override;
    virtual void HandleMatchHasEnded() override;

protected:

    // Finds the board actor in the level
    // Cached after first find
    AConnectIt_BoardManager* GetBoardActor() const;

    // Spawns and registers the AI controller (Adventure mode only)
    void SpawnAndRegisterAI();

    // Initialises the board after all participants are registered
    // Called from HandleMatchHasStarted after tiles have registered
    void InitialiseBoard();

    // Bound to UGameEventTaskSubsystem's ConnectIt_Event_PlayerWin tag
    // completing (see InitialiseBoard). Reads the winning faction from
    // BoardStateComponent's persistent CurrentState rather than the Tag
    // param -- OnManagerComplete's payload is just the tag's own identity
    // (needed so callers with multiple bindings can disambiguate), not
    // event-specific data.
    UFUNCTION()
    void HandleGameOver(FGameplayTag Tag);

    // Bound to UTurnBasedParticipantManagerComponent::OnInvalidNumberOfPlayers
    // (see HandleMatchHasStarted) -- fires when too few active participants
    // remain to continue (forfeit-by-missed-turns or disconnect-past-
    // reconnect-timeout). Resolves the winner-by-default and ends the match
    // via EndMatch(), which re-enters the same GameOver lockout path as a
    // real score win (see OnGameModeWaitingPostMatch in the plugin).
    UFUNCTION()
    void HandleInvalidNumberOfPlayers();

private:

    // Cached board actor -- found once in HandleMatchHasStarted
    UPROPERTY()
    mutable TObjectPtr<AConnectIt_BoardManager> CachedBoardActor = nullptr;

    // Tracks how many human players have connected
    // Used in Online mode to know when to start ready check
    int32 ConnectedHumanCount = 0;

    // Expected human players before game can start
    int32 ExpectedHumanCount() const
    {
        return MatchType == EConnectItMatchType::Adventure ? 1 : 2;
    }
};