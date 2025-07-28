// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMechanics_ScoringLibrary.h"

#include "Kismet/KismetMathLibrary.h"

void UGameMechanics_ScoringLibrary::MakeObjectScoreArray(TArray<FObjectScore>& OutObjectScores, const TMap<UObject*, float>& ScoreMap)
{
	for (const auto& Elem : ScoreMap)
	{
		OutObjectScores.Add(FObjectScore(Elem.Key, Elem.Value));		
	}

	OutObjectScores.Sort();
}

UObject* UGameMechanics_ScoringLibrary::ScoreSelect_TopN(const TMap<UObject*, float>& ScoreMap, const int32 N)
{
	if (ScoreMap.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ScoreMap is empty"));
		return nullptr;
	}

	const TArray<UObject*> SortedMapKeys = GetSortedKeysByValue(ScoreMap);
	const int32 NValue = FMath::Min(FMath::Max(1, N), SortedMapKeys.Num());
	return SortedMapKeys[FMath::RandHelper(NValue)];
}

TMap<UObject*, float> UGameMechanics_ScoringLibrary::ScoreModify_RandomNoise(const TMap<UObject*, float>& ScoreMap,
	float Min, float Max)
{
	TMap<UObject*, float> ModifiedMap = ScoreMap;
	for (auto& Pair : ModifiedMap)
	{
		Pair.Value += FMath::FRandRange(Min, Max);
	}
	
	return ModifiedMap;
}