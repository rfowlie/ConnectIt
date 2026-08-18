// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widget/DWidgetBase.h"
#include "GameplayTagContainer.h"
#include "DWidget_GameEventTaskSubsystem.generated.h"

class UGameEventTaskSubsystem;

// Tracks UGameEventTaskSubsystem specifically -- the tags currently firing
// in its active container. GetActiveEventTags() returns the active set, not
// the full not-yet-started backlog, and OnActiveManagerTagsChanged fires
// once per tag completion, not once per container -- see this plugin's own
// docs (GameEventTaskSubsystem's class comment) and the consuming project's
// debug-widget workflow doc for the full rationale.
UCLASS(Abstract)
class UNREALGAMEMECHANICS_API UDWidget_GameEventTaskSubsystem : public UDWidgetBase
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "Debug")
    const TArray<FGameplayTag>& GetActiveEventTags() const { return CachedActiveEventTags; }

    // False until UGameEventTaskSubsystem has actually been resolved
    UFUNCTION(BlueprintPure, Category = "Debug")
    bool IsSourceValid() const { return bSourceValid; }

protected:

    virtual void BindDelegates() override;
    virtual void UnbindDelegates() override;
    virtual void RefreshFields() override;

private:

    UFUNCTION()
    void HandleActiveManagerTagsChanged();

    UPROPERTY()
    TObjectPtr<UGameEventTaskSubsystem> ResolvedSource = nullptr;

    TArray<FGameplayTag> CachedActiveEventTags;
    bool bSourceValid = false;
};
