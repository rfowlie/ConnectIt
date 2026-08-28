// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DWidgetBase.generated.h"

UNREALGAMEMECHANICS_API DECLARE_LOG_CATEGORY_EXTERN(LogDWidget, Log, All);


// Shared scaffolding for the "one widget per tracked class" debug-widget
// family (see each consuming project's own workflow doc for the concrete
// DWidget_* subclasses built on top of this). A concrete subclass tracks
// exactly one class, binds only the delegates that class itself exposes,
// and pushes each field straight through to its own dedicated
// BlueprintImplementableEvent as it changes -- no cached fields, no
// coarse "something changed, redraw everything" signal. Initial state
// comes from the tracked class's own GetInfo() (a per-class,
// BlueprintPure, struct-returning convention -- not something this base
// class can generalize further, since each source's info shape differs),
// called once from BindDelegates() to seed the same per-field events used
// for later reactive updates. Kept generic and project-agnostic here;
// naming/scope conventions for concrete subclasses are documented
// per-consumer.
UCLASS(Abstract)
class UNREALGAMEMECHANICS_API UDWidgetBase : public UUserWidget
{
    GENERATED_BODY()

protected:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Resolve the tracked source object, bind whatever delegates this
    // widget cares about, and seed initial state by calling the source's
    // own GetInfo() and pushing it through the same per-field events used
    // for later updates. Called once from NativeConstruct.
    virtual void BindDelegates() {}

    // Mirror of BindDelegates -- called once from NativeDestruct.
    virtual void UnbindDelegates() {}

private:

    bool bBound = false;
};
