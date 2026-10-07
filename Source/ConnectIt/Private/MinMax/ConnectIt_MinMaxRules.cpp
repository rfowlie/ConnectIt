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

    const int32 WindowLength = FMath::Max(1, ConnectLength);
    const TArray<FGridDirectionVector>& Directions =
        UConnectIt_LineScoringRule::GetScoringDirections();

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

            if (Window.Num() == WindowLength)
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
    int32 InConnectLength,
    TSharedPtr<const FConnectItWinCheck, ESPMode::ThreadSafe> InWinCheck,
    const TArray<TInstancedStruct<FConnectItMinMaxEvalTerm>>& InEvaluationTerms,
    const TArray<TInstancedStruct<FConnectItMinMaxOrderTerm>>& InOrderingTerms)
    : ConnectLength(InConnectLength)
    , WinCheck(MoveTemp(InWinCheck))
    , Geometry(FConnectItMinMaxGeometry::Build(Board, InConnectLength))
    , EvaluationTerms(InEvaluationTerms)
    , OrderingTerms(InOrderingTerms)
{
    if (!WinCheck.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_MinMaxRules: no win check -- the level's win "
                 "condition can't be tested off the game thread, so the AI "
                 "won't see wins or losses coming"));
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
    return Root;
}

void FConnectItMinMaxRules::GenerateMoves(const FState& State, TArray<FMove>& OutMoves) const
{
    // Same test the default placeable rule (UConnectIt_UnoccupiedTilePlaceableRule)
    // makes: active and unoccupied. Checked by index -- no position lookups.
    const int32 NumTiles = State.Board.NumTiles();
    for (int32 Index = 0; Index < NumTiles; Index++)
    {
        const FConnectItTileData& Tile = State.Board.GetTileDataAt(Index);
        if (Tile.bIsActive && !Tile.bIsOccupied)
        {
            OutMoves.Add({ Index, State.Board.GetPositionAt(Index) });
        }
    }
}

FConnectItMinMaxRules::FState FConnectItMinMaxRules::ApplyMove(
    const FState& State, const FMove& Move) const
{
    // Mirrors UConnectIt_BoardRequestMediator::HandlePlacePieceRequest:
    // place the piece, then score from the placed position.
    FState Child = State;
    Child.Board.TileDataArray[Move.TileIndex].SetFactionPiece(State.SideToMove);

    TArray<FGridPosition> ScoringPositions; // visuals-only output, unused here
    UConnectIt_LineScoringRule::ApplyLineScoring(
        Child.Board, Move.Position, State.SideToMove, ConnectLength, ScoringPositions);

    Child.SideToMove = Opponent(State.SideToMove);
    return Child;
}

bool FConnectItMinMaxRules::IsTerminalState(const FState& State) const
{
    return WinCheck.IsValid() && WinCheck->GetWinningFaction(State.Board) != INDEX_NONE;
}

int32 FConnectItMinMaxRules::EvaluateTerminalState(const FState& State, int32 Ply) const
{
    const int32 Winner = WinCheck.IsValid()
        ? WinCheck->GetWinningFaction(State.Board)
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
