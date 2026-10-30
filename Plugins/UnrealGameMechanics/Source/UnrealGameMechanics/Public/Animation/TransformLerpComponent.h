// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TransformLerpComponent.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransformLerpFinished, bool, bAtEnd);

// Blends one scene component between two relative transforms over Duration
// seconds, forward (start -> end) or back -- e.g. a node scaling up while
// hovered. Reversing mid-way turns around from where it is. Location and
// scale lerp, rotation slerps. Purely local and visual, never replicated.
//
// The start is the target's relative transform when play begins (or
// StartTransform); the end is EndTransform applied on top of the start (or
// EndTransform itself, with bEndRelativeToStart off). Its sibling
// UActorLerpComponent moves whole actors along waypoint paths.
UCLASS(ClassGroup=(Animation), meta=(BlueprintSpawnableComponent))
class UNREALGAMEMECHANICS_API UTransformLerpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTransformLerpComponent();

	// Seconds from start to end
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp", meta = (ClampMin = 0))
	float Duration = 0.15f;

	// The end: applied on top of the start (bEndRelativeToStart), else the
	// target's relative transform itself. Default: 15% bigger.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp")
	FTransform EndTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.15f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp")
	bool bEndRelativeToStart = true;

	// Take the start from the target when play begins (else StartTransform)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp")
	bool bStartFromCurrent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lerp", meta = (EditCondition = "!bStartFromCurrent"))
	FTransform StartTransform;

	// What moves (default: the owner's root component). Set before play
	// begins, or call SetTarget.
	UFUNCTION(BlueprintCallable, Category = "Lerp")
	void SetTarget(USceneComponent* InTarget);

	UFUNCTION(BlueprintCallable, Category = "Lerp")
	void PlayForward();

	UFUNCTION(BlueprintCallable, Category = "Lerp")
	void PlayReverse();

	// 0 = at the start, 1 = at the end
	UFUNCTION(BlueprintPure, Category = "Lerp")
	float GetAlpha() const { return Alpha; }

	// Reached the end (true) or back at the start (false)
	UPROPERTY(BlueprintAssignable, Category = "Lerp")
	FOnTransformLerpFinished OnLerpFinished;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void EnsureTarget();
	void Play(float InDirection);
	void Apply() const;

	UPROPERTY()
	TObjectPtr<USceneComponent> Target;

	FTransform From;
	FTransform To;
	float Alpha = 0.f;
	float Direction = 0.f;
	bool bHasTarget = false;
};
