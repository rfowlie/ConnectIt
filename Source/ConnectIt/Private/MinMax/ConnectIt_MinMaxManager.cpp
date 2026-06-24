// Fill out your copyright notice in the Description page of Project Settings.


#include "MinMax/ConnectIt_MinMaxManager.h"
#include "GridMechanics_GridLibrary.h"
#include "MinMax/MinMaxUtility.h"
#include "Library/ConnectIt_GameRulesLibrary.h"


ConnectIt::FMinMaxNode ConnectIt::ConvertNode(const FConnectItMinMaxNodeStruct& Node)
{
	ConnectIt::FMinMaxNode OutNode;
	for (auto Tile : Node.Tiles)
	{
		OutNode.Tiles.Add(Tile.Key, ConnectIt::FTileDescriptor(Tile.Value.FactionID, Tile.Value.Multiplier));
	}
	
	OutNode.FactionTurn = Node.FactionTurn;
	OutNode.ScoreBoard = Node.ScoreBoard;
	OutNode.MovePlayed = Node.MovePlayed;
	
	return OutNode;
}

void ConnectIt::UpdateBoardIfMoveScoring(FMinMaxNode& Node)
{
	// Get grid positions controlled by faction turn
	TArray<FGridPosition> InGridPositions;
	for (const auto& [GridPosition, Tile] : Node.Tiles)
	{
		if (Tile.FactionPiece == Node.FactionTurn)
		{
			InGridPositions.Add(GridPosition);
		}
	}

	// Call for all lines that are now possible from move played
	TArray<TArray<FGridPosition>> Outlines;
	UGridMechanics_ShapeLibrary::GetLongestLines(
		Outlines, InGridPositions, Node.MovePlayed, 4);

	float MoveScore = 0.f;
	for (const auto& Line : Outlines)
	{
		for (const FGridPosition& GridPosition : Line)
		{
			MoveScore += Node.Tiles[GridPosition].Multiplier;
			Node.Tiles[GridPosition].FactionPiece = -1;
			Node.Tiles[GridPosition].Multiplier   += 1.0f;
		}
	}

	// Place move piece back on board in accordance with game logic
	Node.Tiles[Node.MovePlayed].FactionPiece = Node.FactionTurn;

	// Update scoreboard
	Node.ScoreBoard[Node.FactionTurn] += MoveScore;
}

ConnectIt::FMinMaxNode ConnectIt::BuildChildNode(const FMinMaxNode& Parent, const FGridPosition& GridPosition)
{
	const int32 NextFactionTurn = (Parent.FactionTurn + 1) % Parent.ScoreBoard.Num();
		
	FMinMaxNode Child;
	Child.FactionTurn = NextFactionTurn;
	Child.ScoreBoard  = Parent.ScoreBoard;
	Child.MovePlayed  = GridPosition;
	Child.Tiles       = Parent.Tiles;

	Child.Tiles[GridPosition].FactionPiece = NextFactionTurn;
	UpdateBoardIfMoveScoring(Child);

	return Child;
}

bool ConnectIt::DefaultIsGameOver(const FMinMaxNode& Node)
{
	return UConnectIt_GameRulesLibrary::IsGameOver(Node.ScoreBoard);
}

TArray<ConnectIt::FMinMaxNode> ConnectIt::BuildAllChildNodes(const FMinMaxNode& Parent)
{
	// Guard against uninitialised ScoreBoard
	if (Parent.ScoreBoard.IsEmpty())
	{
		UE_LOG(LogTemp, Error,
			TEXT("BuildChildren: ScoreBoard is empty — root node not initialised correctly"));
		return {};
	}

	TArray<FMinMaxNode> Children;
	for (const auto& [GridPosition, TileDescription] : Parent.Tiles)
	{
		// Only consider empty tiles (-1 == empty)
		if (TileDescription.FactionPiece != -1) continue;

		FMinMaxNode Child = BuildChildNode(Parent, GridPosition);
		Children.Add(MoveTemp(Child));
	}

	return Children;
}

