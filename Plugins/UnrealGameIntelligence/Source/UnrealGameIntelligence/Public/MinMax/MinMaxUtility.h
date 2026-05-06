// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// Concept that enforces the contract on TNode at compile time
// Any type passed as TNode must implement GetChildren() returning a TArray of itself
template<typename TNode>
concept c_min_max_node = requires(const TNode& Node)
{
	{ Node.GetChildren() } -> std::convertible_to<TArray<TNode>>;
};

// Extended concept — TNode must implement both GetChildren and GetOrderingScore
// Get Ordering Score returns a higher value for more promising nodes
template<typename TNode>
concept c_min_max_move_order = requires(const TNode& Node)
{
	{ Node.GetChildren() } -> std::convertible_to<TArray<TNode>>;
	{ Node.GetOrderingScore() } -> std::convertible_to<int32>;
};

template<typename TNode>
concept c_min_max_tree_builder = requires(const TNode& ConstNode, TNode& MutableNode, TArray<TNode> Children)
{
	{ ConstNode.GetChildren() }						-> std::same_as<const TArray<TNode>&>;
	{ MutableNode.SetChildren(MoveTemp(Children)) } -> std::same_as<void>;
	{ MutableNode.GenerateChildren() }				-> std::same_as<void>;
};

template<typename TNode> requires c_min_max_tree_builder<TNode>
static TNode BuildNodeRecursive(
	TNode Node,
	int32 CurrentDepth,
	int32 MaxDepth,
	TFunction<bool(const TNode&)> IsGameOver)
{
	// UE_LOG(LogTemp, Error,
	//    TEXT("BuildNodeRecursive: CurrentDepth %d MaxDepth %d"),
	//    CurrentDepth, MaxDepth);
	//
	// if (CurrentDepth >= MaxDepth)
	// {
	// 	UE_LOG(LogTemp, Warning,
	// 		TEXT("BuildNodeRecursive: Returning at depth %d — max depth reached"),
	// 		CurrentDepth);
	// 	return Node;
	// }
	//
	// if (CurrentDepth >= MaxDepth)
	// {
	// 	return Node;
	// }
	//
	// // Node generates and sets its own children directly
	// // No return value, no extra allocation
	// Node.GenerateChildren();
	//
	// // const TArray<TNode>& Children = Node.GetChildren();
	//
	// // Copy children out immediately — do not hold a reference into Node
	// // The reference would be invalidated when SetChildren is called later
	// TArray<TNode> Children = Node.GetChildren();
	//
	// if (Children.IsEmpty())
	// {
	// 	return Node;
	// }
	//
	// // TArray<UE::Tasks::TTask<TNode>> ChildTasks;
	// // ChildTasks.Reserve(Children.Num());
	// //
	// // for (const TNode& Child : Children)
	// // {
	// // 	UE::Tasks::TTask<TNode> ChildTask = UE::Tasks::Launch(
	// // 		UE_SOURCE_LOCATION,
	// // 		[Child, CurrentDepth, MaxDepth]() mutable -> TNode
	// // 		{
	// // 			return BuildNodeRecursive(Child, CurrentDepth + 1, MaxDepth);
	// // 		},
	// // 		UE::Tasks::ETaskPriority::BackgroundNormal
	// // 	);
	// //
	// // 	ChildTasks.Add(MoveTemp(ChildTask));
	// // }
	// //
	// // TArray<TNode> BuiltChildren;
	// // BuiltChildren.Reserve(ChildTasks.Num());
	// //
	// // for (UE::Tasks::TTask<TNode>& ChildTask : ChildTasks)
	// // {
	// // 	BuiltChildren.Add(ChildTask.GetResult());
	// // }
	//
	// // Build each child depth first on the same thread
	// // No parallel task dispatching inside the recursion
	// TArray<TNode> BuiltChildren;
	// BuiltChildren.Reserve(Children.Num());
	//
	// for (const TNode& Child : Children)
	// {
	// 	BuiltChildren.Add(BuildNodeRecursive(Child, CurrentDepth + 1, MaxDepth));
	// }
	//
	// Node.SetChildren(MoveTemp(BuiltChildren));
	//
	// return Node;

	// UE_LOG(LogTemp, Log,
	// 	TEXT("BuildNodeRecursive: CurrentDepth %d MaxDepth %d"),
	// 	CurrentDepth, MaxDepth);
	//
	// if (CurrentDepth >= MaxDepth)
	// {
	// 	UE_LOG(LogTemp, Log,
	// 		TEXT("BuildNodeRecursive: Depth %d — returning leaf"),
	// 		CurrentDepth);
	// 	return Node;
	// }
	
	if (IsGameOver(Node)) return Node;

	Node.GenerateChildren();
	TArray<TNode> Children = Node.GetChildren();

	// UE_LOG(LogTemp, Log,
	// 	TEXT("BuildNodeRecursive: Depth %d — generated %d children"),
	// 	CurrentDepth, Children.Num());

	if (Children.IsEmpty()) return Node;

	TArray<TNode> BuiltChildren;
	BuiltChildren.Reserve(Children.Num());

	for (const TNode& Child : Children)
	{
		TNode BuiltChild = BuildNodeRecursive(Child, CurrentDepth + 1, MaxDepth);

		// UE_LOG(LogTemp, Log,
		// 	TEXT("BuildNodeRecursive: Depth %d — BuiltChild returned with %d children"),
		// 	CurrentDepth, BuiltChild.GetChildren().Num());

		BuiltChildren.Add(MoveTemp(BuiltChild));
	}

	// UE_LOG(LogTemp, Log,
	// 	TEXT("BuildNodeRecursive: Depth %d — calling SetChildren with %d children"),
	// 	CurrentDepth, BuiltChildren.Num());

	Node.SetChildren(MoveTemp(BuiltChildren));

	// UE_LOG(LogTemp, Log,
	// 	TEXT("BuildNodeRecursive: Depth %d — Node has %d children after SetChildren"),
	// 	CurrentDepth, Node.GetChildren().Num());

	return Node;
}

