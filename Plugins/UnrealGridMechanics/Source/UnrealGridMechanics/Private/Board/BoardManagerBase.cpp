// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardManagerBase.h"

#include "Board/Interpreter/BoardStateInterpreterBase.h"
#include "Piece/GridPieceRegistryBase.h"
#include "Piece/GridPieceSpawnInterpreterBase.h"
#include "Tile/GridTileRegistryBase.h"


ABoardManagerBase::ABoardManagerBase()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ABoardManagerBase::PostInitProperties()
{
    Super::PostInitProperties();

    // Instanced property values -- including any Blueprint-child override
    // -- are already applied by the time this runs, so unlike the
    // constructor (the only place CreateDefaultSubobject can run), this is
    // a safe place to wire the assembled pieces together. Deliberately not
    // BeginPlay(): these are plain UObject sub-objects, not components
    // needing RegisterComponent()/world registration, so there's no reason
    // to wait for the full Actor lifecycle.
    if (IsValid(PieceRegistry))
    {
        PieceRegistry->Initialise(TileRegistry, PieceSpawnInterpreter);
    }

    for (const TObjectPtr<UBoardStateInterpreterBase>& Interpreter : Interpreters)
    {
        if (IsValid(Interpreter))
        {
            Interpreter->BindToBoardStateComponent(GetBoardStateComponentBase());
        }
    }
}

void ABoardManagerBase::BeginPlay()
{
    Super::BeginPlay();

    // Tile discovery needs a populated world, so it waits for BeginPlay
    // rather than riding along with the PostInitProperties wiring above.
    if (IsValid(TileRegistry))
    {
        TileRegistry->InitialiseRegistry();
    }
}

void ABoardManagerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(TileRegistry))
    {
        TileRegistry->ShutdownRegistry();
    }

    Super::EndPlay(EndPlayReason);
}
