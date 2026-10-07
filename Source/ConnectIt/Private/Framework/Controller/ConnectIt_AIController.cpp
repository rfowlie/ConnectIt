// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Controller/ConnectIt_AIController.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Framework/PlayerState/ConnectIt_PlayerState.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedAction.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "AI/ConnectIt_AIProfile.h"
#include "AI/ConnectIt_AIStrategy_MinMax.h"
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
        case ETurnPhase::GameOver:
            bResumeOnUnpause = false;
            CancelDecision();
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
    const UConnectIt_AIProfile* Profile = ResolveAIProfile();
    if (!IsValid(Profile))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIController: no AI profile (set AIProfile on the "
                 "level config, or pick one in the match setup) -- no loadout, "
                 "so every move will be rejected"));
    }
    else
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_AIController: using AI profile '%s'"), *Profile->GetName());
    }

    SeedActionState(IsValid(Profile) ? Profile->Loadout.Get() : nullptr);

    // The profile's strategy is a template inside a shared, loaded-once
    // asset -- duplicate it so this controller's decision state is its own
    // (same reason UConnectIt_BoardRegistrySubsystem duplicates the
    // registry templates).
    if (IsValid(Profile) && IsValid(Profile->Strategy))
    {
        Strategy = DuplicateObject<UConnectIt_AIStrategy>(Profile->Strategy, this);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIController: AI profile has no Strategy -- using "
                 "a default MinMax strategy"));
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

const UConnectIt_AIProfile* AConnectIt_AIController::ResolveAIProfile() const
{
    // The GameMode owns the live match setup (menu choice, else the level's
    // default) -- this controller only ever exists on the server, next to it.
    const UWorld* World = GetWorld();
    const AConnectIt_GameMode* GameMode =
        World ? Cast<AConnectIt_GameMode>(World->GetAuthGameMode()) : nullptr;
    return IsValid(GameMode) ? GameMode->GetAIProfile() : nullptr;
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
