// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BoardStateInterpreterBase.generated.h"

class UBoardStateComponentBase;

// UObject-based prototype counterpart to UBoardStateInterpreter -- same
// public surface (BindToBoardStateComponent/IsBound/GetBoundComponent),
// but not an ActorComponent, since it was already purely delegate-driven
// (no Tick usage at all). Meant to be held in a designer-editable,
// Instanced array (e.g. ABoardManagerBase::Interpreters) for true BP-only
// runtime polymorphism -- see UGridTileRegistryBase's class comment for
// the full reasoning, identical here.
//
// AddDynamic (used internally to bind to UBoardStateComponentBase::
// OnBoardStateChanged) works for any UObject with a matching UFUNCTION(),
// not just Actors/Components, so this needs no special handling versus
// the live ActorComponent version.
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNREALGRIDMECHANICS_API UBoardStateInterpreterBase : public UObject
{
    GENERATED_BODY()

public:

    // Called by project-specific wiring (e.g. ABoardManagerBase::
    // PostInitProperties) -- this interpreter does not find its own board
    // state component, this is the explicit injection point.
    UFUNCTION(BlueprintCallable, Category = "Interpreter")
    void BindToBoardStateComponent(UBoardStateComponentBase* InComponent);

    UFUNCTION(BlueprintPure, Category = "Interpreter")
    bool IsBound() const;

    UFUNCTION(BlueprintPure, Category = "Interpreter")
    UBoardStateComponentBase* GetBoundComponent() const { return BoundComponent; }

protected:

    // Subclasses override this to respond to board state changes. Cast
    // Component to project-specific type to access typed snapshot.
    UFUNCTION(BlueprintNativeEvent, Category = "Interpreter")
    void OnBoardStateChanged(const UBoardStateComponentBase* Component);
    virtual void OnBoardStateChanged_Implementation(const UBoardStateComponentBase* Component);

    UPROPERTY(BlueprintReadOnly, Category = "Interpreter")
    TObjectPtr<UBoardStateComponentBase> BoundComponent = nullptr;

private:

    UFUNCTION()
    void HandleBoardStateChanged();
};
