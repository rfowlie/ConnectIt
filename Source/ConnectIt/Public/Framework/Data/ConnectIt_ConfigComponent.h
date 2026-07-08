// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConnectIt_ConfigComponent.generated.h"

class UActionLoadOutDataAsset;
class AConnectIt_GridPiece;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_ConfigComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UConnectIt_ConfigComponent();

	// --- LoadOuts ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	TObjectPtr<UActionLoadOutDataAsset> PlayerLoadout = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	TObjectPtr<UActionLoadOutDataAsset> EnemyLoadout = nullptr;

	// --- Rules ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	float WinScoreThreshold = 100.f;

	// How many tiles in a line required to score
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config",
		meta = (ClampMin = 3))
	int32 ConnectLength = 4;

	// --- AI ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	int32 AISearchDepth = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	int32 AIThreadDepth = 1;

	// --- Piece Pool ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	TSubclassOf<AConnectIt_GridPiece> PieceActorClass = nullptr;

	// Per-client pool size -- each client manages its own pool
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config",
		meta = (ClampMin = 1))
	int32 PiecePoolInitialSize = 32;


protected:
	virtual void BeginPlay() override;
	
};
