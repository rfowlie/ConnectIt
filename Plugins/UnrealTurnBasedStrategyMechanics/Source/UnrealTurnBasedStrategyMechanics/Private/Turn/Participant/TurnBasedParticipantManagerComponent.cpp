// Fill out your copyright notice in the Description page of Project Settings.

#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Turn/Order/TurnOrderInterface.h"
#include "Turn/Order/TurnOrderManagerBase.h"


UTurnBasedParticipantManagerComponent::UTurnBasedParticipantManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    
}

void UTurnBasedParticipantManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (TurnOrderStrategy == nullptr || !TurnOrderStrategy->Implements<UTurnOrderInterface>())
    {
        // create default turn order
        TurnOrderStrategy = NewObject<UTurnOrderManagerBase>(this);
    }
}

void UTurnBasedParticipantManagerComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, CurrentPhase);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, ActiveParticipantIndex);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, TurnNumber);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, Participants);
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, TurnDuration);
}

// --- Setup ---

void UTurnBasedParticipantManagerComponent::RegisterParticipant(
    AController* Controller,
    EParticipantType Type,
    const FString& DisplayName)
{
    check(!IsRunningClientOnly());

    if (!IsValid(Controller))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantManager: RegisterParticipant called with invalid controller"));
        return;
    }

    FTurnParticipantInfo Info;
    Info.Controller      = Controller;
    Info.ParticipantType = Type;
    Info.DisplayName     = DisplayName;
    Info.SlotIndex       = Participants.Num();
    Info.bReady          = Type == EParticipantType::AI; // AI auto-confirms
    Info.bConnected      = true;

    Participants.Add(Info);

    // Set participant type on the component
    if (UTurnBasedParticipantComponent* Comp = GetParticipantComponent(Controller))
    {
        Comp->ParticipantType = Type;
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Registered %s participant %s at slot %d"),
        Type == EParticipantType::AI ? TEXT("AI") : TEXT("Human"),
        *DisplayName,
        Info.SlotIndex);
}

void UTurnBasedParticipantManagerComponent::SetTurnOrderStrategy(UObject* InStrategy)
{
    // ensure that interface is implemented else do not change strategy
    if (!IsValid(InStrategy))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantManager: SetTurnOrderStrategy called with invalid object"));
        return;
    }

    if (!InStrategy->Implements<UTurnOrderInterface>())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantManager: %s does not implement ITurnOrderStrategy"),
            *InStrategy->GetName());
        return;
    }

    TurnOrderStrategy = InStrategy;

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Strategy set to %s"),
        *ITurnOrderInterface::Execute_GetStrategyName(TurnOrderStrategy));
}

void UTurnBasedParticipantManagerComponent::BeginReadyCheck()
{
    check(!IsRunningClientOnly());
    SetPhase(ETurnPhase::WaitingForReady);
    CheckReadyStatus();
}

// --- Turn Control ---

void UTurnBasedParticipantManagerComponent::NotifyTurnEndSubmitted(
    AController* Controller)
{
    check(!IsRunningClientOnly());

    const int32 SubmitterIndex = FindParticipantIndex(Controller);
    if (SubmitterIndex != ActiveParticipantIndex)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantManager: Turn end submitted by non-active participant"));
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
    DisconnectedParticipantIndex = FindParticipantIndex(Controller);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: %s disconnected"),
        *Info->DisplayName);

    // Only pause if it is this participant's turn
    if (DisconnectedParticipantIndex == ActiveParticipantIndex)
    {
        // Cancel turn timer — reconnect timer takes over
        GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        SetPhase(ETurnPhase::Paused);

        GetWorld()->GetTimerManager().SetTimer(
            ReconnectTimerHandle,
            this,
            &UTurnBasedParticipantManagerComponent::HandleReconnectTimeout,
            ReconnectTimeout,
            false
        );

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedParticipantManager: Paused — waiting %0.1f seconds for reconnect"),
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

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: %s reconnected"),
        *Info->DisplayName);

    // Only resume if this was the disconnected active participant
    if (ReconnectedIndex == DisconnectedParticipantIndex
        && CurrentPhase == ETurnPhase::Paused)
    {
        GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
        DisconnectedParticipantIndex = -1;

        // Resume the turn with remaining time — clients restart from TurnDuration
        // which is an acceptable approximation for turn-based
        SetPhase(ETurnPhase::TurnActive);

        if (UTurnBasedParticipantComponent* Comp = GetParticipantComponent(Controller))
        {
            const FTurnNotification Notification = BuildNotification(
                ReconnectedIndex, ETurnPhase::TurnActive);
            Comp->ClientNotifyTurnResumed(Notification);
        }

        // Restart server turn timer with full duration
        // Turn based — full reset on reconnect is acceptable
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

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Phase -> %s"),
        *UEnum::GetValueAsString(NewPhase));
}

