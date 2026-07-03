// Fill out your copyright notice in the Description page of Project Settings.


#include "Validator/GridValidatorBase.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Board/BoardActor.h"
#include "Tile/GridTileRegistryComponent.h"
#include "Tile/GridTileBase.h"


bool UGridValidatorBase::CanValidateAsset_Implementation(UObject* InAsset) const
{
    // Only validate world assets
    return InAsset->IsA<UWorld>();
}

EDataValidationResult UGridValidatorBase::ValidateLoadedAsset_Implementation(
    UObject* InAsset,
    TArray<FText>& ValidationErrors)
{
    UWorld* World = Cast<UWorld>(InAsset);
    if (!IsValid(World)) return EDataValidationResult::NotValidated;

    // Find the board actor in the level
    ABoardActor* BoardActor = nullptr;
    for (TActorIterator<ABoardActor> It(World); It; ++It)
    {
        BoardActor = *It;
        break;
    }

    if (!IsValid(BoardActor))
    {
        // No board actor in this level — not an error
        return EDataValidationResult::NotValidated;
    }

    UGridTileRegistryComponent* Registry =
        BoardActor->FindComponentByClass<UGridTileRegistryComponent>();

    if (!IsValid(Registry))
    {
        ValidationErrors.Add(FText::FromString(
            TEXT("BoardActor is missing a UGridRegistryComponent")));
        return EDataValidationResult::Invalid;
    }

    const int32 GridSize = Registry->GridSize;
    int32 ErrorCount     = 0;

    // Validate all tile actors in the level
    for (TActorIterator<AGridTileBase> It(World); It; ++It)
    {
        const AGridTileBase* Tile = *It;
        if (!IsValid(Tile)) continue;

        const FVector  WorldLocation  = Tile->GetActorLocation();
        const FGridPosition Expected  = Registry->WorldToGridPosition(WorldLocation);
        // const FGridPosition Actual    = Tile->GetGridPosition();
        const FGridPosition Actual    = FGridPosition();

        if (!(Expected == Actual))
        {
            ValidationErrors.Add(FText::FromString(FString::Printf(
                TEXT("Tile '%s' GridPosition (%d,%d) does not match world position (%d,%d). "
                     "GridSize is %d — check tile placement or GridPosition property."),
                *Tile->GetName(),
                Actual.X, Actual.Y,
                Expected.X, Expected.Y,
                GridSize)));

            ErrorCount++;
        }
    }

    if (ErrorCount > 0)
    {
        FText Text = FText::FromString(
            FString::Printf(
                TEXT("Grid validation failed — %d misaligned tile(s) found"),
                ErrorCount)),
        Errors;
        
        AssetFails(InAsset, Text);
        return EDataValidationResult::Invalid;
    }

    AssetPasses(InAsset);
    return EDataValidationResult::Valid;
}