TArray<ConnectIt::FMinMaxNode> ConnectIt::BuildChildNodesFromConnectionsFloodMap(
	const FMinMaxNode& Parent, const int32 MaxChildNodes,	const TArray<TPair<int32, FGridPosition>>& ConnectionsFloodMap)
{
	// prevent negative numbers
	const int32 NumChildNodes = FMath::Max(1, MaxChildNodes);
	
	TArray<FMinMaxNode> Children;
	for (const auto& [Connections, GridPosition] : ConnectionsFloodMap)
	{
		if (Parent.Tiles.Contains(GridPosition))
		{
			if (Parent.Tiles[GridPosition].FactionPiece == -1)
			{
				FMinMaxNode Child = BuildChildNode(Parent, GridPosition);
				Children.Add(MoveTemp(Child));
				if (Children.Num() >= NumChildNodes) break;
			}
		}
	}
	
	return Children;
}


float ConnectIt::DefaultMoveOrder(const FMinMaxNode& Node)
{
	// assuming board is 7 x 7 and zero indexed
	return -(FMath::Abs(3 - Node.MovePlayed.X) + FMath::Abs(3 - Node.MovePlayed.Y));
}

float ConnectIt::DefaultEvaluator(const FMinMaxNode& InNode, const FMinMaxNode& RootNode)
{
	// check for winning score and send
	for (int32 Index = 0; Index < InNode.ScoreBoard.Num(); Index++)
	{
		if (InNode.ScoreBoard[Index] >= UConnectIt_GameRulesLibrary::ConnectIt_Score_Max)
		{
			if (Index == InNode.FactionTurn)
			{
				return UConnectIt_GameRulesLibrary::ConnectIt_Score_Max;
			}

			return -UConnectIt_GameRulesLibrary::ConnectIt_Score_Max;
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
	int32 OutScore = 0;
	OutScore += (InNode.ScoreBoard[InNode.FactionTurn] - RootNode.ScoreBoard[InNode.FactionTurn]) * 2;
	OutScore += NumberOfPieces;

	// clamp so not to give perfect score
	return FMath::Clamp(OutScore, 0, UConnectIt_GameRulesLibrary::ConnectIt_Score_Max - 1);
}

TArray<TPair<int32, FGridPosition>> ConnectIt::CreateConnectionsFloodMap(const FMinMaxNode& Parent)
{
	// find grid positions that do not have tiles
	TMap<int32, FGridPosition> PossibleConnectionsFloodMap;
	TArray<FGridPosition> GridPositions;
	Parent.Tiles.GetKeys(GridPositions);	
	return UGridMechanics_GridLibrary::CreateConnectionsFloodMap(GridPositions, 4);
}

UConnectIt_MinMaxManager::UConnectIt_MinMaxManager()
{
	// TODO: testing if this works...
	IsGameOverDelegate = ConnectIt::DefaultIsGameOver;
}

void UConnectIt_MinMaxManager::BuildTree(const FConnectItMinMaxNodeStruct& InRootNode, int32 MaxDepth)
{	
	// convert exposed node to C++ node
	ConnectIt::FMinMaxNode InitialNode = ConnectIt::ConvertNode(InRootNode);

	// create weak pointer for access safety
	TWeakObjectPtr WeakThis(this);

	// create flood maps
	auto ConnectionsFloodMap = CreateConnectionsFloodMap(InitialNode);
	
	UE::Tasks::Launch(
		UE_SOURCE_LOCATION,
		[WeakThis, InitialNode, MaxDepth, ConnectionsFloodMap]() mutable
		{
			if (!WeakThis.IsValid()) return;

				ConnectIt::FMinMaxNode BuiltNode = BuildNodeRecursive2<ConnectIt::FMinMaxNode>(
				InitialNode,
				0,
				MaxDepth,
				[WeakThis](const ConnectIt::FMinMaxNode& Node) -> bool
				{
					// return ConnectIt::DefaultIsGameOver(Node);
					
					// if (!WeakThis.IsValid()) return false;
					// return WeakThis->IsGameOver(Node);
					
					if (!WeakThis.IsValid()) return false;
					if (!WeakThis->IsGameOverDelegate) return false;
					return WeakThis->IsGameOverDelegate(Node);
				},
				[WeakThis, &ConnectionsFloodMap](const ConnectIt::FMinMaxNode& Node) -> TArray<ConnectIt::FMinMaxNode>
				{
					// return ConnectIt::BuildAllChildNodes(Node);
					return ConnectIt::BuildChildNodesFromConnectionsFloodMap(Node, 10, ConnectionsFloodMap);

				},
				[WeakThis](const ConnectIt::FMinMaxNode& Node) -> float
				{
					return ConnectIt::DefaultMoveOrder(Node);
				}
			);

			// Shared pointer to transfer ownership safely across the lambda boundary
			TSharedPtr<ConnectIt::FMinMaxNode> SharedNode = MakeShared<ConnectIt::FMinMaxNode>(MoveTemp(BuiltNode));

			AsyncTask(ENamedThreads::GameThread, [WeakThis, SharedNode]() mutable
			{
				if (!WeakThis.IsValid()) return;
				WeakThis->RootNode = MoveTemp(*SharedNode);
				if (WeakThis->OnTreeBuilt.IsBound()) { WeakThis->OnTreeBuilt.Broadcast(); }
			});
		},
		UE::Tasks::ETaskPriority::BackgroundNormal
	);
}

void UConnectIt_MinMaxManager::EvaluateTree(const bool bIsMaximising)
{	
	// Guard against evaluating a non-built tree
	if (RootNode.GetChildren().IsEmpty())
	{
		UE_LOG(LogTemp, Error,
			TEXT("EvaluateTreeAsync: RootNode has no children — call BuildTreeAsync first"));
		return;
	}

	TWeakObjectPtr WeakThis(this);

	// Snapshot the built tree into a shared pointer so the lambda owns it safely
	TSharedPtr<ConnectIt::FMinMaxNode> SharedNode = MakeShared<ConnectIt::FMinMaxNode>(RootNode);

	UE::Tasks::Launch(
		UE_SOURCE_LOCATION,
		[WeakThis, SharedNode, bIsMaximising]() mutable
		{
			if (!WeakThis.IsValid()) return;

			ConnectIt::FMinMaxNode EvaluatedNode = EvaluateNodeRecursive<ConnectIt::FMinMaxNode>(
				MoveTemp(*SharedNode),
				bIsMaximising,
				TNumericLimits<int32>::Min(),
				TNumericLimits<int32>::Max(),
				[SharedNode](const ConnectIt::FMinMaxNode& Node) -> float
				{
					if (!SharedNode.IsValid()) return 0;
					return ConnectIt::DefaultEvaluator(Node, *SharedNode);
				}
			);

			TSharedPtr<ConnectIt::FMinMaxNode> SharedEvaluatedNode = MakeShared<ConnectIt::FMinMaxNode>(MoveTemp(EvaluatedNode));

			// Marshal result back to game thread
			AsyncTask(ENamedThreads::GameThread, [WeakThis, SharedEvaluatedNode]() mutable
			{
				if (!WeakThis.IsValid()) return;
				WeakThis->RootNode = MoveTemp(*SharedEvaluatedNode);
				if (WeakThis->OnTreeEvaluated.IsBound()) { WeakThis->OnTreeEvaluated.Broadcast(); }
			});
		},
		UE::Tasks::ETaskPriority::BackgroundNormal
	);
}

FMoveScoreInfo UConnectIt_MinMaxManager::GetMoveScoreInfo() const
{
	FMoveScoreInfo Out;
	Out.MoveScores = GetMoveScores();
	if (Out.MoveScores.IsEmpty()) { return Out; }
	
	Out.MoveScores.Sort([](const FGridPositionScore& A, const FGridPositionScore& B)
	{
		return A.Score > B.Score;
	});

	Out.HighestScore = Out.MoveScores[0].Score;

	// Best moves are everything at the front that shares the highest score
	for (const FGridPositionScore& MoveScore : Out.MoveScores)
	{
		if (!FMath::IsNearlyEqual(MoveScore.Score, Out.HighestScore)) { break; }
		Out.MovesWithHighestScore.Add(MoveScore);
	}

	return Out;
}

TArray<FGridPositionScore> UConnectIt_MinMaxManager::GetMoveScores() const
{
	TArray<FGridPositionScore> Out;
	Out.Reserve(RootNode.GetChildren().Num());

	for (const auto& Node : RootNode.GetChildren())
	{
		Out.Add(FGridPositionScore(Node.MovePlayed, Node.GetScore()));
	}

	// Sort highest score first
	Algo::Sort(Out, [](const FGridPositionScore& A, const FGridPositionScore& B)
	{
		return A.Score > B.Score;
	});

	return Out;
}

bool UConnectIt_MinMaxManager::IsGameOver(const ConnectIt::FMinMaxNode& Node) const
{
	return ConnectIt::DefaultIsGameOver(Node);
}
