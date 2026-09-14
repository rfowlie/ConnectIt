// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/PlayerState/ConnectIt_PlayerState.h"

#include "Net/UnrealNetwork.h"


void AConnectIt_PlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AConnectIt_PlayerState, SwapUsesRemaining);
}

void AConnectIt_PlayerState::ConsumeSwapUse()
{
    if (!HasAuthority()) return;
    if (SwapUsesRemaining <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_PlayerState: TryConsumeSwapUse — "
                 "%s has no SWAP uses remaining"),
            *GetPlayerName());
        return;
    }

    SwapUsesRemaining--;

    // exact same thing called in OnRep_SwapUsesRemaining(); but this is for server
    BroadcastSwapUsesRemaining();

}

void AConnectIt_PlayerState::OnRep_SwapUsesRemaining()
{
    BroadcastSwapUsesRemaining();
}

void AConnectIt_PlayerState::BroadcastSwapUsesRemaining()
{
    OnSwapUsesRemainingChanged.Broadcast(SwapUsesRemaining);

    UE_LOG(LogTemp, Log,
           TEXT("ConnectIt_PlayerState: %s used SWAP — %d use(s) remaining"),
           *GetPlayerName(), SwapUsesRemaining);
}