//
// #pragma once
// #include "ConnectIt_MinMaxManager.h"
// #include "ConcreteMinMaxExample.generated.h"
//
// template<typename TNode>
// concept c_min_max_concrete = requires(
//     const TNode& ConstNode,
//     TNode&        MutableNode,
//     TArray<TNode> Children,
//     float         Score)
// {
//     { ConstNode.GetChildren() }                     -> std::same_as<const TArray<TNode>&>;
//     { MutableNode.SetChildren(MoveTemp(Children)) } -> std::same_as<void>;
//     { ConstNode.GetScore() }                        -> std::same_as<float>;
//     { MutableNode.SetScore(Score) }                 -> std::same_as<void>;    
// };
//
// // -- Level 1: Pure C++ template (no Unreal) ---------------------------------
//
// template<typename TNode>
// requires c_min_max_concrete<TNode>
// class TMinMaxManagerBase
// {
// public:
//
//     virtual ~TMinMaxManagerBase() = default;
//
//     TFunction<bool(const TNode&)>                   IsGameOverDelegate;
//     TFunction<TArray<TNode>(const TNode&)>          BuildChildrenDelegate;
//     TFunction<float(const TNode&)>                  MoveOrdererDelegate;
//     TFunction<float(const TNode&, const TNode&)>    EvaluatorDelegate;
//
//     int32 CountNodes(const TNode& Node) const
//     {
//         const TArray<TNode>& Children = Node.GetChildren();
//         if (Children.IsEmpty()) { return 1; }
//
//         int32 Count = 1;
//         for (const TNode& Child : Children)
//         {
//             Count += CountNodes(Child);
//         }
//         return Count;
//     }
//     
//     int32 GetDeepestDepth(const TNode& Node, int32 Depth = 0) const
//     {
//         const TArray<TNode>& Children = Node.GetChildren();
//         if (Children.IsEmpty()) { return Depth; }
//
//         int32 MaxDepth = Depth;
//         for (const TNode& Child : Children)
//         {
//             MaxDepth = FMath::Max(MaxDepth, GetDeepestDepth(Child, Depth + 1));
//         }
//         return MaxDepth;
//     }
//     
//     bool         HasTree()     const { return !RootNode.GetChildren().IsEmpty(); }
//     const TNode& GetRootNode() const { return RootNode; }
//
// protected:
//
//     mutable TNode RootNode;
//
//     TNode BuildTree(TNode InRootNode, const int32 MaxDepth)
//     {
//         checkf(IsGameOverDelegate,    TEXT("IsGameOverDelegate not set"));
//         checkf(BuildChildrenDelegate, TEXT("BuildChildrenDelegate not set"));
//         checkf(MoveOrdererDelegate,   TEXT("MoveOrdererDelegate not set"));
//
//         return BuildNodeRecursive<TNode>(
//             MoveTemp(InRootNode),
//             0,
//             MaxDepth,
//             IsGameOverDelegate,
//             BuildChildrenDelegate,
//             MoveOrdererDelegate
//         );
//     }
//
//     TNode EvaluateTree(TNode InRootNode, const bool bIsMaximising)
//     {
//         checkf(EvaluatorDelegate, TEXT("EvaluatorDelegate not set"));
//
//         return UMinMaxUtility::EvaluateNodeRecursive<TNode>(
//             MoveTemp(InRootNode),
//             bIsMaximising,
//             -FLT_MAX,
//             FLT_MAX,
//             EvaluatorDelegate
//         );
//     }
// };
//
// template<typename TNode>
// requires c_min_max_concrete<TNode>
// class TMinMaxManagerBase2
// {
// public:
//
//     virtual ~TMinMaxManagerBase2() = default;
//
//     TFunction<bool(const TNode&)>                   IsGameOverDelegate;
//     TFunction<TArray<TNode>(const TNode&)>          BuildChildrenDelegate;
//     TFunction<float(const TNode&)>                  MoveOrdererDelegate;
//     TFunction<float(const TNode&, const TNode&)>    EvaluatorDelegate;
//
//     bool         HasTree()     const { return !RootNode.GetChildren().IsEmpty(); }
//     const TNode& GetRootNode() const { return RootNode; }
//     
//     int32 CountNodes(const TNode& Node) const
//     {
//         const TArray<TNode>& Children = Node.GetChildren();
//         if (Children.IsEmpty()) { return 1; }
//
//         int32 Count = 1;
//         for (const TNode& Child : Children)
//         {
//             Count += CountNodes(Child);
//         }
//         return Count;
//     }
//     
//     int32 GetDeepestDepth(const TNode& Node, int32 Depth = 0) const
//     {
//         const TArray<TNode>& Children = Node.GetChildren();
//         if (Children.IsEmpty()) { return Depth; }
//
//         int32 MaxDepth = Depth;
//         for (const TNode& Child : Children)
//         {
//             MaxDepth = FMath::Max(MaxDepth, GetDeepestDepth(Child, Depth + 1));
//         }
//         return MaxDepth;
//     }   
//
// protected:
//
//     mutable TNode RootNode;
//
//     void BuildTree(TNode InRootNode, const int32 MaxDepth)
//     {
//         checkf(IsGameOverDelegate,    TEXT("IsGameOverDelegate not set"));
//         checkf(BuildChildrenDelegate, TEXT("BuildChildrenDelegate not set"));
//         checkf(MoveOrdererDelegate,   TEXT("MoveOrdererDelegate not set"));
//
//         RootNode = BuildNodeRecursive<TNode>(
//             MoveTemp(InRootNode),
//             0,
//             MaxDepth,
//             IsGameOverDelegate,
//             BuildChildrenDelegate,
//             MoveOrdererDelegate
//         );
//     }
//
//     // TODO: pass in alpha and beta start values
//     void EvaluateTree(const bool bIsMaximising)
//     {
//         checkf(EvaluatorDelegate, TEXT("EvaluatorDelegate not set"));
//
//         RootNode = EvaluateNodeRecursive<TNode>(
//             MoveTemp(RootNode),
//             bIsMaximising,
//             -FLT_MAX,
//             FLT_MAX,
//             EvaluatorDelegate
//         );
//     }
// };
//
//
// // -- Level 2: Abstract UObject — Unreal scaffolding, no TNode ---------------
//
// DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMinMaxManagerDelegate);
//
// UCLASS(Abstract)
// class CONNECTIT_API UMinMaxManagerBase : public UObject
// {
//     GENERATED_BODY()
//
// public:
//
//     UPROPERTY(BlueprintAssignable)
//     FMinMaxManagerDelegate OnTreeBuilt;
//
//     UPROPERTY(BlueprintAssignable)
//     FMinMaxManagerDelegate OnTreeEvaluated;
//
//     UFUNCTION(BlueprintCallable)
//     virtual void BuildTreeAsync(int32 MaxDepth) PURE_VIRTUAL(UMinMaxManagerBase::BuildTreeAsync,);
//
//     UFUNCTION(BlueprintCallable)
//     virtual void EvaluateTreeAsync(bool bIsMaximising) PURE_VIRTUAL(UMinMaxManagerBase::EvaluateTreeAsync,);
//
//     UFUNCTION(BlueprintCallable)
//     virtual void BuildAndEvaluateTreeAsync(int32 MaxDepth, bool bIsMaximising) PURE_VIRTUAL(UMinMaxManagerBase::BuildAndEvaluateTreeAsync,);
//
//     UFUNCTION(BlueprintCallable)
//     virtual int32 GetNodeCount() const PURE_VIRTUAL(UMinMaxManagerBase::GetNodeCount, return 0;);
//
//     UFUNCTION(BlueprintCallable)
//     virtual int32 GetTreeDepth() const PURE_VIRTUAL(UMinMaxManagerBase::GetTreeDepth, return 0;);
//
// protected:
//
//     bool bPendingIsMaximising = false;
//
//     UFUNCTION()
//     virtual void OnTreeBuilt_Internal() PURE_VIRTUAL(UMinMaxManagerBase::OnTreeBuilt_Internal,);
// };
//
//
// // -- Level 3: Concrete subclass — resolves TNode, wires everything ----------
//
// UCLASS()
// class CONNECTIT_API UConnectIt_MinMax
//     : public UMinMaxManagerBase
//     , public TMinMaxManagerBase<ConnectIt::FMinMaxNode>
// {
//     GENERATED_BODY()
//
// public:
//
//     UConnectIt_MinMax()
//     {
//         IsGameOverDelegate    = &ConnectIt::DefaultIsGameOver;
//         BuildChildrenDelegate = &ConnectIt::DefaultBuildChildNodes;
//         MoveOrdererDelegate   = &ConnectIt::DefaultMoveOrder;
//         EvaluatorDelegate     = &ConnectIt::DefaultEvaluator;
//     }
//     
//     virtual void BuildTreeAsync(int32 MaxDepth) override
//     {
//         TWeakObjectPtr WeakThis(this);
//         TSharedPtr<ConnectIt::FMinMaxNode> SharedRoot = MakeShared<ConnectIt::FMinMaxNode>(RootNode);
//
//         UE::Tasks::Launch(UE_SOURCE_LOCATION,
//             [WeakThis, SharedRoot, MaxDepth]() mutable
//             {
//                 if (!WeakThis.IsValid()) return;
//
//                 // Synchronous C++ template does the work
//                 ConnectIt::FMinMaxNode Built = WeakThis->BuildTree(MoveTemp(*SharedRoot), MaxDepth);
//
//                 TSharedPtr<ConnectIt::FMinMaxNode> SharedBuilt =
//                     MakeShared<ConnectIt::FMinMaxNode>(MoveTemp(Built));
//
//                 AsyncTask(ENamedThreads::GameThread, [WeakThis, SharedBuilt]() mutable
//                 {
//                     if (!WeakThis.IsValid()) return;
//                     WeakThis->RootNode = MoveTemp(*SharedBuilt);
//                     if (WeakThis->OnTreeBuilt.IsBound()) { WeakThis->OnTreeBuilt.Broadcast(); }
//                 });
//             },
//             UE::Tasks::ETaskPriority::BackgroundNormal);
//     }
//
//     virtual void EvaluateTreeAsync(bool bIsMaximising) override
//     {
//         if (!HasTree())
//         {
//             UE_LOG(LogTemp, Error, TEXT("EvaluateTreeAsync: No tree — call BuildTreeAsync first"));
//             return;
//         }
//
//         TWeakObjectPtr WeakThis(this);
//         TSharedPtr<ConnectIt::FMinMaxNode> SharedRoot = MakeShared<ConnectIt::FMinMaxNode>(RootNode);
//
//         UE::Tasks::Launch(UE_SOURCE_LOCATION,
//             [WeakThis, SharedRoot, bIsMaximising]() mutable
//             {
//                 if (!WeakThis.IsValid()) return;
//
//                 ConnectIt::FMinMaxNode Evaluated = WeakThis->EvaluateTree(MoveTemp(*SharedRoot), bIsMaximising);
//
//                 TSharedPtr<ConnectIt::FMinMaxNode> SharedEvaluated =
//                     MakeShared<ConnectIt::FMinMaxNode>(MoveTemp(Evaluated));
//
//                 AsyncTask(ENamedThreads::GameThread, [WeakThis, SharedEvaluated]() mutable
//                 {
//                     if (!WeakThis.IsValid()) return;
//                     WeakThis->RootNode = MoveTemp(*SharedEvaluated);
//                     if (WeakThis->OnTreeEvaluated.IsBound()) { WeakThis->OnTreeEvaluated.Broadcast(); }
//                 });
//             },
//             UE::Tasks::ETaskPriority::BackgroundNormal);
//     }
//
//     virtual void BuildAndEvaluateTreeAsync(const int32 MaxDepth, const bool bIsMaximising) override
//     {
//         bPendingIsMaximising = bIsMaximising;
//         OnTreeBuilt.RemoveDynamic(this, &UConnectIt_MinMax::OnTreeBuilt_Internal);
//         OnTreeBuilt.AddDynamic(this, &UConnectIt_MinMax::OnTreeBuilt_Internal);
//         BuildTreeAsync(MaxDepth);
//     }
//
//     virtual int32 GetNodeCount() const override { return CountNodes(RootNode); }
//     virtual int32 GetTreeDepth() const override { return GetDeepestDepth(RootNode); }
//
// protected:
//     
//     virtual void OnTreeBuilt_Internal() override
//     {
//         OnTreeBuilt.RemoveDynamic(this, &UConnectIt_MinMax::OnTreeBuilt_Internal);
//         EvaluateTreeAsync(bPendingIsMaximising);
//     }
// };