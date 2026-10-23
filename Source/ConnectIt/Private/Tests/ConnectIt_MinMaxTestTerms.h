// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/ConnectIt_MatchCondition.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "AI/ConnectIt_AIDirector.h"
#include "AI/ConnectIt_PlayerDirector.h"
#include "AI/ConnectIt_LevelDirector.h"
#include "AI/ConnectIt_AISelector.h"
#include "Board/Rules/ConnectIt_MatchConditionAsset.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/Powers/ConnectIt_TilePower.h"
#include "ConnectIt_MinMaxTestTerms.generated.h"


// Test-only win condition (ConnectIt.AI.MinMax.WinConditionDrivesTerminal):
// not score-based -- whoever owns the (0,0) corner has won. Not meant for real
// levels.
USTRUCT(meta = (Hidden, DisplayName = "(Test) Corner Owner Wins"))
struct FConnectItMatchCondition_TestCorner : public FConnectItMatchCondition
{
    GENERATED_BODY()

    virtual bool IsMetFor(const FConnectItConditionContext& Context, int32 Faction) const override
    {
        const FConnectItTileData* Corner = Context.Board.GetTileData(FGridPosition(0, 0));
        return Corner && Corner->FactionPiece != -1 && Corner->FactionPiece == Faction;
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

    // The budget the last decision was given
    float LastTimeBudget = -1.f;

protected:

    virtual void BeginDecision_Implementation(const FConnectItAIDecisionContext& Context) override
    {
        LastTimeBudget = Context.TimeBudgetSeconds;
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

    // A better answer so far, as a search would publish one
    void Publish(const FConnectItAIDecision& Decision) { PublishBestSoFar(Decision); }

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
    void TrySetStrategies(const TArray<UConnectIt_AIStrategy*>& Templates) { SetAIStrategies(Templates); }
    void TrySetSelector(UConnectIt_AISelector* Template) { SetAISelector(Template); }
    UActionLoadoutDataAsset* TryGetLoadout() const { return GetAILoadout(); }

protected:

    virtual void OnMatchStarted_Implementation() override
    {
        Log += TEXT("match; ");
    }
    virtual void OnTurnStarted_Implementation(int32 Faction, bool bIsMine, int32 TurnNumber) override
    {
        Log += FString::Printf(TEXT("start %d %s %d; "), Faction, bIsMine ? TEXT("mine") : TEXT("theirs"), TurnNumber);
    }
    virtual void OnTurnEnded_Implementation(int32 Faction, bool bIsMine, int32 TurnNumber) override
    {
        Log += FString::Printf(TEXT("end %d %s %d; "), Faction, bIsMine ? TEXT("mine") : TEXT("theirs"), TurnNumber);
    }
    virtual void OnWinCheckpointReached_Implementation(int32 Faction, bool bIsMine, UConnectIt_MatchConditionAsset* Goal, int32 GroupIndex, FName GroupName) override
    {
        Log += FString::Printf(TEXT("checkpoint %d %s %s %d %s; "),
            Faction, bIsMine ? TEXT("mine") : TEXT("theirs"), *GetNameSafe(Goal), GroupIndex, *GroupName.ToString());
    }
    virtual void OnLoseConditionMet_Implementation(int32 Faction, bool bIsMine, UConnectIt_MatchConditionAsset* Condition) override
    {
        Log += FString::Printf(TEXT("lost %d %s %s; "), Faction, bIsMine ? TEXT("mine") : TEXT("theirs"), *GetNameSafe(Condition));
    }
    virtual void OnMatchWon_Implementation(int32 Faction, bool bIsMine) override
    {
        Log += FString::Printf(TEXT("won %d %s; "), Faction, bIsMine ? TEXT("mine") : TEXT("theirs"));
    }
};

// Test-only level director (ConnectIt.AI.LevelTriggers): records the triggers
// that fire, and re-arms one when asked
UCLASS(Hidden, NotBlueprintable)
class UConnectIt_TestLevelDirector : public UConnectIt_LevelDirector
{
    GENERATED_BODY()

public:

    FString Log;

    // Re-arm this trigger from inside its own reaction
    TObjectPtr<UConnectIt_MatchConditionAsset> RearmOnFire = nullptr;

protected:

    virtual void OnTriggerFired_Implementation(UConnectIt_MatchConditionAsset* Condition, int32 Faction) override
    {
        Log += FString::Printf(TEXT("%s %d; "), *GetNameSafe(Condition), Faction);
        if (Condition == RearmOnFire)
        {
            RearmTrigger(Condition);
        }
    }
};

// Test-only tile power effect (ConnectIt.Board.TilePowerData): does nothing;
// Strength is its one tunable value. Not meant for real levels.
USTRUCT(meta = (Hidden, DisplayName = "(Test) Do Nothing"))
struct FConnectItTilePowerEffect_Test : public FConnectItTilePowerEffect
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Strength = 1;
};
