// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Rules/Scoring/ConnectIt_LineScoringRule.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Board/Rules/Conditions/ConnectIt_MatchConditionDataAsset.h"
#include "Board/Powers/ConnectIt_TilePowerChain.h"


namespace
{
    // Condition assets -> the plain groups the match plays with, in order
    void ResolveGroups(const TCHAR* Side,
        const TArray<TObjectPtr<UConnectIt_MatchConditionDataAsset>>& Assets, TArray<FConnectItMatchConditionGroup>& OutGroups)
    {
        OutGroups.Reset();
        for (int32 Index = 0; Index < Assets.Num(); Index++)
        {
            UConnectIt_MatchConditionDataAsset* Asset = Assets[Index];
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
    }

    const TCHAR* SideText(bool bForAI, bool bWinGoals)
    {
        return bForAI ? (bWinGoals ? TEXT("AI") : TEXT("AI lose"))
                      : (bWinGoals ? TEXT("Player") : TEXT("Player lose"));
    }
}

FConnectItRuleSet::FConnectItRuleSet()
{
    ScoringRule = TInstancedStruct<FConnectItScoringRule>::Make<FConnectItScoringRule_Lines>();
}

void FConnectItRuleSet::ResolveConditionGroups(
    const TArray<TObjectPtr<UConnectIt_MatchConditionDataAsset>>& PlayerWinConditions,
    const TArray<TObjectPtr<UConnectIt_MatchConditionDataAsset>>& AIWinConditions,
    const TArray<TObjectPtr<UConnectIt_MatchConditionDataAsset>>& PlayerLoseConditions,
    const TArray<TObjectPtr<UConnectIt_MatchConditionDataAsset>>& AILoseConditions)
{
    SetConditionGroups(false, true, PlayerWinConditions);
    SetConditionGroups(true, true, AIWinConditions);
    SetConditionGroups(false, false, PlayerLoseConditions);
    SetConditionGroups(true, false, AILoseConditions);
}

void FConnectItRuleSet::SetTilePowers(const TArray<FConnectItTilePower>& Powers, const TArray<FGameplayTag>& LevelWide)
{
    TilePowers.Reset(Powers.Num());
    for (int32 Index = 0; Index < Powers.Num(); Index++)
    {
        const FConnectItTilePower& Power = Powers[Index];
        if (!Power.IsSet())
        {
            UE_LOG(LogTemp, Warning, TEXT("ConnectIt_RuleSet: tile power %d has no name -- skipped"), Index);
            continue;
        }
        if (FindTilePower(Power.Name))
        {
            UE_LOG(LogTemp, Warning, TEXT("ConnectIt_RuleSet: tile power %s is defined twice -- the second skipped"),
                *Power.Name.ToString());
            continue;
        }
        TilePowers.Add(Power);
    }

    LevelWideTilePowers.Reset(LevelWide.Num());
    for (const FGameplayTag& Name : LevelWide)
    {
        if (!FindTilePower(Name))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_RuleSet: level-wide power %s isn't in the tile catalog -- skipped"), *Name.ToString());
            continue;
        }
        if (LevelWideTilePowers.Contains(Name))
        {
            UE_LOG(LogTemp, Warning, TEXT("ConnectIt_RuleSet: level-wide power %s is listed twice -- the second skipped"),
                *Name.ToString());
            continue;
        }
        LevelWideTilePowers.Add(Name);
    }
}

bool FConnectItRuleSet::HasTurnStartTilePowerReady(const FConnectItBoardState& Board, int32 TurnNumber) const
{
    for (int32 Index = 0; Index < LevelWideTilePowers.Num(); Index++)
    {
        const FConnectItTilePower* Power = FindTilePower(LevelWideTilePowers[Index]);
        const int32 ReadyTurn = Board.LevelWideReadyTurns.IsValidIndex(Index) ? Board.LevelWideReadyTurns[Index] : 0;
        if (Power && Power->HasTrigger(EConnectItTilePowerTrigger::TurnStart) && TurnNumber >= ReadyTurn && Board.NumTiles() > 0)
        {
            return true;
        }
    }
    for (const FConnectItTileData& Tile : Board.TileDataArray)
    {
        const FConnectItTilePower* Power = FindTilePower(Tile.TilePowerTag);
        if (Power && Power->HasTrigger(EConnectItTilePowerTrigger::TurnStart) && TurnNumber >= Tile.TilePowerReadyTurn)
        {
            return true;
        }
    }
    return false;
}

float FConnectItRuleSet::ClampMultiplier(const FConnectItTileData& Tile, float Value) const
{
    return FConnectItTilePower::ClampMultiplier(FindTilePower(Tile.TilePowerTag), Value);
}

float FConnectItBoardChanges::ClampMultiplier(const FConnectItTileData& Tile, float Value) const
{
    return Rules ? Rules->ClampMultiplier(Tile, Value) : FConnectItTilePower::ClampMultiplier(nullptr, Value);
}

const FConnectItTilePower* FConnectItRuleSet::FindTilePower(const FGameplayTag& Name) const
{
    return Name.IsValid()
        ? TilePowers.FindByPredicate([&Name](const FConnectItTilePower& Power) { return Power.Name == Name; })
        : nullptr;
}

