// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Input/InputTagBinding.h"
#include "InputTagBinder.generated.h"

class UInputMappingContext;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;

// Builds and owns a single UInputMappingContext from a designer-authored
// list of FInputTagBinding entries, and scopes it to an explicit
// Bind/Unbind lifetime instead of EnhancedInput's usual "add once at
// BeginPlay, leave it running" assumption -- built for consumers whose own
// lifetime is itself dynamic (e.g. an entry on an action stack that
// activates/deactivates repeatedly), where a single always-on mapping
// context per input isn't the right model.
//
// Every InputAction fires through the same OnInputTagTriggered delegate,
// carrying whichever entry's BindingTag matched -- the consumer switches
// on BindingTag rather than needing one bound function per input.
//
// Plain UObject, not an ActorComponent or subsystem: both EnhancedInput
// dependencies are injected via Initialise rather than resolved internally
// via GetWorld(), and lifetime is scoped per-owner (NewObject on whatever
// owns one of these), not global/singleton.
UCLASS(BlueprintType)
class UNREALGAMEMECHANICS_API UInputTagBinder : public UObject
{
    GENERATED_BODY()

public:

    // Call once after construction. Builds the mapping context from
    // InBindings immediately -- GetMappingContext() is valid to call right
    // after this returns, before the first BindAll().
    void Initialise(
        UEnhancedInputComponent* InInputComponent,
        UEnhancedInputLocalPlayerSubsystem* InLocalPlayerSubsystem,
        const TArray<FInputTagBinding>& InBindings,
        int32 InMappingContextPriority = 1);

    // Adds the mapping context (if not already added) and binds every
    // configured InputAction to the shared dispatch handler. Idempotent.
    void BindAll();

    // Removes the mapping context and clears every binding this object
    // registered on the injected input component.
    void UnbindAll();

    // Exposed so a consumer with its own bespoke, non-tag-dispatched input
    // (e.g. UTurnBasedAction's spatial hover+confirm selection pipeline)
    // can add one more key mapping onto the same context instead of
    // standing up a second mapping context of its own.
    UFUNCTION(BlueprintPure, Category = "Input")
    UInputMappingContext* GetMappingContext() const { return InputMappingContext; }

    UPROPERTY(BlueprintAssignable, Category = "Input")
    FOnInputTagTriggered OnInputTagTriggered;

private:

    UPROPERTY()
    TObjectPtr<UInputMappingContext> InputMappingContext = nullptr;

    UPROPERTY()
    TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

    UPROPERTY()
    TObjectPtr<UEnhancedInputLocalPlayerSubsystem> LocalPlayerSubsystem = nullptr;

    TArray<FInputTagBinding> Bindings;
    int32 MappingContextPriority = 1;

    void HandleInputTriggered(const FInputActionInstance& Instance, FGameplayTag BindingTag);
};
