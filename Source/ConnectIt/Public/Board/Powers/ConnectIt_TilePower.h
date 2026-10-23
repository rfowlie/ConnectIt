// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "StructUtils/InstancedStruct.h"
#include "ConnectIt_TilePower.generated.h"

struct FConnectItBoardState;


// When a tile power fires. The first four react to the board change that
// came before scoring (the settle step), Scoring to the lines a scoring pass
// ran through, Sync to nothing: it checks the board on every settle pass and
// changes it only when the board calls for it. See the vault's
// design/tile-power-triggers.md.
UENUM(BlueprintType)
enum class EConnectItPowerTrigger : uint8
{
    // A piece arrived on the tile (placed, swapped in)
    PieceEnter,
    // A piece left the tile (removed, swapped away) -- not a piece cleared by scoring
    PieceExit,
    // The tile's piece changed owner (a capture)
    PieceChanged,
    // The tile itself changed: multiplier, active, power
    TileChanged,
    // A scoring line ran through the tile
    Scoring,
    // Every settle pass, first: keeps some state true (e.g. Chained). No
    // cooldown, and not limited to once per action.
    Sync,
};

// What an effect is given when its power fires
struct FConnectItTilePowerContext
{
    // The board as it is when the power fires
    const FConnectItBoardState& Board;

    // The powered tile (for a level-wide power, the tile that set it off)
    FGridPosition Position;

    // Whose change set it off -- the faction its operations act for (-1: none)
    int32 Faction = INDEX_NONE;

    // The board's seeded random stream: every copy of the board rolls the same
    FRandomStream& Random;
};

// What a power does: picks its targets and returns the board operations to
// carry out (the same operations player actions send). Subclass it for each
// kind of effect; its UPROPERTYs are the power's tunable values. Plain data,
// no UObject references -- it runs in the AI's search, on a worker thread.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItTilePowerEffect
{
    GENERATED_BODY()

    virtual ~FConnectItTilePowerEffect() = default;

    // Adds the operations this power carries out now (each board operation
    // struct, e.g. FConnectItBoardOperation_RemovePiece). Each is checked
    // before it is applied; none at all is fine (the power still shows and
    // cools down).
    virtual void MakeOperations(const FConnectItTilePowerContext& Context, TArray<FInstancedStruct>& OutOperations) const {}
};

// One tile power: what sets it off, what stops it, and what it does. Defined
// once per tag, in the level's tile catalog (UConnectIt_TileCatalog), and
// copied into the rules when the match starts. A tile names its power by tag
// only (FConnectItTileData::TilePowerTag); level-wide powers are named by tag
// on the Level director. Plain data.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItTilePower
{
    GENERATED_BODY()

    // The power's identity (ConnectIt.TilePower.*; a level is part of the
    // tag, e.g. ConnectIt.TilePower.Poison.2). In a catalog it is the entry's
    // key. Empty = no power.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power", meta = (Categories = "ConnectIt.TilePower"))
    FGameplayTag Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
    EConnectItPowerTrigger Trigger = EConnectItPowerTrigger::PieceEnter;

    // The gate: the tile's multiplier must be at least this. 0 = no gate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power", meta = (ClampMin = 0))
    float MinMultiplier = 0.f;

    // Turns (all turns, both sides') before it can fire again. 0 = every time.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power", meta = (ClampMin = 0))
    int32 CooldownTurns = 0;

    // What it does, with its values
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power", meta = (ExcludeBaseStruct))
    TInstancedStruct<FConnectItTilePowerEffect> Effect;

    bool IsSet() const { return Name.IsValid(); }
};
