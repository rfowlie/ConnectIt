// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "TurnBasedMechanicsEnums.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "DWidget_ConnectIt_GameState.generated.h"

// Tracks AConnectIt_GameState specifically -- match phase and match result,
// pushed straight through to BP as each changes (see UDWidgetBase's own
// class comment for the push/GetInfo() convention). No cached fields, no
// pull-based Get*() accessors -- BP holds whatever state it needs from the
// events it receives.
// Deliberately does not surface the board-derived convenience wrappers
// (GetFactionScore/IsTileOccupied/etc.) that live on AConnectIt_GameState --
// those read through to UConnectIt_BoardStateComponent, and duplicating them
// here would just be the same data as
// DWidget_ConnectIt_BoardStateComponent under a second name.
UCLASS(Abstract)
class CONNECTIT_API UDWidget_ConnectIt_GameState : public UDWidgetBase
{
    GENERATED_BODY()

public:

    // False until AConnectIt_GameState has actually been resolved -- the
    // one deliberately pull-only exception to the push model, checked once
    // rather than pushed, since "is this bound yet" is a one-time gate,
    // not a recurring update.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ConnectIt|Debug")
    void OnMatchPhaseUpdated(EMatchPhase InMatchPhase);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ConnectIt|Debug")
    void OnMatchResultUpdated(const FConnectItMatchResult& Result);

private:

    UPROPERTY()
    TObjectPtr<AConnectIt_GameState> ResolvedSource = nullptr;

    bool bSourceValid = false;
};
