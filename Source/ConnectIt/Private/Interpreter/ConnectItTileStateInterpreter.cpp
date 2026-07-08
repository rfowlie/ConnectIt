// Fill out your copyright notice in the Description page of Project Settings.


#include "Interpreter/ConnectItTileStateInterpreter.h"
#include "Board/ConnectItBoardStateComponent.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"


void UConnectItTileStateInterpreter::OnBoardStateChanged_Implementation(
    const UBoardStateComponentBase* Component)
{
    const UConnectItBoardStateComponent* ConnectItComp =
        Cast<UConnectItBoardStateComponent>(Component);

    if (!IsValid(ConnectItComp)) return;

    const FConnectItBoardStateSnapshot Snapshot = ConnectItComp->GetBoardSnapshot();
    const FConnectItBoardState Previous = Snapshot.PreviousState;
    const FConnectItBoardState Current = Snapshot.PreviousState;

    // Only process positions that actually changed
    for (int32 i = 0; i < Current.NumTiles(); i++)
    {
        const FGridPosition Position    = Current.GetPositionAt(i);
        const FConnectItTileData& CurrentTileData = Current.GetTileDataAt(i);
        const FConnectItTileData* PreviousTileData = Previous.GetTileData(Position);

        // Always process on first update (no previous data)
        // or when tile data has changed
        const bool bFirstUpdate = (PreviousTileData == nullptr);
        const bool bOccupationChanged = PreviousTileData
            && PreviousTileData->FactionPiece != CurrentTileData.FactionPiece;
        const bool bActiveChanged = PreviousTileData
            && PreviousTileData->bIsActive != CurrentTileData.bIsActive;
        const bool bMultiplierChanged = PreviousTileData
            && !FMath::IsNearlyEqual(
                PreviousTileData->Multiplier,
                CurrentTileData.Multiplier);

        if (bFirstUpdate || bOccupationChanged
            || bActiveChanged || bMultiplierChanged)
        {
            ProcessTileChange(
                Position,
                PreviousTileData ? *PreviousTileData : FConnectItTileData(),
                CurrentTileData);
        }
    }
}

void UConnectItTileStateInterpreter::ProcessTileChange(
    const FGridPosition& Position,
    const FConnectItTileData& PreviousData,
    const FConnectItTileData& CurrentData)
{
    // Active state changed
    if (!CurrentData.bIsActive)
    {
        SendTagToTile(Position, Tag_TileInactive);
        return;
    }

    // Occupation changed
    if (CurrentData.FactionPiece != PreviousData.FactionPiece)
    {
        if (CurrentData.IsOccupied())
        {
            SendTagToTile(Position, Tag_TileOccupied,
                CurrentData.FactionPiece);
        }
        else
        {
            SendTagToTile(Position, Tag_TileEmpty);
        }
    }

    // Multiplier changed -- notify separately so tile can show feedback
    if (!FMath::IsNearlyEqual(
        PreviousData.Multiplier, CurrentData.Multiplier))
    {
        SendTagToTile(Position, Tag_MultiplierChanged);
    }
}

void UConnectItTileStateInterpreter::SendTagToTile(
    const FGridPosition& Position,
    FGameplayTag Tag,
    int32 FactionPiece)
{
    if (!Tag.IsValid()) return;

    AGridTileBase* Tile = FindTileActor(Position);
    if (!IsValid(Tile)) return;

    Tile->SendGameplayTag(Tag);
}

AGridTileBase* UConnectItTileStateInterpreter::FindTileActor(
    const FGridPosition& Position) const
{
    UWorld* World = GetWorld();
    if (!IsValid(World)) return nullptr;

    UGridWorldSubsystem* GridSub =
        World->GetSubsystem<UGridWorldSubsystem>();
    if (!IsValid(GridSub)) return nullptr;

    // TODO: removed the mapping from subsystem and placed on different component
    // return GridSub->GetTileAtPosition(Position);
    return nullptr;
}

