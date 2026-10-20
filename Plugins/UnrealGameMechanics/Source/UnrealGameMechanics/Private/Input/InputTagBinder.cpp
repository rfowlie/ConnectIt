// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/InputTagBinder.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"


void UInputTagBinder::CreateInputMappingContext()
{
    InputMappingContext = NewObject<UInputMappingContext>(this);

    for (const FInputTagBinding& Binding : Bindings)
    {
        if (!IsValid(Binding.InputAction)) continue;
        InputMappingContext->MapKey(Binding.InputAction, Binding.Key);
    }
}

void UInputTagBinder::Initialise(
    UEnhancedInputComponent* InInputComponent,
    UEnhancedInputLocalPlayerSubsystem* InLocalPlayerSubsystem,
    const TArray<FInputTagBinding>& InBindings,
    int32 InMappingContextPriority)
{
    EnhancedInputComponent = InInputComponent;
    LocalPlayerSubsystem   = InLocalPlayerSubsystem;
    Bindings               = InBindings;
    MappingContextPriority = InMappingContextPriority;

    if (IsValid(InputMappingContext)) return;

    CreateInputMappingContext();
}

void UInputTagBinder::BindAll()
{
    if (!IsValid(InputMappingContext))
    {
        UE_LOG(LogTemp, Error, TEXT("InputTagBinder - BindAll : IMC is null"));
        return;
    }

    if (IsValid(EnhancedInputComponent))
    {
        for (const FInputTagBinding& Binding : Bindings)
        {
            if (!IsValid(Binding.InputAction)) continue;

            EnhancedInputComponent->BindAction(
                Binding.InputAction,
                Binding.TriggerEvent,
                this,
                &UInputTagBinder::HandleInputTriggeredExecuteDelegate);
        }
    }

    if (IsValid(LocalPlayerSubsystem) && !LocalPlayerSubsystem->HasMappingContext(InputMappingContext))
    {
        LocalPlayerSubsystem->AddMappingContext(InputMappingContext, MappingContextPriority);
    }
}

void UInputTagBinder::UnbindAll()
{
    if (IsValid(LocalPlayerSubsystem) && LocalPlayerSubsystem->HasMappingContext(InputMappingContext))
    {
        LocalPlayerSubsystem->RemoveMappingContext(InputMappingContext);
    }

    if (IsValid(EnhancedInputComponent))
    {
        EnhancedInputComponent->ClearBindingsForObject(this);
    }
}

UInputMappingContext* UInputTagBinder::GetMappingContext()
{
    if (!IsValid(InputMappingContext))
    {
        CreateInputMappingContext();
    }

    return InputMappingContext;
}


void UInputTagBinder::HandleInputTriggeredExecuteDelegate(const FInputActionInstance& Instance)
{
    // find the action in the list
    for (const auto Binding : Bindings)
    {
        if (Binding.InputAction == Instance.GetSourceAction())
        {
            const bool bSuccess = Binding.InputActionDelegate.ExecuteIfBound();
            if (!bSuccess)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("InputTagBinder - HandleInputTriggeredExecuteDelegate : "
                         "InputActionDelegate not bound for '%s'"),
                    *Binding.InputAction->GetName());
            }
        }
    }
}
