// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Library/ConnectIt_GameRulesLibrary.h"

bool UConnectIt_GameRulesLibrary::IsGameOver(const TArray<float>& ScoreBoard)
{
	return ScoreBoard.ContainsByPredicate([](const float& Score)
	{
		return Score >= ConnectIt_Score_Max;
	});
}
