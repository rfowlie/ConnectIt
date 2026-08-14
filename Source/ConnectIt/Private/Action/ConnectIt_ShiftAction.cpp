// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_ShiftAction.h"
#include "Board/ConnectIt_BoardManager.h"
#include "ConnectIt_Structs.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UConnectIt_ShiftAction::UConnectIt_ShiftAction()
{
    // Optional -- a shift is a choice, not a requirement to end the turn.
    // No tile selection involved (row/column/direction comes from whatever
    // UI calls RequestShift directly), so bRequiresSelection stays false.
    bIsRequired = false;
    bIsCancellable = true;
    bRequiresSelection = false;
    MaxCompletionsPerTurn = 1;
    CooldownTurns = 0;
}

void UConnectIt_ShiftAction::PostInitialiseAction_Implementation()
{
    BoardManager = UConnectIt_GameUtilityLibrary::GetBoardManager(OwningController);

    TurnBasedParticipantManager = IsValid(OwningController)
        ? OwningController->FindComponentByClass<UTurnBasedParticipantComponent>()
        : nullptr;

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ShiftAction: PostInitialiseAction -- "
                 "no AConnectIt_BoardManager found"));
    }

    if (!IsValid(TurnBasedParticipantManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ShiftAction: PostInitialiseAction -- "
                 "no UTurnBasedParticipantComponent on owning controller"));
    }
}

void UConnectIt_ShiftAction::RequestShift(FShiftOperation Operation)
{
    FTurnActionRequest Request;
    Request.RequestType = Tag_RequestType;
    Request.FactionID = GetOwningFactionID();
    Request.Payload.InitializeAs<FConnectItRequestBoardShift>(
        FConnectItRequestBoardShift{ .ShiftOperation = Operation });

    UE_LOG(LogTemp, Log,
        TEXT("ShiftAction: Requesting shift — %s %d by %d, faction %d"),
        Operation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
        Operation.Index,
        Operation.GetSignedAmount(),
        Request.FactionID);

    // Route to action component which sends to server
    // Complete fires after request is sent -- not after server confirms
    // Server confirmation comes via board state replication
    RequestBoardChange(Request);
    Complete();
}

int32 UConnectIt_ShiftAction::GetOwningFactionID() const
{
    return IsValid(TurnBasedParticipantManager)
        ? TurnBasedParticipantManager->GetActiveParticipantSlotIndex()
        : -1;
}
