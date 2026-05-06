// Fill out your copyright notice in the Description page of Project Settings.


#include "ConnectIt/Public/MinMax/ConnectItMinMaxModel.h"
#include "MinMax/MinMaxAlgorithm.h"


// void UConnectItMinMaxModel::SetBaseNode(FConnectItMinMaxNode InGameBoardScenario)
// {
// 	NodesToExplore.Empty();
// 	BaseNode = InGameBoardScenario;
// 	NodesToExplore.Enqueue(BaseNode);
// 	ExploreNextNode();
// }
//
// // calls templated minmax algo using concrete connectIt struct BaseNode
// int32 UConnectItMinMaxModel::MinMax(const bool bIsMaximisingPlayer)
// {		
// 	return MinMaxAlgorithm::Solve<FConnectItMinMaxNode>(
// 		BaseNode,
// 		bIsMaximisingPlayer,
// 		[this](const FConnectItMinMaxNode& Node) -> bool
// 		{
// 			return IsGameOver(Node);
// 		},
// 		[this](const FConnectItMinMaxNode& Node) -> int32
// 		{
// 			return Evaluate(Node);
// 		}
// 	);
// }
//
// bool UConnectItMinMaxModel::IsGameOver(const FConnectItMinMaxNode& StartNode)
// {
// 	for (const auto Score : StartNode.ScoreBoard)
// 	{
// 		if (Score >= 100) { return true; }
// 	}
// 	
// 	return false;
// }
//
// int32 UConnectItMinMaxModel::Evaluate(const FConnectItMinMaxNode& Node)
// {
// 	return 0;
// }
//
// void UConnectItMinMaxModel::ExploreNextNode()
// {
// 	if (NodesToExplore.IsEmpty()) { return; }
// 	FConnectItMinMaxNode OutScenario;
// 	NodesToExplore.Dequeue(OutScenario);		
// 	CreateChildNodes(OutScenario);
//
// 	// add children to explore list
// 	for (auto Scenario : OutScenario.GetChildren())
// 	{
// 		NodesToExplore.Enqueue(Scenario);
// 	}	
// }
//
// void UConnectItMinMaxModel::CreateChildNodes(FConnectItMinMaxNode& InNode)
// {
// 	// because the search might be async we want to find the next player this way
// 	const int32 FactionTurn = GetNextFactionIndex(InNode.FactionTurn);
// 	
// 	TArray<FGridPosition> GridPositions;
// 	InNode.Tiles.GetKeys(GridPositions);
// 	for (auto GridPosition : GridPositions)
// 	{
// 		// look for empty tiles
// 		if (const FConnectItTileRepresentation Tile = InNode.Tiles[GridPosition]; Tile.FactionPiece == 0)
// 		{
// 			// copy all info and then update changes
// 			FConnectItMinMaxNode Scenario = InNode;
// 			Scenario.FactionTurn = FactionTurn;
// 			Scenario.MovePlayed = GridPosition;
// 			Scenario.Tiles[GridPosition] = FConnectItTileRepresentation(Tile.Multiplier, FactionTurn);
//
// 			// check if move scored and update values accordingly
// 			EvaluateBoard(Scenario);
//
// 			// add as child of in scenario
// 			// InNode.Children.Add(Scenario);
// 		}		
// 	}
// }
//
// void UConnectItMinMaxModel::EvaluateBoard(FConnectItMinMaxNode& InGameBoardScenario)
// {
// 	
// }
//
// int32 UConnectItMinMaxModel::GetNextFactionIndex(int32 InFactionIndex) const
// {
// 	return (InFactionIndex + 1) % 2;
// }
