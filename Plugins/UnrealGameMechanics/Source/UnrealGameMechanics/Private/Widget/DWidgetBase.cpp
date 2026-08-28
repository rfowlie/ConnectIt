// Fill out your copyright notice in the Description page of Project Settings.

#include "Widget/DWidgetBase.h"

DEFINE_LOG_CATEGORY(LogDWidget);


void UDWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    if (!bBound)
    {
        BindDelegates();
        bBound = true;
    }
}

void UDWidgetBase::NativeDestruct()
{
    if (bBound)
    {
        UnbindDelegates();
        bBound = false;
    }

    Super::NativeDestruct();
}
