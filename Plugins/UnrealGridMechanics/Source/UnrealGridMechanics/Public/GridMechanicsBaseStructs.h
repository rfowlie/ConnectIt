// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.generated.h"

class AGridTileBase;
class AGridUnitBase;


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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 X;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
		return (X < Other.X) || (X == Other.X && Y < Other.Y);
	}

	FGridPosition operator+(const FGridPosition& Other) const
	{
		return FGridPosition(X + Other.X, Y + Other.Y);
	}

	FGridPosition& operator+=(const FGridPosition& Other)
	{
		X += Other.X;
		Y += Other.Y;
		return *this;
	}

	FGridPosition operator-(const FGridPosition& Other) const
	{
		return FGridPosition(X - Other.X, Y - Other.Y);
	}

	FGridPosition operator*(const uint8& Scalar) const
	{
		return FGridPosition(X * Scalar, Y * Scalar);
	}
	
	FGridPosition operator/(const float& Float) const
	{
		FGridPosition OutGridPosition;
		OutGridPosition.X = FMath::RoundToInt32(X / Float);
		OutGridPosition.Y = FMath::RoundToInt32(Y / Float);
		return OutGridPosition;
	}
};

// Define a custom GetTypeHash function for hashing
inline uint32 GetTypeHash(const FGridPosition& Struct)
{
	// Use a combination of GetTypeHash for FString and int32
	return HashCombineFast(GetTypeHash(Struct.X), GetTypeHash(Struct.Y));
}

USTRUCT(BlueprintType)
struct FGridPositionArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridPosition> GridPositions;
	
};

USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FGridMovement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AGridTileBase> GridTile = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AGridTileBase> ParentTile = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Cost = 0;
	
	
	bool operator==(const FGridMovement& Other) const
	{
		return GridTile == Other.GridTile;
	}
};

// Define a custom GetTypeHash function for hashing
inline uint32 GetTypeHash(const FGridMovement& Struct)
{
	// Use a combination of GetTypeHash for FString and int32
	return GetTypeHash(Struct.GridTile);
}

USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FGridPair
{
	GENERATED_BODY()
	
	FGridPair() {}
	FGridPair(AGridTileBase* InGridTile, AGridUnitBase* InGridUnit) : GridTile(InGridTile), GridUnit(InGridUnit) {}
	
	UPROPERTY(BlueprintReadOnly)
	AGridTileBase* GridTile = nullptr;

	UPROPERTY(BlueprintReadOnly)
	AGridUnitBase* GridUnit = nullptr;
	
};