void UTurnBasedParticipantManagerComponent::StartTurn(int32 ParticipantIndex)
{
    check(!IsRunningClientOnly());

    if (!Participants.IsValidIndex(ParticipantIndex))
    {
        UE_LOG(LogTemp, Error,
            TEXT("TurnBasedParticipantManager: StartTurn called with invalid index %d"),
            ParticipantIndex);
        return;
    }

    ActiveParticipantIndex = ParticipantIndex;
    TurnNumber++;

    SetPhase(ETurnPhase::TurnStart);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Turn %d started for %s"),
        TurnNumber,
        *Participants[ParticipantIndex].DisplayName);

    // Notify the active participant
    NotifyActiveParticipant(ETurnPhase::TurnStart);

    // Start server authoritative turn timer
    GetWorld()->GetTimerManager().SetTimer(
        TurnTimerHandle,
        this,
        &UTurnBasedParticipantManagerComponent::HandleTurnTimeout,
        TurnDuration,
        false
    );

    SetPhase(ETurnPhase::TurnActive);
}

void UTurnBasedParticipantManagerComponent::EndTurn(ETurnEndReason Reason)
{
    check(!IsRunningClientOnly());

    GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
    SetPhase(ETurnPhase::TurnEnd);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Turn %d ended — reason: %s"),
        TurnNumber,
        *UEnum::GetValueAsString(Reason));

    // Notify the participant whose turn just ended
    NotifyActiveParticipant(ETurnPhase::TurnEnd, Reason);

    AdvanceToNextParticipant();
}

void UTurnBasedParticipantManagerComponent::AdvanceToNextParticipant()
{
    check(!IsRunningClientOnly());
    check(TurnOrderStrategy);

    const int32 NextIndex = ITurnOrderInterface::Execute_GetNextParticipantIndex(
        TurnOrderStrategy, Participants, ActiveParticipantIndex);

    if (NextIndex == -1)
    {
        // No active participants remaining
        SetPhase(ETurnPhase::GameOver);
        OnGameOver.Broadcast();
        return;
    }

    CheckGameOver();
    StartTurn(NextIndex);
}

void UTurnBasedParticipantManagerComponent::HandleTurnTimeout()
{
    check(!IsRunningClientOnly());

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Turn %d timed out for %s"),
        TurnNumber,
        Participants.IsValidIndex(ActiveParticipantIndex)
            ? *Participants[ActiveParticipantIndex].DisplayName
            : TEXT("Unknown"));

    SetPhase(ETurnPhase::TurnTimeout);

    // Increment missed turns on the active participant
    if (Participants.IsValidIndex(ActiveParticipantIndex))
    {
        FTurnParticipantInfo& Info = Participants[ActiveParticipantIndex];
        Info.TurnsMissed++;

        NotifyActiveParticipant(ETurnPhase::TurnTimeout,
            ETurnEndReason::TimedOut);

        // Update PlayerState if human
        if (AController* Controller = Info.Controller)
        {
            if (ATurnBasedPlayerState* PS =
                Controller->GetPlayerState<ATurnBasedPlayerState>())
            {
                PS->Server_IncrementTurnsMissed();

                // Check forfeit threshold
                if (Info.TurnsMissed >= ForfeitThreshold)
                {
                    Info.bForfeited = true;
                    PS->Server_SetForfeited();

                    NotifyActiveParticipant(ETurnPhase::TurnEnd,
                        ETurnEndReason::Forfeited);
                    OnParticipantForfeited.Broadcast(Info);

                    UE_LOG(LogTemp, Log,
                        TEXT("TurnBasedParticipantManager: %s forfeited after %d missed turns"),
                        *Info.DisplayName, Info.TurnsMissed);
                }
            }
        }
    }

    AdvanceToNextParticipant();
}

