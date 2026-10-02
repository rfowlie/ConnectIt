// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Controller/ConnectIt_AIController.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_BoardRules.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/ConnectIt_AIActionsComponent.h"
#include "Action/TurnBasedAction.h"
#include "Action/TurnBasedActionsComponent.h"
#include "AI/ConnectIt_AIStrategy_MinMax.h"
#include "ConnectIt_Structs.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Engine/World.h"
#include "TimerManager.h"


AConnectIt_AIController::AConnectIt_AIController(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UConnectIt_AIActionsComponent>(
        TEXT("ActionsComponent")))
{
    AIDisplayName = TEXT("Opponent");
}

void AConnectIt_AIController::BeginPlay()
{
    // Base creates PlayerState and binds participant delegates
    Super::BeginPlay();

    if (!HasAuthority()) return;

    InitialiseFromLevelConfig();
}

void AConnectIt_AIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelDecision();
    Super::EndPlay(EndPlayReason);
}

void AConnectIt_AIController::InitialiseFromLevelConfig()
{
    const UConnectIt_LevelConfigDataAsset* LevelConfig =
        UConnectIt_GameUtilityLibrary::GetLevelConfig(this);

    if (!IsValid(LevelConfig))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: No ConnectIt_LevelConfigDataAsset "
                 "found for the current level"));
        return;
    }

    if (UActionLoadoutDataAsset* LoadOut = LevelConfig->EnemyLoadout)
    {
        ActionsComponent->InitialiseFromLoadout(LoadOut);
    }

    // The level config's strategy is a template inside a shared, loaded-once
    // asset -- duplicate it so this controller's decision state is its own
    // (same reason UConnectIt_BoardRegistrySubsystem duplicates the
    // registry templates).
    if (IsValid(LevelConfig->AIStrategy))
    {
        Strategy = DuplicateObject<UConnectIt_AIStrategy>(LevelConfig->AIStrategy, this);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: level config has no AIStrategy -- "
                 "using a default MinMax strategy"));
        Strategy = NewObject<UConnectIt_AIStrategy_MinMax>(this);
    }
    Strategy->OnDecisionFinished.BindUObject(this, &AConnectIt_AIController::HandleDecisionFinished);

    // Registration itself is NOT done here -- AConnectIt_GameMode::
    // SpawnAndRegisterAI already calls RegisterAIParticipant right after
    // spawning this controller (which synchronously triggers this BeginPlay
    // by way of SpawnActor). A second call here was redundant: the base
    // RegisterAIParticipant guards against a literal duplicate by taking the
    // "reconnect" branch instead, which is the wrong branch for a controller
    // that was never disconnected. One registration, one call site.
}

bool AConnectIt_AIController::CheckAndApplyForcedMove()
{
    // Blackboard forced move logic here -- Adventure-mode concern, not
    // implemented yet.
    // Returns true if a forced move was applied
    return false;
}

void AConnectIt_AIController::OnMyTurnStarted()
{
    if (!HasAuthority()) return;
    if (CheckAndApplyForcedMove()) return;

    DecisionsThisTurn = 0;
    BeginDecision();
}

void AConnectIt_AIController::BeginDecision()
{
    CancelDecision();

    if (!IsValid(Strategy))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: BeginDecision -- no strategy "
                 "(level config not resolved?), cannot decide"));
        return;
    }

    AConnectIt_GameMode* GameMode = Cast<AConnectIt_GameMode>(GetWorld()->GetAuthGameMode());
    const ATurnBasedGameState* GameState = GetWorld()->GetGameState<ATurnBasedGameState>();
    if (!IsValid(GameMode) || !IsValid(GameState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: BeginDecision -- GameMode is not "
                 "AConnectIt_GameMode or there is no turn-based GameState, "
                 "cannot decide"));
        return;
    }

    UConnectIt_BoardRules* BoardRules = GameMode->GetBoardRules();
    const UConnectIt_BoardStateComponent* BoardState =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const ATurnBasedPlayerState* PS = GetPlayerState<ATurnBasedPlayerState>();

    if (!IsValid(BoardRules) || !IsValid(BoardState) || !IsValid(PS))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: BeginDecision -- BoardRules, "
                 "BoardStateComponent, or PlayerState is null, cannot decide"));
        return;
    }

    // Rules are read here, on the game thread, and handed to the strategy as
    // plain values -- a strategy that searches off the game thread never
    // has to touch a rule UObject.
    FConnectItAIDecisionContext Context;
    Context.Board = BoardState->GetCurrentState();
    Context.OwnSlot = PS->GetSlotIndex();
    Context.WinScoreThreshold = BoardRules->GetTargetScore();
    Context.ConnectLength = BoardRules->GetMinimumConnectLength();
    if (Context.ConnectLength <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: active ScoringRule doesn't report a "
                 "connect length (not UConnectIt_LineScoringRule, or a "
                 "Blueprint override) -- defaulting to 4. The AI may disagree "
                 "with this level's real scoring."));
        Context.ConnectLength = 4;
    }

    ++CurrentDecisionId;
    DecisionTurnNumber = GameState->GetActiveTurnNumber();
    DecisionStartTime = FPlatformTime::Seconds();

    // May call HandleDecisionFinished before returning (a strategy that
    // decides synchronously) -- all state above is already set.
    Strategy->StartDecision(Context);
}

