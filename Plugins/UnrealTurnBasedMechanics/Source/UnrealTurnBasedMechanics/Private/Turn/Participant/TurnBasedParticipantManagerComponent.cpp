// Fill out your copyright notice in the Description page of Project Settings.

#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "UnrealTurnBasedMechanics.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "GameFramework/GameMode.h"
#include "Turn/Order/SequentialTurnOrderStrategy.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UTurnBasedParticipantManagerComponent::UTurnBasedParticipantManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UTurnBasedParticipantManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    // bind to game state match over
    FGameModeEvents::GameModeMatchStateSetEvent.AddUObject(
        this, &ThisClass::OnGameModeWaitingPostMatch);

    if (!TurnOrderStrategy.GetObject())
    {
        TurnOrderStrategy = NewObject<USequentialTurnOrderStrategy>(
            this, USequentialTurnOrderStrategy::StaticClass());

        UE_LOG(LogTurnBasedMechanics, Log,
            TEXT("TurnBasedParticipantManager: No strategy set "
                 "— defaulting to Sequential"));
    }

    if (TurnEndEventTag.IsValid())
    {
        if (UGameEventTaskSubsystem* GameEventSubsystem =
            GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr)
        {
            GameEventSubsystem->BindOnTagComplete(TurnEndEventTag, this,
                GET_FUNCTION_NAME_CHECKED(UTurnBasedParticipantManagerComponent, AdvanceToNextParticipant));
        }
    }
    else
    {
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedParticipantManager: TurnEndEventTag not set on %s "
                 "— EndTurn will fall back to advancing immediately with no "
                 "gated turn-end sequence."),
            *GetOwner()->GetName());
    }
}

void UTurnBasedParticipantManagerComponent::OnGameModeWaitingPostMatch(FName Name)
{
    if (Name != MatchState::WaitingPostMatch) return;

    // Stop any in-flight turn timer -- otherwise a still-running timer
    // could later fire HandleTurnTimeout -> EndTurn and re-enter the
    // turn-advance machinery after the match has already ended.
    GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
    ReplicatedTurnStartServerTime = -1.f;

    SetPhase(ETurnPhase::GameOver);
    SetMatchPhase(EMatchPhase::GameOver);
    OnGameOver.Broadcast();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: MatchState -> WaitingPostMatch -- "
             "entering GameOver, no further turn/board updates will fire"));
}


void UTurnBasedParticipantManagerComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, CurrentPhase);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, ActiveParticipantIndex);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, TurnNumber);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, Participants);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, ReplicatedTurnDuration);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, ReplicatedTurnStartServerTime);
}

// --- Setup ---

void UTurnBasedParticipantManagerComponent::RegisterParticipant(
    AController* Controller,
    const EParticipantType Type)
{
    check(!IsRunningClientOnly());
    if (!IsValid(Controller)) return;

    ATurnBasedPlayerState* PS =
        Controller->GetPlayerState<ATurnBasedPlayerState>();

    if (!IsValid(PS))
    {
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedParticipantManager: %s has no "
                 "ATurnBasedPlayerState — cannot register. "
                 "AI controllers must have PlayerState created "
                 "before registering."),
            *Controller->GetName());
        return;
    }

    if (IsParticipantRegistered(Controller))
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedParticipantManager: %s already registered"),
            *Controller->GetName());
        return;
    }

    const int32 NewSlotIndex = Participants.Num();

    PS->SetSlotIndex(NewSlotIndex);
    PS->SetParticipantType(Type);
    PS->SetReady(Type == EParticipantType::AI);

    FTurnParticipantInfo Info;
    Info.PlayerState     = PS;
    Info.ParticipantType = Type;
    Info.SlotIndex       = NewSlotIndex;
    Info.bConnected      = true;

    Participants.Add(Info);
    ServerControllers.Add(Controller);

    if (UTurnBasedParticipantComponent* Comp =
        GetParticipantComponent(Controller))
    {
        Comp->CachedSlotIndex = NewSlotIndex;
        Comp->ParticipantType = Type;
    }

    // Match is still gathering participants
    SetMatchPhase(EMatchPhase::WaitingForParticipants);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: Registered %s '%s' at slot %d"),
        Type == EParticipantType::AI ? TEXT("AI") : TEXT("Human"),
        *PS->GetPlayerName(),
        NewSlotIndex);

    // if (CheckAllParticipantsRegistered())
    // {
    //     SetMatchPhase(EMatchPhase::WaitingForReady);
    // }
}

