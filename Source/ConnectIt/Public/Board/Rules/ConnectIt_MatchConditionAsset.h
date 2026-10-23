// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Rules/ConnectIt_MatchCondition.h"
#include "ConnectIt_MatchConditionAsset.generated.h"


// One goal of a match, as an asset: one or more conditions that must ALL
// hold at the same time (e.g. Piece Count Equal 0 AND Score Threshold 4: clear
// the board, having scored). The Level director lists each side's goals as
// these assets, in the order they must be completed
// (UConnectIt_LevelDirector::PlayerWinConditions / AIWinConditions); a variant
// of a goal is just another asset.
//
// The conditions are the same plain thread-safe structs as before: when a
// match starts, the rule set copies them out of the assets
// (FConnectItRuleSet::ResolveConditionGroups) and from then on only the copies are
// used -- by the server, the clients and the AI's search alike. The asset
// itself only travels along as the goal's identity (e.g. on the
// WinCheckpointReached event).
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_MatchConditionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // What UI shows for this goal ("Score 20", "Clear the board")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions")
    FText DisplayName;

    // All must hold at once. None = never satisfied ("nothing to satisfy" is
    // not a win), so it blocks every goal after it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions", meta = (ExcludeBaseStruct))
    TArray<TInstancedStruct<FConnectItMatchCondition>> Conditions;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
