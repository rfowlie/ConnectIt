// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_PlacePieceAction.h"
#include "Pooling/ActorPool.h"
#include "EngineUtils.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/BoardStateComponent.h"
#include "Framework/Manager/ConnectIt_BoardManager.h"
#include "Grid/ConnectIt_GridPiece.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "Subsystem/GridWorldSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryComponent.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UConnectIt_PlacePieceAction::UConnectIt_PlacePieceAction()
{
    // Config defaults — these can be overridden in the DataAsset
    bIsRequired       = true;
    bIsCancellable    = true; // Cancellable by shard/power activation
    bRequiresSelection = true;
    MaxUsesPerTurn    = 1;
    CooldownTurns     = 0;
    ActionTag         = FGameplayTag::RequestGameplayTag("ConnectIt.Action.PlacePiece");
}

void UConnectIt_PlacePieceAction::OnActivated_Implementation()
{
    BoardManager = FindBoardActor();

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("PlacePieceAction: No AConnectItBoardActor found in world"));
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Activated — awaiting tile selection"));
}

void UConnectIt_PlacePieceAction::OnCancelled_Implementation()
{
    // If a piece was somehow placed before cancel — return it to pool
    // This should not happen since placement triggers completion
    // but guard defensively
    if (IsValid(PlacedPiece) && IsValid(BoardManager))
    {
        if (UActorPool* Pool = BoardManager->GetPiecePool())
        {
            Pool->ReleaseObject(PlacedPiece);
            PlacedPiece = nullptr;
        }
    }

    BoardManager = nullptr;

    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Cancelled — awaiting reactivation"));
}

void UConnectIt_PlacePieceAction::OnCompleted_Implementation()
{
    BoardManager = nullptr;

    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Piece placed — action complete"));
}

bool UConnectIt_PlacePieceAction::IsValidHoverTile_Implementation(
    AGridTileBase* Tile) const
{
    if (!IsValid(Tile) || !IsValid(BoardManager)) return false;
    
    const UGridTileRegistryComponent* TileRegistry = BoardManager->GetTileRegistry();
    if (!IsValid(TileRegistry)) return false;

    const FGridPosition TilePos = TileRegistry->GetPositionOfTile(Tile);

    // Tile must exist in registry
    if (!TileRegistry->GetTileAtPosition(TilePos)) return false;

    const UGridPieceRegistryComponent* PieceRegistry = BoardManager->GetPieceRegistry();
    if (!IsValid(PieceRegistry)) return false;
    
    // Piece must not be assigned to this position
    if (PieceRegistry->GetPiece(TilePos)) return false;

    // Additional mutation checks go here as ConnectIt grows
    // e.g. The Rift — chasm tiles are never valid
    // Check tile tags or board state for mutation-specific rules

    return true;
}

bool UConnectIt_PlacePieceAction::IsValidSelectionTile_Implementation(
    AGridTileBase* Tile) const
{
    // Selection validity matches hover validity for piece placement
    return IsValidHoverTile_Implementation(Tile);
}

void UConnectIt_PlacePieceAction::HandleValidHover_Implementation(AGridTileBase* Tile)
{
    if (!IsValid(Tile)) return;

    // Communicate valid hover state to tile via GameplayTag
    // AGridTileBase Blueprint subclass responds visually
    Tile->SendGameplayTag(ConnectIt_Tile_ValidHover);
}

void UConnectIt_PlacePieceAction::HandleHoverCleared_Implementation(
    AGridTileBase* PreviousTile)
{
    if (!IsValid(PreviousTile)) return;

    // Return tile to default visual state
    PreviousTile->SendGameplayTag(ConnectIt_Tile_Default);
}

void UConnectIt_PlacePieceAction::HandleValidSelection_Implementation(AGridTileBase* Tile)
{
    if (!IsValid(Tile) || !IsValid(BoardManager)) return;

    const int32 FactionID = GetOwningFactionID();

    // Send placement to server for validation and application
    ServerPlacePiece(Tile, FactionID);
}

void UConnectIt_PlacePieceAction::ServerPlacePiece_Implementation(
    AGridTileBase* TargetTile,
    int32 FactionID)
{
    // Re-validate on server — client validation is convenience only
    if (!IsValidSelectionTile(TargetTile))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("PlacePieceAction: Server rejected placement — tile invalid"));
        return;
    }

    UActorPool* Pool = BoardManager->GetPiecePool();
    if (!IsValid(Pool))
    {
        UE_LOG(LogTemp, Error,
            TEXT("PlacePieceAction: Piece pool not available"));
        return;
    }

    // Get piece from pool
    TArray<AActor*> PooledActors = Pool->GetObjects(1);
    if (PooledActors.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("PlacePieceAction: Pool returned no actors"));
        return;
    }

    AConnectIt_GridPiece* Piece = Cast<AConnectIt_GridPiece>(PooledActors[0]);
    if (!IsValid(Piece))
    {
        UE_LOG(LogTemp, Error,
            TEXT("PlacePieceAction: Pooled actor is not AConnectItGridPiece"));
        return;
    }

    // Position piece at tile world location
    Piece->SetActorLocation(TargetTile->GetActorLocation());
    
    const UGridTileRegistryComponent* TileRegistry = BoardManager->GetTileRegistry();
    if (!IsValid(TileRegistry)) return;
    
    // Initialise faction — triggers OnRep_FactionID on clients
    const FGridPosition TilePos = TileRegistry->GetPositionOfTile(TargetTile);
    Piece->InitialisePiece(FactionID, TilePos);
    PlacedPiece = Piece;

    // Update board state immediately on server
    // UBoardStateComponent replication drives client visual updates
    UBoardStateComponent* BoardState = BoardManager->GetBoardState();
    if (IsValid(BoardState))
    {
        FTileData TileData = BoardState->GetTileData(TilePos);
        TileData.FactionPiece = FactionID;
        BoardState->SetTileData(TilePos, TileData);
    }

    // Notify tile it is now occupied — drives tile visual state
    TargetTile->SendGameplayTag(ConnectIt_Tile_Occupied);

    UE_LOG(LogTemp, Log,
        TEXT("PlacePieceAction: Piece placed at (%d,%d) for faction %d"),
        TilePos.X, TilePos.Y, FactionID);

    // Complete the action — triggers turn end check in action component
    Complete();
}

void UConnectIt_PlacePieceAction::ClearVisuals_Implementation()
{
    // Clear hover state on currently hovered tile
    if (AGridTileBase* Hovered = CurrentHoveredTile.Get())
    {
        Hovered->SendGameplayTag(ConnectIt_Tile_Default);
    }
}

AConnectIt_BoardManager* UConnectIt_PlacePieceAction::FindBoardActor() const
{
    for (TActorIterator<AConnectIt_BoardManager> It(GetWorld()); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

int32 UConnectIt_PlacePieceAction::GetOwningFactionID() const
{
    if (!IsValid(OwningController)) return -1;
    
    UTurnBasedParticipantComponent* ParticipantComp =
        OwningController->FindComponentByClass<UTurnBasedParticipantComponent>();
    
    if (!IsValid(ParticipantComp)) return -1;
    
    // SlotIndex is the faction ID in a 1v1 game
    return ParticipantComp->PlayerId;
}
