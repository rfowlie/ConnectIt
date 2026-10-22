// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_Director.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "ConnectIt_PlayerDirector.generated.h"

class ATurnBasedPlayerState;
class UActionLoadoutDataAsset;

// A human side: what the player starts with and how that changes as the match
// goes on (e.g. new moves when the player reaches a checkpoint). Its goals are
// the level's (UConnectIt_LevelDirector's Player Win / Lose Conditions). One instance
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

    // Its player's faction slot
    virtual int32 GetFaction() const override;

    // The player this director speaks for
    UFUNCTION(BlueprintPure, Category = "Player|Director")
    ATurnBasedPlayerState* GetDirectedPlayerState() const;

    // Set by the GameMode when it creates the director for a player
    void SetDirectedPlayerState(ATurnBasedPlayerState* InPlayerState);

#if WITH_EDITOR
    // A starting loadout is set (checked by the match preset naming this class)
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:

    // Gives the player a different loadout -- by default from their next turn
    // (re-seeding uses mid-turn would hand out extra actions). Uses restart
    // from the new loadout's values.
    UFUNCTION(BlueprintCallable, Category = "Player|Director")
    void SetPlayerLoadout(UActionLoadoutDataAsset* Loadout,
        EConnectItChangeTiming Timing = EConnectItChangeTiming::NextTurnStart);

    // The loadout the player has now
    UFUNCTION(BlueprintPure, Category = "Player|Director")
    UActionLoadoutDataAsset* GetPlayerLoadout() const;

private:

    TWeakObjectPtr<ATurnBasedPlayerState> DirectedPlayerState;
};
