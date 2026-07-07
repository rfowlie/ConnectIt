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

	// --- Load Outs ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	TObjectPtr<UActionLoadOutDataAsset> PlayerLoadout = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	TObjectPtr<UActionLoadOutDataAsset> EnemyLoadout = nullptr;

	// --- Win Condition ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	int32 WinScoreThreshold = 100;

	// --- AI Config ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	int32 AISearchDepth = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Config")
	int32 AIThreadDepth = 1;


protected:
	virtual void BeginPlay() override;
	
};
