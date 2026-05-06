// Fill out your copyright notice in the Description page of Project Settings.


#include "MinMax/ConnectIt_MinMaxTreeBuilder.h"
#include "MinMax/MinMaxABPruning.h"


void UConnectItTreeBuilder::BuildTreeAsync(const FConnectItMinMaxNodeExposed& InRootNode, int32 MaxDepth)
{
	// UE::Tasks::Launch(
	// 	UE_SOURCE_LOCATION,
	// 	[this, InRootNode, MaxDepth]() mutable
	// 	{
	// 		// Block until tree is built then solve on the same background thread
	// 		// FConnectItMinMaxNode BuiltRoot = BuildTreeAsync(RootNode, MaxDepth).GetResult();
	// 		FConnectItMinMaxNode BuiltNode = BuildNodeRecursive<FConnectItMinMaxNode>(
	// 			InRootNode,
	// 			0,
	// 			MaxDepth);
	//
	// 		// Marshal back to game thread — only safe place to touch UProperties
	// 		AsyncTask(ENamedThreads::GameThread, [this, BuiltNode]()
	// 		{
	// 			RootNode = BuiltNode;
	// 			if (OnBuildTreeComplete.IsBound()) { OnBuildTreeComplete.Broadcast(); }
	// 		});
	// 	},
	// 	UE::Tasks::ETaskPriority::BackgroundNormal
	// );

	// convert exposed node to C++ node
	FConnectItMinMaxNode StartNode;
	StartNode.Tiles = InRootNode.Tiles;
	StartNode.ScoreBoard = InRootNode.ScoreBoard;
	StartNode.FactionTurn = InRootNode.FactionTurn;
	StartNode.MovePlayed = InRootNode.MovePlayed;
	
	TWeakObjectPtr<UConnectItTreeBuilder> WeakThis(this);

	UE::Tasks::Launch(
		UE_SOURCE_LOCATION,
		[WeakThis, StartNode, MaxDepth]() mutable
		{
			if (!WeakThis.IsValid()) return;

			FConnectItMinMaxNode BuiltNode = BuildNodeRecursive<FConnectItMinMaxNode>(
				StartNode,
				0,
				MaxDepth,
				[WeakThis](const FConnectItMinMaxNode& Node) -> bool
				{
					if (!WeakThis.IsValid()) return false;
					return WeakThis->IsGameOver(Node);
				}
			);

			UE_LOG(LogTemp, Error,
				TEXT("BuildTreeAsync: BuiltNode has %d children before marshal"),
				BuiltNode.GetChildren().Num());

			// Shared pointer to transfer ownership safely across the lambda boundary
			TSharedPtr<FConnectItMinMaxNode> SharedNode =
				MakeShared<FConnectItMinMaxNode>(MoveTemp(BuiltNode));

			AsyncTask(ENamedThreads::GameThread, [WeakThis, SharedNode]() mutable
			{
				if (!WeakThis.IsValid()) return;

				UE_LOG(LogTemp, Error,
					TEXT("BuildTreeAsync: SharedNode has %d children on game thread"),
					SharedNode->GetChildren().Num());

				WeakThis->RootNode = MoveTemp(*SharedNode);

				UE_LOG(LogTemp, Error,
					TEXT("BuildTreeAsync: RootNode has %d children after assignment"),
					WeakThis->RootNode.GetChildren().Num());

				if (WeakThis->OnBuildTreeComplete.IsBound())
				{
					WeakThis->OnBuildTreeComplete.Broadcast();
				}
			});
		},
		UE::Tasks::ETaskPriority::BackgroundNormal
	);
}

void UConnectItTreeBuilder::SolveTreeAsync(bool bIsMaximisingPlayer) const
{
	// Store the task handle — keeps the task alive
	TWeakObjectPtr<const UConnectItTreeBuilder> WeakThis(this);

    UE::Tasks::Launch(
        UE_SOURCE_LOCATION,
        [WeakThis, bIsMaximisingPlayer]() mutable
        {
            if (!WeakThis.IsValid()) return;

        	TArray<FConnectItMoveOutcome> MoveOutcomes;
	        
            {
                FScopeLock Lock(&WeakThis->RootNodeMutex);

                if (!WeakThis.IsValid()) return;

                for (const FConnectItMinMaxNode& Child : WeakThis->RootNode.GetChildren())
                {
                    const int32 Score = MinMaxABPruningAlgorithm::Solve<FConnectItMinMaxNode>(
                        Child,
                        !bIsMaximisingPlayer,
                        TNumericLimits<int32>::Min(),
                        TNumericLimits<int32>::Max(),
                        [WeakThis](const FConnectItMinMaxNode& Node) -> bool
                        {
                        	if (!WeakThis.IsValid()) return false;
                        	return WeakThis->IsGameOver(Node);
                        },
                        [WeakThis](const FConnectItMinMaxNode& Node) -> int32
                        {
                        	if (!WeakThis.IsValid()) return 0;
                        	return WeakThis->Evaluate(Node);
                        }
                    );

                	FConnectItMoveOutcome Outcome;
					Outcome.GridPosition = Child.MovePlayed;
					Outcome.Score = Score;
					MoveOutcomes.Add(MoveTemp(Outcome));
                }
            }
        	
            AsyncTask(ENamedThreads::GameThread, [WeakThis, MoveOutcomes]() mutable
            {
            	if (!WeakThis.IsValid()) return;

				WeakThis->RootNodeMoveOutcomes = MoveTemp(MoveOutcomes);
				if (WeakThis->OnSolveTreeComplete.IsBound())
				{
					WeakThis->OnSolveTreeComplete.Broadcast();
				}
            });
        },
        UE::Tasks::ETaskPriority::BackgroundNormal
    );
}

// helps to early exit from algo so we do not evaluate child nodes past game over positions
bool UConnectItTreeBuilder::IsGameOver(const FConnectItMinMaxNode& InNode) const
{
	return InNode.ScoreBoard.ContainsByPredicate([](const float& Score)
	{
		return Score >= Connect_It_Score_Max;
	});
}

// need to come up with some metrics to help give a sense of a good position with
// respect to the score of the game
int32 UConnectItTreeBuilder::Evaluate(const FConnectItMinMaxNode& InNode) const
{
	// check for winning score and send
	for (int32 Index = 0; Index < InNode.ScoreBoard.Num(); Index++)
	{
		if (InNode.ScoreBoard[Index] >= Connect_It_Score_Max)
		{
			if (Index == InNode.FactionTurn)
			{
				return Connect_It_Score_Max;
			}

			return -Connect_It_Score_Max;
		}		
	}

	// find number of pieces for faction
	int32 NumberOfPieces = 0;
	for (const auto& [GridPosition, Faction] : InNode.Tiles)
	{
		if (InNode.Tiles[GridPosition].FactionPiece == InNode.FactionTurn)
		{
			NumberOfPieces++;
		}
	}

	// return the improvement to faction turns score and give one point for piece count
	int32 OutScore = (InNode.ScoreBoard[InNode.FactionTurn] - RootNode.ScoreBoard[InNode.FactionTurn]) + NumberOfPieces;

	// clamp so not to give perfect score
	return FMath::Clamp(OutScore, 0, Connect_It_Score_Max - 1);

}