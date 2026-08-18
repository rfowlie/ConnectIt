// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConnectIt_Structs.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "ConnectIt_DebugStateWidget.generated.h"

class AConnectIt_BoardManager;
class UConnectIt_BoardStateComponent;
class UTurnBasedParticipantManagerComponent;
class UTurnBasedActionsComponent;
class UTurnBasedActionBase;
class UTurnBasedAction;
class UGameEventTaskSubsystem;
class UConnectIt_BoardManagerSubsystem;

// See Docs/RuntimeStateAccess.md -- this widget is the "cache everything,
// react to every delegate" companion to that reference. Everything below is
// cached VALUE data (structs/tags/ints), never a live UObject pointer --
// GetTopAction()/GetRootAction() return live, mutable action objects with
// BlueprintCallable Complete()/Cancel() on them (see RuntimeStateAccess.md's
// Safety section); this widget deliberately caches just their ActionTag
// instead of the pointer, so nothing reachable from this widget's cached
// state can accidentally mutate live game state.

USTRUCT(BlueprintType)
struct FConnectItDebugBoardState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FConnectItBoardState CurrentState;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FConnectItBoardChangeEvent LastChangeEvent;

    // False until the board manager/board state component have been found --
    // distinguishes "board genuinely empty" from "haven't resolved it yet"
    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    bool bValid = false;
};

USTRUCT(BlueprintType)
struct FConnectItDebugTurnState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    ETurnPhase TurnPhase = ETurnPhase::WaitingForParticipants;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    
    EMatchPhase MatchPhase = EMatchPhase::WaitingForParticipants;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    int32 ActiveParticipantIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    int32 TurnNumber = 0;

    // Refreshed opportunistically alongside every other turn-state change --
    // UTurnBasedParticipantManagerComponent::Participants has no OnRep of
    // its own (see RuntimeStateAccess.md), so this can lag slightly behind
    // a pure connect/disconnect with no other turn event attached to it.
    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    TArray<FTurnParticipantInfo> Participants;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FConnectItMatchResult MatchResult;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    bool bValid = false;
};

USTRUCT(BlueprintType)
struct FConnectItDebugActionState
{
    GENERATED_BODY()

    // Tag only, never the live action pointer -- see file header comment
    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FGameplayTag TopActionTag;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FGameplayTag RootActionTag;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    int32 StackDepth = 0;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    bool bAwaitingRequestConfirmation = false;

    // False until the local player's ActionsComponent has been resolved
    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    bool bValid = false;
};

/**
 * Prototype debug/dev widget -- caches ConnectIt's full runtime state
 * (board, turn/participant/match, local action stack, event-queue) and
 * keeps the cache current by binding to every delegate that signals a
 * change, so BP-side visuals read the cached UPROPERTYs below instead of
 * re-querying components on every tick.
 *
 * Cross-reference: Docs/RuntimeStateAccess.md documents every accessor this
 * widget reads from and why each one is Blueprint-safe.
 *
 * Client-side only, as all UMG widgets are -- no HasAuthority handling
 * needed, this only ever observes state that already replicated in.
 */
UCLASS(Abstract)
class CONNECTIT_API UConnectIt_DebugStateWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FConnectItDebugBoardState CachedBoardState;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FConnectItDebugTurnState CachedTurnState;

    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    FConnectItDebugActionState CachedActionState;

    // Tags currently firing in UGameEventTaskSubsystem's active container --
    // see RuntimeStateAccess.md's Turn-End & Resolution Sequencing section
    UPROPERTY(BlueprintReadOnly, Category = "ConnectIt|Debug")
    TArray<FGameplayTag> CachedActiveEventTags;

    // Re-resolves every source component and rebuilds every cached category
    // from scratch. Cheap safety valve for BP -- call if something looks
    // stale rather than trying to diagnose which delegate didn't fire.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Debug")
    void ForceRefreshAll();

    // Fired after any single category refreshes (including ForceRefreshAll).
    // BP redraws from the cached UPROPERTYs above -- deliberately one
    // coarse-grained event rather than one per category for this prototype;
    // split it out per-category later if a specific panel needs to avoid
    // redrawing on unrelated updates.
    UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt|Debug")
    void OnDebugStateUpdated();

protected:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:

    void BindAll();
    void UnbindAll();

    // Split out so BindAll can call this again once OnBoardManagerReady
    // fires, for the case where the board manager hasn't registered with
    // UConnectIt_BoardManagerSubsystem yet at widget-construct time (see
    // Workflows/BoardManagerSubsystem_Workflow.txt's documented pattern --
    // this widget follows it rather than polling GetBoardManager from Tick).
    void BindBoardManager(AConnectIt_BoardManager* InBoardManager);

    void RefreshBoardState();
    void RefreshTurnState();
    void RefreshActionState();
    void RefreshEventQueueState();

    // --- Delegate handlers -- each refreshes its own category only ---

    UFUNCTION()
    void HandleBoardManagerReady(AConnectIt_BoardManager* InBoardManager);

    UFUNCTION()
    void HandleBoardStateChanged();

    UFUNCTION()
    void HandleMatchPhaseChanged(EMatchPhase NewPhase);

    UFUNCTION()
    void HandleMatchResultUpdated(const FConnectItMatchResult& Result);

    UFUNCTION()
    void HandleTurnPhaseChanged(ETurnPhase NewPhase);

    UFUNCTION()
    void HandleActiveControllerChanged(AController* NewActiveController);

    UFUNCTION()
    void HandleParticipantForfeited(const FTurnParticipantInfo& ParticipantInfo);

    UFUNCTION()
    void HandleAllParticipantsReady();

    UFUNCTION()
    void HandleGameOver();

    // Shared by OnActionPushed/OnActionPopped -- both are FOnActionBaseEvent
    UFUNCTION()
    void HandleActionStackChanged(UTurnBasedActionBase* Action);

    // Shared by OnActionCompleted/OnActionCancelled -- both are FOnTurnActionsEvent
    UFUNCTION()
    void HandleActionResolved(UTurnBasedAction* Action);

    UFUNCTION()
    void HandleBoardChangeRequested(const FTurnActionRequest& Request);

    UFUNCTION()
    void HandleActiveManagerTagsChanged();

    // --- Resolved sources -- cached once in BindAll ---

    UPROPERTY()
    TObjectPtr<AConnectIt_BoardManager> ResolvedBoardManager = nullptr;

    UPROPERTY()
    TObjectPtr<UConnectIt_BoardStateComponent> ResolvedBoardStateComponent = nullptr;

    UPROPERTY()
    TObjectPtr<AConnectIt_GameState> ResolvedGameState = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedParticipantManagerComponent> ResolvedParticipantManager = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedActionsComponent> ResolvedActionsComponent = nullptr;

    UPROPERTY()
    TObjectPtr<UGameEventTaskSubsystem> ResolvedGameEventSubsystem = nullptr;

    UPROPERTY()
    TObjectPtr<UConnectIt_BoardManagerSubsystem> ResolvedBoardManagerSubsystem = nullptr;

    bool bBound = false;
};
