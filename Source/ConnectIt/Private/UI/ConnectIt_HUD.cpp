// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ConnectIt_HUD.h"

#include "Animation/ActorLerpComponent.h"
#include "Board/Shift/GridMechanics_GridShiftLibrary.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "ConnectIt_Structs.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Tile/GridTileBase.h"
#include "Tile/GridTileRegistryBase.h"


AConnectIt_HUD::AConnectIt_HUD()
{
	ShiftLerpComponent = CreateDefaultSubobject<UActorLerpComponent>(TEXT("ShiftLerpComponent"));
}

void AConnectIt_HUD::BeginPlay()
{
	Super::BeginPlay();

	if (UGameEventTaskSubsystem* GameEventSubsystem = GetWorld()->GetSubsystem<UGameEventTaskSubsystem>())
	{
		GameEventSubsystem->BindOnTagComplete(
			ConnectIt_Event_BoardShifted, this,
			GET_FUNCTION_NAME_CHECKED(AConnectIt_HUD, HandleBoardShiftVisualEvent));
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("ConnectIt_HUD: BeginPlay -- no UGameEventTaskSubsystem in world, "
				 "board-shift visuals will not fire"));
	}
}

void AConnectIt_HUD::HandleBoardShiftVisualEvent(FGameplayTag EventTag)
{
	const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
	if (!IsValid(BoardState)) return;

	const FConnectItBoardChangeEvent& ChangeEvent = BoardState->GetChangeEvent();
	if (!ChangeEvent.bBoardShifted) return;

	UGridTileRegistryBase* TileRegistry = UConnectIt_GameUtilityLibrary::GetTileRegistry(this);
	if (!IsValid(TileRegistry) || !IsValid(ShiftLerpComponent)) return;

	// Resolve every moved tile's actor from its OLD position before
	// building the batch -- nothing here calls SetActorLocation itself
	// (that's ShiftLerpComponent's job once StartLerpBatch runs), so
	// there's no risk of one tile's move confusing another's lookup.
	const int32 Num = FMath::Min(
		ChangeEvent.ShiftStartPositions.Num(), ChangeEvent.ShiftEndPositions.Num());

	TArray<FActorLerpInfo> LerpBatch;
	LerpBatch.Reserve(Num);

	for (int32 Index = 0; Index < Num; Index++)
	{
		AGridTileBase* TileActor = TileRegistry->GetTileAtPosition(ChangeEvent.ShiftStartPositions[Index]);
		if (!IsValid(TileActor))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ConnectIt_HUD: HandleBoardShiftVisualEvent -- no tile actor at "
					 "(%d,%d), skipping its visual"),
				ChangeEvent.ShiftStartPositions[Index].X, ChangeEvent.ShiftStartPositions[Index].Y);
			continue;
		}

		const FVector StartWorld = TileRegistry->GridPositionToWorld(ChangeEvent.ShiftStartPositions[Index]);
		const FVector EndWorld   = TileRegistry->GridPositionToWorld(ChangeEvent.ShiftEndPositions[Index]);

		FActorLerpInfo Info;
		Info.ActorToLerp = TileActor;
		Info.Locations = UGridMechanics_GridShiftLibrary::BuildRaisedShiftPath(
			StartWorld, EndWorld, ShiftRaiseHeight);
		Info.LerpTimePointToPoint = ShiftLerpSegmentDuration;

		LerpBatch.Add(MoveTemp(Info));
	}

	if (!LerpBatch.IsEmpty())
	{
		ShiftLerpComponent->StartLerpBatch(LerpBatch);
	}
}
