// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_Director.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "ConnectIt_PlayerDirector.generated.h"

class ATurnBasedPlayerState;
class UActionLoadoutDataAsset;
class UConnectIt_MatchConditionAsset;

// A human side: what the player starts with and how that changes as the match
// goes on (e.g. new moves when the player reaches a checkpoint). One instance
// per human player, owned by the GameMode; in a versus match both players get
// an instance of the same class. Named by class on the match preset; configure
// it in its Blueprint's Class Defaults.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_PlayerDirector : public UConnectIt_Director
{
    GENERATED_BODY()

public:

    // The player's loadout when they join (change it later with Set Player Loadout)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Director")
    TObjectPtr<UActionLoadoutDataAsset> StartingLoadout = nullptr;

    // The player's goals, in the order they are completed: completing one
    // that isn't the last is a checkpoint, completing the last is the win.
    // Also every faction's goals in a match with no AI. Copied into the match's
    // rules when it starts (fixed for the match).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Conditions")
    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> WinConditions;

    // Ways this side LOSES: if any one holds after a move (all of its
    // conditions at once), this side loses and the other side wins -- unless
    // a win was completed in that same move (wins are checked first). Only in
    // a match with an AI (ignored online for now). E.g. Not Enough Pieces To
    // Score, No Legal Move: Swap Pieces on an exhaustion level.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Conditions")
    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> LoseConditions;

    // Its player's faction slot
    virtual int32 GetFaction() const override;

    // The player this director speaks for
    UFUNCTION(BlueprintPure, Category = "Player|Director")
    ATurnBasedPlayerState* GetDirectedPlayerState() const;

    // Set by the GameMode when it creates the director for a player
    void SetDirectedPlayerState(ATurnBasedPlayerState* InPlayerState);

#if WITH_EDITOR
    // A starting loadout and win conditions are set (checked by the match
    // preset naming this class)
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    virtual void AddConditionRuleWarnings(const FConnectItRuleSet& Rules, FDataValidationContext& Context) const override;
#endif

protected:

    // Gives the player a different loadout -- by default from their next turn
    // (re-seeding uses mid-turn would hand out extra actions). Uses restart
    // from the new loadout's values.
    UFUNCTION(BlueprintCallable, Category = "Player|Director")
    void SetPlayerLoadout(UActionLoadoutDataAsset* Loadout,
        EConnectItLoadoutTiming Timing = EConnectItLoadoutTiming::NextTurnStart);

    // The loadout the player has now
    UFUNCTION(BlueprintPure, Category = "Player|Director")
    UActionLoadoutDataAsset* GetPlayerLoadout() const;

private:

    TWeakObjectPtr<ATurnBasedPlayerState> DirectedPlayerState;
};