void UTurnBasedParticipantManagerComponent::HandleReconnectTimeout()
{
    check(!IsRunningClientOnly());

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Reconnect timeout — skipping turn"));

    DisconnectedParticipantIndex = -1;
    EndTurn(ETurnEndReason::TimedOut);
}

void UTurnBasedParticipantManagerComponent::CheckReadyStatus()
{
    check(!IsRunningClientOnly());

    const bool bAllReady = Participants.FindByPredicate(
        [](const FTurnParticipantInfo& Info)
        {
            return !Info.bReady;
        }) == nullptr;

    if (bAllReady && !Participants.IsEmpty())
    {
        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedParticipantManager: All participants ready — starting game"));

        OnAllParticipantsReady.Broadcast();

        const int32 FirstIndex = ITurnOrderInterface::Execute_GetFirstParticipantIndex(
            TurnOrderStrategy, Participants);
        StartTurn(FirstIndex);
    }
}

void UTurnBasedParticipantManagerComponent::CheckGameOver()
{
    const int32 ActiveCount = Participants.FilterByPredicate(
        [](const FTurnParticipantInfo& Info)
        {
            return Info.IsActiveParticipant();
        }).Num();

    if (ActiveCount <= 1)
    {
        SetPhase(ETurnPhase::GameOver);
        OnGameOver.Broadcast();
    }
}

// --- Participant Helpers ---

FTurnParticipantInfo* UTurnBasedParticipantManagerComponent::FindParticipant(
    AController* Controller)
{
    return Participants.FindByPredicate(
        [Controller](const FTurnParticipantInfo& Info)
        {
            return Info.Controller == Controller;
        });
}

int32 UTurnBasedParticipantManagerComponent::FindParticipantIndex(
    AController* Controller) const
{
    return Participants.IndexOfByPredicate(
        [Controller](const FTurnParticipantInfo& Info)
        {
            return Info.Controller == Controller;
        });
}

UTurnBasedParticipantComponent* UTurnBasedParticipantManagerComponent::GetParticipantComponent(
    AController* Controller) const
{
    if (!IsValid(Controller)) return nullptr;
    return Controller->FindComponentByClass<UTurnBasedParticipantComponent>();
}

// --- Notifications ---

void UTurnBasedParticipantManagerComponent::NotifyActiveParticipant(
    ETurnPhase Phase,
    ETurnEndReason Reason)
{
    if (!Participants.IsValidIndex(ActiveParticipantIndex)) return;

    const FTurnParticipantInfo& Info = Participants[ActiveParticipantIndex];
    UTurnBasedParticipantComponent* Comp = GetParticipantComponent(Info.Controller);
    if (!Comp) return;

    const FTurnNotification Notification = BuildNotification(
        ActiveParticipantIndex, Phase, Reason);

    switch (Phase)
    {
        case ETurnPhase::TurnStart:
            Comp->ClientNotifyTurnStarted(Notification);
            break;
        case ETurnPhase::TurnEnd:
            if (Reason == ETurnEndReason::Forfeited)
                Comp->ClientNotifyForfeited(Notification);
            else if (Reason == ETurnEndReason::Skipped)
                Comp->ClientNotifyTurnSkipped(Notification);
            else
                Comp->ClientNotifyTurnEnded(Notification);
            break;
        case ETurnPhase::TurnTimeout:
            Comp->ClientNotifyTurnTimedOut(Notification);
            break;
        case ETurnPhase::Paused:
            Comp->ClientNotifyTurnPaused(Notification);
            break;
        default:
            break;
    }
}

FTurnNotification UTurnBasedParticipantManagerComponent::BuildNotification(
    int32 ParticipantIndex,
    ETurnPhase Phase,
    ETurnEndReason Reason) const
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

void UTurnBasedParticipantManagerComponent::OnRep_TurnPhase()
{
    OnTurnPhaseChanged.Broadcast(CurrentPhase);
}

void UTurnBasedParticipantManagerComponent::OnRep_ActiveParticipantIndex()
{
    // Clients can react to active participant changing here
    // e.g. highlight the active player in UI
}
