// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/GridFactionInterface.h"
#include "Interface/GridLevelInterface.h"
#include "Interface/GridSizeInterface.h"
#include "Interface/GridTileHandler.h"
#include "Piece/GridPieceBase.h"
#include "ConnectIt_GridPieceBase.generated.h"

/*
 * base class for blueprints to ensure we are adding all necessary interfaces
 */
UCLASS(BlueprintType, Blueprintable)
class CONNECTIT_API AConnectIt_GridPieceBase : public AGridPieceBase,
	public IGridFactionInterface,
	public IGridLevelInterface,
	public IGridSizeInterface,
	public IGridTileHandler
{
	GENERATED_BODY()

public:
	AConnectIt_GridPieceBase();
	
};
