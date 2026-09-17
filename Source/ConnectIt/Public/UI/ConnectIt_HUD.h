// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Action/Interface/TurnBasedActionNotifier.h"
#include "GameFramework/HUD.h"
#include "ConnectIt_HUD.generated.h"

class UActorLerpComponent;

/**
 *
 */
UCLASS()
class CONNECTIT_API AConnectIt_HUD : public AHUD, public ITurnBasedActionNotifier
{
	GENERATED_BODY()

public:
	AConnectIt_HUD();

protected:
	virtual void BeginPlay() override;

	// Owns the client-only visual reaction to a board shift -- matches the
	// established "HUD owns client-only visual/UI concerns" precedent
	// (Actions UI). Purely presentational: reads the already-replicated
	// FConnectItBoardChangeEvent, never mutates board state.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Board Shift Visual")
	TObjectPtr<UActorLerpComponent> ShiftLerpComponent = nullptr;

	// Height (world units) tile actors lift by mid-shift so they don't
	// visually clip through neighboring tiles while sliding (tiles are
	// square). See UGridMechanics_GridShiftLibrary::BuildRaisedShiftPath.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Board Shift Visual")
	float ShiftRaiseHeight = 100.f;

	// Seconds spent on each of the 3 segments of a tile's raised path
	// (start->raised, raised->raised, raised->end) -- a full shift takes
	// 3x this per tile, all tiles animating in parallel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Board Shift Visual")
	float ShiftLerpSegmentDuration = 0.2f;

	// Bound to UGameEventTaskSubsystem's ConnectIt_Event_BoardShifted
	// sequence completing -- same BindOnTagComplete pattern
	// AConnectIt_GameMode already uses for ConnectIt_Event_PlayerWin, the
	// one other place this codebase reacts to a board-event tag today.
	UFUNCTION()
	void HandleBoardShiftVisualEvent(FGameplayTag EventTag);
};
