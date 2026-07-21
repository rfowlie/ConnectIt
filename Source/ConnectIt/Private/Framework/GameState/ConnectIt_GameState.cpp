// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/GameState/ConnectIt_GameState.h"
#include "Net/UnrealNetwork.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


AConnectIt_GameState::AConnectIt_GameState()
{
    bReplicates = true;
}

void AConnectIt_GameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AConnectIt_GameState, MatchResult);
}

// --- Server API ---

void AConnectIt_GameState::SetMatchResult(
    int32 WinningFactionSlot,
    EMatchEndReason EndReason,
    int32 TotalTurns)
{
    check(GetOwner() == nullptr || HasAuthority());

    UConnectIt_BoardStateComponent* BSC = GetBoardStateComponent();
    check(BSC != nullptr);
    
    // Read final scores from board state at time of game over
    const FConnectItBoardState* Previous = BSC->GetBoardSnapShotPrevious();
    const FConnectItBoardState* Current  = BSC->GetBoardSnapShotCurrent();

    MatchResult.WinningFactionSlot = WinningFactionSlot;
    MatchResult.EndReason          = EndReason;
    MatchResult.TotalTurnsPlayed   = TotalTurns;
    MatchResult.bMatchOver         = true;

    if (Current)
    {
        MatchResult.FinalScores = Current->ScoreBoard;
    }

    // Fire on server immediately
    // Clients receive via OnRep_MatchResult
    OnMatchResultUpdated.Broadcast(MatchResult);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameState: Match result set — "
             "faction %d wins, reason: %s, turns: %d"),
        WinningFactionSlot,
        *UEnum::GetValueAsString(EndReason),
        TotalTurns);
}

// --- Board State Accessors ---

float AConnectIt_GameState::GetFactionScore(int32 FactionSlot) const
{
    UConnectIt_BoardStateComponent* BSC = GetBoardStateComponent();
    check(BSC != nullptr);
    
    // Read final scores from board state at time of game over

    if (const FConnectItBoardState* Current  = BSC->GetBoardSnapShotCurrent())
    {
        return Current->GetScore(FactionSlot);
    } 

    return 0.f;
}

TArray<float> AConnectIt_GameState::GetAllScores() const
{
    UConnectIt_BoardStateComponent* BSC = GetBoardStateComponent();
    check(BSC != nullptr);

    if (const FConnectItBoardState* Current = BSC->GetBoardSnapShotCurrent())
    {
        return Current->ScoreBoard;
    }

    return TArray<float>();
}

bool AConnectIt_GameState::IsTileOccupied(FGridPosition Position) const
{
    if (UConnectIt_BoardStateComponent* BSC = GetBoardStateComponent())
    {
        return BSC->GetCurrentState().IsTileOccupied(Position);
    }
    return false;
}

bool AConnectIt_GameState::IsTileValidForPlacement(
    FGridPosition Position) const
{
    if (UConnectIt_BoardStateComponent* BSC = GetBoardStateComponent())
    {
        return BSC->IsTileValidForPlacement(Position);
    }
    return false;
}

FConnectItBoardStateSnapshot AConnectIt_GameState::GetBoardSnapshot() const
{
    if (const UConnectIt_BoardStateComponent* BSC = GetBoardStateComponent())
    {
        return *BSC->GetBoardSnapshot();
    }
    return FConnectItBoardStateSnapshot();
}

// --- Turn Accessors ---

FString AConnectIt_GameState::GetActiveParticipantName() const
{
    if (!IsValid(ParticipantManager)) return TEXT("Unknown");

    const TArray<FTurnParticipantInfo>& Participants =
        ParticipantManager->Participants;

    const int32 ActiveIndex = ParticipantManager->ActiveParticipantIndex;

    if (!Participants.IsValidIndex(ActiveIndex))
        return TEXT("Unknown");

    return Participants[ActiveIndex].GetDisplayName();
}

bool AConnectIt_GameState::IsLocalPlayerTurn() const
{
    if (!IsValid(ParticipantManager)) return false;

    // Find the local player controller and check its slot
    APlayerController* LocalPC =
        GetWorld()->GetFirstPlayerController();
    if (!IsValid(LocalPC)) return false;

    return ParticipantManager->GetControllerAtIndex(
        ParticipantManager->ActiveParticipantIndex) == LocalPC;
}

// --- RepNotify ---

void AConnectIt_GameState::OnRep_MatchResult()
{
    OnMatchResultUpdated.Broadcast(MatchResult);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameState: Match result replicated — "
             "faction %d wins"),
        MatchResult.WinningFactionSlot);
}

// --- Helpers ---

UConnectIt_BoardStateComponent* AConnectIt_GameState::GetBoardStateComponent() const
{
    if (!IsValid(CachedBoardStateComponent))
    {
        CachedBoardStateComponent = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    }

    return CachedBoardStateComponent;
}