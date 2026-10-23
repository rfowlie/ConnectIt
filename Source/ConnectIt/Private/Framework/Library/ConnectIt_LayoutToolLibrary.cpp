// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Library/ConnectIt_LayoutToolLibrary.h"

#if WITH_EDITOR
#include "Framework/Data/ConnectIt_BoardLayout.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Board/ConnectIt_TileDataProvider.h"
#include "Board/ConnectIt_TileBase.h"
#include "Registry/GridDefinition.h"
#include "Tile/GridTileBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"


namespace
{
    // The sandbox level's board: its grid (for positions) and default tile class
    const UConnectIt_LevelConfigDataAsset* FindToolBoard(const UObject* WorldContextObject, const TCHAR* Caller)
    {
        const UConnectIt_LevelConfigDataAsset* LevelConfig = AConnectIt_WorldSettings::FindLevelConfig(WorldContextObject);
        if (!LevelConfig || !IsValid(LevelConfig->GridDefinition))
        {
            UE_LOG(LogTemp, Error,
                TEXT("ConnectIt_LayoutTool: %s -- this level's World Settings name no Level Config with a Grid "
                     "Definition, so tile positions can't be worked out. Set one on the sandbox map."), Caller);
            return nullptr;
        }
        return LevelConfig;
    }

    UWorld* GetToolWorld(const UObject* WorldContextObject)
    {
        return GEngine && IsValid(WorldContextObject)
            ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
            : nullptr;
    }
}

int32 UConnectIt_LayoutToolLibrary::CaptureLayoutFromLevel(const UObject* WorldContextObject, UConnectIt_BoardLayout* Layout)
{
    UWorld* World = GetToolWorld(WorldContextObject);
    const UConnectIt_LevelConfigDataAsset* Board = FindToolBoard(WorldContextObject, TEXT("CaptureLayoutFromLevel"));
    if (!World || !Board || !IsValid(Layout)) return -1;

    // The looks already chosen, kept by position
    TMap<FGridPosition, FGameplayTag> Variants;
    for (const FConnectItBoardLayoutCell& Cell : Layout->Cells)
    {
        if (Cell.VisualVariant.IsValid()) Variants.Add(Cell.Position, Cell.VisualVariant);
    }

    TArray<FConnectItBoardLayoutCell> Cells;
    TSet<FGridPosition> Seen;
    for (TActorIterator<AGridTileBase> It(World); It; ++It)
    {
        AGridTileBase* Tile = *It;
        if (!IsValid(Tile)) continue;

        FConnectItBoardLayoutCell Cell;
        Cell.Position = Board->GridDefinition->WorldToGridPosition(Tile->GetActorLocation());

        bool bSeen = false;
        Seen.Add(Cell.Position, &bSeen);
        if (bSeen)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_LayoutTool: two tiles at (%d,%d) -- '%s' skipped"),
                Cell.Position.X, Cell.Position.Y, *Tile->GetActorLabel());
            continue;
        }

        if (Tile->GetClass() != Board->DefaultTileClass.Get())
        {
            Cell.TileClass = Tile->GetClass();
        }
        if (Tile->Implements<UConnectIt_TileDataProvider>())
        {
            Cell.TileData = IConnectIt_TileDataProvider::Execute_GetInitialTileData(Tile);
        }
        // The power with this placed tile's values
        if (const AConnectIt_TileBase* PoweredTile = Cast<AConnectIt_TileBase>(Tile))
        {
            FString Problem;
            Cell.TilePower = PoweredTile->MakeBoardPower(&Problem);
            if (!Problem.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("ConnectIt_LayoutTool: '%s' at (%d,%d): %s"),
                    *Tile->GetActorLabel(), Cell.Position.X, Cell.Position.Y, *Problem);
            }
        }
        if (const FGameplayTag* Variant = Variants.Find(Cell.Position))
        {
            Cell.VisualVariant = *Variant;
        }
        Cells.Add(Cell);
    }

    Cells.Sort([](const FConnectItBoardLayoutCell& A, const FConnectItBoardLayoutCell& B)
    {
        return A.Position.Y != B.Position.Y ? A.Position.Y < B.Position.Y : A.Position.X < B.Position.X;
    });

    Layout->Modify();
    Layout->Cells = MoveTemp(Cells);
    Layout->MarkPackageDirty();

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_LayoutTool: captured %d tile(s) into '%s'"),
        Layout->Cells.Num(), *Layout->GetName());
    return Layout->Cells.Num();
}

int32 UConnectIt_LayoutToolLibrary::LoadLayoutIntoLevel(
    const UObject* WorldContextObject, UConnectIt_BoardLayout* Layout, bool bClearLevelTiles)
{
    UWorld* World = GetToolWorld(WorldContextObject);
    const UConnectIt_LevelConfigDataAsset* Board = FindToolBoard(WorldContextObject, TEXT("LoadLayoutIntoLevel"));
    if (!World || !Board || !IsValid(Layout)) return 0;

    if (bClearLevelTiles)
    {
        TArray<AGridTileBase*> Existing;
        for (TActorIterator<AGridTileBase> It(World); It; ++It)
        {
            Existing.Add(*It);
        }
        for (AGridTileBase* Tile : Existing)
        {
            World->DestroyActor(Tile);
        }
    }

    int32 Placed = 0;
    for (const FConnectItBoardLayoutCell& Cell : Layout->Cells)
    {
        const TSubclassOf<AGridTileBase> TileClass = Cell.TileClass ? Cell.TileClass : Board->DefaultTileClass;
        if (!TileClass)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_LayoutTool: (%d,%d) names no Tile Class and the board has no Default Tile Class -- "
                     "not placed"), Cell.Position.X, Cell.Position.Y);
            continue;
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AGridTileBase* Tile = World->SpawnActor<AGridTileBase>(
            TileClass, FTransform(Board->GridDefinition->GridPositionToWorld(Cell.Position)), Params);
        if (!IsValid(Tile)) continue;

        Tile->SetActorLabel(FString::Printf(TEXT("%s_%d_%d"), *Layout->GetName(), Cell.Position.X, Cell.Position.Y));
        if (Tile->Implements<UConnectIt_TileDataProvider>())
        {
            IConnectIt_TileDataProvider::Execute_ApplyLayoutCell(Tile, Cell.TileData, Cell.VisualVariant);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_LayoutTool: '%s' doesn't implement the tile data interface -- (%d,%d)'s starting "
                     "values aren't shown on it"), *TileClass->GetName(), Cell.Position.X, Cell.Position.Y);
        }
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_LayoutTool: placed %d of %d tile(s) from '%s'"),
        Placed, Layout->Cells.Num(), *Layout->GetName());
    return Placed;
}

void UConnectIt_LayoutToolLibrary::SetLayoutCamera(UConnectIt_BoardLayout* Layout, bool bHasCamera, const FTransform& CameraTransform)
{
    if (!IsValid(Layout)) return;

    Layout->Modify();
    Layout->bHasCamera = bHasCamera;
    Layout->CameraTransform = CameraTransform;
    Layout->MarkPackageDirty();
}
#endif
