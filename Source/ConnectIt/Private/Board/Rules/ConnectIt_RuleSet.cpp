// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Board/Rules/ConnectIt_WinConditionAsset.h"


FConnectItRuleSet::FConnectItRuleSet()
{
    ScoringRule = TInstancedStruct<FConnectItScoringRule>::Make<FConnectItScoringRule_Lines>();
}

void FConnectItRuleSet::ResolveWinGroups()
{
    const auto Resolve = [](const TCHAR* Side,
        const TArray<TObjectPtr<UConnectIt_WinConditionAsset>>& Assets, TArray<FConnectItWinConditionGroup>& OutGroups)
    {
        OutGroups.Reset();
        for (int32 Index = 0; Index < Assets.Num(); Index++)
        {
            UConnectIt_WinConditionAsset* Asset = Assets[Index];
            if (!IsValid(Asset))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("ConnectIt_RuleSet: %s win condition %d is empty -- skipped"), Side, Index);
                continue;
            }

            FConnectItWinConditionGroup& Group = OutGroups.AddDefaulted_GetRef();
            Group.Name = Asset->GetFName();
            Group.Conditions = Asset->Conditions;
            Group.Source = Asset;
        }
    };
    Resolve(TEXT("Player"), PlayerWinConditions, PlayerWinGroups);
    Resolve(TEXT("AI"), AIWinConditions, AIWinGroups);
}

const TArray<FConnectItWinConditionGroup>& FConnectItRuleSet::GetWinGroupsFor(int32 Faction) const
{
    return (Faction != INDEX_NONE && Faction == AIFaction) ? AIWinGroups : PlayerWinGroups;
}

float FConnectItRuleSet::ResolveBoardChange(
    FConnectItBoardState& Board,
    TConstArrayView<FGridPosition> TouchedPositions,
    bool bArrivingPiecesSurvive,
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

        TotalPoints += Rule->ApplyScoring(
            Board, Position, Tile->FactionPiece, bArrivingPiecesSurvive, OutEvents);
    }
    return TotalPoints;
}

float FConnectItRuleSet::CarryOutOperation(
    const FConnectItBoardOperation& Operation,
    FConnectItBoardState& Board,
    FConnectItBoardChangeEvent* OutEvents) const
{
    // The change itself: the operation only changes the board (and appends
    // the event saying what it did)...
    FConnectItTouchedPositions TouchedPositions;
    Operation.Apply(Board, TouchedPositions, OutEvents);

    // ...what follows from the change is a separate step: scoring where
    // pieces arrived (appending Scored events)
    return ResolveBoardChange(
        Board, TouchedPositions, Operation.ArrivingPiecesSurviveScoring(), OutEvents);
}

float FConnectItRuleSet::ApplyScoring(
    FConnectItBoardState& Board,
    FGridPosition Position,
    int32 Faction,
    bool bArrivingPieceSurvives,
    FConnectItBoardChangeEvent* OutEvents) const
{
    const FConnectItScoringRule* Rule = GetScoringRule();
    return Rule ? Rule->ApplyScoring(Board, Position, Faction, bArrivingPieceSurvives, OutEvents) : 0.f;
}

void FConnectItRuleSet::AdvanceWinGroups(
    FConnectItBoardState& Board, FConnectItBoardChangeEvent* OutEvents) const
{
    // One entry per faction (a board built without them starts every side at 0)
    Board.GroupsCompleted.SetNumZeroed(Board.ScoreBoard.Num());

    for (int32 Faction = 0; Faction < Board.GroupsCompleted.Num(); Faction++)
    {
        const TArray<FConnectItWinConditionGroup>& Groups = GetWinGroupsFor(Faction);
        int32& Completed = Board.GroupsCompleted[Faction];

        while (Completed < Groups.Num() && Groups[Completed].HasFactionWon(Board, Faction))
        {
            const FConnectItWinConditionGroup& Group = Groups[Completed];
            Completed++;

            // Completing the last group is the win, not a checkpoint
            if (OutEvents && Completed < Groups.Num())
            {
                FConnectItBoardEvent_WinCheckpointReached Checkpoint;
                Checkpoint.Faction = Faction;
                Checkpoint.GroupIndex = Completed - 1;
                Checkpoint.GroupName = Group.Name;
                Checkpoint.Asset = Group.Source;
                OutEvents->Add(Checkpoint);
            }
        }
    }
}

