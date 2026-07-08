// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "GridValidatorBase.generated.h"

/**
 * 
 */
UCLASS()
class UNREALGRIDMECHANICSEDITOR_API UGridValidatorBase : public UEditorValidatorBase
{
	GENERATED_BODY()

public:

	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		UObject* InAsset, TArray<FText>& ValidationErrors) override;
};
