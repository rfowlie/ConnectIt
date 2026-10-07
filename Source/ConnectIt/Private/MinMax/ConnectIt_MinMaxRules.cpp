// Fill out your copyright notice in the Description page of Project Settings.

#include "MinMax/ConnectIt_MinMaxRules.h"
#include "Board/Rules/ConnectIt_LineScoringRule.h"
#include "ConnectIt_GameplayTags.h"


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

    // The same lookup as a flat table over the board's bounding box, for the
    // search (see TileIndexAt)
    if (NumTiles > 0)
    {
        int32 MaxX = TNumericLimits<int32>::Lowest();
        int32 MaxY = TNumericLimits<int32>::Lowest();
        Geometry.MinX = TNumericLimits<int32>::Max();
        Geometry.MinY = TNumericLimits<int32>::Max();
        for (int32 Index = 0; Index < NumTiles; Index++)
        {
            const FGridPosition Position = Board.GetPositionAt(Index);
            Geometry.MinX = FMath::Min(Geometry.MinX, Position.X);
            Geometry.MinY = FMath::Min(Geometry.MinY, Position.Y);
            MaxX = FMath::Max(MaxX, Position.X);
            MaxY = FMath::Max(MaxY, Position.Y);
        }

        Geometry.Width = MaxX - Geometry.MinX + 1;
        Geometry.Height = MaxY - Geometry.MinY + 1;
        Geometry.TileIndexGrid.Init(INDEX_NONE, Geometry.Width * Geometry.Height);
        for (int32 Index = 0; Index < NumTiles; Index++)
        {
            const FGridPosition Position = Board.GetPositionAt(Index);
            Geometry.TileIndexGrid[(Position.Y - Geometry.MinY) * Geometry.Width + (Position.X - Geometry.MinX)] = Index;
        }
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
    const FConnectItMinMaxEvaluationWeights& InEvaluationWeights,
    const FConnectItMinMaxOrderingWeights& InOrderingWeights,
    const FGameplayTagContainer& Side0RequestTypes,
    const FGameplayTagContainer& Side1RequestTypes)
    : Rules(InRules)
    , WinCondition(Rules.GetWinCondition())
    , Geometry(FConnectItMinMaxGeometry::Build(
        Board,
        Rules.GetScoringRuleAs<FConnectItScoringRule_Lines>()
            ? Rules.GetScoringRuleAs<FConnectItScoringRule_Lines>()->ConnectLength
            : 0))
    , EvaluationWeights(InEvaluationWeights)
    , OrderingWeights(InOrderingWeights)
{
    if (!Rules.GetScoringRule() || !WinCondition)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_MinMaxRules: the match's rule set is missing a "
                 "rule (scoring %s, win condition %s) -- the AI will play as "
                 "if that rule did nothing"),
            Rules.GetScoringRule() ? TEXT("ok") : TEXT("MISSING"),
            WinCondition ? TEXT("ok") : TEXT("MISSING"));
    }

    // Each side's kinds of move: the ones the search can model (see
    // FConnectItMinMaxMove) that the side was given the request type for
    bSideCanPlace[0] = Side0RequestTypes.HasTagExact(ConnectIt_Game_PlacePiece);
    bSideCanPlace[1] = Side1RequestTypes.HasTagExact(ConnectIt_Game_PlacePiece);
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
    if (!ensure(State.SideToMove == 0 || State.SideToMove == 1)) return;

    if (bSideCanPlace[State.SideToMove])
    {
        FConnectItBoardOperation_PlacePiece::ForEachMove(State.Board, State.SideToMove,
            [&OutMoves](const FConnectItBoardOperation_PlacePiece& Operation)
            {
                OutMoves.Emplace(TInPlaceType<FConnectItBoardOperation_PlacePiece>(), Operation);
            });
    }
}

