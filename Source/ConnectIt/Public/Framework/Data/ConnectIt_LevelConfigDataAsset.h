// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Board/Rules/ConnectIt_ScoringRule.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "ConnectIt_LevelConfigDataAsset.generated.h"

class UActionLoadoutDataAsset;
class AConnectIt_GridPiece;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_ScoringRule"))
    TScriptInterface<IConnectIt_ScoringRule> ScoringRule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules",
        meta = (MustImplement = "/Script/ConnectIt.ConnectIt_WinCondition"))
    TScriptInterface<IConnectIt_WinCondition> WinConditionRule;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext& Context) const override;
#endif
};
