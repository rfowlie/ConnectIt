// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DWidgetBase.generated.h"

// Shared scaffolding for the "one widget per tracked class" debug-widget
// family (see each consuming project's own workflow doc for the concrete
// DWidget_* subclasses built on top of this). A concrete subclass tracks
// exactly one class -- one cached pointer to that class's instance, one
// BlueprintPure getter per field it cares about (no wrapper structs), and
// binds only the delegates that tracked class itself exposes. Kept generic
// and project-agnostic here; naming/scope conventions for concrete
// subclasses are documented per-consumer.
UCLASS(Abstract)
class UNREALGAMEMECHANICS_API UDWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:

    // Re-resolves the tracked source (if not already valid) and rebuilds
    // every cached field from scratch, then fires OnDebugStateUpdated once.
    // Called automatically from NativeConstruct and by every bound delegate
    // handler in each concrete subclass -- also safe to call manually from
    // BP as a "something looks stale" escape hatch.
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void RefreshAll();

    // Fired once at the end of every RefreshAll() call. BP redraws from the
    // subclass's own getters -- one coarse event per widget, not split
    // further, since each widget's own field set is already small.
    UFUNCTION(BlueprintImplementableEvent, Category = "Debug")
    void OnDebugStateUpdated();

protected:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Resolve the tracked source object and bind whatever delegates this
    // widget cares about. Called once from NativeConstruct.
    virtual void BindDelegates() {}

    // Mirror of BindDelegates -- called once from NativeDestruct.
    virtual void UnbindDelegates() {}

    // Re-read every cached field from the tracked source. Must not assume
    // the source is valid -- called before the source may have resolved
    // (e.g. an owning manager/subsystem not registered yet) as well as after.
    virtual void RefreshFields() {}

private:

    bool bBound = false;
};
