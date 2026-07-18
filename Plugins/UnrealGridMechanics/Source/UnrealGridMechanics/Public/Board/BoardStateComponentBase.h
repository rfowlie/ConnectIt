// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMechanicsBaseStructs.h"
#include "BoardStateComponentBase.generated.h"

class UBoardStateInterpreter;


// TODO: move to ConnectIt and rename
// Extensible tile data — add game specific fields in subclass or directly here
USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FTileData
{
    GENERATED_BODY()

    // Which faction owns a piece on this tile — 0 means empty
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    int32 FactionPiece = 0;

    // Tile multiplier — used for scoring
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    float Multiplier = 1.0f;

    // Whether this tile is currently active on the board
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    bool bIsActive = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoardStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileDataChanged, FGridPosition, Position, FTileData, NewData);

UCLASS(Abstract, ClassGroup=(Grid), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UBoardStateComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:

    UBoardStateComponentBase();

    // All visual systems bind here
    // Fires whenever board state changes on any machine
    UPROPERTY(BlueprintAssignable, Category = "Board State")
    FOnBoardStateChanged OnBoardStateChanged;

    // Bind a board state interpreter to this component
    // Called from AConnectItBoardActor::BeginPlay or project wiring
    UFUNCTION(BlueprintCallable, Category = "Board State")
    void RegisterInterpreter(UBoardStateInterpreter* Interpreter);

protected:

    // Subclass calls this after writing new state on the server
    // Fires OnBoardStateChanged immediately on server
    // Replication fires it on clients via OnRep in subclass
    void BroadcastChange();

private:

    // Registered interpreters -- notified on change
    UPROPERTY()
    TArray<TObjectPtr<UBoardStateInterpreter>> Interpreters;
};
