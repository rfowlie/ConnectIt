// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/Powers/ConnectIt_TilePowerChain.h"
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

FConnectItTilePowerChain::FConnectItTilePowerChain(
    const FConnectItRuleSet& InRules, FConnectItBoardState& InBoard, FConnectItBoardChangeEvent* InEvents)
    : Rules(InRules)
    , Board(InBoard)
    , Events(InEvents)
{
    bHasTilePowers = !Rules.TilePowers.IsEmpty();
    bHasSyncTilePowers = Rules.TilePowers.ContainsByPredicate([](const FConnectItTilePower& Power)
    {
        return Power.HasTrigger(EConnectItTilePowerTrigger::Sync);
    });
    for (const FGameplayTag& Name : Rules.LevelWideTilePowers)
    {
        LevelWide.Add(Rules.FindTilePower(Name));
    }
}

void FConnectItTilePowerChain::Settle(FConnectItBoardChanges& InOutStep)
{
    if (!bHasTilePowers) return;

    // The first loop reacts to the whole step; each later one to what the
    // loop before it changed
    FConnectItBoardChanges Triggers = InOutStep;

    // A turn's start: its Turn Start powers fire first, once; what they change
    // is reacted to like the rest of the step
    if (InOutStep.bTurnStarted)
    {
        FConnectItBoardChanges Ticked;
        Ticked.Faction = InOutStep.Faction;
        RunTurnStartTilePowers(InOutStep.Faction, Ticked);
        Triggers.Append(Ticked);
        InOutStep.Append(Ticked);
    }
    for (int32 Loop = 0; Loop < MaxSettleLoops; Loop++)
    {
        // Sync powers first; what they change feeds this loop's triggers
        FConnectItBoardChanges Synced;
        Synced.Faction = InOutStep.Faction;
        const bool bSynced = RunSyncTilePowers(InOutStep.Faction, Synced);
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
        TEXT("ConnectIt_TilePowerChain: settle stopped after %d loops -- powers that never settle are a bug"),
        MaxSettleLoops);
}

void FConnectItTilePowerChain::ReactToScoring(FConnectItBoardChanges& InOutScored)
{
    if (!bHasTilePowers) return;

    FConnectItBoardChanges Reacted;
    Reacted.Faction = InOutScored.Faction;
    for (const FConnectItScoredTile& Scored : InOutScored.ScoredThrough)
    {
        FireAt(Scored.Position, EConnectItTilePowerTrigger::Scoring, Scored.Faction, Scored.Cause, Reacted);
    }
    InOutScored.Append(Reacted);
}

bool FConnectItTilePowerChain::FireTriggers(const FConnectItBoardChanges& Triggers, FConnectItBoardChanges& OutChanges)
{
    bool bFired = false;
    for (const FConnectItPieceArrival& Entered : Triggers.Entered)
    {
        bFired |= FireAt(Entered.Position, EConnectItTilePowerTrigger::PieceEnter, Entered.Faction, Entered.Cause, OutChanges);
    }
    for (const FConnectItPieceExit& Exited : Triggers.Exited)
    {
        bFired |= FireAt(Exited.Position, EConnectItTilePowerTrigger::PieceExit, Exited.Faction, Exited.Cause, OutChanges);
    }
    for (const FConnectItPieceChange& Changed : Triggers.PieceChanged)
    {
        bFired |= FireAt(Changed.Position, EConnectItTilePowerTrigger::PieceChanged, Changed.Faction, Changed.Cause, OutChanges);
    }
    for (const FConnectItTileChange& Changed : Triggers.TileChanged)
    {
        bFired |= FireAt(Changed.Position, EConnectItTilePowerTrigger::TileChanged, Changed.Faction, Changed.Cause, OutChanges);
    }
    return bFired;
}

bool FConnectItTilePowerChain::FireAt(const FGridPosition& Position, EConnectItTilePowerTrigger Trigger, int32 Faction,
    int32 Cause, FConnectItBoardChanges& OutChanges)
{
    const FConnectItTileData* Tile = Board.GetTileData(Position);
    if (!Tile) return false;

    // Looked up before anything fires: a power's operations can move the tile data
    const FConnectItTilePower* Own = Rules.FindTilePower(Tile->TilePowerTag);

    bool bFired = false;
    if (Own && Own->HasTrigger(Trigger))
    {
        bFired |= TryFire(Position, *Own, INDEX_NONE, Faction, Cause, OutChanges);
    }
    for (int32 Index = 0; Index < LevelWide.Num(); Index++)
    {
        if (LevelWide[Index] && LevelWide[Index]->HasTrigger(Trigger))
        {
            bFired |= TryFire(Position, *LevelWide[Index], Index, Faction, Cause, OutChanges);
        }
    }
    return bFired;
}

