// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_TilePlaceableRule.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_LevelConfigDataAsset.generated.h"

class UActionLoadoutDataAsset;
class AConnectIt_GridPiece;
class UConnectIt_TileRegistry;
class UConnectIt_PieceRegistry;
class UGridDefinition;

// Per-level board configuration -- replaces UConnectIt_ConfigComponent
// (used to sit on the level-placed AConnectIt_BoardManager actor, one
// instance configured per level) outright, not alongside it: a designer
// authors one of these per level instead of configuring a placed actor,
// and both AConnectIt_GameMode (server) and AConnectIt_PlayerController
// (client, for its own loadout) resolve the SAME asset independently via
// ConnectIt_LevelConfigSettings' level-name lookup -- symmetric, static,
// un-networked data, so no live server object needs to hand it to clients.
// See "Board architecture overhaul" plan.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_LevelConfigDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    // --- Loadouts ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
    TObjectPtr<UActionLoadoutDataAsset> PlayerLoadout = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
    TObjectPtr<UActionLoadoutDataAsset> EnemyLoadout = nullptr;

    // --- AI ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
    int32 AISearchDepth = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
    int32 AIThreadDepth = 1;

    // --- Piece Pool ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
    TSubclassOf<AConnectIt_GridPiece> PieceActorClass = nullptr;

    // Per-client pool size -- each client manages its own pool
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config",
        meta = (ClampMin = 1))
    int32 PiecePoolInitialSize = 32;

    // --- Rules ---
    // Designer-selectable concrete subclass per level -- replaces the
    // equivalent Instanced selection that used to live on the placed
    // UConnectIt_BoardRulesComponent instance.

    // TObjectPtr<UObject> + Instanced + MustImplement (not TScriptInterface
    // -- Instanced only drives the inline class-pick/edit Details-panel
    // behaviour on an FObjectProperty, which TScriptInterface's
    // FInterfaceProperty is not).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_ScoringRule"))
    TObjectPtr<UObject> ScoringRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_WinCondition"))
    TObjectPtr<UObject> WinConditionRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_TilePlaceableRule"))
    TObjectPtr<UObject> TilePlaceableRule;

    // --- Board Registries ---
    // Instanced concrete-class UObject polymorphism, same shape as the rules
    // above but typed directly to a concrete registry base rather than an
    // interface -- a designer picks a whole UConnectIt_TileRegistry/
    // UConnectIt_PieceRegistry subclass with its own inline-editable
    // sub-properties, the same pattern ABoardManagerBase used to expose
    // these with. Typed to the ConnectIt-specific subclasses (not the
    // plugin's own UGridTileRegistryBase/UGridPieceRegistryBase) so the
    // Details-panel class picker only offers registries that can actually
    // resolve UConnectIt_BoardStateComponent -- see those classes' header
    // comments. Treated purely as a template by
    // UConnectIt_BoardRegistrySubsystem, which duplicates its own per-world
    // runtime instance from these rather than ever using them live -- see
    // that class's header comment.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Board")
    TObjectPtr<UConnectIt_TileRegistry> TileRegistry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Board")
    TObjectPtr<UConnectIt_PieceRegistry> PieceRegistry;

    // Grid geometry (cell size, World<->Grid conversion) both registries
    // above share -- a sibling template, same Instanced/duplicate-per-world
    // pattern, not a sub-property of either registry. See UGridDefinition's
    // own class comment for why this isn't just TileRegistry's business.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ConnectIt|Board")
    TObjectPtr<UGridDefinition> GridDefinition;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext& Context) const override;
#endif
};
