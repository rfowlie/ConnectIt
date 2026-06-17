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


/*
 * template class for game implementation
 */
namespace MinMax
{
	/*
	 * C++ will allow us to inherit from UObject and this class to work effectively with game logic
	 */
	template<typename TNode>
	requires c_min_max_node_2<TNode>
	class TMinMaxManager
	{
		public:

			virtual ~TMinMaxManager() = default;

			// -- Tree Queries --------------------------------------------------------

			int32 CountNodes(const TNode& Node) const
			{
				const TArray<TNode>& Children = Node.GetChildren();
				if (Children.IsEmpty()) { return 1; }

				int32 Count = 1;
				for (const TNode& Child : Children)
				{
					Count += CountNodes(Child);
				}
				return Count;
			}

			int32 GetDeepestDepth(const TNode& Node, const int32 CurrentDepth = 0) const
			{
				const TArray<TNode>& Children = Node.GetChildren();
				if (Children.IsEmpty()) { return CurrentDepth; }

				int32 MaxDepth = CurrentDepth;
				for (const TNode& Child : Children)
				{
					MaxDepth = FMath::Max(MaxDepth, GetDeepestDepth(Child, CurrentDepth + 1));
				}
				return MaxDepth;
			}

			// -- Accessors -----------------------------------------------------------

			const TNode& GetRootNode() const { return RootNode; }
			bool         HasTree()     const { return !RootNode.GetChildren().IsEmpty(); }

			// -- Core Interface (implement in subclass) ------------------------------

			virtual void BuildTreeAsync(int32 MaxDepth)                                  = 0;
			virtual void EvaluateTreeAsync(bool bIsMaximising)                           = 0;
			virtual void BuildAndEvaluateTreeAsync(int32 MaxDepth, bool bIsMaximising)   = 0;

		protected:

			mutable TNode RootNode;
	};

} // namespace MinMax


