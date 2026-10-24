// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/ItemActionInventoryDataAsset.h"
#include "Action/TurnBasedPlayerAction.h"
#include "TurnBasedMechanicsSettings.h"
#include "Misc/DataValidation.h"

int32 UItemActionInventoryDataAsset::GetMaxHeld(TSubclassOf<UTurnBasedPlayerAction> ActionClass) const
{
    const int32* Found = MaxHeld.Find(ActionClass);
    return Found ? FMath::Max(*Found, 0) : 0;
}

const UItemActionInventoryDataAsset* UItemActionInventoryDataAsset::Get()
{
    const UTurnBasedMechanicsSettings* Settings = GetDefault<UTurnBasedMechanicsSettings>();
    return Settings ? Settings->ItemInventory.LoadSynchronous() : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UItemActionInventoryDataAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    for (const TPair<TSubclassOf<UTurnBasedPlayerAction>, int32>& Pair : MaxHeld)
    {
        if (!Pair.Key)
        {
            Context.AddError(FText::FromString(TEXT("ItemActionInventoryDataAsset: an entry has no action.")));
            Result = EDataValidationResult::Invalid;
        }
        else if (Pair.Value < 1)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ItemActionInventoryDataAsset: '%s' has a cap of %d -- every item action can hold at least 1."),
                *Pair.Key->GetName(), Pair.Value)));
            Result = EDataValidationResult::Invalid;
        }
    }
    return Result;
}
#endif
