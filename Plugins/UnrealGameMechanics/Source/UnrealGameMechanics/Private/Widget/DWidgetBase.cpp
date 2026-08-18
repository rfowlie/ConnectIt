// Fill out your copyright notice in the Description page of Project Settings.

#include "Widget/DWidgetBase.h"

void UDWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    if (!bBound)
    {
        BindDelegates();
        bBound = true;
    }

    RefreshAll();
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

void UDWidgetBase::RefreshAll()
{
    RefreshFields();
    OnDebugStateUpdated();
}
