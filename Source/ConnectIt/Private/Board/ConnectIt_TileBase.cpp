// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_TileBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/UnrealType.h"
#endif


FString AConnectIt_TileBase::CheckTilePower(const FGameplayTag& ClassTag, const FConnectItTilePower& Power)
{
    if (!ClassTag.IsValid())
    {
        return Power.IsSet()
            ? FString::Printf(TEXT("its power is named %s but the class has no Tile Power Tag -- the tile has no power"),
                *Power.Name.ToString())
            : FString();
    }
    if (!Power.IsSet())
    {
        return FString::Printf(TEXT("the class's Tile Power Tag is %s but Get Tile Power gives no power"),
            *ClassTag.ToString());
    }
    if (Power.Name != ClassTag)
    {
        return FString::Printf(TEXT("its power is named %s but the class's Tile Power Tag is %s"),
            *Power.Name.ToString(), *ClassTag.ToString());
    }
    if (!Power.Effect.IsValid())
    {
        return FString::Printf(TEXT("power %s has no Effect"), *ClassTag.ToString());
    }
    return FString();
}

FConnectItTilePower AConnectIt_TileBase::MakeBoardPower(FString* OutProblem) const
{
    if (!TilePowerTag.IsValid())
    {
        if (OutProblem) *OutProblem = CheckTilePower(TilePowerTag, GetTilePower());
        return FConnectItTilePower();
    }

    // The class's tag is the identity, whatever the power calls itself
    FConnectItTilePower Power = GetTilePower();
    if (OutProblem) *OutProblem = CheckTilePower(TilePowerTag, Power);
    Power.Name = TilePowerTag;
    return Power;
}

#if WITH_EDITOR
EDataValidationResult AConnectIt_TileBase::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    const FString Problem = CheckTilePower(TilePowerTag, GetTilePower());
    if (!Problem.IsEmpty())
    {
        Context.AddError(FText::FromString(FString::Printf(TEXT("ConnectIt_TileBase '%s': %s."),
            *GetName(), *Problem)));
        Result = EDataValidationResult::Invalid;
    }

    // Only the C++ Tile Power Tag names the power: a Blueprint variable that
    // looks like one, editable on placed tiles, would let a placed tile seem
    // to change its kind
    for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
    {
        const FProperty* Property = *It;
        const UClass* DeclaredIn = Property->GetOwnerClass();
        if (!DeclaredIn || DeclaredIn->HasAnyClassFlags(CLASS_Native)) continue;
        if (!Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance)) continue;

        const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
        const bool bPowerStruct = StructProperty && StructProperty->Struct == FConnectItTilePower::StaticStruct();
        const bool bPowerTag = StructProperty && StructProperty->Struct == FGameplayTag::StaticStruct()
            && Property->GetName().Contains(TEXT("Power"));
        if (bPowerStruct || bPowerTag)
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_TileBase '%s': Blueprint variable '%s' is editable on placed tiles and looks like a power "
                "name -- only Tile Power Tag (Class Defaults) decides a tile's power. Expose the power's values "
                "instead."), *GetClass()->GetName(), *Property->GetName())));
        }
    }
    return Result;
}
#endif