bool FConnectItRuleSet::HasFactionWon(const FConnectItBoardState& Board, int32 Faction) const
{
    const int32 NumGroups = GetWinGroupsFor(Faction).Num();
    return NumGroups > 0
        && Board.GroupsCompleted.IsValidIndex(Faction)
        && Board.GroupsCompleted[Faction] >= NumGroups;
}

int32 FConnectItRuleSet::GetWinningFaction(const FConnectItBoardState& Board) const
{
    // Lowest slot first, so a simultaneous win goes to the lowest slot.
    for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
    {
        if (HasFactionWon(Board, Faction)) return Faction;
    }
    return INDEX_NONE;
}

float FConnectItRuleSet::GetProgress(const FConnectItBoardState& Board, int32 Faction) const
{
    const TArray<FConnectItWinConditionGroup>& Groups = GetWinGroupsFor(Faction);
    if (Groups.IsEmpty()) return 0.f;

    const int32 Completed = Board.GroupsCompleted.IsValidIndex(Faction) ? Board.GroupsCompleted[Faction] : 0;
    if (Completed >= Groups.Num()) return 1.f;

    return (Completed + Groups[Completed].GetProgress(Board, Faction)) / Groups.Num();
}

float FConnectItRuleSet::GetTargetScore(const FConnectItBoardState& Board) const
{
    for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
    {
        const TArray<FConnectItWinConditionGroup>& Groups = GetWinGroupsFor(Faction);
        if (Groups.IsEmpty()) continue;

        // The group being worked on (the last one, once all are done)
        const int32 Completed = Board.GroupsCompleted.IsValidIndex(Faction) ? Board.GroupsCompleted[Faction] : 0;
        float TargetScore = 0.f;
        if (Groups[FMath::Min(Completed, Groups.Num() - 1)].GetTargetScore(TargetScore)) return TargetScore;
    }
    return 0.f;
}

float FConnectItRuleSet::GetTargetScore() const
{
    float TargetScore = 0.f;
    if (!PlayerWinGroups.IsEmpty() && PlayerWinGroups[0].GetTargetScore(TargetScore)) return TargetScore;
    if (!AIWinGroups.IsEmpty() && AIWinGroups[0].GetTargetScore(TargetScore)) return TargetScore;
    return 0.f;
}

bool FConnectItRuleSet::SetTargetScore(float NewTargetScore)
{
    // Both lists always get it (no short-circuit)
    bool bAnySet = false;
    if (!PlayerWinGroups.IsEmpty()) bAnySet |= PlayerWinGroups.Last().SetTargetScore(NewTargetScore);
    if (!AIWinGroups.IsEmpty()) bAnySet |= AIWinGroups.Last().SetTargetScore(NewTargetScore);
    return bAnySet;
}

void FConnectItRuleSet::StampWinState(FConnectItBoardState& Board, FConnectItBoardChangeEvent* OutEvents) const
{
    AdvanceWinGroups(Board, OutEvents);

    // Refreshed on every check (i.e. every move), not only on a win, so UI
    // always sees current values.
    Board.TargetScore = GetTargetScore(Board);

    Board.WinProgress.SetNumZeroed(Board.ScoreBoard.Num());
    for (int32 Faction = 0; Faction < Board.WinProgress.Num(); Faction++)
    {
        Board.WinProgress[Faction] = GetProgress(Board, Faction);
    }

    const int32 Winner = GetWinningFaction(Board);
    if (Winner != INDEX_NONE)
    {
        Board.bGameOver = true;
        Board.WinningFactionSlot = Winner;

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_RuleSet: faction %d has won"), Winner);
    }
}
