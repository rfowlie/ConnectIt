// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "Board/Rules/ConnectIt_ScoreThresholdWinCondition.h"


FConnectItRuleSet::FConnectItRuleSet()
{
    ScoringRule = TInstancedStruct<FConnectItScoringRule>::Make<FConnectItScoringRule_Lines>();
    WinCondition = TInstancedStruct<FConnectItWinCondition>::Make<FConnectItWinCondition_ScoreThreshold>();
}

float FConnectItRuleSet::ResolveBoardChange(
    FConnectItBoardState& Board,
    TConstArrayView<FGridPosition> TouchedPositions,
    FConnectItBoardChangeEvent* OutEvents) const
{
    const FConnectItScoringRule* Rule = GetScoringRule();
    if (!Rule) return 0.f;

    float TotalPoints = 0.f;
    for (const FGridPosition& Position : TouchedPositions)
    {
        // Read now, not up front: scoring an earlier position can clear
        // pieces. A tile holding no faction's piece (empty, or a non-faction
        // blocker) can't score.
        const FConnectItTileData* Tile = Board.GetTileData(Position);
        if (!Tile || Tile->FactionPiece == INDEX_NONE) continue;

        TotalPoints += Rule->ApplyScoring(Board, Position, Tile->FactionPiece, OutEvents);
    }
    return TotalPoints;
}

float FConnectItRuleSet::ApplyScoring(
    FConnectItBoardState& Board,
    FGridPosition Position,
    int32 Faction,
    FConnectItBoardChangeEvent* OutEvents) const
{
    const FConnectItScoringRule* Rule = GetScoringRule();
    return Rule ? Rule->ApplyScoring(Board, Position, Faction, OutEvents) : 0.f;
}

int32 FConnectItRuleSet::GetWinningFaction(const FConnectItBoardState& Board) const
{
    const FConnectItWinCondition* Rule = GetWinCondition();
    return Rule ? Rule->GetWinningFaction(Board) : INDEX_NONE;
}

float FConnectItRuleSet::GetTargetScore() const
{
    float TargetScore = 0.f;
    const FConnectItWinCondition* Rule = GetWinCondition();
    return Rule && Rule->GetTargetScore(TargetScore) ? TargetScore : 0.f;
}

bool FConnectItRuleSet::SetTargetScore(float NewTargetScore)
{
    FConnectItWinCondition* Rule = WinCondition.GetMutablePtr();
    return Rule && Rule->SetTargetScore(NewTargetScore);
}

void FConnectItRuleSet::StampWinState(FConnectItBoardState& Board) const
{
    // Refreshed on every check (i.e. every move), not only on a win, so UI
    // always sees current values.
    Board.TargetScore = GetTargetScore();

    const FConnectItWinCondition* Rule = GetWinCondition();

    Board.WinProgress.SetNumZeroed(Board.ScoreBoard.Num());
    for (int32 Faction = 0; Faction < Board.WinProgress.Num(); Faction++)
    {
        Board.WinProgress[Faction] = Rule ? Rule->GetProgress(Board, Faction) : 0.f;
    }

    const int32 Winner = Rule ? Rule->GetWinningFaction(Board) : INDEX_NONE;
    if (Winner != INDEX_NONE)
    {
        Board.bGameOver = true;
        Board.WinningFactionSlot = Winner;

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_RuleSet: faction %d has won"), Winner);
    }
}
