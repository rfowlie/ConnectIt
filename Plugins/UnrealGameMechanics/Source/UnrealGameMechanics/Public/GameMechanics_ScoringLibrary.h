// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameMechanics_ScoringLibrary.generated.h"



template<typename KeyType, typename ValueType>
TArray<KeyType> GetSortedKeysByValue(const TMap<KeyType, ValueType>& InMap, bool bDescending = true)
{
	static_assert(TIsArithmetic<ValueType>::Value, "ValueType must be an arithmetic (numeric) type.");

	// Convert to key-value pairs
	TArray<TPair<KeyType, ValueType>> Pairs;
	for (auto Pair : InMap)
	{
		Pairs.Add(Pair);
	}
	
	// Sort by value
	Pairs.Sort([bDescending](const TPair<KeyType, ValueType>& A, const TPair<KeyType, ValueType>& B)
	{
		return bDescending ? (A.Value > B.Value) : (A.Value < B.Value);
	});

	// Extract sorted keys
	TArray<KeyType> SortedKeys;
	SortedKeys.Reserve(Pairs.Num());
	for (const TPair<KeyType, ValueType>& Pair : Pairs)
	{
		SortedKeys.Add(Pair.Key);
	}

	return SortedKeys;
}

USTRUCT(BlueprintType)
struct FObjectScore
{
	GENERATED_BODY()

	FObjectScore() {}
	FObjectScore(UObject* InObject, const float InScore) : Object(InObject), Score(InScore) {}
	FObjectScore(const FObjectScore& Other) : Object(Other.Object), Score(Other.Score) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UObject* Object = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Score = 0;

	bool operator==(const FObjectScore& Other) const
	{
		return Score == Other.Score;
	}
	
	auto operator<(const FObjectScore& Other) const
	{
		return Score < Other.Score;
	}
	
};

/**
 * 
 */
UCLASS()
class UNREALGAMEMECHANICS_API UGameMechanics_ScoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void MakeObjectScoreArray(TArray<FObjectScore>& OutObjectScores, const TMap<UObject*, float>& ScoreMap);

# pragma region selectors
	// Given a map of object scores, sort then select randomly from the top N scores
	UFUNCTION(BlueprintCallable, Category = "Scoring | Selectors")
	static UObject* ScoreSelect_TopN(const TMap<UObject*, float>& ScoreMap, const int32 N);

# pragma endregion

# pragma region modifiers
	// return object scores adjusted uniquely by a random value between min and max
	UFUNCTION(BlueprintCallable, Category = "Scoring | Modifiers")
	static TMap<UObject*, float> ScoreModify_RandomNoise(const TMap<UObject*, float>& ScoreMap, float Min, float Max);

# pragma endregion
	
};