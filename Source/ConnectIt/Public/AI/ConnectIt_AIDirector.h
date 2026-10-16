// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConnectIt_AIDirector.generated.h"

class AConnectIt_AIController;
class UActionLoadoutDataAsset;
class UConnectIt_AIStrategy;

// Changes the AI opponent as a match goes on: which moves it has (its
// loadout) and how it picks them (its strategy). Lives on a match preset
// (UConnectIt_MatchPreset::AIDirector), because what it reacts to -- the
// preset's win checkpoints -- is defined there. Subclass it in Blueprint and
// override the events you need.
//
// Examples (see the recipe "Direct the AI during a match"):
//   * alternate between two loadouts every turn;
//   * give the AI new moves, or a smarter strategy, when either side reaches
//     a win checkpoint;
//   * a "tell": set the AI's NEXT loadout in OnAITurnEnded -- the loadout
//     replicates, so the UI can show what's coming during the player's turn.
//
// Server-only, like the AI controller it belongs to (each controller
// duplicates the preset's director). Events only -- it never decides a move;
// strategies do that.
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_AIDirector : public UObject
{
    GENERATED_BODY()

public:

    // The AI's turn has started, before it decides anything
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnAITurnStarted(int32 TurnNumber);
    virtual void OnAITurnStarted_Implementation(int32 TurnNumber) {}

    // The AI's turn is over (ended, timed out or skipped). A loadout set here
    // is the one it plays its next turn with.
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnAITurnEnded(int32 TurnNumber);
    virtual void OnAITurnEnded_Implementation(int32 TurnNumber) {}

    // A faction completed one of its win groups (not its last). bIsAI: it
    // was the AI's. GroupIndex / GroupName: which one, as in the preset's
    // rules (PlayerWinGroups or AIWinGroups).
    UFUNCTION(BlueprintNativeEvent, Category = "AI|Director")
    void OnWinCheckpointReached(int32 Faction, bool bIsAI, int32 GroupIndex, FName GroupName);
    virtual void OnWinCheckpointReached_Implementation(int32 Faction, bool bIsAI, int32 GroupIndex, FName GroupName) {}

    // The controller this director belongs to (its outer)
    UFUNCTION(BlueprintPure, Category = "AI|Director")
    AConnectIt_AIController* GetOwningController() const;

    virtual UWorld* GetWorld() const override;

protected:

    // Gives the AI a different loadout. During the AI's own turn the change
    // waits until that turn ends (so it can't hand itself extra moves
    // mid-turn); otherwise it applies at once. Uses restart from the new
    // loadout's values.
    UFUNCTION(BlueprintCallable, Category = "AI|Director")
    void SetAILoadout(UActionLoadoutDataAsset* Loadout);

    // Gives the AI a different strategy, from its next decision on.
    // StrategyTemplate is copied (e.g. an instanced strategy property on
    // this director).
    UFUNCTION(BlueprintCallable, Category = "AI|Director")
    void SetAIStrategy(UConnectIt_AIStrategy* StrategyTemplate);

    // The loadout the AI has now
    UFUNCTION(BlueprintPure, Category = "AI|Director")
    UActionLoadoutDataAsset* GetAILoadout() const;
};
