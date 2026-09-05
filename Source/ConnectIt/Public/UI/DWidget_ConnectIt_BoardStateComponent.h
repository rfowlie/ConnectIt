// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "ConnectIt_Structs.h"
#include "DWidget_ConnectIt_BoardStateComponent.generated.h"

class UConnectIt_BoardStateComponent;

// Tracks UConnectIt_BoardStateComponent specifically -- current board state
// and the most recent change event, pushed straight through to BP as each
// changes (see UDWidgetBase's own class comment for the push/GetInfo()
// convention). Only reacts to OnBoardStateChanged, so nothing outside a
// genuine board mutation ever triggers an update here. No cached fields, no
// pull-based Get*() accessors.
UCLASS(Abstract)
class CONNECTIT_API UDWidget_ConnectIt_BoardStateComponent : public UDWidgetBase
{
    GENERATED_BODY()

public:

    // False until UConnectIt_BoardStateComponent has actually been resolved
    // -- the one deliberately pull-only exception to the push model,
    // checked once rather than pushed.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ConnectIt|Debug")
    void OnCurrentStateUpdated(const FConnectItBoardState& NewState);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ConnectIt|Debug")
    void OnLastChangeEventUpdated(const FConnectItBoardChangeEvent& ChangeEvent);

private:

    UFUNCTION()
    void HandleBoardStateChanged();

    UPROPERTY()
    TObjectPtr<UConnectIt_BoardStateComponent> ResolvedSource = nullptr;

    bool bSourceValid = false;
};
