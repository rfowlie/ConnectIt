// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_TileRegistry.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"


UConnectIt_BoardStateComponent* UConnectIt_TileRegistry::GetBoardState() const
{
    return UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
}
