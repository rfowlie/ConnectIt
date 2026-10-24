// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/GigafirePawnHandler.h"
#include "GigafireCombatPawn.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFollowPawnDelegate);

UCLASS(Blueprintable, BlueprintType)
class GIGAFIRE_API AGigafireCombatPawn : public APawn, public IGigafirePawnHandler
{
	GENERATED_BODY()

public:
	AGigafireCombatPawn();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FollowThreshold = 0.25f;
	
	FFollowPawnDelegate OnFollowTargetComplete;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCursorCanTick = false;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* CameraComponent;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* SpringArmComponent;
	
	UPROPERTY(EditAnywhere, Category="_Settings")
	float FollowSpeed = 5.0f;  // Smooth camera follow speed

	UPROPERTY(EditAnywhere, Category="_Settings")
	float CursorFollowSpeed = 2000.0f;  // Speed of camera movement with mouse

	// distance from each screen edge we want to trigger at
	UPROPERTY(EditAnywhere, Category="_Settings", meta=(ClampMin=0.f, ClampMax=0.5f, UIMin=0.f, UIMax=0.5f))
	float EdgeThresholdPercentX = 0.15f;
	UPROPERTY(EditAnywhere, Category="_Settings", meta=(ClampMin=0.f, ClampMax=0.5f, UIMin=0.f, UIMax=0.5f))
	float EdgeThresholdPercentY = 0.1f;

	// Map bounds (required otherwise player might shoot way out from the map)
	UPROPERTY(EditAnywhere, Category="_Settings")
	FVector2D MapMinBounds = FVector2D(-5000, -5000);
	UPROPERTY(EditAnywhere, Category="_Settings")
	FVector2D MapMaxBounds = FVector2D(5000, 5000);

	UFUNCTION(BlueprintCallable)
	void SetMapBounds(FVector2D MinBounds, FVector2D MaxBounds);

	// Handle
	UFUNCTION()
	void HandleFollowCursor(float DeltaTime);
	
	UPROPERTY()
	AActor* FollowTarget;

	UPROPERTY(EditAnywhere, Category="_Settings")
	float TargetFollowSpeed = 5.0f;
	
	UFUNCTION()
	void HandleFollowTarget(float DeltaTime);

	DECLARE_DELEGATE_OneParam(FFollowDelegate, float);
	FFollowDelegate FollowDelegate;

	UFUNCTION()
	void HandleMoveToLocation(float DeltaTime);

	UPROPERTY()
	FVector MoveToLocation;

	// Interface
	virtual void SetCursorCanTick_Implementation(const bool bActive) override;
	virtual void SetFollowCursor_Implementation() override;
	virtual void SetFollowTarget_Implementation(AActor* InTarget) override;
	virtual void SetMoveToLocation_Implementation(FVector Location) override;
};
