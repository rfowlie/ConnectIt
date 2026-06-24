// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMechanics_ShapeLibrary.h"
#include "GridMechanics_Structs.h"


FGridDirectionVector UGridMechanics_ShapeLibrary::GetDirectionVector(EGridDirection GridDirection)
{
    switch (GridDirection)
    {
        case EGridDirection::Row:				return {  0,  1 };
        case EGridDirection::Column:			return {  1,  0 };
        case EGridDirection::Diagonal_TopDown:	return {  1,  1 };
        case EGridDirection::Diagonal_BottomUp:	return { -1,  1 };
        default:								return {  0,  0 };
    }
}

void UGridMechanics_ShapeLibrary::GetLongestLines(TArray<TArray<FGridPosition>>& OutLines,
	const TArray<FGridPosition>& InGridPositions, const FGridPosition& StartingPosition, const int32 MinimumLength)
{
	TArray AllGridDirections
	{
		EGridDirection::Column,
		EGridDirection::Row,
		EGridDirection::Diagonal_BottomUp,
		EGridDirection::Diagonal_TopDown
	};

	TArray<FGridPosition> TempGridPositions;
	for (const EGridDirection Direction : AllGridDirections)
	{
		TempGridPositions.Empty();
		GetLongestLine(TempGridPositions, InGridPositions, StartingPosition, Direction);
		if (TempGridPositions.Num() >= MinimumLength)
		{
			OutLines.Add(TempGridPositions);
		}
	}
}

void UGridMechanics_ShapeLibrary::GetLinesOfLength(
    TArray<TArray<FGridPosition>>& OutLines,
    const TSet<FGridPosition>& InGridPositions,
    const EGridDirection GridDirection,
    const int32 RequiredLength,
    const bool bExactLength)
{
    OutLines.Empty();

    if (InGridPositions.IsEmpty() || RequiredLength <= 0) return;

    const FGridDirectionVector Dir = GetDirectionVector(GridDirection);
    const int32 MaxSteps = InGridPositions.Num();

    // Track positions we have already used as a start point
    // Prevents reporting the same line multiple times from different starting positions
    TSet<FGridPosition> VisitedStarts;

    for (const FGridPosition& Position : InGridPositions)
    {
        if (VisitedStarts.Contains(Position)) continue;

        // Walk negative direction to find the true start of this line
        // This ensures we always begin from the leftmost/topmost point
        // and never report the same line twice
        FGridPosition LineStart = Position;
        for (int32 Step = 1; Step <= MaxSteps; Step++)
        {
            const FGridPosition Candidate(
                LineStart.X - Dir.Row,
                LineStart.Y - Dir.Column);

            if (!InGridPositions.Contains(Candidate)) break;
            LineStart = Candidate;
        }

        // Already processed a line starting here
        if (VisitedStarts.Contains(LineStart)) continue;
        VisitedStarts.Add(LineStart);

        // Walk positive direction from the true start to build the full line
        TArray<FGridPosition> CurrentLine;
        CurrentLine.Add(LineStart);

        FGridPosition Current = LineStart;
        for (int32 Step = 1; Step <= MaxSteps; Step++)
        {
            const FGridPosition Next(
                Current.X + Dir.Row,
                Current.Y + Dir.Column);

            if (!InGridPositions.Contains(Next)) break;
            CurrentLine.Add(Next);
            Current = Next;
        }

        // Filter by length requirement
        if (bExactLength)
        {
            // Exact length — only report lines of exactly RequiredLength
            if (CurrentLine.Num() == RequiredLength)
            {
                OutLines.Add(MoveTemp(CurrentLine));
            }
        }
        else
        {
            // Minimum length — report any line of RequiredLength or longer
            // Optionally split into all valid sub-lines of exactly RequiredLength
            if (CurrentLine.Num() >= RequiredLength)
            {
                // Slide a window of RequiredLength across the full line
                // This reports every valid sub-line within a longer line
                for (int32 WindowStart = 0;
                    WindowStart <= CurrentLine.Num() - RequiredLength;
                    WindowStart++)
                {
                    TArray<FGridPosition> SubLine;
                    SubLine.Reserve(RequiredLength);

                    for (int32 i = WindowStart; i < WindowStart + RequiredLength; i++)
                    {
                        SubLine.Add(CurrentLine[i]);
                    }

                    OutLines.Add(MoveTemp(SubLine));
                }
            }
        }
    }
}

