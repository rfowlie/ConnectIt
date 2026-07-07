// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Manager/ConnectIt_BoardManager.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"


AConnectIt_BoardManager::AConnectIt_BoardManager()
{
	PrimaryActorTick.bCanEverTick = false;

	ConnectItConfig = CreateDefaultSubobject<UConnectIt_ConfigComponent>(TEXT("ConnectItConfig"));
}

const UConnectIt_ConfigComponent* AConnectIt_BoardManager::GetConnectItConfig() const
{
	return ConnectItConfig;
}
