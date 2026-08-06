// Fill out your copyright notice in the Description page of Project Settings.


#include "GameIntelligence/InfluenceMap/PieceControlMapVisualizer.h"


FGameplayTag UPieceControlMapVisualizer::GetTag() const
{
	return FGameplayTag();
}

FText UPieceControlMapVisualizer::GetDisplayName() const
{
	return FText::FromString("Text");
}

bool UPieceControlMapVisualizer::IsActive() const
{
	return false;
}

void UPieceControlMapVisualizer::Activate()
{
}

void UPieceControlMapVisualizer::Deactivate()
{
}
