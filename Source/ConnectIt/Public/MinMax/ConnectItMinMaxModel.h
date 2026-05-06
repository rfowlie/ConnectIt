// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GridMechanics_Structs.h"
#include "GridMechanics_ShapeLibrary.h"
#include "ConnectItMinMaxModel.generated.h"


USTRUCT(BlueprintType)
struct FConnectItTileRepresentation
{
	GENERATED_BODY()

	FConnectItTileRepresentation() = default;
	FConnectItTileRepresentation(const float InMultiplier, const int32 InFactionPiece) :
	Multiplier(InMultiplier), FactionPiece(InFactionPiece) {}
	
	UPROPERTY(BlueprintReadWrite)
	float Multiplier = 1;

	// the owner of the active piece on this tile, zero meaning no piece
	UPROPERTY(BlueprintReadWrite)
	int32 FactionPiece = 0;
};

USTRUCT(BlueprintType)
struct FConnectItMinMaxNodeExposed
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TMap<FGridPosition, FConnectItTileRepresentation> Tiles;

	UPROPERTY(BlueprintReadWrite)
	TArray<float> ScoreBoard;
	
	// The faction whose turn was played, zero meaning none
	UPROPERTY(BlueprintReadWrite)
	int32 FactionTurn = 0;

	UPROPERTY(BlueprintReadWrite)
	FGridPosition MovePlayed;
	
};

struct FConnectItMinMaxNode
{
	FConnectItMinMaxNode() = default;
	FConnectItMinMaxNode(const FConnectItMinMaxNode&) = default;
	FConnectItMinMaxNode& operator=(const FConnectItMinMaxNode&) = default;
	
	TMap<FGridPosition, FConnectItTileRepresentation> Tiles;
	
	TArray<float> ScoreBoard;
	
	// The faction whose turn was played, zero meaning none
	int32 FactionTurn = 0;
	
	FGridPosition MovePlayed;

	// const is required to satisfy the CMinMaxNode concept
	const TArray<FConnectItMinMaxNode>& GetChildren() const
	{
		return Children;
	}

	void SetChildren(TArray<FConnectItMinMaxNode>&& InChildren)
	{
		Children = MoveTemp(InChildren);
	}

	void GenerateChildren()
	{
		// Guard against uninitialised ScoreBoard
		if (ScoreBoard.IsEmpty())
		{
			UE_LOG(LogTemp, Error,
				TEXT("GenerateChildren: ScoreBoard is empty — root node not initialised correctly"));
			return;
		}

		const int32 NextFactionTurn = (FactionTurn + 1) % ScoreBoard.Num();
		TArray<FConnectItMinMaxNode> NewChildren;

		for (const auto& [GridPosition, Tile] : Tiles)
		{
			// Only consider empty tiles (-1 == empty)
			if (Tile.FactionPiece != -1) continue;

			FConnectItMinMaxNode ChildNode;
			ChildNode.FactionTurn = NextFactionTurn;
			ChildNode.ScoreBoard  = ScoreBoard;
			ChildNode.MovePlayed  = GridPosition;
			ChildNode.Tiles       = Tiles;

			ChildNode.Tiles[GridPosition].FactionPiece = NextFactionTurn;
			ChildNode.UpdateBoardIfMoveScoring();

			NewChildren.Add(MoveTemp(ChildNode));
		}

		// Use SetChildren so GetChildren() returns correctly in BuildNodeRecursive
		SetChildren(MoveTemp(NewChildren));
	}

	void UpdateBoardIfMoveScoring()
	{
		// get grid positions controlled by faction turn 
		TSet<FGridPosition> InGridPositions;
		for (const auto& [GridPosition, Faction] : Tiles)
		{
			if (Tiles[GridPosition].FactionPiece == FactionTurn)
			{
				InGridPositions.Add(GridPosition);
			}
		}

		// call for all lines that are now possible from move played
		TArray<TArray<FGridPosition>> Outlines;
		UGridMechanics_ShapeLibrary::GetLongestLines(
			Outlines, InGridPositions, this->MovePlayed, 4);

		float MoveScore = 0;
		for (auto Line : Outlines)
		{
			for (auto GridPosition : Line)
			{
				MoveScore += Tiles[GridPosition].Multiplier;
				Tiles[GridPosition].FactionPiece = -1;
				Tiles[GridPosition].Multiplier += 1.0f;
			}
		}

		// place move piece back on board in accordance with game logic
		Tiles[MovePlayed].FactionPiece = FactionTurn;
		// update scoreboard
		ScoreBoard[FactionTurn] += MoveScore;
	}

private:
	TArray<FConnectItMinMaxNode> Children;
	
};

/*
 * system to run a flexible minmax system
 * allow for custom depth and execution of search and eval in an async wayS
 */
UCLASS(BlueprintType)
class CONNECTIT_API UConnectItMinMaxModel : public UObject
{
	GENERATED_BODY()

// public:
//
// 	// ideally this will create a base node using its own control rather than something being passed in
// 	UFUNCTION(BlueprintCallable)
// 	void SetBaseNode(FConnectItMinMaxNode InGameBoardScenario);
//
// 	// with the currently explored and cached tree, provide a next move for the current board state
// 	UFUNCTION(BlueprintCallable)
// 	int32 MinMax(bool bIsMaximisingPlayer);
//
// protected:
// 	
// 	UFUNCTION()
// 	bool IsGameOver(const FConnectItMinMaxNode& StartNode);
//
// 	// evaluate function to be used with minmax algo
// 	UFUNCTION()
// 	int32 Evaluate(const FConnectItMinMaxNode& Node);
// 	
// 	// dequeue next node to be explored and create child nodes
// 	UFUNCTION()
// 	void ExploreNextNode();
//
// 	// create all child nodes of given node
// 	UFUNCTION()
// 	void CreateChildNodes(FConnectItMinMaxNode& InNode);
//
// 	// check board, if move scoring update changes to board and scoring
// 	UFUNCTION()
// 	void EvaluateBoard(FConnectItMinMaxNode& InGameBoardScenario);
//
// 	// UFUNCTION()
// 	// void CheckMoveIsScoring(FConnectItMinMaxNode& InGameBoardScenario);
// 	//
// 	// UFUNCTION()
// 	// void UpdateBoardFromScoringMove(FConnectItMinMaxNode& InGameBoardScenario);
// 	
// 	UFUNCTION()
// 	int32 GetNextFactionIndex(int32 InFactionIndex) const;	
// 	
// 	UPROPERTY()
// 	FConnectItMinMaxNode BaseNode;
// 	
// 	TQueue<FConnectItMinMaxNode> NodesToExplore;
};