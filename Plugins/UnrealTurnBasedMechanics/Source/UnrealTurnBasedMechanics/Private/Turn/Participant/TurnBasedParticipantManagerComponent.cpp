// Fill out your copyright notice in the Description page of Project Settings.


#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "UnrealTurnBasedMechanics.h"
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
    if (!TurnOrderStrategy.GetObject())
    {
        TurnOrderStrategy = NewObject<USequentialTurnOrderStrategy>(
            this, USequentialTurnOrderStrategy::StaticClass());

        UE_LOG(LogTurnBasedMechanics, Log,
            TEXT("TurnBasedParticipantManager: No strategy set "
                 "— defaulting to Sequential"));
    }
}

AController* UTurnBasedParticipantManagerComponent::GetControllerAtIndex(int32 Index) const
{
    if (!ServerControllers.IsValidIndex(Index)) return nullptr;
    return ServerControllers[Index].Get();
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
    const EParticipantType Type,
    const FString& DisplayName)
{
    check(!IsRunningClientOnly());
    if (!IsValid(Controller)) return;

    // Ensure PlayerState exists -- AI needs InitPlayerState called
    ATurnBasedPlayerState* PS =
        Controller->GetPlayerState<ATurnBasedPlayerState>();

    if (!IsValid(PS))
    {
        UE_LOG(LogTemp, Error,
            TEXT("TurnBasedParticipantManager: %s has no "
                 "ATurnBasedPlayerState — cannot register. "
                 "AI controllers must call InitPlayerState() before "
                 "registering."),
            *Controller->GetName());
        return;
    }

    // Warn on duplicate
    if (IsParticipantRegistered(Controller))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TurnBasedParticipantManager: %s already registered"),
            *Controller->GetName());
        return;
    }

    const int32 NewSlotIndex = Participants.Num();

    // Write per-player state to PlayerState
    PS->SetSlotIndex(NewSlotIndex);
    PS->SetParticipantType(Type);
    PS->SetReady(Type == EParticipantType::AI);  // AI auto-ready

    // Build replicated info
    FTurnParticipantInfo Info;
    Info.PlayerState     = PS;
    Info.ParticipantType = Type;
    Info.SlotIndex       = NewSlotIndex;
    Info.bConnected      = true;

    Participants.Add(Info);

    // Server-only controller reference -- index matched
    ServerControllers.Add(Controller);

    // Set slot on participant component
    if (UTurnBasedParticipantComponent* Comp = GetParticipantComponent(Controller))
    {
        Comp->CachedSlotIndex = NewSlotIndex;
        Comp->ParticipantType = Type;
    }

    UE_LOG(LogTemp, Log,
        TEXT("TurnBasedParticipantManager: Registered %s '%s' at slot %d"),
        Type == EParticipantType::AI ? TEXT("AI") : TEXT("Human"),
        *PS->GetPlayerName(),
        NewSlotIndex);
}

bool UTurnBasedParticipantManagerComponent::IsParticipantRegistered(AController* Controller) const
{
    return FindParticipantIndex(Controller) != INDEX_NONE;
}

void UTurnBasedParticipantManagerComponent::NotifyParticipantReady(
    AController* Controller)
{
    check(!IsRunningClientOnly());

    const FTurnParticipantInfo* Info = FindParticipant(Controller);
    if (!Info) return;

    Info->PlayerState->SetReady(true);

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: %s confirmed ready"),
        *Info->GetDisplayName());

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

    // Only pause if it is this participant's turn
    if (DisconnectedIndex == ActiveParticipantIndex
        && CurrentPhase == ETurnPhase::TurnActive)
    {
        GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
        DisconnectedParticipantIndex = DisconnectedIndex;
        SetPhase(ETurnPhase::TurnPaused);

        GetWorld()->GetTimerManager().SetTimer(
            ReconnectTimerHandle,
            this,
            &UTurnBasedParticipantManagerComponent::HandleReconnectTimeout,
            ReconnectTimeout,
            false
        );

        UE_LOG(LogTurnBasedMechanics, Log,
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

    UE_LOG(LogTurnBasedMechanics, Log,
        TEXT("TurnBasedParticipantManager: %s reconnected"),
        *Info->GetDisplayName());

    if (ReconnectedIndex == DisconnectedParticipantIndex
        && CurrentPhase == ETurnPhase::TurnPaused)
    {
        GetWorld()->GetTimerManager().ClearTimer(ReconnectTimerHandle);
        DisconnectedParticipantIndex = -1;

        SetPhase(ETurnPhase::TurnActive);

        // Single RPC call -- old ClientNotifyTurnResumed removed
        if (UTurnBasedParticipantComponent* Comp =
            GetParticipantComponent(Controller))
        {
            Comp->ClientReceiveTurnNotification(
                BuildNotification(ReconnectedIndex, ETurnPhase::TurnActive));
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
        TEXT("TurnBasedParticipantManager: Phase -> %s"),
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
    TurnNumber++;

    SetPhase(ETurnPhase::TurnStart);

    // Broadcast to all participants
    // Active participant receives ClientReceiveTurnNotification
    // All others receive ReceiveOpponentTurnStarted
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
    SetPhase(ETurnPhase::TurnEnd);
    NotifyActiveParticipant(ETurnPhase::TurnEnd, Reason);

    UE_LOG(LogTurnBasedMechanics, Log,
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
    check(TurnOrderStrategy.GetInterface() != nullptr);

    const int32 NextIndex =
        ITurnOrderInterface::Execute_GetNextParticipantIndex(
            TurnOrderStrategy.GetObject(),
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

    const bool bAllReady = !Participants.FindByPredicate(
        [](const FTurnParticipantInfo& Info){ return !Info.PlayerState->IsReady(); });

    if (bAllReady && !Participants.IsEmpty())
    {
        UE_LOG(LogTurnBasedMechanics, Log,
            TEXT("TurnBasedParticipantManager: All participants ready"));

        OnAllParticipantsReady.Broadcast();

        check(TurnOrderStrategy.GetInterface() != nullptr);

        const int32 FirstIndex =
            ITurnOrderInterface::Execute_GetFirstParticipantIndex(
                TurnOrderStrategy.GetObject(), Participants);

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

void UTurnBasedParticipantManagerComponent::BroadcastTurnStart(int32 ActiveIndex)
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
            Comp->ReceiveOpponentTurnStarted(ActiveSlotIndex);
        }
    }

    // Fire active controller changed for board ownership
    OnActiveControllerChanged.Broadcast(GetControllerAtIndex(ActiveIndex));
}

void UTurnBasedParticipantManagerComponent::BroadcastControllerChanged(int32 ActiveIndex)
{
    OnActiveControllerChanged.Broadcast(GetControllerAtIndex(ActiveIndex));
}

// --- Helpers ---

FTurnParticipantInfo* UTurnBasedParticipantManagerComponent::FindParticipant(
    AController* Controller)
{
    const int32 Index = FindParticipantIndex(Controller);
    return Participants.IsValidIndex(Index) ? &Participants[Index] : nullptr;
}

int32 UTurnBasedParticipantManagerComponent::FindParticipantIndex(
    AController* Controller) const
{
    // Server only -- searches the controller array
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

    UTurnBasedParticipantComponent* Comp = GetParticipantComponent(GetControllerAtIndex(ActiveParticipantIndex));
    if (!IsValid(Comp)) return;

    Comp->ClientReceiveTurnNotification(BuildNotification(ActiveParticipantIndex, Phase, Reason));
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