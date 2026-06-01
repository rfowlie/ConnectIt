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
		TNode BuiltChild = BuildNodeRecursive(Child, CurrentDepth + 1, MaxDepth, IsGameOver);
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


///////////////////////////////////////////////////////////////////////
template<typename TNode>
concept c_min_max_node_2 = requires(
	const TNode& ConstNode,
	TNode&        MutableNode,
	TArray<TNode> Children,
	float         Score)
{
	{ ConstNode.GetChildren() }                     -> std::same_as<const TArray<TNode>&>;
	{ MutableNode.SetChildren(MoveTemp(Children)) } -> std::same_as<void>;
	{ MutableNode.SetScore(Score) }                 -> std::same_as<void>;
	{ ConstNode.GetScore() }                        -> std::same_as<float>;
};

// Generates raw child nodes from a parent
template<typename TBuilderFunc, typename TNode>
concept c_child_builder = requires(const TBuilderFunc& Builder, const TNode& Node)
{
	{ Builder(Node) } -> std::same_as<TArray<TNode>>;
};

// Returns true if the game has reached a terminal state
template<typename TGameOverFunc, typename TNode>
concept c_game_over_predicate = requires(const TGameOverFunc& Func, const TNode& Node)
{
	{ Func(Node) } -> std::same_as<bool>;
};

// Assigns a rough ordering score to a node — cheap, not required to be accurate
template<typename TOrdererFunc, typename TNode>
concept c_move_orderer = requires(const TOrdererFunc& Orderer, const TNode& Node)
{
	{ Orderer(Node) } -> std::same_as<float>;
};

// Full static evaluation at leaf nodes
template<typename TEvaluatorFunc, typename TNode>
concept c_evaluator = requires(const TEvaluatorFunc& Evaluator, const TNode& Node)
{
	{ Evaluator(Node) } -> std::same_as<float>;
};

// Pass 1: Build and order the tree
template<typename TNode, typename TBuilderFunc, typename TGameOverFunc, typename TOrdererFunc>
    requires c_min_max_node_2<TNode>
		  && c_game_over_predicate<TGameOverFunc, TNode>
          && c_child_builder<TBuilderFunc, TNode>          
          && c_move_orderer<TOrdererFunc, TNode>
static TNode BuildNodeRecursive2(
    TNode                Node,
    const int32          CurrentDepth,
    const int32          MaxDepth,
    const TGameOverFunc& IsGameOver,
    const TBuilderFunc&  ChildBuilder,    
    const TOrdererFunc&  MoveOrderer)
{
    if (CurrentDepth >= MaxDepth) { return Node; }
    if (IsGameOver(Node))         { return Node; }

    TArray<TNode> Children = ChildBuilder(Node);
    if (Children.IsEmpty()) { return Node; }
	
    Children.Sort([&MoveOrderer](const TNode& A, const TNode& B)
    {
        return MoveOrderer(A) > MoveOrderer(B);
    });

    TArray<TNode> BuiltChildren;
    BuiltChildren.Reserve(Children.Num());

    for (TNode& Child : Children)
    {
        BuiltChildren.Add(BuildNodeRecursive2(
            MoveTemp(Child),
            CurrentDepth + 1,
            MaxDepth,
            IsGameOver,
            ChildBuilder,
            MoveOrderer));
    }

    Node.SetChildren(MoveTemp(BuiltChildren));
    return Node;
}

// Pass 2: Alpha-beta min-max evaluation
template<typename TNode, typename TEvaluatorFunc>
	requires c_min_max_node_2<TNode>
		  && c_evaluator<TEvaluatorFunc, TNode>
static TNode EvaluateNodeRecursive(
	TNode                 Node,
	const bool            bIsMaximising,
	float                 Alpha,           // best score maximiser is guaranteed
	float                 Beta,            // best score minimiser is guaranteed
	const TEvaluatorFunc& Evaluator)
{
	const TArray<TNode>& Children = Node.GetChildren();

	// Leaf: score from static evaluator
	if (Children.IsEmpty())
	{
		Node.SetScore(Evaluator(Node));
		return Node;
	}

	TArray<TNode> EvaluatedChildren;
	EvaluatedChildren.Reserve(Children.Num());

	float BestScore = bIsMaximising ? -FLT_MAX : FLT_MAX;

	for (const TNode& Child : Children)
	{
		TNode EvaluatedChild = EvaluateNodeRecursive(
			Child,
			!bIsMaximising,
			Alpha,
			Beta,
			Evaluator);

		const float ChildScore = EvaluatedChild.GetScore();
		EvaluatedChildren.Add(MoveTemp(EvaluatedChild));

		if (bIsMaximising)
		{
			BestScore = FMath::Max(BestScore, ChildScore);
			Alpha     = FMath::Max(Alpha, BestScore);
		}
		else
		{
			BestScore = FMath::Min(BestScore, ChildScore);
			Beta      = FMath::Min(Beta, BestScore);
		}

		// Beta cutoff: minimiser already has a better option up the tree,
		// so the maximiser would never choose this branch. Stop searching.
		// Alpha cutoff: symmetrically for the minimiser's perspective.
		if (Beta <= Alpha)
		{
			break; // Prune remaining siblings
		}
	}

	Node.SetChildren(MoveTemp(EvaluatedChildren));
	Node.SetScore(BestScore);
	return Node;
}