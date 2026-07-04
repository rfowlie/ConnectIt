// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "UObject/Interface.h"
#include "GridTileSelector.generated.h"

class UGridTileSelectorComponent;
class AGridTileBase;

UINTERFACE(Blueprintable, BlueprintType)
class UNREALGRIDMECHANICS_API UGridTileSelector : public UInterface
{
    GENERATED_BODY()
};

// Blueprint friendly — dynamic delegate for graph binding
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FGridTileSelectorDelegate,
    AGridTileBase*, GridTile,
    FGridPosition,  GridPosition);

// C++ friendly — non-dynamic multicast for direct binding
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FGridTileSelectorMulticastDelegate,
    AGridTileBase*, 
    FGridPosition);

/**
 * Pattern: there is a task that will potentially take time
 * Usage: bind to delegate then call function to begin calculations, unbind afterward if needed
 *
 * Blueprint callers — use SelectGridTileRegister / SelectGridTileUnregister
 * C++ callers       — use BindOnGridTileSelected / UnbindOnGridTileSelected
 *                     or GetOnGridTileSelectedDelegate() for full multicast access
 */
class UNREALGRIDMECHANICS_API IGridTileSelector
{
    GENERATED_BODY()

public:

    // --- Lifecycle ---
    //
    // UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Tile Selector")
    // void SelectGridTileEnter();
    //
    // UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Tile Selector")
    // void SelectGridTileExit();

    // --- Blueprint Friendly ---
    // Dynamic delegate passing pattern — ideal for Blueprint graphs

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Tile Selector|Register")
    void SelectGridTileRegister(const FGridTileSelectorDelegate& Callback);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Tile Selector|Register")
    void SelectGridTileUnregister(const FGridTileSelectorDelegate& Callback);

    // --- C++ Friendly ---
    // Direct binding without constructing a dynamic delegate manually

    // Bind a lambda or free function — returns a handle for later removal
    // Usage:
    //   FDelegateHandle Handle = Tile->BindOnGridTileSelected(
    //       [this](AGridTileBase* Tile, FGridPosition Pos){ ... });
    virtual FDelegateHandle BindOnGridTileSelected(TFunction<void(AGridTileBase*, FGridPosition)> Callback)
    {
        return FDelegateHandle();
    }

    // Remove a specific binding by handle
    // Usage:
    //   Tile->UnbindOnGridTileSelected(Handle);
    virtual void UnbindOnGridTileSelected(FDelegateHandle Handle) {}

    // Remove all bindings from a specific object — useful in destructors
    // Usage:
    //   Tile->UnbindAllOnGridTileSelected(this);
    virtual void UnbindAllOnGridTileSelected(const void* Object) {}

    // Direct access to the underlying multicast delegate
    // Allows AddUObject, AddWeakLambda, RemoveAll and other TMulticastDelegate methods
    // Usage:
    //   Tile->GetOnGridTileSelectedDelegate()->AddWeakLambda(this,
    //       [this](AGridTileBase* Tile, FGridPosition Pos){ ... });
    virtual FGridTileSelectorMulticastDelegate* GetOnGridTileSelectedDelegate()
    {
        return nullptr;
    }

    // --- Deprecated ---

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Tile Selector",
        meta = (DeprecatedFunction, DeprecationMessage = "Use GetOnGridTileSelectedDelegate instead."))
    UGridTileSelectorComponent* GetGridTileSelectorComponent();
};