// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActorLerpComponent.generated.h"

// One actor's waypoint path -- pure input/config, no runtime state. Locations
// are world-space; UActorLerpComponent moves ActorToLerp linearly through
// them in order, spending LerpTimePointToPoint seconds on each segment.
// 0 or 1 Locations is treated as nothing-to-do (skipped, counts toward batch
// completion immediately) -- not an error, since a caller building a batch
// from mixed data shouldn't have to filter degenerate entries out first.
USTRUCT(BlueprintType)
struct FActorLerpInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp")
	TArray<FVector> Locations;

	// Seconds spent on each point-to-point segment -- the same value for
	// every segment of this actor's path (a 4-point path takes 3x this).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp")
	float LerpTimePointToPoint = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp")
	TObjectPtr<AActor> ActorToLerp = nullptr;
};

// Fires once every entry in the most recent StartLerpBatch has finished (or
// was skipped for having <2 Locations / an invalid actor) -- not per-actor.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActorLerpBatchComplete);

// Moves a batch of actors through their own independent waypoint sequences
// in parallel, purely via SetActorLocation on Tick -- no grid/board/game
// knowledge at all. Fires OnLerpComplete once the whole batch is done.
//
// Purely a local visual mover, never replicated -- whatever real state
// change this is visualizing (e.g. a board shift) already replicated on its
// own; this component just reacts to it independently on whichever machine
// it's ticking on.
//
// Runs one batch at a time -- a second StartLerpBatch call while one is
// still in progress replaces it (logged as a Warning, not silently
// dropped). A caller needing two overlapping batches should use two
// components.
UCLASS(ClassGroup=(Animation), meta=(BlueprintSpawnableComponent))
class UNREALGAMEMECHANICS_API UActorLerpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActorLerpComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Lerp")
	void StartLerpBatch(const TArray<FActorLerpInfo>& LerpInfos);

	UFUNCTION(BlueprintPure, Category = "Lerp")
	bool IsLerping() const { return ActiveLerps.Num() > 0; }

	UPROPERTY(BlueprintAssignable, Category = "Lerp")
	FOnActorLerpBatchComplete OnLerpComplete;

private:

	struct FActiveActorLerp
	{
		FActorLerpInfo Info;

		// Index into Info.Locations we're currently moving TOWARD -- starts
		// at 1 (index 0 is where the actor already is).
		int32 NextWaypointIndex = 1;

		float ElapsedInSegment = 0.f;
		bool bComplete = false;
	};

	TArray<FActiveActorLerp> ActiveLerps;

	// Advances one entry by DeltaTime, moving ActorToLerp via
	// SetActorLocation. Returns true once this call is the one that
	// completes it (so a future caller could distinguish "just finished"
	// from "was already finished" if that ever matters).
	bool TickOneLerp(FActiveActorLerp& Lerp, float DeltaTime) const;
};
