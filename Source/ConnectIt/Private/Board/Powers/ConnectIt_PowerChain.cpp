// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Powers/ConnectIt_PowerChain.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_Structs.h"


namespace
{
    // A bug guard, not a rule: the once-per-action guard ends every chain
    constexpr int32 MaxSettleLoops = 64;

    // The guard's key for a tile: its id, or (a board built without ids) its position
    int64 TileKey(const FGridPosition& Position, const FConnectItTileData& Tile)
    {
        return Tile.TileId != INDEX_NONE
            ? static_cast<int64>(Tile.TileId)
            : (int64(1) << 62) | (static_cast<int64>(static_cast<uint32>(Position.X) & 0x3FFFFFFF) << 32)
                | static_cast<uint32>(Position.Y);
    }
}

FConnectItPowerChain::FConnectItPowerChain(
    const FConnectItRuleSet& InRules, FConnectItBoardState& InBoard, FConnectItBoardChangeEvent* InEvents)
    : Rules(InRules)
    , Board(InBoard)
    , Events(InEvents)
{
    bHasPowers = !Rules.TilePowers.IsEmpty();
    bHasSyncPowers = Rules.TilePowers.ContainsByPredicate([](const FConnectItTilePower& Power)
    {
        return Power.Trigger == EConnectItPowerTrigger::Sync;
    });
    for (const FGameplayTag& Name : Rules.LevelWidePowers)
    {
        LevelWide.Add(Rules.FindTilePower(Name));
    }
}

void FConnectItPowerChain::Settle(FConnectItBoardChanges& InOutStep)
{
    if (!bHasPowers) return;

    // The first loop reacts to the whole step; each later one to what the
    // loop before it changed
    FConnectItBoardChanges Triggers = InOutStep;
    for (int32 Loop = 0; Loop < MaxSettleLoops; Loop++)
    {
        // Sync powers first; what they change feeds this loop's triggers
        FConnectItBoardChanges Synced;
        Synced.Faction = InOutStep.Faction;
        const bool bSynced = RunSyncPowers(InOutStep.Faction, Synced);
        Triggers.Append(Synced);

        FConnectItBoardChanges Reacted;
        Reacted.Faction = InOutStep.Faction;
        const bool bFired = FireTriggers(Triggers, Reacted);

        // All of it is scored
        InOutStep.Append(Synced);
        InOutStep.Append(Reacted);

        if (!bFired && !bSynced) return;
        Triggers = MoveTemp(Reacted);
    }

    UE_LOG(LogTemp, Error,
        TEXT("ConnectIt_PowerChain: settle stopped after %d loops -- powers that never settle are a bug"),
        MaxSettleLoops);
}

void FConnectItPowerChain::ReactToScoring(FConnectItBoardChanges& InOutScored)
{
    if (!bHasPowers) return;

    FConnectItBoardChanges Reacted;
    Reacted.Faction = InOutScored.Faction;
    for (const FConnectItScoredTile& Scored : InOutScored.ScoredThrough)
    {
        FireAt(Scored.Position, EConnectItPowerTrigger::Scoring, Scored.Faction, Scored.Cause, Reacted);
    }
    InOutScored.Append(Reacted);
}

bool FConnectItPowerChain::FireTriggers(const FConnectItBoardChanges& Triggers, FConnectItBoardChanges& OutChanges)
{
    bool bFired = false;
    for (const FConnectItPieceArrival& Entered : Triggers.Entered)
    {
        bFired |= FireAt(Entered.Position, EConnectItPowerTrigger::PieceEnter, Entered.Faction, Entered.Cause, OutChanges);
    }
    for (const FConnectItPieceExit& Exited : Triggers.Exited)
    {
        bFired |= FireAt(Exited.Position, EConnectItPowerTrigger::PieceExit, Exited.Faction, Exited.Cause, OutChanges);
    }
    for (const FConnectItPieceChange& Changed : Triggers.PieceChanged)
    {
        bFired |= FireAt(Changed.Position, EConnectItPowerTrigger::PieceChanged, Changed.Faction, Changed.Cause, OutChanges);
    }
    for (const FConnectItTileChange& Changed : Triggers.TileChanged)
    {
        bFired |= FireAt(Changed.Position, EConnectItPowerTrigger::TileChanged, Changed.Faction, Changed.Cause, OutChanges);
    }
    return bFired;
}

bool FConnectItPowerChain::FireAt(const FGridPosition& Position, EConnectItPowerTrigger Trigger, int32 Faction,
    int32 Cause, FConnectItBoardChanges& OutChanges)
{
    const FConnectItTileData* Tile = Board.GetTileData(Position);
    if (!Tile) return false;

    // Looked up before anything fires: a power's operations can move the tile data
    const FConnectItTilePower* Own = Rules.FindTilePower(Tile->TilePowerTag);

    bool bFired = false;
    if (Own && Own->Trigger == Trigger)
    {
        bFired |= TryFire(Position, *Own, INDEX_NONE, Faction, Cause, OutChanges);
    }
    for (int32 Index = 0; Index < LevelWide.Num(); Index++)
    {
        if (LevelWide[Index] && LevelWide[Index]->Trigger == Trigger)
        {
            bFired |= TryFire(Position, *LevelWide[Index], Index, Faction, Cause, OutChanges);
        }
    }
    return bFired;
}

