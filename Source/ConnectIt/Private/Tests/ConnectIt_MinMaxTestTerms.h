// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/Rules/Conditions/ConnectIt_MatchCondition.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "AI/ConnectIt_AIDirector.h"
#include "AI/ConnectIt_PlayerDirector.h"
#include "AI/ConnectIt_LevelDirector.h"
#include "AI/ConnectIt_AISelector.h"
#include "Board/Rules/Conditions/ConnectIt_MatchConditionDataAsset.h"
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
    virtual void OnWinCheckpointReached_Implementation(int32 Faction, bool bIsMine, UConnectIt_MatchConditionDataAsset* Goal, int32 GroupIndex, FName GroupName) override
    {
        Log += FString::Printf(TEXT("checkpoint %d %s %s %d %s; "),
            Faction, bIsMine ? TEXT("mine") : TEXT("theirs"), *GetNameSafe(Goal), GroupIndex, *GroupName.ToString());
    }
    virtual void OnLoseConditionMet_Implementation(int32 Faction, bool bIsMine, UConnectIt_MatchConditionDataAsset* Condition) override
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
    TObjectPtr<UConnectIt_MatchConditionDataAsset> RearmOnFire = nullptr;

protected:

    virtual void OnTriggerFired_Implementation(UConnectIt_MatchConditionDataAsset* Condition, int32 Faction) override
    {
        Log += FString::Printf(TEXT("%s %d; "), *GetNameSafe(Condition), Faction);
        if (Condition == RearmOnFire)
        {
            RearmTrigger(Condition);
        }
    }
};

// Every test effect's base: unlike a shipped effect, a test sets the power
// tag it reports (one test effect serves several test tags) and its gate
USTRUCT(meta = (Hidden))
struct FConnectItTilePowerEffect_TestBase : public FConnectItTilePowerEffect
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag TestTag;

    UPROPERTY()
    FConnectItMultiplierGate Gate;

    virtual FGameplayTag GetPowerTag() const override { return TestTag; }
    virtual bool PassesGate(const FConnectItTilePowerContext& Context) const override { return Gate.Passes(Context); }
};

// Test-only tile power effect (ConnectIt.Board.TilePowerData): does nothing;
// Strength is its one tunable value. Not meant for real levels.
USTRUCT(meta = (Hidden, DisplayName = "(Test) Do Nothing"))
struct FConnectItTilePowerEffect_Test : public FConnectItTilePowerEffect_TestBase
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Strength = 1;
};

// --- Test-only tile power effects (ConnectIt.Board.TilePowerChain). Not meant
// for real levels: the first real powers are designed separately. ---

// Force places a piece (for the power's faction) at each offset from the powered tile
USTRUCT(meta = (Hidden, DisplayName = "(Test) Place At Offsets"))
struct FConnectItTilePowerEffect_TestPlace : public FConnectItTilePowerEffect_TestBase
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FIntPoint> Offsets;

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override
    {
        for (const FIntPoint& Offset : Offsets)
        {
            FConnectItBoardOperation_ForcePlacePiece Place;
            Place.Position = FGridPosition(Context.Position.X + Offset.X, Context.Position.Y + Offset.Y);
            OutOperations.Add(FInstancedStruct::Make(Place));
        }
    }
};

// "Moves" the piece at From to To (offsets from the powered tile): removes it
// and places one for the power's faction -- there is no move operation yet
USTRUCT(meta = (Hidden, DisplayName = "(Test) Move A Neighbour"))
struct FConnectItTilePowerEffect_TestMove : public FConnectItTilePowerEffect_TestBase
{
    GENERATED_BODY()

    UPROPERTY()
    FIntPoint From = FIntPoint::ZeroValue;

    UPROPERTY()
    FIntPoint To = FIntPoint::ZeroValue;

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override
    {
        const FGridPosition FromPosition(Context.Position.X + From.X, Context.Position.Y + From.Y);
        if (!Context.Board.IsTileOccupied(FromPosition)) return;

        FConnectItBoardOperation_RemovePiece Remove;
        Remove.Position = FromPosition;
        OutOperations.Add(FInstancedStruct::Make(Remove));

        FConnectItBoardOperation_ForcePlacePiece Place;
        Place.Position = FGridPosition(Context.Position.X + To.X, Context.Position.Y + To.Y);
        OutOperations.Add(FInstancedStruct::Make(Place));
    }
};

// Places a piece on an empty tile picked at random (the board's seeded stream)
USTRUCT(meta = (Hidden, DisplayName = "(Test) Place At Random"))
struct FConnectItTilePowerEffect_TestRandomPlace : public FConnectItTilePowerEffect_TestBase
{
    GENERATED_BODY()

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override
    {
        TArray<FGridPosition> Empty;
        for (int32 Index = 0; Index < Context.Board.NumTiles(); Index++)
        {
            if (!Context.Board.GetTileDataAt(Index).bIsOccupied) Empty.Add(Context.Board.GetPositionAt(Index));
        }
        if (Empty.IsEmpty()) return;

        FConnectItBoardOperation_ForcePlacePiece Place;
        Place.Position = Empty[Context.Random.RandRange(0, Empty.Num() - 1)];
        OutOperations.Add(FInstancedStruct::Make(Place));
    }
};

// A Sync power: the powered tile can't hold a piece while the tile at Watch
// (an offset) has none -- it removes its own piece whenever that is so
USTRUCT(meta = (Hidden, DisplayName = "(Test) Sync: Clear While Watched Tile Empty"))
struct FConnectItTilePowerEffect_TestSyncClear : public FConnectItTilePowerEffect_TestBase
{
    GENERATED_BODY()

    UPROPERTY()
    FIntPoint Watch = FIntPoint::ZeroValue;

    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const override
    {
        const FGridPosition Watched(Context.Position.X + Watch.X, Context.Position.Y + Watch.Y);
        if (!Context.Board.IsTileOccupied(Context.Position) || Context.Board.IsTileOccupied(Watched)) return;

        FConnectItBoardOperation_RemovePiece Remove;
        Remove.Position = Context.Position;
        OutOperations.Add(FInstancedStruct::Make(Remove));
    }
};
