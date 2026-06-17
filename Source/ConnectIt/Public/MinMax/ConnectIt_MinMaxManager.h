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

	// Default functions
	static FMinMaxNode ConvertNode(const FConnectItMinMaxNodeStruct& Node);
	
	static void UpdateBoardIfMoveScoring(FMinMaxNode& Node);
	static FMinMaxNode BuildChildNode(const FMinMaxNode& Parent, const FGridPosition& GridPosition);

	static bool DefaultIsGameOver(const FMinMaxNode& Node);
	static TArray<FMinMaxNode> DefaultBuildChildNodes(const FMinMaxNode& Parent);
	static float DefaultMoveOrder(const FMinMaxNode& Node);
	static float DefaultEvaluator(const FMinMaxNode& InNode, const FMinMaxNode& RootNode);
	
}

USTRUCT(BlueprintType)
struct FGridPositionScore
{
	GENERATED_BODY()

	FGridPositionScore() = default;
	FGridPositionScore(const FGridPosition& InGridPosition, const float InScore) :
		GridPosition(InGridPosition), Score(InScore) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGridPosition GridPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Score = 0;
		
};

USTRUCT(BlueprintType)
struct FMoveScoreInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HighestScore = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridPositionScore> MovesWithHighestScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridPositionScore> MoveScores;
	
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FConnectItMinMaxDelegate);

/*
 * 
 */
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_MinMaxManager : public UObject
{
	GENERATED_BODY()

public:
	UConnectIt_MinMaxManager();
	
	UPROPERTY(BlueprintAssignable)
	FConnectItMinMaxDelegate OnTreeBuilt;
    
	UPROPERTY(BlueprintAssignable)
	FConnectItMinMaxDelegate OnTreeEvaluated;

	UFUNCTION(BlueprintCallable)
	void BuildTree(const FConnectItMinMaxNodeStruct& InRootNode, int32 MaxDepth);

	UFUNCTION(BlueprintCallable)
	void EvaluateTree(const bool bIsMaximising);
	
	// UFUNCTION(BlueprintCallable)
	// void BuildAndEvaluateTree(const FConnectItMinMaxNodeStruct& InRootNode, int32 MaxDepth, bool bIsMaximising);

	UFUNCTION(BlueprintCallable)
	FMoveScoreInfo GetMoveScoreInfo() const;
	
	UFUNCTION(BlueprintCallable)
	TArray<FGridPositionScore> GetMoveScores() const;
	
protected:	
	mutable ConnectIt::FMinMaxNode RootNode;
	
};
