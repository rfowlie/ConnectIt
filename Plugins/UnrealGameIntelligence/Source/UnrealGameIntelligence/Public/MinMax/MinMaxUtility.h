// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


template<typename TNode>
concept c_min_max_node = requires(const TNode& Node)
{
	{ Node.GetChildren() } -> std::convertible_to<TArray<TNode>>;
};

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

// single threaded
template<typename TNode> requires c_min_max_tree_builder<TNode>
static TNode BuildNodeRecursive(
	TNode Node,
	const int32 CurrentDepth,
	const int32 MaxDepth,
	TFunction<bool(const TNode&)> IsGameOver)
{	
	if (CurrentDepth >= MaxDepth) { return Node; }	
	if (IsGameOver(Node)) { return Node; }

	Node.GenerateChildren();
	TArray<TNode> Children = Node.GetChildren();
	if (Children.IsEmpty()) { return Node; }

	TArray<TNode> BuiltChildren;
	BuiltChildren.Reserve(Children.Num());	
	for (const TNode& Child : Children)
	{
		TNode BuiltChild = BuildNodeRecursive(Child, CurrentDepth + 1, MaxDepth);
		BuiltChildren.Add(MoveTemp(BuiltChild));
	}

	Node.SetChildren(MoveTemp(BuiltChildren));
	return Node;
}

// run calculations on different threads up to a certain depth
template<typename TNode> requires c_min_max_tree_builder<TNode>
static TNode BuildNodeRecursiveWithThreadDepth(
	TNode Node,
	const int32 CurrentDepth,
	const int32 MaxDepth,
	const int32 ThreadDepth,
	TFunction<bool(const TNode&)> IsGameOver)
{
	if (CurrentDepth >= MaxDepth) return Node;
	if (IsGameOver(Node)) return Node;

	Node.GenerateChildren();
	TArray<TNode> Children = Node.GetChildren();
	if (Children.IsEmpty()) return Node;

	TArray<TNode> BuiltChildren;
	BuiltChildren.Reserve(Children.Num());

	if (CurrentDepth <= ThreadDepth)
	{
		// Parallel — dispatch each child as its own task
		TArray<UE::Tasks::TTask<TNode>> ChildTasks;
		ChildTasks.Reserve(Children.Num());

		for (const TNode& Child : Children)
		{
			UE::Tasks::TTask<TNode> ChildTask = UE::Tasks::Launch(
				UE_SOURCE_LOCATION,
				[Child, CurrentDepth, MaxDepth, ThreadDepth, IsGameOver]() mutable -> TNode
				{
					return BuildNodeRecursiveWithThreadDepth(
						Child,
						CurrentDepth + 1,
						MaxDepth,
						ThreadDepth,
						IsGameOver
					);
				},
				UE::Tasks::ETaskPriority::BackgroundNormal
			);

			ChildTasks.Add(MoveTemp(ChildTask));
		}

		for (UE::Tasks::TTask<TNode>& ChildTask : ChildTasks)
		{
			BuiltChildren.Add(ChildTask.GetResult());
		}
	}
	else
	{
		// Serial — build depth first on the same thread
		for (const TNode& Child : Children)
		{
			BuiltChildren.Add(BuildNodeRecursive(
				Child,
				CurrentDepth + 1,
				MaxDepth,
				IsGameOver
			));
		}
	}

	Node.SetChildren(MoveTemp(BuiltChildren));
	return Node;
}
