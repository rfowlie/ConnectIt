// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "ConnectIt_Structs.h"
#include "DWidget_ConnectIt_BoardStateComponent.generated.h"

class AConnectIt_BoardManager;
class UConnectIt_BoardStateComponent;
class UConnectIt_BoardManagerSubsystem;

// Tracks UConnectIt_BoardStateComponent specifically -- current board state
// and the most recent change event. Only reacts to OnBoardStateChanged, so
// nothing outside a genuine board mutation ever triggers a refresh here.
UCLASS(Abstract)
class CONNECTIT_API UDWidget_ConnectIt_BoardStateComponent : public UDWidgetBase
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    const FConnectItBoardState& GetCurrentState() const { return CachedCurrentState; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    const FConnectItBoardChangeEvent& GetLastChangeEvent() const { return CachedLastChangeEvent; }

    // False until UConnectIt_BoardStateComponent has actually been resolved
    // -- distinguishes "board genuinely empty" from "haven't found it yet"
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;
    virtual void RefreshFields() override;

private:

    // Board manager may not have registered with the subsystem yet at
    // widget-construct time (level-placed actor, BeginPlay-ordering
    // dependent) -- same documented ready-signal pattern
    // UConnectIt_DebugStateWidget uses, see
    // Workflows/BoardManagerSubsystem_Workflow.txt.
    void BindBoardManager(AConnectIt_BoardManager* InBoardManager);

    UFUNCTION()
    void HandleBoardManagerReady(AConnectIt_BoardManager* InBoardManager);

    UFUNCTION()
    void HandleBoardStateChanged();

    UPROPERTY()
    TObjectPtr<UConnectIt_BoardManagerSubsystem> ResolvedBoardManagerSubsystem = nullptr;

    UPROPERTY()
    TObjectPtr<UConnectIt_BoardStateComponent> ResolvedSource = nullptr;

    FConnectItBoardState CachedCurrentState;
    FConnectItBoardChangeEvent CachedLastChangeEvent;
    bool bSourceValid = false;
};
