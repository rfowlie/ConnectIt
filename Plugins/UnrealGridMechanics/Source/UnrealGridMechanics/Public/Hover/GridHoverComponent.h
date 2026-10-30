// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "GridHoverComponent.generated.h"

class UGridHoverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridHoverComponentDelegate, UGridHoverComponent*, Component);

/*
 * Makes its owner hoverable and clickable with no Blueprint wiring: on
 * BeginPlay it binds the owner's native cursor events (Begin / End Cursor
 * Over, Clicked) in C++ and broadcasts OnHoverBegin, OnHoverEnd and
 * OnClicked. It decides nothing -- listeners (UGridHoverSubsystem, and through
 * it the player actions) decide what a hover or click means.
 *
 * Hover needs a primitive of the owner that blocks the cursor trace channel
 * (map check warns when none does). Local only: cursor events happen on the
 * machine with the mouse; nothing replicates.
 */
UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UGridHoverComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UGridHoverComponent();

    // The cursor arrived (only while enabled and CanHover allows it)
    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverComponentDelegate OnHoverBegin;

    // The cursor left after a hover began (or hover was disabled during one)
    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverComponentDelegate OnHoverEnd;

    // The owner was clicked (only while enabled and CanHover allows it)
    UPROPERTY(BlueprintAssignable, Category = "Grid|Hover")
    FGridHoverComponentDelegate OnClicked;

    // What kind of hoverable this is, for listeners that filter (e.g. a tile,
    // a map node). Empty = none given.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Hover")
    FGameplayTag HoverCategory;

    // Off: no hover or click is reported. Turning it off during a hover ends
    // the hover (OnHoverEnd).
    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void SetHoverEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Grid|Hover")
    bool IsHoverEnabled() const { return bHoverEnabled; }

    UFUNCTION(BlueprintPure, Category = "Grid|Hover")
    bool IsHovered() const { return bHovered; }

    // A veto on hover and click (e.g. a tile that shouldn't react right now).
    // Default: always allowed.
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Hover")
    bool CanHover() const;

    // What the owner's cursor events call; also for other input (touch,
    // gamepad) to drive the same path
    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void NotifyHoverBegin();

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void NotifyHoverEnd();

    UFUNCTION(BlueprintCallable, Category = "Grid|Hover")
    void NotifyClicked();

    // Whether any primitive of the owner can be hit by the cursor trace
    bool DoesOwnerBlockCursorTrace() const;

    // What stops the cursor reaching this owner, for the map check: nothing
    // blocks the cursor channel (UGridMechanicsSettings) -- and, after a
    // switch from Visibility, whether it blocks only Visibility. Empty if
    // the owner can be hovered.
    FString GetCursorTraceProblem() const;

#if WITH_EDITOR
    virtual void CheckForErrors() override;
#endif

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    bool bHoverEnabled = true;
    bool bHovered = false;

    UFUNCTION()
    void HandleOwnerBeginCursorOver(AActor* TouchedActor);

    UFUNCTION()
    void HandleOwnerEndCursorOver(AActor* TouchedActor);

    UFUNCTION()
    void HandleOwnerClicked(AActor* TouchedActor, FKey ButtonPressed);
};
