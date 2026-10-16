// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_WinCondition.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "AI/ConnectIt_AIDirector.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "ConnectIt_MinMaxTestTerms.generated.h"


// Test-only win condition (ConnectIt.AI.MinMax.WinConditionDrivesTerminal):
// not score-based -- whoever owns the (0,0) corner has won. Not meant for real
// levels.
USTRUCT(meta = (Hidden, DisplayName = "(Test) Corner Owner Wins"))
struct FConnectItWinCondition_TestCorner : public FConnectItWinCondition
{
    GENERATED_BODY()

    virtual int32 GetWinningFaction(const FConnectItBoardState& Board) const override
    {
        const FConnectItTileData* Corner = Board.GetTileData(FGridPosition(0, 0));
        return Corner && Corner->FactionPiece != -1 ? Corner->FactionPiece : INDEX_NONE;
    }
};

// Test-only AI strategy (ConnectIt.AI.Candidates): answers at once with
// NumCandidates place moves along row 0, in order. Not meant for real profiles.
UCLASS(Hidden, NotBlueprintable)
class UConnectIt_TestAIStrategy_FixedCandidates : public UConnectIt_AIStrategy
{
    GENERATED_BODY()

public:

    int32 NumCandidates = 8;

    // Which board row its moves are on
    int32 Row = 0;

protected:

    virtual void BeginDecision_Implementation(const FConnectItAIDecisionContext& Context) override
    {
        FConnectItAIDecision Decision;
        for (int32 Index = 0; Index < NumCandidates; Index++)
        {
            FConnectItBoardOperation_PlacePiece Operation;
            Operation.Position = FGridPosition(Row, Index);
            Decision.Candidates.Add({ FInstancedStruct::Make(Operation), FString::FromInt(Index) });
        }
        FinishDecision(Decision);
    }
};

// Test-only AI strategy (ConnectIt.AI.Coordinator): never answers on its own,
// like a slow search. Counts cancellations.
UCLASS(Hidden, NotBlueprintable)
class UConnectIt_TestAIStrategy_Silent : public UConnectIt_AIStrategy
{
    GENERATED_BODY()

public:

    int32 TimesCancelled = 0;

    // Answers now, as a late search would
    void AnswerLate(const FConnectItAIDecision& Decision) { FinishDecision(Decision); }

protected:

    virtual void BeginDecision_Implementation(const FConnectItAIDecisionContext& Context) override {}
    virtual void CancelDecision_Implementation() override { TimesCancelled++; }
};

// Test-only AI director (ConnectIt.AI.Director): records the events it gets
UCLASS(Hidden, NotBlueprintable)
class UConnectIt_TestAIDirector : public UConnectIt_AIDirector
{
    GENERATED_BODY()

public:

    FString Log;

    void TrySetLoadout(UActionLoadoutDataAsset* Loadout) { SetAILoadout(Loadout); }
    void TrySetStrategy(UConnectIt_AIStrategy* Strategy) { SetAIStrategy(Strategy); }
    UActionLoadoutDataAsset* TryGetLoadout() const { return GetAILoadout(); }

protected:

    virtual void OnAITurnStarted_Implementation(int32 TurnNumber) override
    {
        Log += FString::Printf(TEXT("start %d; "), TurnNumber);
    }
    virtual void OnAITurnEnded_Implementation(int32 TurnNumber) override
    {
        Log += FString::Printf(TEXT("end %d; "), TurnNumber);
    }
    virtual void OnWinCheckpointReached_Implementation(int32 Faction, bool bIsAI, int32 GroupIndex, FName GroupName) override
    {
        Log += FString::Printf(TEXT("checkpoint %d %s %d %s; "),
            Faction, bIsAI ? TEXT("AI") : TEXT("player"), GroupIndex, *GroupName.ToString());
    }
};
