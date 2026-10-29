// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_PieceRegistry.h"
#include "Board/Actor/ConnectIt_PieceBase.h"
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

AConnectIt_PieceBase* UConnectIt_PieceRegistry::GetPieceAt(FGridPosition Position) const
{
    const TObjectPtr<AGridPieceBase>* Found = PieceMap.Find(Position);
    AGridPieceBase* Piece = Found ? Found->Get() : nullptr;
    if (!Piece) return nullptr;

    AConnectIt_PieceBase* ConnectItPiece = Cast<AConnectIt_PieceBase>(Piece);
    if (!ConnectItPiece)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_PieceRegistry: the piece at (%d, %d) is a %s, not a ConnectIt piece (AConnectIt_PieceBase)"),
            Position.X, Position.Y, *Piece->GetClass()->GetName());
    }
    return ConnectItPiece;
}

AGridPieceBase* UConnectIt_PieceRegistry::MovePieceActor(FGridPosition From, FGridPosition To)
{
    TObjectPtr<AGridPieceBase> Piece;
    if (!PieceMap.RemoveAndCopyValue(From, Piece) || !Piece) return nullptr;

    PieceMap.Add(To, Piece);
    return Piece;
}

void UConnectIt_PieceRegistry::MapPiece(const FGridPosition& Position, AGridPieceBase* Piece)
{
    if (!Piece) return;
    PieceMap.Add(Position, Piece);
}

AGridPieceBase* UConnectIt_PieceRegistry::UnmapPiece(const FGridPosition& Position)
{
    TObjectPtr<AGridPieceBase> Piece;
    PieceMap.RemoveAndCopyValue(Position, Piece);
    return Piece;
}

void UConnectIt_PieceRegistry::SwapPieceActors(const FGridPosition& A, const FGridPosition& B)
{
    TObjectPtr<AGridPieceBase> PieceA;
    TObjectPtr<AGridPieceBase> PieceB;
    PieceMap.RemoveAndCopyValue(A, PieceA);
    PieceMap.RemoveAndCopyValue(B, PieceB);
    if (PieceB) PieceMap.Add(A, PieceB);
    if (PieceA) PieceMap.Add(B, PieceA);
}

void UConnectIt_PieceRegistry::ShiftPieceActors(
    const TArray<FGridPosition>& StartPositions, const TArray<FGridPosition>& EndPositions)
{
    if (StartPositions.Num() != EndPositions.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_PieceRegistry: ShiftPieceActors -- start / end positions differ in count"));
        return;
    }

    // A rotation, not independent moves: a later pair's start can be an
    // earlier pair's end, so read every moving piece before writing any
    TArray<TObjectPtr<AGridPieceBase>> Moving;
    Moving.Reserve(StartPositions.Num());
    for (const FGridPosition& Start : StartPositions)
    {
        TObjectPtr<AGridPieceBase> Piece;
        PieceMap.RemoveAndCopyValue(Start, Piece);
        Moving.Add(Piece);
    }
    for (int32 Index = 0; Index < EndPositions.Num(); Index++)
    {
        if (Moving[Index]) PieceMap.Add(EndPositions[Index], Moving[Index]);
    }
}