bool UTurnBasedParticipantManagerComponent::IsParticipantRegistered(
    AController* Controller) const
{
    return FindParticipantIndex(Controller) != INDEX_NONE;
}

void UTurnBasedParticipantManagerComponent::NotifyParticipantReady(
    AController* Controller)
{
    check(!IsRunningClientOnly());

    const FTurnParticipantInfo* Info = FindParticipant(Controller);
    if (!Info) return;
    if (!IsValid(Info->PlayerState)) return;

    if (ATurnBasedPlayerState* PS =
        Cast<ATurnBasedPlayerState>(Info->PlayerState))
    {
        PS->SetReady(true);
    }

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: %s confirmed ready"),
        *Info->GetDisplayName());

    CheckReadyStatus();
}

void UTurnBasedParticipantManagerComponent::BeginReadyCheck()
{
    check(!IsRunningClientOnly());

    SetMatchPhase(EMatchPhase::WaitingForReady);
    CheckReadyStatus();
}

// --- Turn Control ---

void UTurnBasedParticipantManagerComponent::NotifyTurnEndSubmitted(
    AController* Controller)
{
    check(!IsRunningClientOnly());

    if (CurrentPhase != ETurnPhase::TurnActive)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedParticipantManager: TurnEnd submitted outside "
                 "TurnActive phase — ignored"));
        return;
    }

    const int32 SubmitterIndex = FindParticipantIndex(Controller);
    if (SubmitterIndex != ActiveParticipantIndex)
    {
        UE_LOG(LogTurnBasedMechanics, Warning,
            TEXT("TurnBasedParticipantManager: TurnEnd submitted by "
                 "non-active participant — ignored"));
        return;
    }

    EndTurn(ETurnEndReason::ParticipantEnded);
}

void UTurnBasedParticipantManagerComponent::NotifyParticipantDisconnected(
    AController* Controller)
{
    check(!IsRunningClientOnly());

    FTurnParticipantInfo* Info = FindParticipant(Controller);
    if (!Info) return;

    Info->bConnected = false;
    const int32 DisconnectedIndex = FindParticipantIndex(Controller);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: %s disconnected"),
        *Info->GetDisplayName());

    if (DisconnectedIndex == ActiveParticipantIndex
        && CurrentPhase == ETurnPhase::TurnActive)
    {
        GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        ReplicatedTurnStartServerTime = -1.f;
        DisconnectedParticipantIndex = DisconnectedIndex;

        SetPhase(ETurnPhase::TurnPaused);
        SetMatchPhase(EMatchPhase::Paused);

        GetWorld()->GetTimerManager().SetTimer(
            ReconnectTimerHandle,
            this,
            &UTurnBasedParticipantManagerComponent::HandleReconnectTimeout,
            ReconnectTimeout,
            false
        );

        UE_LOG(LogTurnBasedMechanics, Log,
            TEXT("TurnBasedParticipantManager: Paused — "
                 "waiting %.1fs for reconnect"),
            ReconnectTimeout);
    }
}

void UTurnBasedParticipantManagerComponent::NotifyParticipantReconnected(
    AController* Controller)
{
    check(!IsRunningClientOnly());

    FTurnParticipantInfo* Info = FindParticipant(Controller);
    if (!Info) return;

    Info->bConnected = true;
    const int32 ReconnectedIndex = FindParticipantIndex(Controller);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: %s reconnected"),
        *Info->GetDisplayName());

    if (ReconnectedIndex == DisconnectedParticipantIndex
        && CurrentPhase == ETurnPhase::TurnPaused)
    {
        GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
        DisconnectedParticipantIndex = -1;

        SetPhase(ETurnPhase::TurnActive);
        SetMatchPhase(EMatchPhase::InProgress);

        if (UTurnBasedParticipantComponent* Comp =
            GetParticipantComponent(Controller))
        {
            Comp->ClientReceiveTurnNotification(
                BuildNotification(ReconnectedIndex, ETurnPhase::TurnActive));
        }

        // Restarts with the full TurnDuration, not the remaining time at
        // disconnect -- pre-existing behaviour, not changed here. Re-stamp
        // the start time to match so the client-derived countdown mirrors
        // exactly what actually happens rather than showing a stale value.
        if (const ATurnBasedGameState* GS = GetOwningGameState())
        {
            ReplicatedTurnStartServerTime = GS->GetServerWorldTimeSeconds();
        }

        GetWorld()->GetTimerManager().SetTimer(
            TurnTimerHandle,
            this,
            &UTurnBasedParticipantManagerComponent::HandleTurnTimeout,
            TurnDuration,
            false
        );
    }
}

