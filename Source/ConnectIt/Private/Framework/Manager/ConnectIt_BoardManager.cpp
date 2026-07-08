// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Manager/ConnectIt_BoardManager.h"

#include "Board/ConnectItBoardStateComponent.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"


AConnectIt_BoardManager::AConnectIt_BoardManager()
{
	PrimaryActorTick.bCanEverTick = false;

	ConnectItConfigComponent = CreateDefaultSubobject<UConnectIt_ConfigComponent>(TEXT("ConnectItConfig"));
	ConnectItBoardStateComponent = CreateDefaultSubobject<UConnectItBoardStateComponent>(TEXT("ConnectItBoardState"));

}

UConnectIt_ConfigComponent* AConnectIt_BoardManager::GetConnectItConfig() const
{
	return ConnectItConfigComponent;
}

UConnectItBoardStateComponent* AConnectIt_BoardManager::GetConnectItBoardState() const
{
	return ConnectItBoardStateComponent;
}
