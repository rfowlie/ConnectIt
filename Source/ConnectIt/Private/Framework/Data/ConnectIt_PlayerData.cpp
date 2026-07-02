// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Data/ConnectIt_PlayerData.h"

void UConnectIt_PlayerData::AddToScore(const int32 InScore)
{
	Score += InScore;

	// TODO: fire callbacks?
}