bool UGridMechanics_ShapeLibrary::IsSquare(const TArray<FGridPosition>& Positions, TArray<FGridPosition>& OutCorners)
{
	 OutCorners.Empty();
    
        if (Positions.Num() < 4) return false;
    
        // Build a set for O(1) lookup
        TSet<FGridPosition> PositionSet(Positions);
    
        // Find bounding box extents
        int32 MinX = TNumericLimits<int32>::Max();
        int32 MaxX = TNumericLimits<int32>::Min();
        int32 MinY = TNumericLimits<int32>::Max();
        int32 MaxY = TNumericLimits<int32>::Min();
    
        for (const FGridPosition& Position : Positions)
        {
            MinX = FMath::Min(MinX, Position.X);
            MaxX = FMath::Max(MaxX, Position.X);
            MinY = FMath::Min(MinY, Position.Y);
            MaxY = FMath::Max(MaxY, Position.Y);
        }
    
        const int32 Width  = MaxX - MinX;
        const int32 Height = MaxY - MinY;
    
        // Bounding box must be square and non-degenerate
        if (Width != Height || Width == 0) return false;
    
        // All 4 corners must exist in the position set
        const FGridPosition TopLeft     = FGridPosition(MinX, MinY);
        const FGridPosition TopRight    = FGridPosition(MaxX, MinY);
        const FGridPosition BottomLeft  = FGridPosition(MinX, MaxY);
        const FGridPosition BottomRight = FGridPosition(MaxX, MaxY);
    
        if (!PositionSet.Contains(TopLeft))     return false;
        if (!PositionSet.Contains(TopRight))    return false;
        if (!PositionSet.Contains(BottomLeft))  return false;
        if (!PositionSet.Contains(BottomRight)) return false;
    
        OutCorners.Add(TopLeft);
        OutCorners.Add(TopRight);
        OutCorners.Add(BottomLeft);
        OutCorners.Add(BottomRight);
    
        return true;
}

void UGridMechanics_ShapeLibrary::GetLongestLine(
	TArray<FGridPosition>& OutGridPositions, const TArray<FGridPosition>& InGridPositions,
	const FGridPosition& StartingPosition, const EGridDirection GridDirection)
{
	OutGridPositions.Empty();
	if (!InGridPositions.Contains(StartingPosition)) return;

	const FGridDirectionVector Dir = GetDirectionVector(GridDirection);

	// Walk positive direction
	int32 PositiveExtent = 0;
	while (true)
	{
		const FGridPosition Offset(PositiveExtent * Dir.Row, PositiveExtent * Dir.Column);
		if (!InGridPositions.Contains(StartingPosition + Offset)) { break; };
		PositiveExtent++;
	}	

	// Walk negative direction
	int32 NegativeExtent = 0;
	while (true)
	{
		const FGridPosition Offset(-NegativeExtent * Dir.Row, -NegativeExtent * Dir.Column);
		if (!InGridPositions.Contains(StartingPosition + Offset)) break;
		NegativeExtent++;
	}

	// Collect the full line — negative extent to positive extent inclusive
	OutGridPositions.Reserve(PositiveExtent + NegativeExtent + 1);

	for (int32 Index = -NegativeExtent; Index <= PositiveExtent; Index++)
	{
		OutGridPositions.Add(
			StartingPosition + FGridPosition(Index * Dir.Row, Index * Dir.Column));
	}
}
