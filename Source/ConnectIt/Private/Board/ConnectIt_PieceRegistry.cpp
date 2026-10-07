// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_PieceRegistry.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "Framework/Library/ConnectIt_BoardEventLibrary.h"
#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Piece/GridPieceBase.h"


UConnectIt_BoardStateComponent* UConnectIt_PieceRegistry::GetBoardState() const
{
    return UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
}

void UConnectIt_PieceRegistry::HandleBoardShifted()
{
    const FConnectItBoardEvent_BoardShifted* Shift =
        UConnectIt_BoardEventLibrary::GetActiveBoardEventAs<FConnectItBoardEvent_BoardShifted>(this);
    if (!Shift)
    {
        UE_LOG(LogTemp, Error, TEXT(
            "ConnectIt_PieceRegistry: HandleBoardShifted — "
            "the board event being played is not a Board Shifted event"));
        return;
    }

    if (Shift->StartPositions.Num() != Shift->EndPositions.Num())
    {
        UE_LOG(LogTemp, Error, TEXT(
            "ConnectIt_PieceRegistry: HandleBoardShifted — "
            "ShiftStartPositions/ShiftEndPositions length mismatch"));
        return;
    }

    // This is a rotation, not independent moves -- a later pair's Start
    // position can equal an earlier pair's End position, so read every
    // moving piece BEFORE writing any of them (two passes). A naive
    // per-pair Remove-then-Add would stomp data a later iteration still
    // needs to read.
    TArray<TObjectPtr<AGridPieceBase>> MovingPieces;
    MovingPieces.Reserve(Shift->StartPositions.Num());
    for (const FGridPosition& StartPos : Shift->StartPositions)
    {
        TObjectPtr<AGridPieceBase>* Found = PieceMap.Find(StartPos);
        MovingPieces.Add(Found ? *Found : nullptr);
    }

    for (const FGridPosition& StartPos : Shift->StartPositions)
    {
        PieceMap.Remove(StartPos);
    }

    for (int32 Index = 0; Index < Shift->EndPositions.Num(); Index++)
    {
        if (AGridPieceBase* Piece = MovingPieces[Index])
        {
            PieceMap.Add(Shift->EndPositions[Index], Piece);
        }
    }
}

void UConnectIt_PieceRegistry::HandleBoardPiecesSwapped()
{
    const FConnectItBoardEvent_PiecesSwapped* Swap =
        UConnectIt_BoardEventLibrary::GetActiveBoardEventAs<FConnectItBoardEvent_PiecesSwapped>(this);
    if (!Swap)
    {
        UE_LOG(LogTemp, Error, TEXT(
            "ConnectIt_PieceRegistry: HandleBoardPiecesSwapped — "
            "the board event being played is not a Pieces Swapped event"));
        return;
    }

    const FGridPosition& A = Swap->PositionA;
    const FGridPosition& B = Swap->PositionB;

    TObjectPtr<AGridPieceBase>* FoundA = PieceMap.Find(A);
    TObjectPtr<AGridPieceBase>* FoundB = PieceMap.Find(B);
    AGridPieceBase* PieceA = FoundA ? FoundA->Get() : nullptr;
    AGridPieceBase* PieceB = FoundB ? FoundB->Get() : nullptr;

    if (PieceB) PieceMap.Add(A, PieceB); else PieceMap.Remove(A);
    if (PieceA) PieceMap.Add(B, PieceA); else PieceMap.Remove(B);
}

void UConnectIt_PieceRegistry::HandleBoardPieceRemoved()
{
    const FConnectItBoardEvent_PieceRemoved* Removed =
        UConnectIt_BoardEventLibrary::GetActiveBoardEventAs<FConnectItBoardEvent_PieceRemoved>(this);
    if (!Removed)
    {
        UE_LOG(LogTemp, Error, TEXT(
            "ConnectIt_PieceRegistry: HandleBoardPieceRemoved — "
            "the board event being played is not a Piece Removed event"));
        return;
    }

    PieceMap.Remove(Removed->Position);
}