bool FConnectItPowerChain::RunSyncPowers(int32 Faction, FConnectItBoardChanges& OutChanges)
{
    if (!bHasSyncPowers) return false;

    // The positions as they are now (a Sync power may add or remove tiles)
    const TArray<FGridPosition, TInlineAllocator<64>> Positions(Board.TilePositions);

    bool bChanged = false;
    for (const FGridPosition& Position : Positions)
    {
        const FConnectItTileData* Tile = Board.GetTileData(Position);
        if (!Tile) continue;

        const FConnectItTilePower* Own = Rules.FindTilePower(Tile->TilePowerTag);
        if (Own && Own->Trigger == EConnectItPowerTrigger::Sync)
        {
            bChanged |= TryFire(Position, *Own, INDEX_NONE, Faction, INDEX_NONE, OutChanges);
        }
        for (int32 Index = 0; Index < LevelWide.Num(); Index++)
        {
            if (LevelWide[Index] && LevelWide[Index]->Trigger == EConnectItPowerTrigger::Sync)
            {
                bChanged |= TryFire(Position, *LevelWide[Index], Index, Faction, INDEX_NONE, OutChanges);
            }
        }
    }
    return bChanged;
}

bool FConnectItPowerChain::TryFire(const FGridPosition& Position, const FConnectItTilePower& Power,
    int32 LevelWideIndex, int32 Faction, int32 Cause, FConnectItBoardChanges& OutChanges)
{
    FConnectItTileData* Tile = Board.GetTileDataMutable(Position);
    if (!Tile) return false;

    // The gate
    if (Power.MinMultiplier > 0.f && Tile->Multiplier < Power.MinMultiplier) return false;

    const bool bSync = Power.Trigger == EConnectItPowerTrigger::Sync;
    int32 ReadyTurn = Tile->PowerReadyTurn;
    if (LevelWideIndex != INDEX_NONE)
    {
        Board.LevelWideReadyTurns.SetNumZeroed(FMath::Max(Board.LevelWideReadyTurns.Num(), LevelWideIndex + 1));
        ReadyTurn = Board.LevelWideReadyTurns[LevelWideIndex];
    }

    // Sync powers only follow state: no cooldown, not limited to once per action
    if (!bSync)
    {
        if (Board.TurnNumber < ReadyTurn) return false;

        const TPair<int64, int32> Key(TileKey(Position, *Tile), LevelWideIndex + 1);
        if (Fired.Contains(Key)) return false;
        Fired.Add(Key);

        ReadyTurn = Board.TurnNumber + Power.CooldownTurns;
        if (LevelWideIndex == INDEX_NONE)
        {
            Tile->PowerReadyTurn = ReadyTurn;
        }
        else
        {
            Board.LevelWideReadyTurns[LevelWideIndex] = ReadyTurn;
        }
    }

    // What the effect wants to do, from the board as it is now; it rolls from
    // the board's seeded stream, which moves on
    TArray<FInstancedStruct> Operations;
    if (const FConnectItTilePowerEffect* Effect = Power.Effect.GetPtr())
    {
        FRandomStream Random(Board.RandomSeed);
        Effect->MakeOperations(FConnectItTilePowerContext{ Board, Position, Faction, Random }, Operations);
        Board.RandomSeed = Random.GetCurrentSeed();
    }

    int32 TriggeredIndex = INDEX_NONE;
    if (Events)
    {
        FConnectItBoardEvent_PowerTriggered Triggered;
        Triggered.CauseIndex     = Cause;
        Triggered.Position       = Position;
        Triggered.PowerName      = Power.Name;
        Triggered.Faction        = Faction;
        Triggered.LevelWideIndex = LevelWideIndex;
        Triggered.ReadyTurn      = ReadyTurn;
        TriggeredIndex = Events->Events.Num();
        Events->Add(Triggered);
    }

    int32 Applied = 0;
    for (FInstancedStruct& Entry : Operations)
    {
        FConnectItBoardOperation* Operation = Entry.GetMutablePtr<FConnectItBoardOperation>();
        if (!Operation)
        {
            UE_LOG(LogTemp, Warning, TEXT("ConnectIt_PowerChain: power %s gave something that isn't a board operation (%s)"),
                *Power.Name.ToString(), *GetNameSafe(Entry.GetScriptStruct()));
            continue;
        }

        // It acts for the change that set it off, and only if it can (locked
        // tiles included)
        Operation->Faction = Faction;
        if (!Rules.CanCarryOut(*Operation, Board)) continue;

        const int32 FirstEvent = Events ? Events->Events.Num() : INDEX_NONE;
        FConnectItBoardChanges Changed;
        Changed.Faction = Faction;
        Operation->Apply(Board, Changed, Events);
        Applied++;

        // Its events were caused by this power; what it changed by its own event
        int32 ChangeCause = TriggeredIndex;
        if (Events)
        {
            for (int32 Index = FirstEvent; Index < Events->Events.Num(); Index++)
            {
                if (FConnectItBoardEvent* Event = Events->Events[Index].GetMutablePtr<FConnectItBoardEvent>())
                {
                    Event->CauseIndex = TriggeredIndex;
                }
            }
            if (Events->Events.Num() > FirstEvent) ChangeCause = FirstEvent;
        }
        Changed.SetCause(ChangeCause);
        OutChanges.Append(Changed);
    }

    if (Applied == 0)
    {
        // A Sync power that changed nothing didn't fire at all
        if (bSync)
        {
            if (Events && TriggeredIndex != INDEX_NONE) Events->Events.RemoveAt(TriggeredIndex);
            return false;
        }
        if (Events && TriggeredIndex != INDEX_NONE)
        {
            Events->Events[TriggeredIndex].GetMutable<FConnectItBoardEvent_PowerTriggered>().bNoEffect = true;
        }
    }
    return true;
}