// --- State Machine ---

void UTurnBasedParticipantManagerComponent::SetPhase(ETurnPhase NewPhase)
{
    CurrentPhase = NewPhase;
    OnTurnPhaseChanged.Broadcast(NewPhase);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: Turn phase -> %s"),
        *UEnum::GetValueAsString(NewPhase));
}

void UTurnBasedParticipantManagerComponent::StartTurn(int32 ParticipantIndex)
{
    check(!IsRunningClientOnly());

    if (!Participants.IsValidIndex(ParticipantIndex))
    {
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedParticipantManager: StartTurn — invalid index %d"),
            ParticipantIndex);
        return;
    }

    ActiveParticipantIndex = ParticipantIndex;
    ReplicatedTurnDuration = TurnDuration;
    if (const ATurnBasedGameState* GS = GetOwningGameState())
    {
        ReplicatedTurnStartServerTime = GS->GetServerWorldTimeSeconds();
    }
    ++TurnNumber;
    Participants[ParticipantIndex].TurnsTaken++;

    // Match is now actively running a turn
    // Fired before turn notifications so clients know updating has ended
    SetMatchPhase(EMatchPhase::InProgress);

    SetPhase(ETurnPhase::TurnStart);
    BroadcastTurnStart(ParticipantIndex);

    GetWorld()->GetTimerManager().SetTimer(
        TurnTimerHandle,
        this,
        &UTurnBasedParticipantManagerComponent::HandleTurnTimeout,
        TurnDuration,
        false
    );

    SetPhase(ETurnPhase::TurnActive);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: Turn %d started for %s"),
        TurnNumber,
        *Participants[ParticipantIndex].GetDisplayName());
}

void UTurnBasedParticipantManagerComponent::EndTurn(ETurnEndReason Reason)
{
    check(!IsRunningClientOnly());

    GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
    ReplicatedTurnStartServerTime = -1.f;

    SetPhase(ETurnPhase::TurnEnd);
    NotifyActiveParticipant(ETurnPhase::TurnEnd, Reason);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: Turn %d ended — %s"),
        TurnNumber, *UEnum::GetValueAsString(Reason));

    // Enter updating -- all clients suspend input while resolution plays
    SetMatchPhase(EMatchPhase::Updating);

    // Gate AdvanceToNextParticipant on TurnEndEventTag -- fires immediately
    // if nothing is registered against it, or once every registered gated
    // task completes. See BeginPlay for the AdvanceToNextParticipant bind.
    // QueueTagContainer (not the subsystem's private TriggerTag) is the
    // correct entry point for any external caller -- it also means this
    // enqueue takes its place in the subsystem's own FIFO queue, which is
    // what guarantees it can't start firing until every board-event tag a
    // project might have enqueued ahead of it (e.g. ConnectIt's
    // ConnectIt_BoardStateComponent) has already fully completed.
    UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld() ? GetWorld()->GetSubsystem<UGameEventTaskSubsystem>() : nullptr;

    if (IsValid(GameEventSubsystem) && TurnEndEventTag.IsValid())
    {
        GameEventSubsystem->QueueTagContainer(FGameplayTagContainer(TurnEndEventTag));
    }
    else
    {
        AdvanceToNextParticipant(TurnEndEventTag);
    }
}

void UTurnBasedParticipantManagerComponent::AdvanceToNextParticipant(FGameplayTag Tag)
{
    check(!IsRunningClientOnly());
    check(TurnOrderStrategy.GetInterface() != nullptr);

    // Check enough active participants remain before selecting next one
    if (CheckValidNumberOfPlayers()) return;

    const int32 NextIndex =
        ITurnOrderInterface::Execute_GetNextParticipantIndex(
            TurnOrderStrategy.GetObject(),
            Participants,
            ActiveParticipantIndex
        );

    if (NextIndex == INDEX_NONE)
    {
        // Same category of "plugin cannot validly continue" as
        // CheckValidNumberOfPlayers -- defer to the project rather than
        // unilaterally declaring the match over.
        NotifyInvalidNumberOfPlayers();
        return;
    }

    StartTurn(NextIndex);
}

