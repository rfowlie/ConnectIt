// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/BoardStateComponentBase.h"


UBoardStateComponentBase::UBoardStateComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBoardStateComponentBase::BroadcastChange() const
{
	OnBoardStateChanged.Broadcast();
}


