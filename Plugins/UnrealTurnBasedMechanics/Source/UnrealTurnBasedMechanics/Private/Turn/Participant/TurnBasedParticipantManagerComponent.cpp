// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
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

    // Default to Sequential if no strategy set in editor
    if (!IsValid(TurnOrderStrategy))
    {
        TurnOrderStrategy = NewObject<USequentialTurnOrderStrategy>(
            this, USequentialTurnOrderStrategy::StaticClass());

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedParticipantManager: No strategy set "
                 "— defaulting to Sequential"));
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
    DOREPLIFETIME(UTurnBasedParticipantManagerComponent, ReplicatedTurnDuration);
}

// --- Setup ---

void UTurnBasedParticipantManagerComponent::RegisterParticipant(
    AController* Controller,
    EParticipantType Type,
    const FString& DisplayName)
{
    check(!IsRunningClientOnly());

    if (!IsValid(Controller)) return;

    // Warn on duplicate
    if (FindParticipant(Controller))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantManager: %s already registered"),
            *Controller->GetName());
        return;
    }

    FTurnParticipantInfo Info;
    Info.Controller      = Controller;
    Info.ParticipantType = Type;
    Info.DisplayName     = DisplayName;
    Info.SlotIndex       = Participants.Num();
    Info.bReady          = (Type == EParticipantType::AI); // AI auto-confirms
    Info.bConnected      = true;

    Participants.Add(Info);

    if (UTurnBasedParticipantComponent* Comp = GetParticipantComponent(Controller))
    {
        Comp->ParticipantType = Type;
        Comp->CachedSlotIndex = Info.SlotIndex;
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Registered %s '%s' at slot %d"),
        Type == EParticipantType::AI ? TEXT("AI") : TEXT("Human"),
        *DisplayName, Info.SlotIndex);
}

void UTurnBasedParticipantManagerComponent::NotifyParticipantReady(
    AController* Controller)
{
    check(!IsRunningClientOnly());

    FTurnParticipantInfo* Info = FindParticipant(Controller);
    if (!Info) return;

    Info->bReady = true;

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: %s confirmed ready"),
        *Info->DisplayName);

    CheckReadyStatus();
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

    if (CurrentPhase != ETurnPhase::TurnActive)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantManager: TurnEnd submitted outside "
                 "TurnActive phase — ignored"));
        return;
    }

    const int32 SubmitterIndex = FindParticipantIndex(Controller);
    if (SubmitterIndex != ActiveParticipantIndex)
    {
        UE_LOG(LogTemp, Warning,
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

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: %s disconnected"),
        *Info->DisplayName);

    // Only pause if it is this participant's turn
    if (DisconnectedIndex == ActiveParticipantIndex
        && CurrentPhase == ETurnPhase::TurnActive)
    {
        GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        DisconnectedParticipantIndex = DisconnectedIndex;
        SetPhase(ETurnPhase::Paused);

        GetWorld()->GetTimerManager().SetTimer(
            ReconnectTimerHandle,
            this,
            &UTurnBasedParticipantManagerComponent::HandleReconnectTimeout,
            ReconnectTimeout,
            false
        );

        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedParticipantManager: Paused — waiting %.1fs for reconnect"),
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

    if (ReconnectedIndex == DisconnectedParticipantIndex
        && CurrentPhase == ETurnPhase::Paused)
    {
        GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
        DisconnectedParticipantIndex = -1;

        SetPhase(ETurnPhase::TurnActive);

        // Notify the reconnected participant to resume
        if (UTurnBasedParticipantComponent* Comp = GetParticipantComponent(Controller))
        {
            Comp->ClientNotifyTurnResumed(
                BuildNotification(ReconnectedIndex, ETurnPhase::TurnActive));
        }

        // Restart full turn duration — acceptable for turn based
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
            TEXT("TurnBasedParticipantManager: StartTurn — invalid index %d"),
            ParticipantIndex);
        return;
    }

    ActiveParticipantIndex  = ParticipantIndex;
    ReplicatedTurnDuration  = TurnDuration;
    TurnNumber++;

    // Broadcast to all participants before notifying active one
    // Each participant component ticks cooldowns and fires its own delegate
    BroadcastTurnStartToAllParticipants(ParticipantIndex);

    SetPhase(ETurnPhase::TurnStart);
    NotifyActiveParticipant(ETurnPhase::TurnStart);

    GetWorld()->GetTimerManager().SetTimer(
        TurnTimerHandle,
        this,
        &UTurnBasedParticipantManagerComponent::HandleTurnTimeout,
        TurnDuration,
        false
    );

    SetPhase(ETurnPhase::TurnActive);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Turn %d started for %s"),
        TurnNumber,
        *Participants[ParticipantIndex].DisplayName);
}

