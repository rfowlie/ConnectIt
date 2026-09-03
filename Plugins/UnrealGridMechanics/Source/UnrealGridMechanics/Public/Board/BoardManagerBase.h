// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoardManagerBase.generated.h"

class UGridTileRegistryBase;
class UGridPieceRegistryBase;
class UBoardStateComponentBase;

// Abstract, project-agnostic board manager base -- assembles the pieces
// that are genuinely generic (tile registry, piece registry) behind
// UObject-based, Instanced properties instead of hardcoded
// CreateDefaultSubobject component slots. That's a deliberate pivot, not
// the obvious choice -- see "Prototype ABoardManagerBase..." in the plan
// history for the full reasoning: neither assembled class ever ticked, so
// nothing is lost by them not being ActorComponents, and an Instanced
// UObject property gives genuine BP-only runtime polymorphism (pick a
// different subclass in the Details panel, its own sub-properties expand
// inline, zero C++ required) that a named component slot structurally
// cannot.
//
// Deliberately does NOT try to find a generic shape for anything
// ConnectIt-specific (config/rules/shift/board-state itself) -- no second
// implementer exists anywhere in this codebase to justify it, matching the
// documented lesson from IGridBoardManagerInterface's removal (exactly one
// implementer ever, forced downcasts at every real call site).
//
// Pure, additive prototype -- nothing existing changes as a result of this
// class existing. AConnectIt_BoardManager is not migrated to derive from
// this, and no other code references it yet.
UCLASS(Abstract, Blueprintable)
class UNREALGRIDMECHANICS_API ABoardManagerBase : public AActor
{
    GENERATED_BODY()

public:

    ABoardManagerBase();

    UFUNCTION(BlueprintPure, Category = "Board")
    UGridTileRegistryBase* GetTileRegistry() const { return TileRegistry; }

    UFUNCTION(BlueprintPure, Category = "Board")
    UGridPieceRegistryBase* GetPieceRegistry() const { return PieceRegistry; }


protected:

    // True BP-only polymorphism -- a designer picks a different subclass
    // directly in the Details panel on a concrete subclass's (or its BP
    // child's) instance, no C++ required. Left unset by default here
    // (this base is Abstract and never placed); a concrete subclass or its
    // BP child is expected to assign these -- either via the Details panel
    // directly, or via NewObject in its own constructor for a baked-in
    // default. Exactly how a real project's board manager would default
    // this is left to that future migration, not this prototype.
    UPROPERTY(EditAnywhere, Instanced, Category = "Board")
    TObjectPtr<UGridTileRegistryBase> TileRegistry;

    UPROPERTY(EditAnywhere, Instanced, Category = "Board")
    TObjectPtr<UGridPieceRegistryBase> PieceRegistry;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
