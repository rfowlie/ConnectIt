// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "GameplayTagContainer.h"
#include "DWidget_GameEventTaskSubsystem.generated.h"

class UGameEventTaskSubsystem;

// Tracks UGameEventTaskSubsystem specifically -- the tags currently firing
// in its active container, pushed straight through to BP as they change
// (see UDWidgetBase's own class comment for the push/GetInfo() convention).
// OnActiveManagerTagsChanged is zero-param by design (see
// UGameEventTaskSubsystem's own class comment), so the handler reads
// GetTagsInQueue() after the ping -- single-field source, so no wrapper
// GetInfo() struct, GetTagsInQueue() already serves that role.
// OnActiveEventTagsUpdated fires once per tag completion, not once per
// container -- see this plugin's own docs and the consuming project's
// debug-widget workflow doc for the full rationale.
UCLASS(Abstract)
class UNREALGAMEMECHANICS_API UDWidget_GameEventTaskSubsystem : public UDWidgetBase
{
    GENERATED_BODY()

public:

    // False until UGameEventTaskSubsystem has actually been resolved --
    // the one deliberately pull-only exception to the push model, checked
    // once rather than pushed.
    UFUNCTION(BlueprintPure, Category = "Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Debug")
    void OnActiveEventTagsUpdated(FGameplayTagContainer TagContainer);

private:

    UFUNCTION()
    void HandleActiveManagerTagsChanged(const FGameplayTagContainer& TagContainer);

    UPROPERTY()
    TObjectPtr<UGameEventTaskSubsystem> ResolvedSource = nullptr;

    bool bSourceValid = false;
};
