// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedAction.h"
#include "Board/Shift/GridShiftTypes.h"
#include "ConnectIt_ShiftAction.generated.h"


class UTurnBasedParticipantComponent;
class AConnectIt_BoardManager;

// ConnectIt concrete action -- shifts a row or column of the board
// Deliberately minimal: does not decide which row/column/direction to shift
// itself -- whatever UI/input eventually drives that decision calls
// RequestShift(Operation) directly. Responsibilities here are just the
// FTurnActionRequest plumbing, mirroring UConnectIt_PlacePieceAction.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_ShiftAction : public UTurnBasedAction
{
    GENERATED_BODY()

    UConnectIt_ShiftAction();

public:

    // Request type tag sent to board manager -- must match
    // ConnectIt_Game_Shift (Source/ConnectIt/Public/ConnectIt_GameplayTags.h)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag Tag_RequestType;

    // Entry point for whatever UI/input decides which row/column/direction
    // to shift -- builds the request and completes the action. Board
    // manager handles all mutation; this action has no knowledge of it.
    UFUNCTION(BlueprintCallable, Category = "Action")
    void RequestShift(FShiftOperation Operation);

protected:

    // Resolves and caches BoardManager/TurnBasedParticipantManager via
    // OwningController -- mirrors UConnectIt_PlacePieceAction's pattern.
    virtual void PostInitialiseAction_Implementation() override;

private:

    UPROPERTY()
    TObjectPtr<AConnectIt_BoardManager> BoardManager = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedParticipantComponent> TurnBasedParticipantManager = nullptr;

    // Gets owning faction ID from participant component slot index
    int32 GetOwningFactionID() const;
};
