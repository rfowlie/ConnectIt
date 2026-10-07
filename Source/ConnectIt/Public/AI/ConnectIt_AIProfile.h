// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ConnectIt_AIProfile.generated.h"

class UActionLoadoutDataAsset;
class UConnectIt_AIStrategy;


// One AI opponent, fully described: how it decides (a configured strategy)
// and what it may do (its loadout). Presets are assets -- e.g. a "Classic
// Easy" and a "Classic Hard" profile using the same MinMax strategy with
// different settings -- so they're saved, reusable and swappable.
//
// Referenced by a level config (that level's default opponent) and, later,
// picked from the main menu. The strategy here is a template: each
// AConnectIt_AIController duplicates it for its own match.
//
// One asset type for every strategy -- a strategy's own properties are its
// configuration, so there is no paired data-asset class per strategy.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_AIProfile : public UDataAsset
{
    GENERATED_BODY()

public:

    // Shown in menus
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Profile")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Profile", meta = (MultiLine = true))
    FText Description;

    // What this opponent may do on its turn -- action configs and turn-end
    // requirements, same as a player's loadout (system actions aren't needed:
    // AI controllers have no action stack). The Mediator holds the AI to it
    // exactly like a human.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Profile")
    TObjectPtr<UActionLoadoutDataAsset> Loadout = nullptr;

    // How this opponent picks its moves, configured inline -- e.g. "MinMax
    // (Classic)" with its depth / time / mistake settings and term lists.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "AI Profile")
    TObjectPtr<UConnectIt_AIStrategy> Strategy = nullptr;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
