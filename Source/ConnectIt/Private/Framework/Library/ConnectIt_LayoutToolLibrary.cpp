// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Library/ConnectIt_LayoutToolLibrary.h"

#if WITH_EDITOR
#include "Framework/Data/ConnectIt_BoardLayout.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Board/ConnectIt_TileDataProvider.h"
#include "Board/ConnectIt_BuilderTile.h"
#include "Board/Powers/ConnectIt_TileCatalog.h"
#include "Framework/Data/ConnectIt_ProjectSettings.h"
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

    // Marks a catalog preview's actors (with RF_Transient) so they can be found again
    const FName PreviewActorTag(TEXT("ConnectIt.TileCatalogPreview"));

    bool IsPreviewActor(const AActor* Actor)
    {
        return Actor && Actor->HasAnyFlags(RF_Transient) && Actor->ActorHasTag(PreviewActorTag);
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
        if (!IsValid(Tile) || IsPreviewActor(Tile)) continue;

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

        if (Tile->Implements<UConnectIt_TileDataProvider>())
        {
            Cell.TileData = IConnectIt_TileDataProvider::Execute_GetInitialTileData(Tile);
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

    const UConnectIt_ProjectSettings* Settings = GetDefault<UConnectIt_ProjectSettings>();
    const TSubclassOf<AConnectIt_BuilderTile> BuilderClass = Settings->BuilderTileClass.IsNull()
        ? TSubclassOf<AConnectIt_BuilderTile>(AConnectIt_BuilderTile::StaticClass())
        : TSubclassOf<AConnectIt_BuilderTile>(Settings->BuilderTileClass.LoadSynchronous());
    if (!BuilderClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_LayoutTool: LoadLayoutIntoLevel -- Project Settings > ConnectIt > Builder Tile Class "
                 "doesn't load"));
        return 0;
    }

    if (bClearLevelTiles)
    {
        ClearTileCatalogPreview(WorldContextObject);
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
    TMap<FGridPosition, AConnectIt_BuilderTile*> PlacedAt;
    for (const FConnectItBoardLayoutCell& Cell : Layout->Cells)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AConnectIt_BuilderTile* Tile = World->SpawnActor<AConnectIt_BuilderTile>(
            BuilderClass, FTransform(Board->GridDefinition->GridPositionToWorld(Cell.Position)), Params);
        if (!IsValid(Tile)) continue;

        Tile->SetActorLabel(FString::Printf(TEXT("%s_%d_%d"), *Layout->GetName(), Cell.Position.X, Cell.Position.Y));
        IConnectIt_TileDataProvider::Execute_ApplyLayoutCell(Tile, Cell.TileData, Cell.VisualVariant);
        PlacedAt.Add(Cell.Position, Tile);
        Placed++;
    }

    // Chain partners back as pickers, once every tile is placed
    for (const TPair<FGridPosition, AConnectIt_BuilderTile*>& Pair : PlacedAt)
    {
        AConnectIt_BuilderTile* Tile = Pair.Value;
        if (Tile->TileData.bHasChainPartner)
        {
            if (AConnectIt_BuilderTile* const* Partner = PlacedAt.Find(Tile->TileData.ChainPartner))
            {
                Tile->ChainPartnerTile = *Partner;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_LayoutTool: placed %d of %d tile(s) from '%s'"),
        Placed, Layout->Cells.Num(), *Layout->GetName());
    return Placed;
}

int32 UConnectIt_LayoutToolLibrary::PreviewTileCatalog(const UObject* WorldContextObject, const UConnectIt_TileCatalog* Catalog)
{
    UWorld* World = GetToolWorld(WorldContextObject);
    if (!World || !IsValid(Catalog)) return 0;

    ClearTileCatalogPreview(WorldContextObject);

    int32 Previewed = 0;
    for (TActorIterator<AConnectIt_BuilderTile> It(World); It; ++It)
    {
        AConnectIt_BuilderTile* Builder = *It;
        if (!IsValid(Builder)) continue;

        const TSubclassOf<AGridTileBase> TileClass = Catalog->GetTileClassFor(Builder->TileData.TilePowerTag);
        if (!TileClass)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_LayoutTool: tile catalog '%s' has no tile class for '%s' -- '%s' stays as a builder "
                     "tile"), *Catalog->GetName(), *Builder->TileData.TilePowerTag.ToString(), *Builder->GetActorLabel());
            continue;
        }

        // Transient: never saved with the level
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.ObjectFlags |= RF_Transient;
        Params.bHideFromSceneOutliner = true;
        AGridTileBase* Preview = World->SpawnActor<AGridTileBase>(TileClass, Builder->GetActorTransform(), Params);
        if (!IsValid(Preview)) continue;

        Preview->Tags.Add(PreviewActorTag);
        if (Preview->Implements<UConnectIt_TileDataProvider>())
        {
            IConnectIt_TileDataProvider::Execute_ApplyLayoutCell(Preview, Builder->TileData, Builder->VisualVariant);
        }

        // The editor's temporary hide: not saved either
        Builder->SetIsTemporarilyHiddenInEditor(true);
        Previewed++;
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_LayoutTool: previewing %d builder tile(s) as tile catalog '%s'"),
        Previewed, *Catalog->GetName());
    return Previewed;
}

void UConnectIt_LayoutToolLibrary::ClearTileCatalogPreview(const UObject* WorldContextObject)
{
    UWorld* World = GetToolWorld(WorldContextObject);
    if (!World) return;

    TArray<AActor*> Previews;
    for (TActorIterator<AGridTileBase> It(World); It; ++It)
    {
        if (IsPreviewActor(*It)) Previews.Add(*It);
    }
    for (AActor* Preview : Previews)
    {
        World->DestroyActor(Preview);
    }

    for (TActorIterator<AConnectIt_BuilderTile> It(World); It; ++It)
    {
        It->SetIsTemporarilyHiddenInEditor(false);
    }
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
