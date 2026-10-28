// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsStructs.h"
#include "TurnBasedMechanicsEnums.h"
#include "Framework/GameMode/TurnBasedGameMode.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Framework/GameMode/ConnectIt_LevelChange.h"
#include "ConnectIt_GameMode.generated.h"

class AConnectIt_AIController;
class ATurnBasedPlayerState;
class UActionLoadoutDataAsset;
class UConnectIt_AIDirector;
class UConnectIt_LevelDirector;
class UConnectIt_PlayerDirector;
class UConnectIt_Director;
class UConnectIt_MatchConditionDataAsset;
class UConnectIt_BoardLayoutDataAsset;
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

// Which side of a match a rule or loadout change is for. There is one human
// side and one AI side today (Online has no AI side: both humans are Player).
UENUM(BlueprintType)
enum class EConnectItRole : uint8
{
    Player  UMETA(DisplayName = "Player"),
    AI      UMETA(DisplayName = "AI")
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

    // How long a board change may stay resolving before the lock gives up
    // (an error is logged and new changes are allowed again). Only a visual
    // task that never reports complete gets here -- it also holds up the
    // event queue behind it, so the log says what is stuck.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Config", meta = (ClampMin = 1, Units = "s"))
    float BoardChangeTimeoutSeconds = 15.f;

    // --- Overrides ---

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void HandleMatchHasStarted() override;
    virtual void HandleMatchHasEnded() override;

    // Single entry point for all board change requests -- forwards to
    // BoardRequestMediator so callers (AConnectIt_PlayerController's server
    // RPC) depend on GameMode's public surface, not on the mediator's
    // existence directly. Structurally server-only: this whole object
    // (and therefore BoardRequestMediator) doesn't exist on any client.
    // RequestingFaction is who is asking, as the SERVER knows it (the
    // sending controller's PlayerState slot) -- a request doesn't say.
    // Refused until the board is settled (IsBoardSettled): a change is
    // resolving, or level changes are waiting to play.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    bool ProcessBoardRequest(const FTurnActionRequest& Request, int32 RequestingFaction);

    // --- One board change at a time ---
    // A board change (a move, or a level operation) resolves from the moment
    // it is carried out until its visuals have played out (the server's own
    // event queue: ConnectIt_Event_BoardChangeComplete). Meanwhile requests
    // are refused and level changes wait, so every change reaches clients as
    // its own snapshot with its own events.

    // A board change is being carried out or its visuals are still playing
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    bool IsBoardChangeResolving() const;

    // Nothing resolving and no level change waiting: a decision made now is
    // made from the board the next move will be applied to
    bool IsBoardSettled() const;

    // Board changes committed so far this match: compare two reads to tell
    // whether the board changed in between
    int32 GetNumBoardChanges() const { return NumBoardChanges; }

    // Fires when the board becomes settled (IsBoardSettled) after a change
    FSimpleMulticastDelegate OnBoardChangeSettled;

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

    // --- Directors (see UConnectIt_Director) ---

    // The match preset's AI director class (the AI controller makes its own
    // instance); null = none set
    TSubclassOf<UConnectIt_AIDirector> GetAIDirectorClass() const { return AIDirectorClass; }

    // This match's directors (null if there is none)
    UConnectIt_LevelDirector* GetLevelDirector() const { return LevelDirector; }
    UConnectIt_PlayerDirector* GetPlayerDirector(int32 Faction) const;
    UConnectIt_AIDirector* GetAIDirector() const;

    // The one way to change the match's rules after setup: Change edits this
    // GameMode's rule set, then the GameState mirror is refreshed (clients and
    // UI are notified). The Mediator and the AI read the same rule set, so the
    // change applies from the next move.
    void ModifyRules(const TFunctionRef<void(FConnectItRuleSet&)>& Change);

    // Replaces a side's loadout (Player: every human; AI: the AI). Re-seeds all
    // of that side's action uses and cooldowns from the new loadout.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Match")
    void SetLoadout(EConnectItRole PlayerRole, UActionLoadoutDataAsset* Loadout,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::NextTurnStart);

    // --- Level changes (what the Level director calls) ---

    // Carries out a board operation outside any player's move (see
    // UConnectIt_BoardRequestMediator::ProcessLevelOperation), at Timing
    void ApplyLevelOperation(const FInstancedStruct& Operation, int32 AsFaction, EConnectItChangeTiming Timing);

    // Replaces the match's scoring rule at Timing (clients see it through the
    // replicated rules; the AI from its next decision). False if ScoringRule
    // isn't a scoring rule struct.
    bool SetScoringRule(const FInstancedStruct& ScoringRule, EConnectItChangeTiming Timing);

    // Replaces a side's win goals (bWinGoals) or lose conditions at Timing.
    // Applied as a board change: the rules are re-resolved (and replicate),
    // then a Change Goals operation restarts that side's goal progress and
    // says so, and the win check measures the new goals (one already met
    // completes at once). Score carries over.
    void SetConditions(EConnectItRole Side, bool bWinGoals,
        const TArray<UConnectIt_MatchConditionDataAsset*>& Conditions, EConnectItChangeTiming Timing);

    // Changes the board's tiles to Layout at Timing, as one board change
    // (Change Layout, worked out against the board when it is applied)
    void SetLayout(UConnectIt_BoardLayoutDataAsset* Layout, EConnectItChangeTiming Timing);

    // The same for one player (what a Player director calls for its own player)
    void SetLoadoutForPlayer(ATurnBasedPlayerState* PlayerState, UActionLoadoutDataAsset* Loadout,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::NextTurnStart);

protected:

