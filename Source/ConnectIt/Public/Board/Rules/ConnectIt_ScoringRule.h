// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "ConnectIt_ScoringRule.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UConnectIt_ScoringRule : public UInterface
{
    GENERATED_BODY()
};

// Pluggable board-scoring strategy -- how a just-placed piece scores, and
// how the board mutates as a result. Assigned via UConnectIt_BoardRulesComponent::
// ScoringRule (TScriptInterface, EditAnywhere Instanced), mirroring
// ITurnOrderInterface/TurnOrderStrategy's precedent. UConnectIt_LineScoringRule
// is the default implementation (N-in-a-row); other shapes (e.g. matching a
// fixed pattern instead of a line) can be added as new implementers without
// touching AConnectIt_BoardManager.
class CONNECTIT_API IConnectIt_ScoringRule
{
    GENERATED_BODY()

public:

    // Called after FactionSlot's piece has already been written into
    // MutableState at Position. Implementations mutate MutableState with any
    // further scoring consequences (clearing tiles, incrementing
    // multipliers, updating ScoreBoard) and return the total points awarded
    // for this placement (0 if none). OutScoringPositions is appended with
    // every tile that was part of a completed line (left untouched if
    // nothing scored) -- the caller starts it empties each call.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|Scoring")
    float ApplyScoring(
        UPARAM(ref) FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot,
        UPARAM(ref) TArray<FGridPosition>& OutScoringPositions);
};
