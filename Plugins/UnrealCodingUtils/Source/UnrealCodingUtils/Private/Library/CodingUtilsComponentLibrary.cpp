// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/CodingUtilsComponentLibrary.h"


bool UCodingUtilsComponentLibrary::IsAuthoritative(const UActorComponent* Component)
{
	return IsValid(Component)
				&& IsValid(Component->GetOwner())
				&& Component->GetOwner()->HasAuthority();
}