    // Resolves the live match setup from the level config + main-menu
    // choices, once (players log in before the match starts, so both
    // PostLogin and HandleMatchHasStarted call it), and publishes it.
    void EnsureMatchSetupResolved();

    // Pushes the match-wide setup to the GameState's replicated mirror
    void PublishMatchSetup();

    // Seeds a joining human's PlayerState action state (uses/caps/cooldowns)
    // from their Player director's StartingLoadout (making the director first). Server-side, called from PostLogin; a no-op if
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

    // Bound to ConnectIt_Event_BoardChangeComplete completing: one committed
    // change has played out
    UFUNCTION()
    void HandleBoardChangeComplete(FGameplayTag Tag);

    // Director events: the participant manager's turn phases (server side) ->
    // OnTurnStarted / OnTurnEnded for every director
    UFUNCTION()
    void HandleTurnPhaseChanged(ETurnPhase NewPhase);

    // Director events: every committed board change (server side, before its
    // visuals) -> checkpoint / match won for every director
    UFUNCTION()
    void HandleBoardStateChangedForDirectors();

    // Bound to UTurnBasedParticipantManagerComponent::OnInvalidNumberOfPlayers
    // (see HandleMatchHasStarted) -- fires when too few active participants
    // remain to continue (forfeit-by-missed-turns or disconnect-past-
    // reconnect-timeout). Resolves the winner-by-default and ends the match
    // via EndMatch(), which re-enters the same GameOver lockout path as a
    // real score win (see OnGameModeWaitingPostMatch in the plugin).
    UFUNCTION()
    void HandleInvalidNumberOfPlayers();

    // Bound to UTurnBasedParticipantManagerComponent::OnActiveControllerChanged
    // (server side, as each turn starts): gives the side whose turn it now is
    // the loadout waiting for it, if SetLoadout parked one.
    UFUNCTION()
    void HandleActiveControllerChanged(AController* NewActiveController);

private:

    // Seeds a PlayerState's action state from Loadout, now
    void ApplyLoadout(ATurnBasedPlayerState* PlayerState, UActionLoadoutDataAsset* Loadout);

    // Warns about a missing win group that would make a side unable to win
    // (see the level config's validation)
    void WarnAboutMissingWinConditions() const;

    // Calls Event on every director, in dispatch order (Level, Players by
    // slot, AI -- UConnectIt_Director::InDispatchOrder)
    void ForEachDirector(TFunctionRef<void(UConnectIt_Director&)> Event) const;

    // The Player director of PlayerState's player, made from the preset's
    // class on first ask
    UConnectIt_PlayerDirector* FindOrCreatePlayerDirector(ATurnBasedPlayerState* PlayerState);

    // Level changes waiting for the board to settle / the next turn
    FConnectItLevelChangeQueue LevelChanges;

    // The board-change lock (see IsBoardChangeResolving). Set when a change
    // is committed; cleared when every committed change's marker has
    // completed, or by the timeout.
    bool bBoardChangeResolving = false;

    // Committed changes whose completion marker hasn't completed yet. Markers
    // complete in order, so the lock lifts when this reaches 0 (or a timeout
    // lifts it early, leaving this to catch up).
    int32 NumChangesPlaying = 0;

    int32 NumBoardChanges = 0;

    // Level changes applied since the last player / AI move or turn start: a
    // level whose triggers keep feeding each other is stopped
    int32 LevelChangesInARow = 0;

    FTimerHandle BoardChangeTimeoutHandle;

    // OnBoardChangeCommitted (the board state component): lock
    void HandleBoardChangeCommitted();

    // The lock is lifted: waiting level changes are applied from the next
    // tick (never inside the call that lifted it, which may be a change
    // still being carried out)
    void ReleaseBoardChangeLock();
    void HandleBoardChangeTimeout();

    // Mirrors "not settled" to the GameState for clients and UI
    // (AConnectIt_GameState::IsBoardChangeResolving)
    void PublishBoardChangeResolving() const;

    // Applies waiting level changes in order until one changes the board
    // (the next waits for that change to play out) or none are left; then,
    // if the board is settled, says so (OnBoardChangeSettled)
    void DrainLevelChanges();

    void ApplyLevelChange(const FConnectItLevelChange& Change);
    void ApplyGoalsChange(const FConnectItLevelChange& Change);
    void ApplyLayoutChange(const FConnectItLevelChange& Change);
    void QueueLevelChange(const FConnectItLevelChange& Change, EConnectItChangeTiming Timing);

    // Loadouts parked by SetLoadout (NextTurnStart), by faction slot
    UPROPERTY()
    TMap<int32, TObjectPtr<UActionLoadoutDataAsset>> PendingLoadouts;


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

    // The match preset's director classes
    UPROPERTY()
    TSubclassOf<UConnectIt_LevelDirector> LevelDirectorClass;

    UPROPERTY()
    TSubclassOf<UConnectIt_PlayerDirector> PlayerDirectorClass;

    UPROPERTY()
    TSubclassOf<UConnectIt_AIDirector> AIDirectorClass;

    // The directors this GameMode owns (the AI's belongs to its controller)
    UPROPERTY()
    TObjectPtr<UConnectIt_LevelDirector> LevelDirector = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UConnectIt_PlayerDirector>> PlayerDirectors;

    // The AI controller spawned for an Adventure match (for its director)
    TWeakObjectPtr<AConnectIt_AIController> SpawnedAIController;

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