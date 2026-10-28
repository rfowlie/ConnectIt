// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Actor/ConnectIt_TileBase.h"
#include "Board/ConnectIt_TileReaction.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Tile/GridTileRegistryBase.h"


namespace
{
    void FinishNow(UConnectIt_TileReaction* Reaction)
    {
        if (IsValid(Reaction)) Reaction->Finish();
    }
}

void AConnectIt_TileBase::OnTilePowerTriggered_Implementation(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_TileReaction* Reaction) { FinishNow(Reaction); }
void AConnectIt_TileBase::OnPieceEnter_Implementation(int32 Faction, FGameplayTag Cause, UConnectIt_TileReaction* Reaction) { FinishNow(Reaction); }
void AConnectIt_TileBase::OnPieceExit_Implementation(int32 Faction, FGameplayTag Cause, UConnectIt_TileReaction* Reaction) { FinishNow(Reaction); }
void AConnectIt_TileBase::OnMultiplierChanged_Implementation(float NewMultiplier, UConnectIt_TileReaction* Reaction) { FinishNow(Reaction); }
void AConnectIt_TileBase::OnLockChanged_Implementation(bool bLocked, UConnectIt_TileReaction* Reaction) { FinishNow(Reaction); }
void AConnectIt_TileBase::OnActiveChanged_Implementation(bool bActive, UConnectIt_TileReaction* Reaction) { FinishNow(Reaction); }
void AConnectIt_TileBase::OnChainPartnerChanged_Implementation(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_TileReaction* Reaction) { FinishNow(Reaction); }

void AConnectIt_TileBase::HoldReaction(UConnectIt_TileReaction* Reaction)
{
    if (!IsValid(Reaction)) return;
    Reaction->Tile = this;
    PendingReactions.AddUnique(Reaction);
}

void AConnectIt_TileBase::ForgetReaction(UConnectIt_TileReaction* Reaction)
{
    PendingReactions.Remove(Reaction);
}

void AConnectIt_TileBase::FinishPendingReactions()
{
    // Finishing removes each from the list: work on a copy
    const TArray<TObjectPtr<UConnectIt_TileReaction>> Pending = PendingReactions;
    for (UConnectIt_TileReaction* Reaction : Pending)
    {
        FinishNow(Reaction);
    }
    PendingReactions.Reset();
}

void AConnectIt_TileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FinishPendingReactions();
    Super::EndPlay(EndPlayReason);
}

void AConnectIt_TileBase::RefreshFromBoard_Implementation()
{
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const UGridTileRegistryBase* Tiles = UConnectIt_GameUtilityLibrary::GetTileRegistry(this);
    if (!BoardState || !Tiles) return;

    if (const FConnectItTileData* Data = BoardState->GetDisplayBoard().GetTileData(Tiles->GetPositionOfTile(this)))
    {
        OnRefreshedFromBoard(*Data);
    }
}
