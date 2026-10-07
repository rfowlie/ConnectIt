// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsStructs.h"
#include "Framework/GameMode/TurnBasedGameMode.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "ConnectIt_GameMode.generated.h"

class AConnectIt_AIController;
class UActionLoadoutDataAsset;
class UConnectIt_AIProfile;
class UTurnBasedParticipantManagerComponent;
class UConnectIt_BoardRequestMediator;


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

    // Single entry point for all board change requests -- forwards to
    // BoardRequestMediator so callers (AConnectIt_PlayerController's server
    // RPC) depend on GameMode's public surface, not on the mediator's
    // existence directly. Structurally server-only: this whole object
    // (and therefore BoardRequestMediator) doesn't exist on any client.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    bool ProcessBoardRequest(const FTurnActionRequest& Request);

    // --- Live match setup ---
    // This GameMode is the authority for what the match is actually using.
    // It reads the level's config (the starting template) once, applies the
    // main menu's choices, and from then on owns the live values -- the only
    // reader of the config's rules / loadout / AI profile. Clients can't reach
    // a GameMode, so the values are published: rules and the AI profile to
    // the GameState (replicated), each player's loadout to their PlayerState.
    // Server code may read them here; everything else reads those mirrors
    // (see UConnectIt_GameUtilityLibrary's "Match setup" accessors).

    // The match's rules, including per-match and mid-level changes
    const FConnectItRuleSet& GetRules() const { return Rules; }

    // The loadout human players are seeded with
    UActionLoadoutDataAsset* GetPlayerLoadout() const { return PlayerLoadout; }

    // The AI opponent for this match (menu choice, else the level's default)
    const UConnectIt_AIProfile* GetAIProfile() const { return AIProfile; }

    // The one way to change the match's rules after setup: Change edits this
    // GameMode's rule set, then the GameState mirror is refreshed (clients and
    // UI are notified). The Mediator and the AI read the same rule set, so the
    // change applies from the next move.
    void ModifyRules(const TFunctionRef<void(FConnectItRuleSet&)>& Change);

protected:

    // Resolves the live match setup from the level config + main-menu
    // choices, once (players log in before the match starts, so both
    // PostLogin and HandleMatchHasStarted call it), and publishes it.
    void EnsureMatchSetupResolved();

    // Pushes the match-wide setup to the GameState's replicated mirror
    void PublishMatchSetup();

    // Seeds a joining human's PlayerState action state (uses/caps/cooldowns)
    // from GetPlayerLoadout(). Server-side, called from PostLogin; a no-op if
    // the state is already seeded. This is the only place a human's action
    // state is seeded -- their controller then builds its actions from the
    // PlayerState's loadout.
    void SeedActionStateForPlayer(APlayerController* NewPlayer);

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

    // Board request handling + rules -- server-only UObjects, constructed
    // in HandleMatchHasStarted (not the constructor -- NewObject there
    // would run before Blueprint-child property overrides are applied,
    // the same CDO/archetype-timing pitfall this project has hit before).
    // Replaces the now-retired AConnectIt_BoardManager actor and its
    // UConnectIt_BoardRulesComponent -- see "Board architecture overhaul"
    // plan.
    UPROPERTY()
    TObjectPtr<UConnectIt_BoardRequestMediator> BoardRequestMediator = nullptr;

    UPROPERTY()
    FConnectItRuleSet Rules;

    UPROPERTY()
    TObjectPtr<UActionLoadoutDataAsset> PlayerLoadout = nullptr;

    UPROPERTY()
    TObjectPtr<UConnectIt_AIProfile> AIProfile = nullptr;

    bool bMatchSetupResolved = false;

    // Tracks how many human players have connected
    // Used in Online mode to know when to start ready check
    int32 ConnectedHumanCount = 0;

    // Expected human players before game can start
    int32 ExpectedHumanCount() const
    {
        return MatchType == EConnectItMatchType::Adventure ? 1 : 2;
    }
};