void UTurnBasedParticipantManagerComponent::HandleTurnTimeout()
{
    check(!IsRunningClientOnly());
    if (!Participants.IsValidIndex(ActiveParticipantIndex)) return;

    ATurnBasedPlayerState* PS = Cast<ATurnBasedPlayerState>(
        Participants[ActiveParticipantIndex].PlayerState);

    if (IsValid(PS))
    {
        PS->IncrementTurnsMissed();

        if (PS->GetTurnsMissed() >= ForfeitThreshold)
        {
            PS->SetForfeited(true);
            OnParticipantForfeited.Broadcast(
                Participants[ActiveParticipantIndex]);

            UE_LOG(LogTurnBasedMechanics, Log,
                TEXT("TurnBasedParticipantManager: %s forfeited "
                     "after %d missed turns"),
                *Participants[ActiveParticipantIndex].GetDisplayName(),
                PS->GetTurnsMissed());
        }
    }

    SetPhase(ETurnPhase::TurnTimeout);
    NotifyActiveParticipant(ETurnPhase::TurnTimeout, ETurnEndReason::TimedOut);
    EndTurn(ETurnEndReason::TimedOut);
}

void UTurnBasedParticipantManagerComponent::HandleReconnectTimeout()
{
    DisconnectedParticipantIndex = -1;
    EndTurn(ETurnEndReason::TimedOut);
}

void UTurnBasedParticipantManagerComponent::CheckReadyStatus()
{
    check(!IsRunningClientOnly());

    const bool bAllReady = !Participants.ContainsByPredicate(
        [](const FTurnParticipantInfo& Info)
        {
            const ATurnBasedPlayerState* PS =
                Cast<ATurnBasedPlayerState>(Info.PlayerState);
            return !IsValid(PS) || !PS->IsReady();
        });

    if (!bAllReady || Participants.IsEmpty()) return;

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: All participants ready"));

    OnAllParticipantsReady.Broadcast();

    check(TurnOrderStrategy.GetInterface() != nullptr);

    const int32 FirstIndex =
        ITurnOrderInterface::Execute_GetFirstParticipantIndex(
            TurnOrderStrategy.GetObject(), Participants);

    if (FirstIndex == INDEX_NONE)
    {
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedParticipantManager: Turn order strategy "
                 "returned no valid first participant"));
        return;
    }

    StartTurn(FirstIndex);
}

bool UTurnBasedParticipantManagerComponent::CheckValidNumberOfPlayers() const
{
    const int32 ActiveCount = Participants.FilterByPredicate(
        [](const FTurnParticipantInfo& Info)
        {
            return Info.IsActiveParticipant();
        }).Num();

    if (ActiveCount > 1) return false;

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: Invalid number of active "
             "participants remaining (%d)"),
        ActiveCount);

    NotifyInvalidNumberOfPlayers();
    return true;
}

void UTurnBasedParticipantManagerComponent::NotifyInvalidNumberOfPlayers() const
{
    SetMatchPhase(EMatchPhase::InvalidNumberOfPlayers);
    OnInvalidNumberOfPlayers.Broadcast();

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: Invalid number of active "
             "participants -- notifying project to resolve (fix it, or "
             "call EndMatch())"));
}

void UTurnBasedParticipantManagerComponent::BroadcastTurnStart(const int32 ActiveIndex)
{
    if (!Participants.IsValidIndex(ActiveIndex)) return;

    const int32 ActiveSlotIndex = Participants[ActiveIndex].SlotIndex;

    for (int32 i = 0; i < Participants.Num(); i++)
    {
        AController* Controller = GetControllerAtIndex(i);
        if (!IsValid(Controller)) continue;

        UTurnBasedParticipantComponent* Comp =
            GetParticipantComponent(Controller);
        if (!IsValid(Comp)) continue;

        if (i == ActiveIndex)
        {
            Comp->ClientReceiveTurnNotification(
                BuildNotification(i, ETurnPhase::TurnStart));
        }
        else
        {
            Comp->ClientReceiveOpponentTurnStarted(ActiveSlotIndex);
        }
    }

    // Notify listeners that board ownership should transfer
    OnActiveControllerChanged.Broadcast(GetControllerAtIndex(ActiveIndex));
}

void UTurnBasedParticipantManagerComponent::BroadcastControllerChanged(int32 ActiveIndex)
{
    // TODO: where and why do we need this?
}

// --- Queries ---

FTurnParticipantInfo UTurnBasedParticipantManagerComponent::GetActiveParticipant(
    bool& bOutValid) const
{
    if (Participants.IsValidIndex(ActiveParticipantIndex))
    {
        bOutValid = true;
        return Participants[ActiveParticipantIndex];
    }

    bOutValid = false;
    return FTurnParticipantInfo();
}

