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

void UGridMechanics_ShapeLibrary::GetLongestLine2(
    TArray<FGridPosition>& OutGridPositions,
    const TSet<FGridPosition>& InGridPositions,
    const FGridPosition& StartingPosition,
    const EGridDirection GridDirection)
{
    OutGridPositions.Empty();

    if (!InGridPositions.Contains(StartingPosition)) return;

    const FGridDirectionVector Dir = GetDirectionVector(GridDirection);

    // Cap to input size — no line can be longer than the total positions
    const int32 MaxSteps = InGridPositions.Num();

    // Walk positive direction
    int32 PositiveExtent = 0;
    for (int32 Step = 1; Step <= MaxSteps; Step++)
    {
        const FGridPosition Offset(Step * Dir.Row, Step * Dir.Column);
        if (!InGridPositions.Contains(StartingPosition + Offset)) break;
        PositiveExtent = Step;
    }

    // Walk negative direction
    int32 NegativeExtent = 0;
    for (int32 Step = 1; Step <= MaxSteps; Step++)
    {
        const FGridPosition Offset(-Step * Dir.Row, -Step * Dir.Column);
        if (!InGridPositions.Contains(StartingPosition + Offset)) break;
        NegativeExtent = Step;
    }

    // Collect the full line — negative extent to positive extent inclusive
    OutGridPositions.Reserve(PositiveExtent + NegativeExtent + 1);

    for (int32 Index = -NegativeExtent; Index <= PositiveExtent; Index++)
    {
        OutGridPositions.Add(
            StartingPosition + FGridPosition(Index * Dir.Row, Index * Dir.Column));
    }
}

void UGridMechanics_ShapeLibrary::GetLongestLines(TArray<TArray<FGridPosition>>& OutLines,
	const TSet<FGridPosition>& InGridPositions, const FGridPosition& StartingPosition, const int32 MinimumLength)
{
	TArray AllGridDirections { EGridDirection::Column, EGridDirection::Row, EGridDirection::Diagonal_BottomUp, EGridDirection::Diagonal_TopDown };

	TArray<FGridPosition> TempGridPositions;
	for (const EGridDirection Direction : AllGridDirections)
	{
		TempGridPositions.Empty();
		GetLongestLine2(TempGridPositions, InGridPositions, StartingPosition, Direction);
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

void UGridMechanics_ShapeLibrary::GetLongestLine(
	TArray<FGridPosition>& OutGridPositions, const TArray<FGridPosition>& InGridPositions,
	const FGridPosition& StartingPosition, const EGridDirection GridDirection)
{
	// if (!InGridPositions.Contains(StartingPosition)) { return; }
	TMap<FGridPosition, bool> GridMap;
	for (auto GridPosition : InGridPositions) { GridMap.Add(GridPosition, false); }

	int32 RowDirectionMultiplier = 0;
	int32 ColumnDirectionMultiplier = 0;
	
	switch (GridDirection){
	case Row:
		ColumnDirectionMultiplier = 1;
		break;
	case Column:
		RowDirectionMultiplier = 1;
		break;
	case Diagonal_TopDown:
		RowDirectionMultiplier = 1;
		ColumnDirectionMultiplier = 1;
		break;
	case Diagonal_BottomUp:
		RowDirectionMultiplier = -1;
		ColumnDirectionMultiplier = 1;
		break;
	}
	
	int32 Positive = 0;
	for (int32 Index = 1; Index <= MAX_int32; Index++)
	{
		FGridPosition TempPosition (
			Index * RowDirectionMultiplier, Index * ColumnDirectionMultiplier);
		if (!GridMap.Contains(StartingPosition + TempPosition)) { break; }
		Positive = Index;
	}
	
	int32 Negative = 0;
	for (int32 Index = 1; Index <= MAX_int32; Index++)
	{
		FGridPosition TempPosition (
			-Index * RowDirectionMultiplier, -Index * ColumnDirectionMultiplier);
		if (!GridMap.Contains(StartingPosition + TempPosition)) { break; }
		Negative = -Index;
	}

	for (int32 Index = Negative; Index <= Positive; Index++)
	{
		OutGridPositions.Add(
			StartingPosition +
			FGridPosition(Index * RowDirectionMultiplier, Index * ColumnDirectionMultiplier));
	}	
}