FConnectItMinMaxRules::FState FConnectItMinMaxRules::ApplyMove(
    const FState& State, const FMove& Move) const
{
    FState Child = State;

    // 1. The move: the operation changes the board, and only that
    FConnectItTouchedPositions TouchedPositions;
    bool bArrivingPiecesSurvive = true;
    Visit([&Child, &TouchedPositions, &bArrivingPiecesSurvive](const auto& Operation)
    {
        Operation.Apply(Child.Board, TouchedPositions, nullptr);
        bArrivingPiecesSurvive = Operation.ArrivingPiecesSurviveScoring();
    }, Move);

    // 2. What follows from the change -- the same step the Mediator runs
    //    after a real move
    Rules.ResolveBoardChange(Child.Board, TouchedPositions, bArrivingPiecesSurvive);

    // 3. The turn passes. The one thing still assumed here: every move ends
    //    the turn.
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
    const int32 Side = State.SideToMove;
    const int32 Other = Opponent(Side);

    float Value = 0.f;

    if (EvaluationWeights.ScoreDifference != 0.f)
    {
        Value += EvaluationWeights.ScoreDifference * ScoreDifference(State.Board, Side);
    }

    // Mine minus theirs. Skipped entirely when unweighted: it walks every
    // line window on the board.
    if (EvaluationWeights.LinePotential != 0.f)
    {
        Value += EvaluationWeights.LinePotential
            * (LinePotential(State.Board, Side) - LinePotential(State.Board, Other));
    }

    // Keep heuristic values clear of the win band
    const float Limit = static_cast<float>(WinValue / 2);
    return FMath::RoundToInt(FMath::Clamp(Value, -Limit, Limit));
}

int32 FConnectItMinMaxRules::EvaluateMove(const FState& State, const FMove& Move) const
{
    // Placing a piece: judged by the tile it goes on
    if (const FConnectItBoardOperation_PlacePiece* Place = Move.TryGet<FConnectItBoardOperation_PlacePiece>())
    {
        const int32 TileIndex = Geometry.TileIndexAt(Place->Position);
        if (TileIndex == INDEX_NONE) return 0;

        return FMath::RoundToInt(
            OrderingWeights.TileMultiplier * TileMultiplierAt(State.Board, TileIndex)
            + OrderingWeights.AdjacentPieces * CountAdjacentPieces(State.Board, TileIndex));
    }

    return 0;
}

// --- The factors ---

float FConnectItMinMaxRules::ScoreDifference(const FConnectItBoardState& Board, int32 Side)
{
    return Board.GetScore(Side) - Board.GetScore(Opponent(Side));
}

float FConnectItMinMaxRules::LinePotential(const FConnectItBoardState& Board, int32 Side) const
{
    float Potential = 0.f;

    for (const TArray<int32>& Window : Geometry.LineWindows)
    {
        int32 Pieces = 0;
        float Multiplier = 0.f;
        bool bOpen = true;

        for (const int32 Index : Window)
        {
            const FConnectItTileData& Tile = Board.GetTileDataAt(Index);
            if (Tile.FactionPiece == Side)
            {
                Pieces++;
            }
            else if (!Tile.bIsActive || Tile.bIsOccupied)
            {
                // Inactive, the other faction's piece, or a non-faction blocker
                bOpen = false;
                break;
            }

            Multiplier += Tile.Multiplier;
        }

        if (bOpen && Pieces > 0)
        {
            Potential += static_cast<float>(Pieces * Pieces) * Multiplier;
        }
    }

    return Potential;
}

float FConnectItMinMaxRules::TileMultiplierAt(const FConnectItBoardState& Board, int32 TileIndex)
{
    return Board.TileDataArray.IsValidIndex(TileIndex)
        ? Board.GetTileDataAt(TileIndex).Multiplier
        : 0.f;
}

int32 FConnectItMinMaxRules::CountAdjacentPieces(const FConnectItBoardState& Board, int32 TileIndex) const
{
    if (!Geometry.Neighbours.IsValidIndex(TileIndex)) return 0;

    int32 Count = 0;
    for (const int32 NeighbourIndex : Geometry.Neighbours[TileIndex])
    {
        if (Board.GetTileDataAt(NeighbourIndex).FactionPiece != -1)
        {
            Count++;
        }
    }
    return Count;
}
