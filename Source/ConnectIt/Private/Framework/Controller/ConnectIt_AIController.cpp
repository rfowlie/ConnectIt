// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Controller/ConnectIt_AIController.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Framework/PlayerState/ConnectIt_PlayerState.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedAction.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "AI/ConnectIt_AIDirector.h"
#include "AI/ConnectIt_AICoordinator.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_Structs.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.h"
#include "Engine/World.h"
#include "TimerManager.h"


AConnectIt_AIController::AConnectIt_AIController(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AIDisplayName = TEXT("Opponent");
}

void AConnectIt_AIController::BeginPlay()
{
    // Base creates the PlayerState
    Super::BeginPlay();

    if (!HasAuthority()) return;

    InitialiseFromMatchSetup();

    // Turn start/end reach an AI through its participant component: the
    // manager's "client" notification runs locally on the server for a
    // controller with no owning connection.
    if (IsValid(ParticipantComponent))
    {
        TurnNotificationHandle = ParticipantComponent->OnTurnNotificationReceived_Native.AddUObject(
            this, &AConnectIt_AIController::HandleTurnNotification);
        OpponentTurnHandle = ParticipantComponent->OnOpponentTurnStarted_Native.AddUObject(
            this, &AConnectIt_AIController::HandleOpponentTurnStarted);
    }

    if (ATurnBasedGameState* GameState = GetWorld()->GetGameState<ATurnBasedGameState>())
    {
        MatchPhaseHandle = GameState->OnMatchPhaseChanged_Native.AddUObject(
            this, &AConnectIt_AIController::HandleMatchPhaseChanged);
    }
}

void AConnectIt_AIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelDecision();

    if (IsValid(ParticipantComponent))
    {
        ParticipantComponent->OnTurnNotificationReceived_Native.Remove(TurnNotificationHandle);
        ParticipantComponent->OnOpponentTurnStarted_Native.Remove(OpponentTurnHandle);
    }
    if (UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this))
    {
        BoardState->OnBoardStateChanged.RemoveDynamic(this, &AConnectIt_AIController::HandleBoardStateChanged);
    }
    if (const UWorld* World = GetWorld())
    {
        if (ATurnBasedGameState* GameState = World->GetGameState<ATurnBasedGameState>())
        {
            GameState->OnMatchPhaseChanged_Native.Remove(MatchPhaseHandle);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AConnectIt_AIController::SeedActionState(UActionLoadoutDataAsset* Loadout)
{
    ATurnBasedPlayerState* PS = GetPlayerState<ATurnBasedPlayerState>();
    if (!IsValid(PS))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: no ATurnBasedPlayerState to seed -- "
                 "every board request will be rejected"));
        return;
    }

    if (!IsValid(Loadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: no AI loadout -- every board request "
                 "will be rejected"));
        return;
    }

    PS->InitialiseActionState(Loadout);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_AIController: seeded action state from loadout '%s' "
             "(%d permanent, %d numbered)"),
        *Loadout->LoadoutName, Loadout->PermanentActions.Num(),
        Loadout->NumberedActions.Num());
}

void AConnectIt_AIController::HandleTurnNotification(const FTurnNotification& Notification)
{
    switch (Notification.Phase)
    {
        case ETurnPhase::TurnStart:
        case ETurnPhase::TurnActive:
            if (Notification.TurnNumber != LastStartedTurnNumber)
            {
                LastStartedTurnNumber = Notification.TurnNumber;
                OnMyTurnStarted();
            }
            break;

        case ETurnPhase::TurnEnd:
        case ETurnPhase::TurnTimeout:
        case ETurnPhase::TurnSkipped:
            bResumeOnUnpause = false;
            CancelDecision();
            OnMyTurnEnded(Notification.TurnNumber);
            break;

        case ETurnPhase::GameOver:
            bResumeOnUnpause = false;
            CancelDecision();
            bTurnInProgress = false;
            break;

        default:
            break;
    }
}

