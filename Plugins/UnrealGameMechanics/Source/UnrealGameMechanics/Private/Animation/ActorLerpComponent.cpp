// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/ActorLerpComponent.h"


UActorLerpComponent::UActorLerpComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UActorLerpComponent::StartLerpBatch(const TArray<FActorLerpInfo>& LerpInfos)
{
	if (IsLerping())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ActorLerpComponent: StartLerpBatch called while a batch is already "
				 "in progress -- replacing it"));
	}

	ActiveLerps.Reset();
	ActiveLerps.Reserve(LerpInfos.Num());

	for (const FActorLerpInfo& Info : LerpInfos)
	{
		FActiveActorLerp& NewLerp = ActiveLerps.AddDefaulted_GetRef();
		NewLerp.Info = Info;
	}

	SetComponentTickEnabled(true);
}

void UActorLerpComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveLerps.IsEmpty())
	{
		SetComponentTickEnabled(false);
		return;
	}

	bool bAllComplete = true;
	for (FActiveActorLerp& Lerp : ActiveLerps)
	{
		if (!Lerp.bComplete)
		{
			TickOneLerp(Lerp, DeltaTime);
		}
		bAllComplete &= Lerp.bComplete;
	}

	if (bAllComplete)
	{
		ActiveLerps.Reset();
		SetComponentTickEnabled(false);
		OnLerpComplete.Broadcast();
	}
}

bool UActorLerpComponent::TickOneLerp(FActiveActorLerp& Lerp, float DeltaTime) const
{
	if (Lerp.bComplete) return false;

	// Nothing to animate -- skip immediately, counts toward batch completion.
	if (!IsValid(Lerp.Info.ActorToLerp) || Lerp.Info.Locations.Num() < 2)
	{
		Lerp.bComplete = true;
		return true;
	}

	Lerp.ElapsedInSegment += DeltaTime;

	const float SegmentTime = FMath::Max(Lerp.Info.LerpTimePointToPoint, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(Lerp.ElapsedInSegment / SegmentTime, 0.f, 1.f);

	const FVector SegmentStart = Lerp.Info.Locations[Lerp.NextWaypointIndex - 1];
	const FVector SegmentEnd   = Lerp.Info.Locations[Lerp.NextWaypointIndex];
	Lerp.Info.ActorToLerp->SetActorLocation(FMath::Lerp(SegmentStart, SegmentEnd, Alpha));

	if (Alpha < 1.f)
	{
		return false;
	}

	// Segment finished -- carry any overshoot into the next segment instead
	// of dropping it, so a very short LerpTimePointToPoint relative to frame
	// time doesn't visibly lag a multi-segment path.
	const float Overflow = Lerp.ElapsedInSegment - SegmentTime;
	Lerp.NextWaypointIndex++;
	Lerp.ElapsedInSegment = FMath::Max(Overflow, 0.f);

	if (Lerp.NextWaypointIndex >= Lerp.Info.Locations.Num())
	{
		// Snap to the exact final waypoint -- avoids any float drift from
		// the overflow carry leaving the actor a hair off its destination.
		Lerp.Info.ActorToLerp->SetActorLocation(Lerp.Info.Locations.Last());
		Lerp.bComplete = true;
		return true;
	}

	return false;
}
