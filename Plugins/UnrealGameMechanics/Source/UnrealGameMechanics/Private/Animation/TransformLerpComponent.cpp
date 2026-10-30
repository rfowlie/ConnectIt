// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/TransformLerpComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"


UTransformLerpComponent::UTransformLerpComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTransformLerpComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureTarget();
}

void UTransformLerpComponent::SetTarget(USceneComponent* InTarget)
{
	Target = InTarget;
	bHasTarget = false;
	Alpha = 0.f;
	EnsureTarget();
}

void UTransformLerpComponent::EnsureTarget()
{
	if (bHasTarget) return;
	if (!Target && GetOwner())
	{
		Target = GetOwner()->GetRootComponent();
	}
	if (!Target) return;

	From = bStartFromCurrent ? Target->GetRelativeTransform() : StartTransform;
	To = bEndRelativeToStart ? EndTransform * From : EndTransform;
	bHasTarget = true;
}

void UTransformLerpComponent::PlayForward()
{
	Play(1.f);
}

void UTransformLerpComponent::PlayReverse()
{
	Play(-1.f);
}

void UTransformLerpComponent::Play(float InDirection)
{
	EnsureTarget();
	if (!bHasTarget) return;

	Direction = InDirection;
	SetComponentTickEnabled(true);
}

void UTransformLerpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Direction == 0.f || !bHasTarget) return;

	Alpha = Duration > 0.f ? FMath::Clamp(Alpha + Direction * DeltaTime / Duration, 0.f, 1.f) : (Direction > 0.f ? 1.f : 0.f);
	Apply();

	const bool bAtEnd = Direction > 0.f && Alpha >= 1.f;
	const bool bAtStart = Direction < 0.f && Alpha <= 0.f;
	if (bAtEnd || bAtStart)
	{
		Direction = 0.f;
		SetComponentTickEnabled(false);
		OnLerpFinished.Broadcast(bAtEnd);
	}
}

void UTransformLerpComponent::Apply() const
{
	if (!Target) return;

	FTransform Blended;
	Blended.SetLocation(FMath::Lerp(From.GetLocation(), To.GetLocation(), Alpha));
	Blended.SetRotation(FQuat::Slerp(From.GetRotation(), To.GetRotation(), Alpha));
	Blended.SetScale3D(FMath::Lerp(From.GetScale3D(), To.GetScale3D(), Alpha));
	Target->SetRelativeTransform(Blended);
}
