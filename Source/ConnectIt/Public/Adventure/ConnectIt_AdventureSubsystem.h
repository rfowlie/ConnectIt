// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Adventure/ConnectIt_AdventureProgress.h"
#include "ConnectIt_AdventureSubsystem.generated.h"

class UConnectIt_MapNodeDataAsset;
class UActionLoadoutDataAsset;
class ATurnBasedPlayerState;


// The player's Adventure progress for the whole session: loads the save when
// the game starts, answers questions about it, changes it by the rules
// (FConnectItAdventureProgress) and saves after every change. One save slot.
//
// Permanent uses come from the shards through the project's action shard
// table; item caps from the turn-based plugin's item inventory asset.
//
// Playing a node: StartNode (from the hub) remembers the node and the hub map
// and starts its level. In the match the GameMode asks SeedPlayerState to set
// the player up, and at match end hands over the result
// (RecordPlayingNodeResult). ReturnToHub opens the hub again (e.g. from the
// results screen), which focuses the last-played node.
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

    // --- Playing a node ---

    // Starts Node's level (its match preset), remembering Node and the hub
    // map WorldContextObject is in. False -- nothing started -- if Node can't
    // be played: completed (no replays), still gated, a sub-map node (the hub
    // opens those) or no level. Options: OpenLevel URL options.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure", meta = (WorldContext = "WorldContextObject"))
    bool StartNode(const UObject* WorldContextObject, UConnectIt_MapNodeDataAsset* Node, const FString& Options);

    // The node being played, if WorldContextObject is in its level (else
    // null: e.g. a level opened straight in the editor)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure", meta = (WorldContext = "WorldContextObject"))
    UConnectIt_MapNodeDataAsset* GetPlayingNode(const UObject* WorldContextObject) const;

    // Server: sets up PlayerState for the node being played -- a closed node
    // from Loadout (the level's Player director's), an open one from Loadout
    // and the shards (BuildOpenPermanentActions); either way the saved
    // inventory and the node's item budget. False (nothing done) if this
    // level isn't a node being played.
    bool SeedPlayerState(ATurnBasedPlayerState* PlayerState, UActionLoadoutDataAsset* Loadout) const;

    // Server, at match end: records the node being played (RecordResult) and
    // saves. Once per match.
    void RecordPlayingNodeResult(const UObject* WorldContextObject, bool bWon, const TArray<FItemActionEntry>& Inventory);

    // Opens the hub map the node was started from (it focuses the
    // last-played node). False if no node was started from a hub.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure", meta = (WorldContext = "WorldContextObject"))
    bool ReturnToHub(const UObject* WorldContextObject);

    // A match of Node ended (FConnectItAdventureProgress::RecordResult): the
    // inventory the match left, and on a first win the node and its rewards.
    // Returns whether rewards were granted.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Adventure")
    bool RecordResult(const UConnectIt_MapNodeDataAsset* Node, bool bWon, const TArray<FItemActionEntry>& Inventory);

private:

    UPROPERTY()
    FConnectItAdventureProgress Progress;

    // The node being played, and the hub map it was started from
    UPROPERTY()
    TObjectPtr<UConnectIt_MapNodeDataAsset> PlayingNode;

    TSoftObjectPtr<UWorld> HubMap;

    // The playing node's result is in (one per match)
    bool bPlayingNodeRecorded = false;
};
