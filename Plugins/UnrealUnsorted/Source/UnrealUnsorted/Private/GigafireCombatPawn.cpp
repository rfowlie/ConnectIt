// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Pawn/GigafireCombatPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"


AGigafireCombatPawn::AGigafireCombatPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create the Spring Arm
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 1200.0f;  // Adjust for desired zoom height
	SpringArmComponent->bDoCollisionTest = false;  // No need for collision in grid-based games

	// Create Camera and Attach to Spring Arm
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

	// default state
	Execute_SetFollowCursor(this);
}

void AGigafireCombatPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bCursorCanTick && FollowDelegate.IsBound()) { FollowDelegate.Execute(DeltaTime); }	
}

void AGigafireCombatPawn::SetMapBounds(FVector2D MinBounds, FVector2D MaxBounds)
{
	MapMinBounds = MinBounds;
	MapMaxBounds = MaxBounds;
}

void AGigafireCombatPawn::SetCursorCanTick_Implementation(const bool bActive)
{
	bCursorCanTick = bActive;
}

void AGigafireCombatPawn::SetFollowCursor_Implementation()
{
	FollowDelegate.Unbind();
	FollowDelegate.BindUObject(this, &ThisClass::HandleFollowCursor);
	Execute_SetCursorCanTick(this, true);
}

void AGigafireCombatPawn::SetFollowTarget_Implementation(AActor* InTarget)
{
	if (!IsValid(InTarget))
	{
		if (OnFollowTargetComplete.IsBound()) { OnFollowTargetComplete.Broadcast(); }
		return;
	}

	FollowTarget = InTarget;
	FollowDelegate.Unbind();
	FollowDelegate.BindUObject(this, &ThisClass::HandleFollowTarget);
	Execute_SetCursorCanTick(this, true);
}

void AGigafireCombatPawn::SetMoveToLocation_Implementation(FVector Location)
{
	MoveToLocation = Location;
	FollowDelegate.Unbind();
	FollowDelegate.BindUObject(this, &ThisClass::HandleMoveToLocation);
	Execute_SetCursorCanTick(this, true);
}

void AGigafireCombatPawn::HandleFollowCursor(float DeltaTime)
{
	// seems janky
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	float MouseX, MouseY;
	PC->GetMousePosition(MouseX, MouseY);
	int32 ScreenX, ScreenY;
	PC->GetViewportSize(ScreenX, ScreenY);
	FVector2D PositionPercentage = FVector2D(MouseX/ScreenX, MouseY/ScreenY);

	FVector MouseDirection = FVector::ZeroVector;
	// flip directions as in screenspace X is left right and Y is up down
	MouseDirection.Y = PositionPercentage.X < EdgeThresholdPercentX ? -1.f :
		PositionPercentage.X > 1.f - EdgeThresholdPercentX ? 1.f : 0;
	MouseDirection.X = PositionPercentage.Y < EdgeThresholdPercentY ? 1.f :
		PositionPercentage.Y > 1.f - EdgeThresholdPercentY ? -1.f : 0;

	// move direction should be relative to actor
	FVector MoveDirection = MouseDirection.RotateAngleAxis(
		FMath::RoundToFloat(GetActorRotation().Yaw), FVector::UpVector);

	// get new actor location after factoring move amount
	FVector NewLocation = GetActorLocation() + (MoveDirection * CursorFollowSpeed * DeltaTime);

	// UE_LOG(LogTemp, Log, TEXT("Forward: %s"), *GetActorForwardVector().ToString());
	// UE_LOG(LogTemp, Log, TEXT("Mouse: %s"), *MouseDirection.ToString());
	// UE_LOG(LogTemp, Log, TEXT("Move: %s"), *MoveDirection.ToString());

	// Clamp movement within map boundaries
	NewLocation.X = FMath::Clamp(NewLocation.X, MapMinBounds.X, MapMaxBounds.X);
	NewLocation.Y = FMath::Clamp(NewLocation.Y, MapMinBounds.Y, MapMaxBounds.Y);

	SetActorLocation(NewLocation);
}

void AGigafireCombatPawn::HandleFollowTarget(float DeltaTime)
{
	if (IsValid(FollowTarget))
	{
		FVector NewLocation = FMath::VInterpTo(GetActorLocation(), FollowTarget->GetActorLocation(), DeltaTime, TargetFollowSpeed);
		NewLocation.Z = GetActorLocation().Z;
		SetActorLocation(NewLocation);

		const FVector2D Self = FVector2D(GetActorLocation());
		const FVector2D Target = FVector2D(FollowTarget->GetActorLocation());
		if (FMath::Abs((Target - Self).Size()) < FollowThreshold)
		{
			if (OnFollowTargetComplete.IsBound()) { OnFollowTargetComplete.Broadcast(); }
		}
	}
	else
	{
		FollowTarget = nullptr;
		FollowDelegate.BindUObject(this, &ThisClass::HandleFollowCursor);
	}
}

void AGigafireCombatPawn::HandleMoveToLocation(float DeltaTime)
{
	FVector NewLocation = FMath::VInterpTo(GetActorLocation(), MoveToLocation, DeltaTime, TargetFollowSpeed);
	// keeps pawn at same height
	NewLocation.Z = GetActorLocation().Z;
	SetActorLocation(NewLocation);

	const FVector2D Self = FVector2D(NewLocation);
	const FVector2D Target = FVector2D(MoveToLocation);
	if (FMath::Abs((Target - Self).Size()) < FollowThreshold)
	{
		if (OnFollowTargetComplete.IsBound()) { OnFollowTargetComplete.Broadcast(); }
	}
}

