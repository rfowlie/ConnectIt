// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "ConnectIt_MatchPreset.generated.h"

class UActionLoadoutDataAsset;
class UConnectIt_AIProfile;

// A way to play a match -- its rules, what the player can do, and who the AI
// is -- independent of the stage it is played on (the level config holds the
// stage: board layout, registries, pieces).
//
// A level config names the preset it plays as by default (its Adventure
// setup, say). The main menu can pick a different one for the same level
// ("Versus Classic", "Versus Blitz"...) through FConnectItMatchSettings, and
// tweak a few values on top. The GameMode copies the chosen preset when the
// match starts and plays on its own copy, so changes during a match never
// touch this asset.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_MatchPreset : public UDataAsset
{
    GENERATED_BODY()

public:

    // What the menu shows for this preset
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Preset")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Preset", meta = (MultiLine = true))
    FText Description;

    // How a match scores and is won: pick each rule's type and set its values
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules")
    FConnectItRuleSet Rules;

    // The actions human players get
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Loadout")
    TObjectPtr<UActionLoadoutDataAsset> PlayerLoadout = nullptr;

    // The AI opponent (strategy + loadout) when the match is played against
    // the AI. Unset for presets meant only for human-vs-human; the main menu
    // can also choose one.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|AI")
    TObjectPtr<UConnectIt_AIProfile> AIProfile = nullptr;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext& Context) const override;
#endif
};
