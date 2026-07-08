// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/BoardManager.h"
#include "ConnectItBoardManagerComponent.h"
#include "ConnectItBoardStateComponent.h"
#include "ConnectItBoardManager.generated.h"

class UActionLoadOutDataAsset;
class UConnectIt_ConfigComponent;
class UConnectItTileStateInterpreter;
class UConnectItPieceSpawnInterpreter;
class UConnectItScoreInterpreter;


UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectItBoardManager : public ABoardManager
{
    GENERATED_BODY()

public:

    AConnectItBoardManager();

    // --- Components ---

    // ConnectIt specific board state -- typed access
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UConnectItBoardStateComponent> ConnectItBoardState = nullptr;

    // Owns all server side board logic
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UConnectItBoardManagerComponent> BoardManager = nullptr;

    // Designer configuration -- loadouts, pool size, AI config
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_ConfigComponent> ConnectItConfig = nullptr;

    // --- Interpreters ---
    // All visible in Details panel as components
    // Designer can add additional interpreters without code changes

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectItTileStateInterpreter> TileStateInterpreter = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectItPieceSpawnInterpreter> PieceSpawnInterpreter = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectItScoreInterpreter> ScoreInterpreter = nullptr;

    // --- Convenience Accessors ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UActionLoadOutDataAsset* GetPlayerLoadout() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UActionLoadOutDataAsset* GetEnemyLoadout() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    float GetWinScoreThreshold() const;

    // --- Setup ---

    // Called by GameMode after all participants are registered
    // and all tiles have registered with UGridWorldSubsystem
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    void InitialiseBoard(int32 NumFactions);

protected:

    virtual void BeginPlay() override;

private:

    // Wires all interpreters to board state component
    // Called in BeginPlay -- this is the project specific wiring
    void BindInterpreters();

    // Wires board manager to action component passthrough delegate
    // and shift component result delegate
    // Called in BeginPlay
    void BindBoardManager();
};