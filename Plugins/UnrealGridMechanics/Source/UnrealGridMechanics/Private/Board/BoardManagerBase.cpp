// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardManagerBase.h"
#include "Piece/GridPieceRegistryBase.h"
#include "Tile/GridTileRegistryBase.h"


ABoardManagerBase::ABoardManagerBase()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ABoardManagerBase::BeginPlay()
{
    Super::BeginPlay();

    // TODO: do we want to call this here or just let each Registry run this itself?
    // do we need to pass in a world context?
    
    // Tile discovery needs a populated world, so it waits for BeginPlay
    // rather than riding along with the PostInitProperties wiring above.
    if (IsValid(TileRegistry))
    {
        TileRegistry->InitialiseRegistry();
    }
    
    // Tile discovery needs a populated world, so it waits for BeginPlay
    // rather than riding along with the PostInitProperties wiring above.
    if (IsValid(PieceRegistry))
    {
        PieceRegistry->InitialiseRegistry();
    }
}

void ABoardManagerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // TODO: is this even necessary?
    if (IsValid(TileRegistry))
    {
        TileRegistry->ShutdownRegistry();
    }

    // TODO: shutdown piece registry
    
    Super::EndPlay(EndPlayReason);
}
