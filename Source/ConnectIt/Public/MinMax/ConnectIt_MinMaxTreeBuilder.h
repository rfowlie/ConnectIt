// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/Task.h"
#include "ConnectItMinMaxModel.h"
#include "ConnectIt_MinMaxTreeBuilder.generated.h"


// CombatTypes.h or ConnectItTypes.h
USTRUCT(BlueprintType)
struct FConnectItMoveOutcome
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition GridPosition;

    UPROPERTY(BlueprintReadWrite)
    int32 Score = 0;
};

// Delegate fired on the game thread when the tree is fully built
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTreeBuilt);

UCLASS(BlueprintType)
class CONNECTIT_API UConnectItTreeBuilder : public UObject
{
    GENERATED_BODY()
    
public:
    static constexpr int32 Connect_It_Score_Max = 100;
    
    UPROPERTY(BlueprintAssignable)
    FOnTreeBuilt OnBuildTreeComplete;
    
    UPROPERTY(BlueprintAssignable)
    FOnTreeBuilt OnSolveTreeComplete;

    UFUNCTION(BlueprintCallable)
    void BuildTreeAsync(const FConnectItMinMaxNodeExposed& InRootNode, int32 MaxDepth);
    UFUNCTION(BlueprintCallable)
    void SolveTreeAsync(bool bIsMaximisingPlayer) const;

    bool IsGameOver(const FConnectItMinMaxNode& InNode) const;
    int32 Evaluate(const FConnectItMinMaxNode& InNode) const;

    UFUNCTION(BlueprintCallable)
    TArray<FConnectItMoveOutcome> GetMoveSuggestions() const { return RootNodeMoveOutcomes; }
    
private:
    
    mutable FCriticalSection RootNodeMutex;
    
    mutable FConnectItMinMaxNode RootNode;

    UPROPERTY()
    mutable TArray<FConnectItMoveOutcome> RootNodeMoveOutcomes;
    
};