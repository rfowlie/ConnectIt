// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanics_Structs.generated.h"


UENUM(BlueprintType)
enum EGridDirection
{
	Row,
	Column,
	Diagonal_TopDown,
	Diagonal_BottomUp
};

// Encapsulate direction as a pair — cleaner than two loose multipliers
USTRUCT(BlueprintType)
struct FGridDirectionVector
{
	GENERATED_BODY()

	FGridDirectionVector() = default;
	FGridDirectionVector(const int32 InRow, const int32 InColumn) : Row(InRow), Column(InColumn) {}

	UPROPERTY()
	int32 Row = 0;

	UPROPERTY()
	int32 Column = 0;
};

USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FGridPosition
{
	GENERATED_BODY()

	FGridPosition() : X(0), Y(0) {}
	FGridPosition(const int32 ValX, const int32 ValY) : X(ValX), Y(ValY) {}
	
	UPROPERTY(BlueprintReadWrite)
	int32 X;
	
	UPROPERTY(BlueprintReadWrite)
	int32 Y;

	// FOR NOW
	int32 GetDistance(const FGridPosition& Other) const
	{
		// Chebyshev distance
		return FMath::Max(FMath::Abs(Other.X - X), FMath::Abs(Other.Y - Y));
	}
	
	bool operator==(const FGridPosition& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
	
	bool operator<(const FGridPosition& Other) const
	{
		return (X < Other.X) ? true : (Y < Other.Y) ? true : false;
	}

	FGridPosition operator+(const FGridPosition& Other) const
	{
		return FGridPosition(X + Other.X, Y + Other.Y);
	}

	FGridPosition operator-(const FGridPosition& Other) const
	{
		return FGridPosition(X - Other.X, Y - Other.Y);
	}
};

// Define a custom GetTypeHash function for hashing
inline uint32 GetTypeHash(const FGridPosition& Struct)
{
	// Use a combination of GetTypeHash for FString and int32
	return HashCombineFast(GetTypeHash(Struct.X), GetTypeHash(Struct.Y));
}
