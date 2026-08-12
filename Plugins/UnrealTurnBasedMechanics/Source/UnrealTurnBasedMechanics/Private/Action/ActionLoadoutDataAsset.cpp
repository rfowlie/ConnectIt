// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "Misc/DataValidation.h"


// --- System Action Vending ---

UTurnBasedAction* UActionLoadoutDataAsset::GetRootAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedAction>(RootActionClass, Outer);
}

UTurnBasedSpectatorAction* UActionLoadoutDataAsset::GetIdleViewerAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedSpectatorAction>(IdleViewerActionClass, Outer);
}

UTurnBasedSpectatorAction* UActionLoadoutDataAsset::GetSpectatorAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedSpectatorAction>(SpectatorViewerActionClass, Outer);
}

UTurnBasedSpectatorAction* UActionLoadoutDataAsset::GetPauseAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedSpectatorAction>(PauseViewerActionClass, Outer);
}

// --- Turn Action Accessors ---

TArray<UTurnBasedAction*> UActionLoadoutDataAsset::GetPermittedActions() const
{
    TArray<UTurnBasedAction*> Permitted;
    Permitted.Reserve(Actions.Num());

    for (UTurnBasedAction* Action : Actions)
    {
        if (!IsValid(Action)) continue;
        if (BannedActionTags.HasTag(Action->ActionTag)) continue;
        Permitted.Add(Action);
    }

    return Permitted;
}

TArray<UTurnBasedAction*> UActionLoadoutDataAsset::GetRequiredActions() const
{
    return FilterPermittedActionsByRequired(true);
}

TArray<UTurnBasedAction*> UActionLoadoutDataAsset::GetOptionalActions() const
{
    return FilterPermittedActionsByRequired(false);
}

TArray<UTurnBasedAction*> UActionLoadoutDataAsset::FilterPermittedActionsByRequired(bool bRequired) const
{
    TArray<UTurnBasedAction*> Out;

    for (UTurnBasedAction* Action : GetPermittedActions())
    {
        if (IsValid(Action) && Action->bIsRequired == bRequired)
        {
            Out.Add(Action);
        }
    }

    return Out;
}

bool UActionLoadoutDataAsset::IsActionPermitted(FGameplayTag ActionTag) const
{
    if (BannedActionTags.HasTag(ActionTag)) return false;

    return Actions.ContainsByPredicate(
        [ActionTag](const UTurnBasedAction* Action)
        {
            return IsValid(Action) && Action->ActionTag == ActionTag;
        });
}

#if WITH_EDITOR
EDataValidationResult UActionLoadoutDataAsset::IsDataValid(
    FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);

    // Warn if no root action set
    if (!RootActionClass)
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': No RootActionClass set. "
                 "Root action is mandatory -- create a do-nothing action "
                 "if your game has no turn idle state."),
            *LoadoutName)));
    }

    // Warn if no required actions
    // if (GetRequiredActions().IsEmpty())
    // {
    //     Context.AddWarning(FText::FromString(FString::Printf(
    //         TEXT("ActionLoadoutDataAsset '%s': No required actions. "
    //              "Turn end will always be available immediately."),
    //         *LoadoutName)));
    // }

    // Warn on duplicate tags
    TSet<FGameplayTag> SeenTags;
    for (const UTurnBasedAction* Action : Actions)
    {
        if (!IsValid(Action)) continue;

        if (!Action->ActionTag.IsValid())
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("ActionLoadoutDataAsset '%s': "
                     "An action has no ActionTag set."),
                *LoadoutName)));
            continue;
        }

        if (SeenTags.Contains(Action->ActionTag))
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("ActionLoadoutDataAsset '%s': "
                     "Duplicate ActionTag '%s' found."),
                *LoadoutName,
                *Action->ActionTag.ToString())));
        }

        SeenTags.Add(Action->ActionTag);
    }

    return Result;
}
#endif