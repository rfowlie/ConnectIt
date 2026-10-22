// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "ConnectIt_MatchPreset.generated.h"

class UConnectIt_LevelDirector;
class UConnectIt_PlayerDirector;
class UConnectIt_AIDirector;

// A way to play a match -- its rules and its three directors (the level's
// script, the human side(s), the AI) -- independent of the stage it is played
// on (the level config holds the stage: board layout, registries, pieces). A
// "mode" of playing against the AI (Classic Easy, Classic Hard...) is a preset
// naming its directors.
//
// A map names the preset it plays as by default in its World Settings (its
// Adventure setup, say). The main menu can pick a different one for the same
// map ("Versus Classic", "Versus Blitz"...) through FConnectItMatchSettings,
// and tweak a few values on top. The GameMode copies the chosen preset when the
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

    // --- Directors: who scripts the level, the human side(s) and the AI ---
    // Each is a class whose Blueprint Class Defaults hold its setup (see
    // UConnectIt_Director). One per adventure level of each kind, or shared
    // ones for a way of playing (Classic, versus...).

    // The level's script, and both sides' goals (win and lose conditions).
    // Required: with none, nobody has goals.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Directors")
    TSubclassOf<UConnectIt_LevelDirector> LevelDirectorClass;

    // Every human player's director (one instance each): their starting
    // loadout and how it changes. Required -- without it players have no actions.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Directors")
    TSubclassOf<UConnectIt_PlayerDirector> PlayerDirectorClass;

    // The AI opponent when the match is played against the AI: its loadout,
    // strategies and selector, and how they change. Unset for versus-only
    // presets (the default director has no loadout).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Directors")
    TSubclassOf<UConnectIt_AIDirector> AIDirectorClass;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext& Context) const override;
#endif
};
