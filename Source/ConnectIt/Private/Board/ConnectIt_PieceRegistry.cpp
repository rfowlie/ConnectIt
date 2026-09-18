// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_PieceRegistry.h"
#include "Board/ConnectIt_BoardStateComponent.h"
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
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    if (!IsValid(BoardState)) return;

    const FConnectItBoardChangeEvent& ChangeEvent = BoardState->GetChangeEvent();
    if (ChangeEvent.ShiftStartPositions.Num() != ChangeEvent.ShiftEndPositions.Num())
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
    MovingPieces.Reserve(ChangeEvent.ShiftStartPositions.Num());
    for (const FGridPosition& StartPos : ChangeEvent.ShiftStartPositions)
    {
        TObjectPtr<AGridPieceBase>* Found = PieceMap.Find(StartPos);
        MovingPieces.Add(Found ? *Found : nullptr);
    }

    for (const FGridPosition& StartPos : ChangeEvent.ShiftStartPositions)
    {
        PieceMap.Remove(StartPos);
    }

    for (int32 Index = 0; Index < ChangeEvent.ShiftEndPositions.Num(); Index++)
    {
        if (AGridPieceBase* Piece = MovingPieces[Index])
        {
            PieceMap.Add(ChangeEvent.ShiftEndPositions[Index], Piece);
        }
    }
}

void UConnectIt_PieceRegistry::HandleBoardPiecesSwapped()
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    if (!IsValid(BoardState)) return;

    const FConnectItBoardChangeEvent& ChangeEvent = BoardState->GetChangeEvent();
    const FGridPosition& A = ChangeEvent.SwapPositionA;
    const FGridPosition& B = ChangeEvent.SwapPositionB;

    TObjectPtr<AGridPieceBase>* FoundA = PieceMap.Find(A);
    TObjectPtr<AGridPieceBase>* FoundB = PieceMap.Find(B);
    AGridPieceBase* PieceA = FoundA ? FoundA->Get() : nullptr;
    AGridPieceBase* PieceB = FoundB ? FoundB->Get() : nullptr;

    if (PieceB) PieceMap.Add(A, PieceB); else PieceMap.Remove(A);
    if (PieceA) PieceMap.Add(B, PieceA); else PieceMap.Remove(B);
}

void UConnectIt_PieceRegistry::HandleBoardPieceRemoved()
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    if (!IsValid(BoardState)) return;

    PieceMap.Remove(BoardState->GetChangeEvent().RemovedPosition);
}
