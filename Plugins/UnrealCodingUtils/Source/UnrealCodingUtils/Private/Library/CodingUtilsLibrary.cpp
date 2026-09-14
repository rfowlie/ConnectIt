// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/CodingUtilsLibrary.h"


bool UCodingUtilsLibrary::IsAuthoritativeActor(const AActor* Actor)
{
	return IsValid(Actor)
		&& IsValid(Actor->GetOwner())
		&& Actor->GetOwner()->HasAuthority();
}

bool UCodingUtilsLibrary::IsAuthoritativeComponent(const UActorComponent* Component)
{
	return IsValid(Component)
		&& IsValid(Component->GetOwner())
		&& Component->GetOwner()->HasAuthority();
}
