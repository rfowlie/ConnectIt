// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ConnectIt_Director.generated.h"

class AConnectIt_GameMode;
class UConnectIt_LevelDirector;
class UConnectIt_PlayerDirector;
class UConnectIt_AIDirector;
class UConnectIt_MatchConditionAsset;

// What every director shares. A match has three kinds, each the script and
// setup for one context, all named by class on the match preset and
// configured in their Blueprint's Class Defaults:
//   * UConnectIt_LevelDirector -- the level itself;
//   * UConnectIt_PlayerDirector -- a human side (one per human player);
//   * UConnectIt_AIDirector -- the AI opponent.
//
// The GameMode sends every director the same events, in a fixed order: the
// Level director, then the Player directors by faction slot, then the AI
// director. Each event says whose it is (Faction) and whether that is this
// director's own side (bIsMine; always false for the Level director).
//
// Directors react to committed state -- these events come from the turn
// order and the board's committed changes -- never to visual event tags,
// which are paced by the presentation queue. Each director changes only its
// own context (a side's loadout is its director's to change); to affect
// another, ask that director (Get Player Director / Get AI Director...).
//
// Server-only: directors exist where the GameMode does.
UCLASS(Abstract, Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_Director : public UObject
{
    GENERATED_BODY()

public:

    // --- Events ---

    // The match has started (board ready, every side registered): set up here
    UFUNCTION(BlueprintNativeEvent, Category = "Director")
    void OnMatchStarted();
    virtual void OnMatchStarted_Implementation() {}

    // Faction's turn has started (before it acts). For the AI: before it
    // decides anything. A loadout a director sets for its own side here,
    // during the OTHER side's turn, applies at once and replicates -- the
    // place for a tell.
    UFUNCTION(BlueprintNativeEvent, Category = "Director")
    void OnTurnStarted(int32 Faction, bool bIsMine, int32 TurnNumber);
    virtual void OnTurnStarted_Implementation(int32 Faction, bool bIsMine, int32 TurnNumber) {}

    // Faction's turn is over (ended, timed out or skipped)
    UFUNCTION(BlueprintNativeEvent, Category = "Director")
    void OnTurnEnded(int32 Faction, bool bIsMine, int32 TurnNumber);
    virtual void OnTurnEnded_Implementation(int32 Faction, bool bIsMine, int32 TurnNumber) {}

    // Faction completed one of its goals (not its last). Goal: the condition
    // asset reached -- compare against your own asset references. GroupIndex:
    // its position in that side's goal list; GroupName: the asset's name.
    UFUNCTION(BlueprintNativeEvent, Category = "Director")
    void OnWinCheckpointReached(int32 Faction, bool bIsMine, UConnectIt_MatchConditionAsset* Goal, int32 GroupIndex, FName GroupName);
    virtual void OnWinCheckpointReached_Implementation(int32 Faction, bool bIsMine, UConnectIt_MatchConditionAsset* Goal, int32 GroupIndex, FName GroupName) {}

    // Faction has won the match (the match ends right after)
    UFUNCTION(BlueprintNativeEvent, Category = "Director")
    void OnMatchWon(int32 Faction, bool bIsMine);
    virtual void OnMatchWon_Implementation(int32 Faction, bool bIsMine) {}

    // --- Queries ---

    // The faction slot this director speaks for; INDEX_NONE for the Level
    // director (and before its side has a slot)
    UFUNCTION(BlueprintPure, Category = "Director")
    virtual int32 GetFaction() const { return INDEX_NONE; }

    // Faction is this director's own side
    UFUNCTION(BlueprintPure, Category = "Director")
    bool IsMine(int32 Faction) const { return Faction != INDEX_NONE && Faction == GetFaction(); }

    // The other directors of this match (null if there is none)
    UFUNCTION(BlueprintPure, Category = "Director")
    UConnectIt_LevelDirector* GetLevelDirector() const;

    // The director of the human player in Faction's slot
    UFUNCTION(BlueprintPure, Category = "Director")
    UConnectIt_PlayerDirector* GetPlayerDirector(int32 Faction) const;

    UFUNCTION(BlueprintPure, Category = "Director")
    UConnectIt_AIDirector* GetAIDirector() const;

    // The order events are sent in: Level, Player directors by faction slot
    // (stable for equal slots), AI. Nulls are skipped.
    static TArray<UConnectIt_Director*> InDispatchOrder(
        UConnectIt_LevelDirector* Level,
        const TArray<UConnectIt_PlayerDirector*>& Players,
        UConnectIt_AIDirector* AI);

    virtual UWorld* GetWorld() const override;

protected:

    // The server's GameMode (null without a world or off the server)
    AConnectIt_GameMode* GetGameMode() const;
};