void AConnectIt_AIController::HandleDecisionFinished(const FConnectItAIDecision& Decision)
{
    const int32 DecisionId = CurrentDecisionId;
    if (!IsDecisionStillCurrent(DecisionId)) return;

    if (!Decision.HasMove())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: strategy %s returned no move"),
            *GetNameSafe(Strategy));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_AIController: decided -- %s"), *Decision.Summary);

    const double RemainingThink = Strategy->MinThinkSeconds
        - (FPlatformTime::Seconds() - DecisionStartTime);

    if (RemainingThink > 0.0)
    {
        GetWorld()->GetTimerManager().SetTimer(
            ThinkTimerHandle,
            FTimerDelegate::CreateUObject(
                this, &AConnectIt_AIController::SubmitDecision, DecisionId, Decision),
            static_cast<float>(RemainingThink),
            false);
    }
    else
    {
        SubmitDecision(DecisionId, Decision);
    }
}

bool AConnectIt_AIController::IsDecisionStillCurrent(int32 DecisionId) const
{
    if (DecisionId != CurrentDecisionId) return false;

    const UWorld* World = GetWorld();
    const ATurnBasedGameState* GameState =
        World ? World->GetGameState<ATurnBasedGameState>() : nullptr;
    if (!IsValid(GameState)) return false;

    if (GameState->GetMatchPhase() == EMatchPhase::GameOver) return false;
    if (GameState->GetActiveTurnNumber() != DecisionTurnNumber) return false;

    bool bValid = false;
    const FTurnParticipantInfo Active = GameState->GetActiveParticipant(bValid);
    return bValid && Active.PlayerState == GetPlayerState<ATurnBasedPlayerState>();
}

void AConnectIt_AIController::CancelDecision()
{
    // Invalidates any in-flight result or pending think timer
    CurrentDecisionId++;

    if (IsValid(Strategy))
    {
        Strategy->StopDecision();
    }

    if (const UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ThinkTimerHandle);
    }
}

void AConnectIt_AIController::SubmitDecision(int32 DecisionId, FConnectItAIDecision Decision)
{
    if (!IsDecisionStillCurrent(DecisionId))
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_AIController: SubmitDecision -- decision is stale "
                 "(turn moved on), dropping it"));
        return;
    }

    ATurnBasedPlayerState* PS = GetPlayerState<ATurnBasedPlayerState>();
    UActionLoadoutDataAsset* Loadout = IsValid(PS) ? PS->GetLoadout() : nullptr;
    if (!IsValid(Loadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: SubmitDecision -- no PlayerState or "
                 "loadout, cannot resolve which action to submit as"));
        return;
    }

    // Submit as the loadout action allowed to produce this request type --
    // found by asking each configured class, not by hardcoding a reference
    // (the class-keyed action-config convention: config references the
    // class, the tag is derived from it). The Mediator gate checks the same.
    TSubclassOf<UTurnBasedAction> ActionClass = nullptr;
    auto ConsiderClass = [&ActionClass, &Decision](const TSubclassOf<UTurnBasedAction>& Candidate)
    {
        if (ActionClass || !Candidate) return;
        const UTurnBasedAction* CDO = Candidate->GetDefaultObject<UTurnBasedAction>();
        if (IsValid(CDO) && CDO->ProducesRequestType(Decision.RequestType))
        {
            ActionClass = Candidate;
        }
    };
    for (const FPermanentActionConfig& Config : Loadout->PermanentActions)
    {
        ConsiderClass(Config.ActionClass);
    }
    for (const FNumberedActionConfig& Config : Loadout->NumberedActions)
    {
        ConsiderClass(Config.ActionClass);
    }

    if (!ActionClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: SubmitDecision -- loadout '%s' has "
                 "no configured action that produces '%s' requests"),
            *Loadout->LoadoutName, *Decision.RequestType.ToString());
        return;
    }

    AConnectIt_GameMode* GameMode = Cast<AConnectIt_GameMode>(GetWorld()->GetAuthGameMode());
    if (!IsValid(GameMode))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: SubmitDecision -- GameMode is not "
                 "AConnectIt_GameMode"));
        return;
    }

    FTurnActionRequest Request;
    Request.RequestType = Decision.RequestType;
    Request.ActionTag = UTurnBasedAction::GetTagForClass(ActionClass);
    Request.FactionID = PS->GetSlotIndex();
    Request.Payload = Decision.Payload;

    const bool bSucceeded = GameMode->ProcessBoardRequest(Request);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_AIController: '%s' as %s -- %s"),
        *Decision.RequestType.ToString(), *ActionClass->GetName(),
        bSucceeded ? TEXT("accepted") : TEXT("REJECTED"));

    if (!bSucceeded)
    {
        // Stop rather than loop resubmitting the same rejection
        return;
    }

    // Nothing on this controller's action stack ever completes (the AI never
    // pushes/activates a real UTurnBasedAction instance -- it calls the
    // Mediator directly), so CheckAutoEndTurn's usual HandleActionCompleted
    // trigger never fires for it. ConsumeActionUse above already updated
    // PlayerState synchronously (server, same call stack -- no replication
    // lag to wait out, unlike a human client's limbo), so it's safe to check
    // turn end immediately.
    if (ActionsComponent->CanEndTurn())
    {
        ActionsComponent->RequestTurnEnd();
        return;
    }

    // This loadout's turn-end requirements need more than one move --
    // decide again from the post-move board, same as a fresh turn would.
    DecisionsThisTurn++;
    if (DecisionsThisTurn >= MaxDecisionsPerTurn)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: hit the per-turn decision cap (%d) "
                 "without satisfying turn-end -- stopping to avoid looping "
                 "forever. Check the AI loadout's turn-end requirements are "
                 "actually reachable."),
            MaxDecisionsPerTurn);
        return;
    }

    BeginDecision();
}