void AConnectIt_AIController::HandleMatchPhaseChanged(EMatchPhase NewPhase)
{
    switch (NewPhase)
    {
        case EMatchPhase::Paused:
            // Only worth resuming if this AI was mid-turn
            bResumeOnUnpause = IsMyTurnNow();
            CancelDecision();
            break;

        case EMatchPhase::GameOver:
        case EMatchPhase::InvalidNumberOfPlayers:
            bResumeOnUnpause = false;
            CancelDecision();
            break;

        case EMatchPhase::InProgress:
            if (bResumeOnUnpause)
            {
                bResumeOnUnpause = false;
                if (IsMyTurnNow())
                {
                    BeginDecision();
                }
            }
            break;

        default:
            break;
    }
}

bool AConnectIt_AIController::IsMyTurnNow() const
{
    const UWorld* World = GetWorld();
    const ATurnBasedGameState* GameState =
        World ? World->GetGameState<ATurnBasedGameState>() : nullptr;
    if (!IsValid(GameState)) return false;

    bool bValid = false;
    const FTurnParticipantInfo Active = GameState->GetActiveParticipant(bValid);
    return bValid && Active.PlayerState == GetPlayerState<ATurnBasedPlayerState>();
}

void AConnectIt_AIController::InitialiseFromMatchSetup()
{
    // The match preset's director is a template inside a shared, loaded-once
    // asset -- duplicate it (and with it its strategies and selector) so this
    // controller's decision state is its own (same reason
    // UConnectIt_BoardRegistrySubsystem duplicates the registry templates).
    const AConnectIt_GameMode* GameMode = GetWorld() ? Cast<AConnectIt_GameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
    if (const UConnectIt_AIDirector* DirectorTemplate = GameMode ? GameMode->GetAIDirectorTemplate() : nullptr)
    {
        Director = DuplicateObject<UConnectIt_AIDirector>(DirectorTemplate, this);
        UE_LOG(LogTemp, Log, TEXT("ConnectIt_AIController: AI director '%s'"), *GetNameSafe(DirectorTemplate->GetClass()));
    }
    else
    {
        Director = NewObject<UConnectIt_AIDirector>(this);
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: the match preset has no AI Director -- using the default one "
                 "(MinMax, First Available), which has no loadout: set an AI Director with a Starting Loadout"));
    }

    Coordinator = NewObject<UConnectIt_AICoordinator>(this);

    if (UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this))
    {
        BoardState->OnBoardStateChanged.AddUniqueDynamic(this, &AConnectIt_AIController::HandleBoardStateChanged);
    }

    SeedActionState(Director->StartingLoadout);
    Director->OnMatchStarted();

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

    bTurnInProgress = true;
    if (IsValid(Director))
    {
        Director->OnAITurnStarted(LastStartedTurnNumber);
    }

    DecisionsThisTurn = 0;
    BeginDecision();
}

void AConnectIt_AIController::BeginDecision()
{
    CancelDecision();

    if (!IsValid(Director) || !IsValid(Coordinator))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: BeginDecision -- no director "
                 "(match setup not resolved?), cannot decide"));
        return;
    }

    // Strategy / selector changes wait for a decision boundary
    Director->ApplyPendingChanges();

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

    const UConnectIt_BoardStateComponent* BoardState =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const ATurnBasedPlayerState* PS = GetPlayerState<ATurnBasedPlayerState>();

    if (!IsValid(BoardState) || !IsValid(PS))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: BeginDecision -- BoardStateComponent "
                 "or PlayerState is null, cannot decide"));
        return;
    }

    // The board and the match's rules are copied here, on the game thread.
    // Both are plain data, so a strategy that searches off the game thread
    // can use them directly -- the same rule code the Mediator runs.
    FConnectItAIDecisionContext Context;
    Context.Board = BoardState->GetCurrentState();
    Context.OwnSlot = PS->GetSlotIndex();
    Context.Rules = GameMode->GetRules();

    // Which actions each side has. Two factions -- the opponent is the
    // other slot.
    Context.OwnLoadout = PS->GetLoadout();
    if (const ATurnBasedPlayerState* OpponentPS =
        UConnectIt_GameUtilityLibrary::GetPlayerStateForFaction(this, 1 - Context.OwnSlot))
    {
        Context.OpponentLoadout = OpponentPS->GetLoadout();
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: no PlayerState for the opposing "
                 "faction -- the AI will plan as if its opponent can't move"));
    }

    ++CurrentDecisionId;
    DecisionTurnNumber = GameState->GetActiveTurnNumber();
    Context.TurnNumber = DecisionTurnNumber;
    Context.DecisionIndexThisTurn = DecisionsThisTurn;

    // The kinds of move this AI may make right now: produced by an action in
    // its loadout that is usable this decision -- the same two checks the
    // server's gate makes of a real request
    if (const UActionLoadoutDataAsset* OwnLoadout = PS->GetLoadout())
    {
        const auto Consider = [&Context, PS](const TSubclassOf<UTurnBasedAction>& ActionClass)
        {
            const UTurnBasedAction* CDO = ActionClass ? ActionClass->GetDefaultObject<UTurnBasedAction>() : nullptr;
            if (!IsValid(CDO) || !PS->CanUseAction(ActionClass)) return;

            for (const FGameplayTag& RequestType : FConnectItBoardOperation::GetAllRequestTypes())
            {
                if (CDO->ProducesRequestType(RequestType))
                {
                    Context.AvailableRequestTypes.AddTag(RequestType);
                }
            }
        };
        for (const FPermanentActionConfig& Config : OwnLoadout->PermanentActions)
        {
            Consider(Config.ActionClass);
        }
        for (const FNumberedActionConfig& Config : OwnLoadout->NumberedActions)
        {
            Consider(Config.ActionClass);
        }
    }
    DecisionStartTime = FPlatformTime::Seconds();

    // May call HandleCoordinatorConcluded before returning (strategies that
    // decide synchronously) -- all state above is already set.
    TArray<UConnectIt_AIStrategy*> Strategies;
    for (UConnectIt_AIStrategy* Strategy : Director->Strategies)
    {
        Strategies.Add(Strategy);
    }
    Coordinator->Begin(Context, Strategies, Director->Selector,
        Director->ThinkTimeBudgetSeconds, Director->StrategyBudgetShare);
}

