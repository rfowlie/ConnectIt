// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/Task.h"
#include "GridMechanicsBaseStructs.h"
#include "GridMechanics_ShapeLibrary.h"
#include "ConnectIt_MinMaxTreeBuilder.generated.h"


USTRUCT(BlueprintType)
struct FConnectItTileRepresentation
{
	GENERATED_BODY()

	FConnectItTileRepresentation() = default;
	FConnectItTileRepresentation(const float InMultiplier, const int32 InFactionPiece) :
	Multiplier(InMultiplier), FactionID(InFactionPiece) {}
	
	UPROPERTY(BlueprintReadWrite)
	float Multiplier = 1;

	// the owner of the active piece on this tile, zero meaning no piece
	UPROPERTY(BlueprintReadWrite)
	int32 FactionID = 0;
};

/*
 * Blueprint facing version of FConnectItMinMaxNode
 */
USTRUCT(BlueprintType)
struct FConnectItMinMaxNodeStruct
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

/*
 * C++ representation of ConnectItGameState for min max
 */
struct FConnectItMinMaxNode
{
	FConnectItMinMaxNode() = default;
	FConnectItMinMaxNode(const FConnectItMinMaxNode&) = default;
	FConnectItMinMaxNode& operator=(const FConnectItMinMaxNode&) = default;
	
	TMap<FGridPosition, FConnectItTileRepresentation> Tiles;
	FGridPosition MovePlayed;
	TArray<float> ScoreBoard;
	
	// The faction whose turn was played
	int32 FactionTurn = 0;	

	// const is required to satisfy the CMinMaxNode concept
	const TArray<FConnectItMinMaxNode>& GetChildren() const
	{
		return Children;
	}

	void SetChildren(TArray<FConnectItMinMaxNode>&& InChildren)
	{
		Children = MoveTemp(InChildren);
	}

	/*
	 * Manual Constraints - max children per node, beam width, depth scaling
	 * Game Constraints - symmetry pruning, Influence zones, forced move detection, threat detection,
	 * historical move filtering, transpoition detection
	 *
	 */
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
			if (Tile.FactionID != -1) continue;

			FConnectItMinMaxNode ChildNode;
			ChildNode.FactionTurn = NextFactionTurn;
			ChildNode.ScoreBoard  = ScoreBoard;
			ChildNode.MovePlayed  = GridPosition;
			ChildNode.Tiles       = Tiles;

			ChildNode.Tiles[GridPosition].FactionID = NextFactionTurn;
			ChildNode.UpdateBoardIfMoveScoring();

			NewChildren.Add(MoveTemp(ChildNode));
		}

		// Use SetChildren so GetChildren() returns correctly in BuildNodeRecursive
		SetChildren(MoveTemp(NewChildren));
	}

	void UpdateBoardIfMoveScoring()
	{
		// get grid positions controlled by faction turn 
		TArray<FGridPosition> InGridPositions;
		for (const auto& [GridPosition, Faction] : Tiles)
		{
			if (Tiles[GridPosition].FactionID == FactionTurn)
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
				Tiles[GridPosition].FactionID = -1;
				Tiles[GridPosition].Multiplier += 1.0f;
			}
		}

		// place move piece back on board in accordance with game logic
		Tiles[MovePlayed].FactionID = FactionTurn;
		// update scoreboard
		ScoreBoard[FactionTurn] += MoveScore;
	}

private:
	TArray<FConnectItMinMaxNode> Children;
	
};

// CombatTypes.h or ConnectItTypes.h
USTRUCT(BlueprintType)
struct FConnectItMoveOutcome
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition GridPosition;

    UPROPERTY(BlueprintReadWrite)
    int32 Score = 0;
};

// Delegate fired on the game thread when the tree is fully built
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTreeBuilt);

UCLASS(BlueprintType)
class CONNECTIT_API UConnectItTreeBuilder : public UObject
{
    GENERATED_BODY()
    
public:
    
    UPROPERTY(BlueprintAssignable)
    FOnTreeBuilt OnBuildTreeComplete;
    
    UPROPERTY(BlueprintAssignable)
    FOnTreeBuilt OnSolveTreeComplete;

    UFUNCTION(BlueprintCallable)
    void BuildTreeAsync(const FConnectItMinMaxNodeStruct& InRootNode, int32 MaxDepth);
    UFUNCTION(BlueprintCallable)
    void SolveTreeAsync(bool bIsMaximisingPlayer) const;

    bool IsGameOver(const FConnectItMinMaxNode& InNode) const;
    int32 Evaluate(const FConnectItMinMaxNode& InNode) const;

    UFUNCTION(BlueprintCallable)
    TArray<FConnectItMoveOutcome> GetMoveSuggestions() const { return RootNodeMoveOutcomes; }
    
private:
    
    mutable FCriticalSection RootNodeMutex;
    
    mutable FConnectItMinMaxNode RootNode;

    UPROPERTY()
    mutable TArray<FConnectItMoveOutcome> RootNodeMoveOutcomes;
    
};