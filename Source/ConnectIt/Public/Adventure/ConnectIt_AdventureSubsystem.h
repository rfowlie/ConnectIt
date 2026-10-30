// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "ConnectIt_AdventureSubsystem.generated.h"

class UConnectIt_MapNodeDataAsset;


// The player's Adventure progress for the whole session: loads the save when
// the game starts, answers questions about it, changes it by the rules
// (FConnectItAdventureProgress) and saves after every change. One save slot.
//
// Permanent uses come from the shards through the project's action shard
// table; item caps from the turn-based plugin's item inventory asset.
UCLASS()
class CONNECTIT_API UConnectIt_AdventureSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    // The save slot's name
    static const FString SaveSlotName;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // --- Load / save ---

    // Reads the save into the progress (a fresh start if there is none).
    // False if a save exists but can't be read (the progress is then fresh).
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure")
    bool LoadProgress();

    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure")
    bool SaveProgress();

    // --- Queries ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    const FConnectItAdventureProgress& GetProgress() const { return Progress; }

    // Where Node stands. Hidden unless the hub has revealed it (bRevealed: its
    // reveal walk decides); OutCheck says what a gated node still needs.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    EConnectItNodeState GetNodeState(const UConnectIt_MapNodeDataAsset* Node, bool bRevealed,
        FConnectItNodeRequirementCheck& OutCheck) const;

    // Permanent uses of Action, from its shards
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    int32 GetPermanentUses(FGameplayTag Action) const;

    // Whether Node's seal can be broken now (the panel's Break Seal button),
    // and why not
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure")
    bool CanBreakSeal(const UConnectIt_MapNodeDataAsset* Node, FString& OutWhyNot) const;

    // --- Changes (each saves) ---

    // Spends the seals Node's gate needs; it stays open for good. False if
    // they can't be afforded.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure")
    bool BreakSeal(const UConnectIt_MapNodeDataAsset* Node);

    // A match of Node ended (FConnectItAdventureProgress::RecordResult): the
    // inventory the match left, and on a first win the node and its rewards.
    // Returns whether rewards were granted.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure")
    bool RecordResult(const UConnectIt_MapNodeDataAsset* Node, bool bWon, const TArray<FItemActionEntry>& Inventory);

private:

    UPROPERTY()
    FConnectItAdventureProgress Progress;
};