FTurnParticipantInfo UTurnBasedParticipantManagerComponent::GetParticipantBySlot(
    int32 InSlotIndex, bool& bOutValid) const
{
    if (const FTurnParticipantInfo* Found = Participants.FindByPredicate(
        [InSlotIndex](const FTurnParticipantInfo& Info)
        {
            return Info.SlotIndex == InSlotIndex;
        }))
    {
        bOutValid = true;
        return *Found;
    }

    bOutValid = false;
    return FTurnParticipantInfo();
}

// --- Helpers ---

AController* UTurnBasedParticipantManagerComponent::GetControllerAtIndex(
    int32 Index) const
{
    if (!ServerControllers.IsValidIndex(Index)) return nullptr;
    return ServerControllers[Index].Get();
}

ATurnBasedGameState*
UTurnBasedParticipantManagerComponent::GetOwningGameState() const
{
    return GetOwner<ATurnBasedGameState>();
}

void UTurnBasedParticipantManagerComponent::SetMatchPhase(
    EMatchPhase NewPhase) const
{
    if (ATurnBasedGameState* GS = GetOwningGameState())
    {
        GS->SetMatchPhase(NewPhase);
    }
}

FTurnParticipantInfo* UTurnBasedParticipantManagerComponent::FindParticipant(
    AController* Controller)
{
    const int32 Index = FindParticipantIndex(Controller);
    return Participants.IsValidIndex(Index) ? &Participants[Index] : nullptr;
}

const FTurnParticipantInfo* UTurnBasedParticipantManagerComponent::FindParticipant(
    AController* Controller) const
{
    const int32 Index = FindParticipantIndex(Controller);
    return Participants.IsValidIndex(Index) ? &Participants[Index] : nullptr;
}

int32 UTurnBasedParticipantManagerComponent::FindParticipantIndex(
    AController* Controller) const
{
    return ServerControllers.IndexOfByPredicate(
        [Controller](const TWeakObjectPtr<AController>& Weak)
        {
            return Weak.Get() == Controller;
        });
}

UTurnBasedParticipantComponent* UTurnBasedParticipantManagerComponent::GetParticipantComponent(
    AController* Controller) const
{
    if (!IsValid(Controller)) return nullptr;
    return Controller->FindComponentByClass<UTurnBasedParticipantComponent>();
}

void UTurnBasedParticipantManagerComponent::NotifyActiveParticipant(
    const ETurnPhase Phase, const ETurnEndReason Reason)
{
    if (!Participants.IsValidIndex(ActiveParticipantIndex)) return;

    UTurnBasedParticipantComponent* Comp = GetParticipantComponent(
        GetControllerAtIndex(ActiveParticipantIndex));

    if (!IsValid(Comp)) return;

    Comp->ClientReceiveTurnNotification(
        BuildNotification(ActiveParticipantIndex, Phase, Reason));
}

FTurnNotification UTurnBasedParticipantManagerComponent::BuildNotification(
    const int32 ParticipantIndex,
    const ETurnPhase Phase,
    const ETurnEndReason Reason) const
{
    FTurnNotification Notification;

    if (Participants.IsValidIndex(ParticipantIndex))
    {
        Notification.ParticipantInfo = Participants[ParticipantIndex];
    }

    Notification.Phase        = Phase;
    Notification.EndReason    = Reason;
    Notification.TurnNumber   = TurnNumber;
    Notification.TurnDuration = TurnDuration;

    return Notification;
}

// --- RepNotify ---

void UTurnBasedParticipantManagerComponent::OnRep_CurrentPhase()
{
    OnTurnPhaseChanged.Broadcast(CurrentPhase);
}

void UTurnBasedParticipantManagerComponent::OnRep_ActiveParticipantIndex()
{
    // Clients react here -- e.g. highlight active player in UI
    OnParticipantIndexChanged.Broadcast(ActiveParticipantIndex);
}

void UTurnBasedParticipantManagerComponent::OnRep_TurnNumber()
{
    // Reuses OnTurnPhaseChanged rather than a dedicated delegate -- TurnNumber
    // and CurrentPhase change together every turn, and listeners only ever
    // care that turn state changed, not specifically which property tripped
    // the notification. See TurnNumber's own doc comment for why this needs
    // its own ReplicatedUsing instead of piggybacking on OnRep_CurrentPhase.
    OnTurnPhaseChanged.Broadcast(CurrentPhase);
    OnTurnChanged.Broadcast(TurnNumber);
}