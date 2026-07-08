// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ActionLoadoutDataAsset.h"
#include "Misc/DataValidation.h"


TArray<UTurnBasedAction*> UActionLoadOutDataAsset::GetPermittedActions() const
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

TArray<UTurnBasedAction*> UActionLoadOutDataAsset::GetRequiredActions() const
{
    TArray<UTurnBasedAction*> Required;

    for (UTurnBasedAction* Action : GetPermittedActions())
    {
        if (Action->bIsRequired)
        {
            Required.Add(Action);
        }
    }

    return Required;
}

TArray<UTurnBasedAction*> UActionLoadOutDataAsset::GetOptionalActions() const
{
    TArray<UTurnBasedAction*> Optional;

    for (UTurnBasedAction* Action : GetPermittedActions())
    {
        if (!Action->bIsRequired)
        {
            Optional.Add(Action);
        }
    }

    return Optional;
}

bool UActionLoadOutDataAsset::IsActionPermitted(FGameplayTag ActionTag) const
{
    if (BannedActionTags.HasTag(ActionTag)) return false;

    return Actions.ContainsByPredicate(
        [ActionTag](const UTurnBasedAction* Action)
        {
            return IsValid(Action) && Action->ActionTag == ActionTag;
        });
}

#if WITH_EDITOR
EDataValidationResult UActionLoadOutDataAsset::IsDataValid(
    FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Warn if no required actions are defined
    if (GetRequiredActions().IsEmpty())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s' has no required actions. "
                 "Turn end will be available immediately."),
            *LoadoutName)));
    }

    // Warn on duplicate action tags
    TSet<FGameplayTag> SeenTags;
    for (const UTurnBasedAction* Action : Actions)
    {
        if (!IsValid(Action)) continue;
        if (!Action->ActionTag.IsValid())
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("ActionLoadoutDataAsset '%s': An action has no ActionTag set."),
                *LoadoutName)));
            continue;
        }

        if (SeenTags.Contains(Action->ActionTag))
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("ActionLoadoutDataAsset '%s': Duplicate ActionTag '%s' detected."),
                *LoadoutName,
                *Action->ActionTag.ToString())));
        }

        SeenTags.Add(Action->ActionTag);
    }

    return Result;
}
#endif