void AConnectIt_AIController::HandleCoordinatorConcluded(const FConnectItAIDecision& Decision)
{
    const int32 DecisionId = CurrentDecisionId;
    if (!IsDecisionStillCurrent(DecisionId)) return;

    if (!Decision.HasMove())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: no strategy of director %s proposed a move"),
            *GetNameSafe(Director));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_AIController: decided -- %d candidate(s), best: [%s] %s"),
        Decision.Candidates.Num(), *Decision.Candidates[0].SourceLabel.ToString(), *Decision.Candidates[0].Summary);

    const double RemainingThink = (IsValid(Director) ? Director->MinThinkSeconds : 0.f)
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

bool AConnectIt_AIController::SubmitCandidate(const FConnectItAICandidate& Candidate, int32 Index)
{
    ATurnBasedPlayerState* PS = GetPlayerState<ATurnBasedPlayerState>();
    UActionLoadoutDataAsset* Loadout = IsValid(PS) ? PS->GetLoadout() : nullptr;
    if (!IsValid(Loadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: SubmitCandidate -- no PlayerState or "
                 "loadout, cannot resolve which action to submit as"));
        return false;
    }

    // What kind of request this is comes from the candidate's operation
    const FConnectItBoardOperation* Operation = Candidate.Payload.GetPtr<FConnectItBoardOperation>();
    if (!Operation)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: candidate %d's payload is not a board "
                 "operation -- skipped"), Index);
        return false;
    }
    const FGameplayTag RequestType = Operation->GetRequestType();

    // Submit as the loadout action allowed to produce this request type --
    // found by asking each configured class, not by hardcoding a reference
    // (the class-keyed action-config convention: config references the
    // class, the tag is derived from it). The Mediator gate checks the same.
    TSubclassOf<UTurnBasedAction> ActionClass = nullptr;
    auto ConsiderClass = [&ActionClass, &RequestType](const TSubclassOf<UTurnBasedAction>& Candidate)
    {
        if (ActionClass || !Candidate) return;
        const UTurnBasedAction* CDO = Candidate->GetDefaultObject<UTurnBasedAction>();
        if (IsValid(CDO) && CDO->ProducesRequestType(RequestType))
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
            TEXT("ConnectIt_AIController: candidate %d -- loadout '%s' has no "
                 "configured action that produces '%s' requests"),
            Index, *Loadout->LoadoutName, *RequestType.ToString());
        return false;
    }

    AConnectIt_GameMode* GameMode = Cast<AConnectIt_GameMode>(GetWorld()->GetAuthGameMode());
    if (!IsValid(GameMode))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: SubmitCandidate -- GameMode is not "
                 "AConnectIt_GameMode"));
        return false;
    }

    FTurnActionRequest Request;
    Request.ActionTag = UTurnBasedAction::GetTagForClass(ActionClass);
    Request.Payload = Candidate.Payload;

    const bool bSucceeded = GameMode->ProcessBoardRequest(Request, PS->GetSlotIndex());

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_AIController: candidate %d '%s' %s as %s -- %s"),
        Index, *RequestType.ToString(), *Operation->Describe(), *ActionClass->GetName(),
        bSucceeded ? TEXT("accepted") : TEXT("REJECTED"));

    return bSucceeded;
}