void UTurnBasedParticipantManagerComponent::EndTurn(ETurnEndReason Reason)
{
    check(!IsRunningClientOnly());

    GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
    SetPhase(ETurnPhase::TurnEnd);
    NotifyActiveParticipant(ETurnPhase::TurnEnd, Reason);

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Turn %d ended — %s"),
        TurnNumber, *UEnum::GetValueAsString(Reason));

    // Enter resolution phase
    // External systems bind to OnTurnResolutionStarted for
    // cinematics, scoring visuals, dialogue etc.
    SetPhase(ETurnPhase::TurnResolution);
    OnTurnResolutionStarted.Broadcast();

    // Auto-advance after resolution duration
    // External systems that need longer can bind and call
    // AdvanceToNextParticipant directly if needed in future
    GetWorld()->GetTimerManager().SetTimer(
        ResolutionTimerHandle,
        this,
        &UTurnBasedParticipantManagerComponent::HandleResolutionComplete,
        TurnResolutionDuration,
        false
    );
}

void UTurnBasedParticipantManagerComponent::HandleResolutionComplete()
{
    AdvanceToNextParticipant();
}

void UTurnBasedParticipantManagerComponent::AdvanceToNextParticipant()
{
    check(!IsRunningClientOnly());
    check(IsValid(TurnOrderStrategy));
    check(TurnOrderStrategy->Implements<UTurnOrderInterface>());

    const int32 NextIndex =
        ITurnOrderInterface::Execute_GetNextParticipantIndex(
            TurnOrderStrategy,
            Participants,
            ActiveParticipantIndex
        );

    if (NextIndex == -1)
    {
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

    SetPhase(ETurnPhase::TurnTimeout);

    if (!Participants.IsValidIndex(ActiveParticipantIndex)) return;

    FTurnParticipantInfo& Info = Participants[ActiveParticipantIndex];
    Info.TurnsMissed++;

    NotifyActiveParticipant(ETurnPhase::TurnTimeout, ETurnEndReason::TimedOut);

    if (AController* Controller = Info.Controller.Get())
    {
        if (ATurnBasedPlayerState* PS =
            Controller->GetPlayerState<ATurnBasedPlayerState>())
        {
            PS->Server_IncrementTurnsMissed();

            if (Info.TurnsMissed >= ForfeitThreshold)
            {
                Info.bForfeited = true;
                PS->Server_SetForfeited();

                NotifyActiveParticipant(
                    ETurnPhase::TurnEnd, ETurnEndReason::Forfeited);
                OnParticipantForfeited.Broadcast(Info);

                UE_LOG(LogTemp, Log,
                    TEXT("TurnBasedParticipantManager: %s forfeited "
                         "after %d missed turns"),
                    *Info.DisplayName, Info.TurnsMissed);
            }
        }
    }

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

    const bool bAllReady = !Participants.FindByPredicate(
        [](const FTurnParticipantInfo& Info){ return !Info.bReady; });

    if (bAllReady && !Participants.IsEmpty())
    {
        UE_LOG(LogTemp, Log,
            TEXT("TurnBasedParticipantManager: All participants ready"));

        OnAllParticipantsReady.Broadcast();

        check(IsValid(TurnOrderStrategy));
        check(TurnOrderStrategy->Implements<UTurnOrderInterface>());

        const int32 FirstIndex =
            ITurnOrderInterface::Execute_GetFirstParticipantIndex(
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

void UTurnBasedParticipantManagerComponent::BroadcastTurnStartToAllParticipants(
    int32 NewActiveIndex)
{
    for (int32 i = 0; i < Participants.Num(); i++)
    {
        const FTurnParticipantInfo& Info = Participants[i];
        if (!IsValid(Info.Controller)) continue;

        UTurnBasedParticipantComponent* ParticipantComp =
            GetParticipantComponent(Info.Controller.Get());
        if (!IsValid(ParticipantComp)) continue;

        const bool bIsMyTurn = (i == NewActiveIndex);

        // Participant component fires OnAnyParticipantTurnStarted(bIsMyTurn)
        // Action component binds to this for cooldown ticking
        ParticipantComp->HandleAnyParticipantTurnStarted(bIsMyTurn);
    }
}

// --- Helpers ---

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

void UTurnBasedParticipantManagerComponent::NotifyActiveParticipant(
    const ETurnPhase Phase, const ETurnEndReason Reason)
{
    if (!Participants.IsValidIndex(ActiveParticipantIndex)) return;

    const FTurnParticipantInfo& Info = Participants[ActiveParticipantIndex];
    UTurnBasedParticipantComponent* Comp =
        GetParticipantComponent(Info.Controller.Get());
    if (!IsValid(Comp)) return;

    const FTurnNotification Notification =
        BuildNotification(ActiveParticipantIndex, Phase, Reason);

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
    const int32 ParticipantIndex, const ETurnPhase Phase, const ETurnEndReason Reason) const
{
    FTurnNotification Notification;

    if (Participants.IsValidIndex(ParticipantIndex))
        Notification.ParticipantInfo = Participants[ParticipantIndex];

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
    // Clients react to active participant changing here
    // e.g. highlight active player in UI
}