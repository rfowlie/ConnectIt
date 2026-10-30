// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/GameMode/ConnectIt_GameMode_AdventureHub.h"
#include "Adventure/ConnectIt_AdventureHubComponent.h"


AConnectIt_GameMode_AdventureHub::AConnectIt_GameMode_AdventureHub()
{
    AdventureHub = CreateDefaultSubobject<UConnectIt_AdventureHubComponent>(TEXT("AdventureHub"));
}