// template<typename TNode>
// requires c_min_max_node_2<TNode>
// class TMinMaxManagerBase
// {
// public:
//
//     // -- Delegate Injection --------------------------------------------------
//     // Concrete subclass sets these before calling build/evaluate
//
//     TFunction<bool(const TNode&)>           IsGameOverDelegate;
//     TFunction<TArray<TNode>(const TNode&)>  BuildChildrenDelegate;
//     TFunction<float(const TNode&)>          MoveOrdererDelegate;
//     TFunction<float(const TNode&)>          EvaluatorDelegate;
//
//     // -- Core API ------------------------------------------------------------
//
//     void BuildTreeAsync(TNode InRootNode, int32 MaxDepth)
//     {
//         checkf(IsGameOverDelegate,    TEXT("BuildTreeAsync: IsGameOverDelegate not set"));
//         checkf(BuildChildrenDelegate, TEXT("BuildTreeAsync: BuildChildrenDelegate not set"));
//         checkf(MoveOrdererDelegate,   TEXT("BuildTreeAsync: MoveOrdererDelegate not set"));
//
//         TWeakObjectPtr<TMinMaxManagerBase> WeakThis(this);
//
//         // Capture delegates by value so they are safe across thread boundary
//         auto IsGameOver    = IsGameOverDelegate;
//         auto BuildChildren = BuildChildrenDelegate;
//         auto MoveOrderer   = MoveOrdererDelegate;
//
//         UE::Tasks::Launch(
//             UE_SOURCE_LOCATION,
//             [WeakThis, InRootNode, MaxDepth, IsGameOver, BuildChildren, MoveOrderer]() mutable
//             {
//                 if (!WeakThis.IsValid()) return;
//
//                 TNode BuiltNode = BuildNodeRecursive<TNode>(
//                     InRootNode,
//                     0,
//                     MaxDepth,
//                     [&IsGameOver](const TNode& Node) -> bool
//                     {
//                         return IsGameOver(Node);
//                     },
//                     [&BuildChildren](const TNode& Node) -> TArray<TNode>
//                     {
//                         return BuildChildren(Node);
//                     },
//                     [&MoveOrderer](const TNode& Node) -> float
//                     {
//                         return MoveOrderer(Node);
//                     }
//                 );
//
//                 TSharedPtr<TNode> SharedNode = MakeShared<TNode>(MoveTemp(BuiltNode));
//
//                 AsyncTask(ENamedThreads::GameThread, [WeakThis, SharedNode]() mutable
//                 {
//                     if (!WeakThis.IsValid()) return;
//                     WeakThis->Manager.SetRootNode(MoveTemp(*SharedNode));
//                     if (WeakThis->OnTreeBuilt.IsBound()) { WeakThis->OnTreeBuilt.Broadcast(); }
//                 });
//             },
//             UE::Tasks::ETaskPriority::BackgroundNormal
//         );
//     }
//
//     void EvaluateTreeAsync(bool bIsMaximising)
//     {
//         checkf(EvaluatorDelegate, TEXT("EvaluateTreeAsync: EvaluatorDelegate not set"));
//
//         if (!Manager.HasTree())
//         {
//             UE_LOG(LogTemp, Error, TEXT("EvaluateTreeAsync: No tree built — call BuildTreeAsync first"));
//             return;
//         }
//
//         TWeakObjectPtr<TMinMaxManagerBase> WeakThis(this);
//
//         auto Evaluator = EvaluatorDelegate;
//         TSharedPtr<TNode> SharedNode = MakeShared<TNode>(Manager.GetRootNode());
//
//         UE::Tasks::Launch(
//             UE_SOURCE_LOCATION,
//             [WeakThis, SharedNode, bIsMaximising, Evaluator]() mutable
//             {
//                 if (!WeakThis.IsValid()) return;
//
//                 TNode EvaluatedNode = EvaluateNodeRecursive<TNode>(
//                     MoveTemp(*SharedNode),
//                     bIsMaximising,
//                     -FLT_MAX,
//                     FLT_MAX,
//                     [&Evaluator](const TNode& Node) -> float
//                     {
//                         return Evaluator(Node);
//                     }
//                 );
//
//                 TSharedPtr<TNode> SharedEvaluatedNode = MakeShared<TNode>(MoveTemp(EvaluatedNode));
//
//                 AsyncTask(ENamedThreads::GameThread, [WeakThis, SharedEvaluatedNode]() mutable
//                 {
//                     if (!WeakThis.IsValid()) return;
//                     WeakThis->Manager.SetRootNode(MoveTemp(*SharedEvaluatedNode));
//                     if (WeakThis->OnTreeEvaluated.IsBound()) { WeakThis->OnTreeEvaluated.Broadcast(); }
//                 });
//             },
//             UE::Tasks::ETaskPriority::BackgroundNormal
//         );
//     }
//
//     void BuildAndEvaluateTreeAsync(TNode InRootNode, int32 MaxDepth, bool bIsMaximising)
//     {
//         bPendingIsMaximising = bIsMaximising;
//         OnTreeBuilt.RemoveDynamic(this, &TMinMaxManagerBase::OnTreeBuilt_Internal);
//         OnTreeBuilt.AddDynamic(this, &TMinMaxManagerBase::OnTreeBuilt_Internal);
//         BuildTreeAsync(MoveTemp(InRootNode), MaxDepth);
//     }
//
//     // -- Helpers -------------------------------------------------------------
//
//     UFUNCTION(BlueprintCallable)
//     int32 GetNodeCount() const { return Manager.CountNodes(Manager.GetRootNode()); }
//
//     UFUNCTION(BlueprintCallable)
//     int32 GetTreeDepth() const { return Manager.GetDeepestDepth(Manager.GetRootNode()); }
//
// protected:
//
//     MinMax::TMinMaxManager<TNode> Manager;
//
//     void OnTreeBuilt_Internal() override
//     {
//         OnTreeBuilt.RemoveDynamic(this, &TMinMaxManagerBase::OnTreeBuilt_Internal);
//         EvaluateTreeAsync(bPendingIsMaximising);
//     }
// };