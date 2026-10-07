// Fill out your copyright notice in the Description page of Project Settings.

#include "MinMax/ConnectIt_MinMaxRules.h"
#include "Board/Rules/ConnectIt_LineScoringRule.h"


// --- Geometry ---

FConnectItMinMaxGeometry FConnectItMinMaxGeometry::Build(
    const FConnectItBoardState& Board, int32 ConnectLength)
{
    FConnectItMinMaxGeometry Geometry;

    const int32 NumTiles = Board.NumTiles();

    TMap<FGridPosition, int32> IndexByPosition;
    IndexByPosition.Reserve(NumTiles);
    for (int32 Index = 0; Index < NumTiles; Index++)
    {
        IndexByPosition.Add(Board.GetPositionAt(Index), Index);
    }

    // No line scoring, no line windows
    const int32 WindowLength = FMath::Max(0, ConnectLength);
    const TArray<FGridDirectionVector>& Directions =
        FConnectItScoringRule_Lines::GetScoringDirections();

    Geometry.Neighbours.SetNum(NumTiles);

    for (int32 Index = 0; Index < NumTiles; Index++)
    {
        const FGridPosition Start = Board.GetPositionAt(Index);

        // Each window is generated once, from its first tile, walking the
        // axis's positive direction only.
        for (const FGridDirectionVector& Dir : Directions)
        {
            TArray<int32> Window;
            Window.Reserve(WindowLength);
            for (int32 Step = 0; Step < WindowLength; Step++)
            {
                const FGridPosition Cell(
                    Start.X + Step * Dir.Row, Start.Y + Step * Dir.Column);
                const int32* CellIndex = IndexByPosition.Find(Cell);
                if (!CellIndex) break;
                Window.Add(*CellIndex);
            }

            if (WindowLength > 0 && Window.Num() == WindowLength)
            {
                Geometry.LineWindows.Add(MoveTemp(Window));
            }
        }

        for (int32 DX = -1; DX <= 1; DX++)
        {
            for (int32 DY = -1; DY <= 1; DY++)
            {
                if (DX == 0 && DY == 0) continue;
                if (const int32* NeighbourIndex =
                    IndexByPosition.Find(FGridPosition(Start.X + DX, Start.Y + DY)))
                {
                    Geometry.Neighbours[Index].Add(*NeighbourIndex);
                }
            }
        }
    }

    return Geometry;
}

// --- Rules ---

FConnectItMinMaxRules::FConnectItMinMaxRules(
    const FConnectItBoardState& Board,
    const FConnectItRuleSet& InRules,
    const TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>>& InEvaluationTerms,
    const TArray<TInstancedStruct<FConnectItMinMaxOrderTerm>>& InOrderingTerms)
    : Rules(InRules)
    , ScoringRule(Rules.GetScoringRule())
    , WinCondition(Rules.GetWinCondition())
    , TilePlaceableRule(Rules.GetTilePlaceableRule())
    , Geometry(FConnectItMinMaxGeometry::Build(
        Board,
        Rules.GetScoringRuleAs<FConnectItScoringRule_Lines>()
            ? Rules.GetScoringRuleAs<FConnectItScoringRule_Lines>()->ConnectLength
            : 0))
    , EvaluationTerms(InEvaluationTerms)
    , OrderingTerms(InOrderingTerms)
{
    if (!ScoringRule || !WinCondition || !TilePlaceableRule)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_MinMaxRules: the match's rule set is missing a "
                 "rule (scoring %s, win condition %s, placement %s) -- the AI "
                 "will play as if that rule did nothing"),
            ScoringRule ? TEXT("ok") : TEXT("MISSING"),
            WinCondition ? TEXT("ok") : TEXT("MISSING"),
            TilePlaceableRule ? TEXT("ok") : TEXT("MISSING"));
    }

    // Pointers into the arrays above -- stable, since this object is never
    // copied and the arrays never change after this.
    for (const TInstancedStruct<FConnectItMinMaxEvalTerm>& Term : EvaluationTerms)
    {
        const FConnectItMinMaxEvalTerm* Ptr = Term.GetPtr();
        if (Ptr && Ptr->Weight != 0.f)
        {
            ActiveEvaluationTerms.Add(Ptr);
        }
    }

    for (const TInstancedStruct<FConnectItMinMaxOrderTerm>& Term : OrderingTerms)
    {
        const FConnectItMinMaxOrderTerm* Ptr = Term.GetPtr();
        if (Ptr && Ptr->Weight != 0.f)
        {
            ActiveOrderingTerms.Add(Ptr);
        }
    }
}

