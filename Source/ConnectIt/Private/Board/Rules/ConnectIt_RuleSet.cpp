// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Board/Rules/ConnectIt_MatchConditionAsset.h"


FConnectItRuleSet::FConnectItRuleSet()
{
    ScoringRule = TInstancedStruct<FConnectItScoringRule>::Make<FConnectItScoringRule_Lines>();
}

void FConnectItRuleSet::ResolveConditionGroups(
    const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& PlayerWinConditions,
    const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& AIWinConditions,
    const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& PlayerLoseConditions,
    const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& AILoseConditions)
{
    const auto Resolve = [](const TCHAR* Side,
        const TArray<TObjectPtr<UConnectIt_MatchConditionAsset>>& Assets, TArray<FConnectItMatchConditionGroup>& OutGroups)
    {
        OutGroups.Reset();
        for (int32 Index = 0; Index < Assets.Num(); Index++)
        {
            UConnectIt_MatchConditionAsset* Asset = Assets[Index];
            if (!IsValid(Asset))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("ConnectIt_RuleSet: %s win condition %d is empty -- skipped"), Side, Index);
                continue;
            }

            FConnectItMatchConditionGroup& Group = OutGroups.AddDefaulted_GetRef();
            Group.Name = Asset->GetFName();
            Group.Conditions = Asset->Conditions;
            Group.Source = Asset;
        }
    };
    Resolve(TEXT("Player"), PlayerWinConditions, PlayerWinGroups);
    Resolve(TEXT("AI"), AIWinConditions, AIWinGroups);
    Resolve(TEXT("Player lose"), PlayerLoseConditions, PlayerLoseGroups);
    Resolve(TEXT("AI lose"), AILoseConditions, AILoseGroups);
}

const TArray<FConnectItMatchConditionGroup>& FConnectItRuleSet::GetWinGroupsFor(int32 Faction) const
{
    return (Faction != INDEX_NONE && Faction == AIFaction) ? AIWinGroups : PlayerWinGroups;
}

const TArray<FConnectItMatchConditionGroup>& FConnectItRuleSet::GetLoseGroupsFor(int32 Faction) const
{
    static const TArray<FConnectItMatchConditionGroup> None;
    if (AIFaction == INDEX_NONE) return None;
    return Faction == AIFaction ? AILoseGroups : PlayerLoseGroups;
}

int32 FConnectItRuleSet::GetLosingFaction(const FConnectItBoardState& Board, const FConnectItMatchConditionGroup** OutGroup) const
{
    const FConnectItConditionContext Context{ Board, *this };
    for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
    {
        for (const FConnectItMatchConditionGroup& Group : GetLoseGroupsFor(Faction))
        {
            if (Group.IsMetFor(Context, Faction))
            {
                if (OutGroup) *OutGroup = &Group;
                return Faction;
            }
        }
    }
    return INDEX_NONE;
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
    const FConnectItConditionContext Context{ Board, *this };

    for (int32 Faction = 0; Faction < Board.GroupsCompleted.Num(); Faction++)
    {
        const TArray<FConnectItMatchConditionGroup>& Groups = GetWinGroupsFor(Faction);
        int32& Completed = Board.GroupsCompleted[Faction];

        while (Completed < Groups.Num() && Groups[Completed].IsMetFor(Context, Faction))
        {
            const FConnectItMatchConditionGroup& Group = Groups[Completed];
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
    // Wins first, lowest slot first, so a simultaneous win goes to the lowest slot
    for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
    {
        if (HasFactionWon(Board, Faction)) return Faction;
    }

    // Then a loss: the other side wins (lose conditions only exist with two
    // factions -- a match with an AI)
    const int32 Loser = GetLosingFaction(Board);
    if (Loser != INDEX_NONE && Board.ScoreBoard.Num() == 2)
    {
        return 1 - Loser;
    }
    return INDEX_NONE;
}

float FConnectItRuleSet::GetProgress(const FConnectItBoardState& Board, int32 Faction) const
{
    const TArray<FConnectItMatchConditionGroup>& Groups = GetWinGroupsFor(Faction);
    if (Groups.IsEmpty()) return 0.f;

    const int32 Completed = Board.GroupsCompleted.IsValidIndex(Faction) ? Board.GroupsCompleted[Faction] : 0;
    if (Completed >= Groups.Num()) return 1.f;

    return (Completed + Groups[Completed].GetProgress(FConnectItConditionContext{ Board, *this }, Faction)) / Groups.Num();
}

float FConnectItRuleSet::GetTargetScore(const FConnectItBoardState& Board) const
{
    for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
    {
        const TArray<FConnectItMatchConditionGroup>& Groups = GetWinGroupsFor(Faction);
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
        // Won by the other side meeting a lose condition (goals are checked
        // first, so a completed goal never shows up as a loss)
        if (!HasFactionWon(Board, Winner) && Board.LosingFactionSlot == INDEX_NONE)
        {
            const FConnectItMatchConditionGroup* LoseGroup = nullptr;
            const int32 Loser = GetLosingFaction(Board, &LoseGroup);
            Board.LosingFactionSlot = Loser;

            if (OutEvents && LoseGroup)
            {
                FConnectItBoardEvent_LoseConditionMet Lost;
                Lost.Faction = Loser;
                Lost.Asset = LoseGroup->Source;
                Lost.GroupName = LoseGroup->Name;
                OutEvents->Add(Lost);
            }

            UE_LOG(LogTemp, Log,
                TEXT("ConnectIt_RuleSet: faction %d met lose condition '%s'"),
                Loser, LoseGroup ? *LoseGroup->Name.ToString() : TEXT("?"));
        }

        Board.bGameOver = true;
        Board.WinningFactionSlot = Winner;

        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_RuleSet: faction %d has won"), Winner);
    }
}