void AConnectIt_AIController::ChangeLoadout(UActionLoadoutDataAsset* Loadout)
{
    if (!HasAuthority()) return;

    if (bTurnInProgress)
    {
        // Re-seeding mid-turn would reset this turn's use counts
        PendingLoadout = Loadout;
        bHasPendingLoadout = true;
        UE_LOG(LogTemp, Log, TEXT("ConnectIt_AIController: loadout changes to '%s' when this turn ends"),
            IsValid(Loadout) ? *Loadout->LoadoutName : TEXT("none"));
        return;
    }

    SeedActionState(Loadout);
}

void AConnectIt_AIController::OnMyTurnEnded(int32 TurnNumber)
{
    if (!bTurnInProgress) return;
    bTurnInProgress = false;

    // A loadout change asked for during the turn
    if (bHasPendingLoadout)
    {
        bHasPendingLoadout = false;
        SeedActionState(PendingLoadout);
        PendingLoadout = nullptr;
    }

    if (IsValid(Director))
    {
        Director->OnAITurnEnded(TurnNumber);
    }
}

void AConnectIt_AIController::HandleOpponentTurnStarted(int32 ActiveSlotIndex)
{
    if (!IsValid(Director)) return;

    // Anything set here is what the AI plays its next turn with (and, being
    // on the PlayerState, what the player can be shown in the meantime)
    const ATurnBasedGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ATurnBasedGameState>() : nullptr;
    Director->OnOpponentTurnStarted(IsValid(GameState) ? GameState->GetActiveTurnNumber() : INDEX_NONE);
}

void AConnectIt_AIController::HandleBoardStateChanged()
{
    if (!IsValid(Director)) return;

    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const ATurnBasedPlayerState* PS = GetPlayerState<ATurnBasedPlayerState>();
    if (!IsValid(BoardState)) return;

    for (const FInstancedStruct& Entry : BoardState->GetChangeEvent().Events)
    {
        if (const FConnectItBoardEvent_WinCheckpointReached* Checkpoint =
            Entry.GetPtr<FConnectItBoardEvent_WinCheckpointReached>())
        {
            const bool bIsAI = IsValid(PS) && Checkpoint->Faction == PS->GetSlotIndex();
            Director->OnWinCheckpointReached(Checkpoint->Faction, bIsAI, Checkpoint->GroupIndex, Checkpoint->GroupName);
        }
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

    if (IsValid(Coordinator))
    {
        Coordinator->Cancel();
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
    if (!IsValid(PS)) return;

    // Best first, until the server accepts one -- the AI's version of a human
    // trying again after a rejected move. Nothing changes on the board
    // between attempts, so the strategy needn't be asked again.
    bool bSucceeded = false;
    for (int32 Index = 0; Index < Decision.Candidates.Num() && !bSucceeded; Index++)
    {
        bSucceeded = SubmitCandidate(Decision.Candidates[Index], Index);
    }

    if (!bSucceeded)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: the server rejected all %d candidate(s) "
                 "-- ending the AI's turn. The Mediator's log above says why each "
                 "was refused; a strategy should only offer moves of "
                 "Context.AvailableRequestTypes that can be applied to the board."),
            Decision.Candidates.Num());
        ParticipantComponent->ServerSubmitTurnEnd();
        return;
    }

    // ConsumeActionUse above already updated the PlayerState synchronously
    // (server, same call stack -- no replication lag to wait out, unlike a
    // human client's limbo), so it's safe to check turn end immediately.
    if (PS->CanEndTurn())
    {
        ParticipantComponent->ServerSubmitTurnEnd();
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
