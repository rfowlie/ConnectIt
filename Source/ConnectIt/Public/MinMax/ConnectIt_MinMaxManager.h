// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GridMechanics_Structs.h"
#include "ConnectIt_MinMaxTreeBuilder.h"
#include "ConnectIt_MinMaxManager.generated.h"


namespace ConnectIt
{
	// C++ representations of ConnectItGameState for min max
	struct FTileDescriptor
	{
		FTileDescriptor() = default;
		FTileDescriptor(const int32 InFactionPiece, const float InMultiplier) :
			FactionPiece(InFactionPiece), Multiplier(InMultiplier) {}

		// the owner of the active piece on this tile, (-1 meaning no piece)S
		int32 FactionPiece = -1;
		float Multiplier = 1;
		
	};

	struct FMinMaxNode
	{
		FMinMaxNode() = default;
		FMinMaxNode(const FMinMaxNode&) = default;
		FMinMaxNode& operator=(const FMinMaxNode&) = default;
		
		TMap<FGridPosition, FTileDescriptor> Tiles;
		FGridPosition MovePlayed;
		TArray<float> ScoreBoard;
		
		// The faction whose turn was played
		int32 FactionTurn = -1;	

		// const is required to satisfy the CMinMaxNode concept
		const TArray<FMinMaxNode>& GetChildren() const
		{
			return Children;
		}

		void SetChildren(TArray<FMinMaxNode>&& InChildren)
		{
			Children = MoveTemp(InChildren);
		}

		float GetScore() const
		{
			return Score;
		}

		void SetScore(const float InScore)
		{
			Score = InScore;
		}
		
	private:
		TArray<FMinMaxNode> Children;
		float Score = 0.0f;
		
	};

	// struct FMoveOutcome
	// {
	// 	FGridPosition GridPosition;
	// 	int32 Score = 0;
	// };

	// Default functions
	static FMinMaxNode ConvertNode(const FConnectItMinMaxNodeStruct& Node);
	
	static void UpdateBoardIfMoveScoring(FMinMaxNode& Node);
	static FMinMaxNode BuildChildNode(const FMinMaxNode& Parent, const FGridPosition& GridPosition);

	static bool DefaultIsGameOver(const FMinMaxNode& Node);
	static TArray<FMinMaxNode> DefaultBuildChildNodes(const FMinMaxNode& Parent);
	static float DefaultMoveOrder(const FMinMaxNode& Node);
	static float DefaultEvaluator(const FMinMaxNode& InNode, const FMinMaxNode& RootNode);
	
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConnectItMinMaxDelegate);

/*
 * 
 */
UCLASS()
class CONNECTIT_API UConnectIt_MinMaxManager : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FConnectItMinMaxDelegate OnTreeBuilt;
    
	UPROPERTY(BlueprintAssignable)
	FConnectItMinMaxDelegate OnTreeEvaluated;

	UFUNCTION(BlueprintCallable)
	void BuildTreeAsync(const FConnectItMinMaxNodeStruct& InRootNode, int32 MaxDepth);

	UFUNCTION(BlueprintCallable)
	void EvaluateTreeAsync(bool bIsMaximisingPlayer) const;

protected:	
	mutable ConnectIt::FMinMaxNode RootNode;

};