bool FConnectItTilePowerChain::RunSyncTilePowers(int32 Faction, FConnectItBoardChanges& OutChanges)
{
    if (!bHasSyncTilePowers) return false;

    // The positions as they are now (a Sync power may add or remove tiles)
    const TArray<FGridPosition, TInlineAllocator<64>> Positions(Board.TilePositions);

    bool bChanged = false;
    for (const FGridPosition& Position : Positions)
    {
        const FConnectItTileData* Tile = Board.GetTileData(Position);
        if (!Tile) continue;

        const FConnectItTilePower* Own = Rules.FindTilePower(Tile->TilePowerTag);
        if (Own && Own->HasTrigger(EConnectItTilePowerTrigger::Sync))
        {
            bChanged |= TryFire(Position, *Own, INDEX_NONE, Faction, INDEX_NONE, OutChanges);
        }
        for (int32 Index = 0; Index < LevelWide.Num(); Index++)
        {
            if (LevelWide[Index] && LevelWide[Index]->HasTrigger(EConnectItTilePowerTrigger::Sync))
            {
                bChanged |= TryFire(Position, *LevelWide[Index], Index, Faction, INDEX_NONE, OutChanges);
            }
        }
    }
    return bChanged;
}

bool FConnectItTilePowerChain::RunTurnStartTilePowers(int32 Faction, FConnectItBoardChanges& OutChanges)
{
    // The positions as they are now (a power may add or remove tiles)
    const TArray<FGridPosition, TInlineAllocator<64>> Positions(Board.TilePositions);

    bool bFired = false;
    for (const FGridPosition& Position : Positions)
    {
        const FConnectItTileData* Tile = Board.GetTileData(Position);
        if (!Tile) continue;

        const FConnectItTilePower* Own = Rules.FindTilePower(Tile->TilePowerTag);
        if (Own && Own->HasTrigger(EConnectItTilePowerTrigger::TurnStart))
        {
            bFired |= TryFire(Position, *Own, INDEX_NONE, Faction, INDEX_NONE, OutChanges);
        }
        for (int32 Index = 0; Index < LevelWide.Num(); Index++)
        {
            if (LevelWide[Index] && LevelWide[Index]->HasTrigger(EConnectItTilePowerTrigger::TurnStart))
            {
                bFired |= TryFire(Position, *LevelWide[Index], Index, Faction, INDEX_NONE, OutChanges);
            }
        }
    }
    return bFired;
}

bool FConnectItTilePowerChain::TryFire(const FGridPosition& Position, const FConnectItTilePower& Power,
    int32 LevelWideIndex, int32 Faction, int32 Cause, FConnectItBoardChanges& OutChanges)
{
    FConnectItTileData* Tile = Board.GetTileDataMutable(Position);
    if (!Tile) return false;

    // The gate (the effect's own, if it has one). The stream is only rolled
    // by the effect, below.
    FRandomStream Random(Board.RandomSeed);
    const FConnectItTilePowerContext Context{ Board, Position, Faction, Random, &Rules };
    if (!Power.PassesGate(Context)) return false;

    const bool bSync = Power.HasTrigger(EConnectItTilePowerTrigger::Sync);
    int32 ReadyTurn = Tile->TilePowerReadyTurn;
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
            Tile->TilePowerReadyTurn = ReadyTurn;
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
        Effect->MakeOperations(Context, Operations);
        Board.RandomSeed = Random.GetCurrentSeed();
    }

    int32 TriggeredIndex = INDEX_NONE;
    if (Events)
    {
        FConnectItBoardEvent_TilePowerTriggered Triggered;
        Triggered.CauseIndex     = Cause;
        Triggered.Position       = Position;
        Triggered.TilePowerName  = Power.GetTag();
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
            UE_LOG(LogTemp, Warning, TEXT("ConnectIt_TilePowerChain: power %s gave something that isn't a board operation (%s)"),
                *Power.GetTag().ToString(), *GetNameSafe(Entry.GetScriptStruct()));
            continue;
        }

        // It acts for the change that set it off, and only if it can (locked
        // tiles included)
        Operation->Faction = Faction;
        if (!Rules.CanCarryOut(*Operation, Board)) continue;

        const int32 FirstEvent = Events ? Events->Events.Num() : INDEX_NONE;
        FConnectItBoardChanges Changed;
        Changed.Faction = Faction;
        Changed.Rules = &Rules;
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
            Events->Events[TriggeredIndex].GetMutable<FConnectItBoardEvent_TilePowerTriggered>().bNoEffect = true;
        }
    }
    return true;
}