void FConnectItRuleSet::SetConditionGroups(bool bForAI, bool bWinGoals,
    const TArray<TObjectPtr<UConnectIt_MatchConditionDataAsset>>& Conditions)
{
    TArray<FConnectItMatchConditionGroup>& Groups = bForAI
        ? (bWinGoals ? AIWinGroups : AILoseGroups)
        : (bWinGoals ? PlayerWinGroups : PlayerLoseGroups);
    ResolveGroups(SideText(bForAI, bWinGoals), Conditions, Groups);
}

const TArray<FConnectItMatchConditionGroup>& FConnectItRuleSet::GetConditionGroups(bool bForAI, bool bWinGoals) const
{
    return bForAI
        ? (bWinGoals ? AIWinGroups : AILoseGroups)
        : (bWinGoals ? PlayerWinGroups : PlayerLoseGroups);
}

bool FConnectItRuleSet::IsScoringRule(const FInstancedStruct& Rule)
{
    const UScriptStruct* Struct = Rule.GetScriptStruct();
    return Struct && Struct->IsChildOf(FConnectItScoringRule::StaticStruct());
}

bool FConnectItRuleSet::SetScoringRuleFrom(const FInstancedStruct& NewRule)
{
    if (!IsScoringRule(NewRule)) return false;
    ScoringRule.InitializeAsScriptStruct(NewRule.GetScriptStruct(), NewRule.GetMemory());
    return true;
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
    const FConnectItBoardChanges& Changes,
    FConnectItBoardChangeEvent* OutEvents) const
{
    const FConnectItScoringRule* Rule = GetScoringRule();
    if (!Rule) return 0.f;

    // A bug guard, not a rule: each tile power fires at most once per action,
    // which is what ends every chain
    constexpr int32 MaxPasses = 64;

    float TotalPoints = 0.f;

    // This action's tile powers (with none, only scoring runs and nothing is copied)
    FConnectItTilePowerChain Powers(*this, Board, OutEvents);

    // What the last step changed: the operation's record, then each pass's
    FConnectItBoardChanges Next;
    FConnectItBoardChanges Settled;
    const FConnectItBoardChanges* Step = &Changes;
    for (int32 Pass = 0; Pass < MaxPasses; Pass++)
    {
        // 1. Settle: the pre-scoring powers Step sets off, until quiet, adding
        //    what they changed (and brought in) to what is scored
        if (Powers.HasTilePowers())
        {
            Settled = *Step;
            Powers.Settle(Settled);
            Step = &Settled;
        }

        // 2. Score every piece that arrived since the last pass
        if (!Step->HasScoringArrivals()) return TotalPoints;

        FConnectItScoringArrivals Arrivals;
        Step->GetScoringArrivals(Arrivals);

        FConnectItBoardChanges Scored;
        Scored.Faction = Step->Faction;
        Scored.Rules = this;
        TotalPoints += Rule->ScorePass(Board, Arrivals, Scored, OutEvents);

        // The chain ends at the first pass that scores nothing
        if (Scored.ScoredThrough.IsEmpty()) return TotalPoints;

        // 3. Scoring reactions: the Scoring powers on the tiles the lines ran
        //    through, adding what they changed to Scored
        Powers.ReactToScoring(Scored);

        // Back to settle with what scoring and its reactions changed
        Next = MoveTemp(Scored);
        Step = &Next;
    }

    UE_LOG(LogTemp, Error,
        TEXT("ConnectIt_RuleSet: ResolveBoardChange stopped after %d passes -- a chain that never ends is a bug"),
        MaxPasses);
    return TotalPoints;
}

bool FConnectItRuleSet::CanCarryOut(const FConnectItBoardOperation& Operation, const FConnectItBoardState& Board,
    FString* OutWhyNot) const
{
    if (!Operation.CanApply(Board, OutWhyNot)) return false;
    if (Operation.CanOperateOnLockedTiles()) return true;

    TArray<FGridPosition> Targets;
    Operation.GetTargetPositions(Targets);
    for (const FGridPosition& Target : Targets)
    {
        const FConnectItTileData* Tile = Board.GetTileData(Target);
        if (Tile && Tile->bIsLocked)
        {
            if (OutWhyNot) *OutWhyNot = FString::Printf(TEXT("tile (%d,%d) is locked"), Target.X, Target.Y);
            return false;
        }
    }
    return true;
}

float FConnectItRuleSet::CarryOutOperation(
    const FConnectItBoardOperation& Operation,
    FConnectItBoardState& Board,
    FConnectItBoardChangeEvent* OutEvents) const
{
    // The change itself: the operation only changes the board, reporting
    // what it did (and appending the event saying so)...
    FConnectItBoardChanges Changes;
    Changes.Faction = Operation.Faction;
    Changes.Rules = this;
    Operation.Apply(Board, Changes, OutEvents);

    // ...what follows from the change is a separate step: scoring where
    // pieces arrived (appending Scored events)
    return ResolveBoardChange(Board, Changes, OutEvents);
}

float FConnectItRuleSet::ApplyScoring(
    FConnectItBoardState& Board,
    FGridPosition Position,
    bool bArrivingPieceSurvives,
    FConnectItBoardChangeEvent* OutEvents) const
{
    const FConnectItScoringRule* Rule = GetScoringRule();
    const FConnectItTileData* Tile = Board.GetTileData(Position);
    if (!Rule || !Tile) return 0.f;

    const FConnectItPieceArrival Arrival{ Position, Tile->FactionPiece, bArrivingPieceSurvives };
    FConnectItBoardChanges Scored;
    Scored.Rules = this;
    return Rule->ScorePass(Board, MakeArrayView(&Arrival, 1), Scored, OutEvents);
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