// template<typename TNode> requires c_min_max_tree_builder<TNode>
// static TNode BuildNodeRecursive(
// 	TNode Node,
// 	int32 CurrentDepth,
// 	int32 MaxDepth,
// 	TFunction<TArray<TNode>(const TNode&)> GenerateChildren)
// {
// 	if (CurrentDepth >= MaxDepth)
// 	{
// 		return Node;
// 	}
//
// 	TArray<TNode> GeneratedChildren = GenerateChildren(Node);
//
// 	if (GeneratedChildren.IsEmpty())
// 	{
// 		return Node;
// 	}
//
// 	TArray<UE::Tasks::TTask<TNode>> ChildTasks;
// 	ChildTasks.Reserve(GeneratedChildren.Num());
//
// 	for (TNode& Child : GeneratedChildren)
// 	{
// 		UE::Tasks::TTask<TNode> ChildTask = UE::Tasks::Launch(
// 			UE_SOURCE_LOCATION,
// 			[Child, CurrentDepth, MaxDepth, GenerateChildren]() mutable -> TNode
// 			{
// 				return BuildNodeRecursive(Child, CurrentDepth + 1, MaxDepth, GenerateChildren);
// 			},
// 			UE::Tasks::ETaskPriority::BackgroundNormal
// 		);
//
// 		ChildTasks.Add(MoveTemp(ChildTask));
// 	}
//
// 	// Collect into a local array then move assign in one operation
// 	TArray<TNode> BuiltChildren;
// 	BuiltChildren.Reserve(ChildTasks.Num());
//
// 	for (UE::Tasks::TTask<TNode>& ChildTask : ChildTasks)
// 	{
// 		BuiltChildren.Add(ChildTask.GetResult());
// 	}
//
// 	Node.SetChildren(MoveTemp(BuiltChildren));
//
// 	return Node;
// }
