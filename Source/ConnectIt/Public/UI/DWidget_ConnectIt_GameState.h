// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "TurnBasedMechanicsEnums.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "DWidget_ConnectIt_GameState.generated.h"

// Tracks AConnectIt_GameState specifically -- match phase and match result.
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

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    EMatchPhase GetMatchPhase() const { return CachedMatchPhase; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    const FConnectItMatchResult& GetMatchResult() const { return CachedMatchResult; }

    // False until AConnectIt_GameState has actually been resolved
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;
    virtual void RefreshFields() override;

private:

    UFUNCTION()
    void HandleMatchPhaseChanged(EMatchPhase NewPhase);

    UFUNCTION()
    void HandleMatchResultUpdated(const FConnectItMatchResult& Result);

    UPROPERTY()
    TObjectPtr<AConnectIt_GameState> ResolvedSource = nullptr;

    EMatchPhase CachedMatchPhase = EMatchPhase::WaitingForParticipants;
    FConnectItMatchResult CachedMatchResult;
    bool bSourceValid = false;
};
