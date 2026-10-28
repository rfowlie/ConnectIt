// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Hover/GridHoverComponent.h"
#include "GridHoverTestTypes.generated.h"


// --- Test-only types (UnrealGridMechanics.Hover). Not meant for real use. ---

// Counts what a hover component broadcasts
UCLASS(Hidden, NotBlueprintable)
class UGridHoverTestListener : public UObject
{
    GENERATED_BODY()

public:

    int32 Begins = 0;
    int32 Ends = 0;
    int32 Clicks = 0;

    void Listen(UGridHoverComponent* Component)
    {
        Component->OnHoverBegin.AddDynamic(this, &UGridHoverTestListener::HandleBegin);
        Component->OnHoverEnd.AddDynamic(this, &UGridHoverTestListener::HandleEnd);
        Component->OnClicked.AddDynamic(this, &UGridHoverTestListener::HandleClicked);
    }

    UFUNCTION()
    void HandleBegin(UGridHoverComponent* Component) { Begins++; }

    UFUNCTION()
    void HandleEnd(UGridHoverComponent* Component) { Ends++; }

    UFUNCTION()
    void HandleClicked(UGridHoverComponent* Component) { Clicks++; }
};

// A hover component whose CanHover is bAllow
UCLASS(Hidden, NotBlueprintable)
class UGridHoverTestVetoComponent : public UGridHoverComponent
{
    GENERATED_BODY()

public:

    bool bAllow = false;

    virtual bool CanHover_Implementation() const override { return bAllow; }
};
