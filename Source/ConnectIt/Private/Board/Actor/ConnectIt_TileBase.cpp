// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Actor/ConnectIt_TileBase.h"
#include "Board/ConnectIt_BoardVisualizer.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Tile/GridTileRegistryBase.h"


// Defaults: nothing animates, so nothing holds the handle
void AConnectIt_TileBase::OnTilePowerTriggered_Implementation(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_TileBase::OnPieceEnter_Implementation(int32 Faction, FGameplayTag Cause, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_TileBase::OnPieceExit_Implementation(int32 Faction, FGameplayTag Cause, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_TileBase::OnMultiplierChanged_Implementation(float NewMultiplier, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_TileBase::OnLockChanged_Implementation(bool bLocked, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_TileBase::OnActiveChanged_Implementation(bool bActive, UConnectIt_VisualHandle* Handle) {}
void AConnectIt_TileBase::OnChainPartnerChanged_Implementation(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_VisualHandle* Handle) {}

void AConnectIt_TileBase::ReleaseVisualHolds()
{
    if (AConnectIt_BoardVisualizer* Visualizer = AConnectIt_BoardVisualizer::GetBoardVisualizer(this))
    {
        Visualizer->ReleaseHoldsBy(this);
    }
}

void AConnectIt_TileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseVisualHolds();
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