FConnectItMinMaxRules::FState FConnectItMinMaxRules::MakeRoot(
    const FConnectItBoardState& Board, int32 SideToMove) const
{
    FState Root;
    Root.Board = Board;
    Root.SideToMove = SideToMove;
    // UI-only data the search never reads -- dropped so each node's board
    // copy doesn't carry (and allocate) it
    Root.Board.WinProgress.Empty();
    return Root;
}

void FConnectItMinMaxRules::GenerateMoves(const FState& State, TArray<FMove>& OutMoves) const
{
    // The match's own placement rule, asked by tile index (no position
    // lookups).
    if (!TilePlaceableRule) return;

    const int32 NumTiles = State.Board.NumTiles();
    for (int32 Index = 0; Index < NumTiles; Index++)
    {
        if (TilePlaceableRule->IsTilePlaceable(State.Board, Index))
        {
            OutMoves.Add({ Index, State.Board.GetPositionAt(Index) });
        }
    }
}

FConnectItMinMaxRules::FState FConnectItMinMaxRules::ApplyMove(
    const FState& State, const FMove& Move) const
{
    // Mirrors UConnectIt_BoardRequestMediator::HandlePlacePieceRequest:
    // place the piece, then run the match's scoring rule from the placed
    // position.
    FState Child = State;
    Child.Board.TileDataArray[Move.TileIndex].SetFactionPiece(State.SideToMove);

    if (ScoringRule)
    {
        TArray<FGridPosition> ScoringPositions; // visuals-only output, unused here
        ScoringRule->ApplyScoring(Child.Board, Move.Position, State.SideToMove, ScoringPositions);
    }

    Child.SideToMove = Opponent(State.SideToMove);
    return Child;
}

bool FConnectItMinMaxRules::IsTerminalState(const FState& State) const
{
    return WinCondition && WinCondition->GetWinningFaction(State.Board) != INDEX_NONE;
}

int32 FConnectItMinMaxRules::EvaluateTerminalState(const FState& State, int32 Ply) const
{
    const int32 Winner = WinCondition
        ? WinCondition->GetWinningFaction(State.Board)
        : INDEX_NONE;

    if (Winner == INDEX_NONE) return 0;

    // Ply makes nearer wins and farther losses score better (see WinValue)
    return Winner == State.SideToMove ? WinValue - Ply : -(WinValue - Ply);
}

int32 FConnectItMinMaxRules::EvaluateState(const FState& State) const
{
    float Value = 0.f;
    for (const FConnectItMinMaxEvalTerm* Term : ActiveEvaluationTerms)
    {
        Value += Term->Weight * Term->Evaluate(*this, State.Board, State.SideToMove);
    }

    // Keep heuristic values clear of the win band
    const float Limit = static_cast<float>(WinValue / 2);
    return FMath::RoundToInt(FMath::Clamp(Value, -Limit, Limit));
}

int32 FConnectItMinMaxRules::EvaluateMove(const FState& State, const FMove& Move) const
{
    float Score = 0.f;
    for (const FConnectItMinMaxOrderTerm* Term : ActiveOrderingTerms)
    {
        Score += Term->Weight * Term->Score(*this, State.Board, Move.TileIndex, State.SideToMove);
    }
    return FMath::RoundToInt(Score);
}
