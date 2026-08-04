#include "InfluenceMap/GI_InfluenceMapSubsystem.h"
#include "InfluenceMap/GI_InfluenceMapWidget.h"
#include "InfluenceMap/GI_InfluenceMapTypes.h"
#include "InfluenceMap/GI_InfluenceMapVisualiserInterface.h"


// -- Lifecycle ---------------------------------------------------------------

void UGI_InfluenceMapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Registry.Reset();
    ActiveTag = FGameplayTag::EmptyTag;
    ActiveWidget = nullptr;
}

void UGI_InfluenceMapSubsystem::Deinitialize()
{
    HideDebugUI();
    ClearSelection();
    Registry.Reset();
    Super::Deinitialize();
}

// -- Registration ------------------------------------------------------------

bool UGI_InfluenceMapSubsystem::RegisterMap(const FGI_InfluenceMapEntry& Entry)
{
    if (!Entry.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::RegisterMap — Entry is invalid. "
                 "Tag must be set and Map must not be null."));
        return false;
    }

    if (FindEntryIndexByTag(Entry.Tag) != INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::RegisterMap — Tag '%s' is already registered. "
                 "Unregister the existing entry first."),
            *Entry.Tag.ToString());
        return false;
    }

    Registry.Add(Entry);

    // If the widget is currently showing, refresh it to include the new entry
    if (ActiveWidget)
    {
        ActiveWidget->SetupButtons(GetRegisteredTags());
    }

    return true;
}

bool UGI_InfluenceMapSubsystem::UnregisterMap(FGameplayTag Tag)
{
    const int32 Index = FindEntryIndexByTag(Tag);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::UnregisterMap — Tag '%s' is not registered."),
            *Tag.ToString());
        return false;
    }

    // Deactivate the visualiser if this map is currently selected
    if (ActiveTag == Tag)
    {
        ClearSelection();
    }

    Registry.RemoveAt(Index);

    // Refresh the widget if showing
    if (ActiveWidget)
    {
        ActiveWidget->SetupButtons(GetRegisteredTags());
    }

    return true;
}

// -- Selection ---------------------------------------------------------------

bool UGI_InfluenceMapSubsystem::SelectMap(FGameplayTag Tag)
{
    const int32 Index = FindEntryIndexByTag(Tag);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::SelectMap — Tag '%s' is not registered."),
            *Tag.ToString());
        return false;
    }

    // Deactivate current visualiser before switching
    ClearSelection();

    ActiveTag = Tag;

    // Activate the new visualiser if one is paired with this map
    const FGI_InfluenceMapEntry& Entry = Registry[Index];
    if (Entry.Visualiser != nullptr)
    {
        Entry.Visualiser->Activate();
    }

    OnInfluenceMapSelected.Broadcast(Tag, Index);
    return true;
}

bool UGI_InfluenceMapSubsystem::SelectMapByIndex(int32 Index)
{
    if (!Registry.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::SelectMapByIndex — Index %d is out of range. "
                 "Registry contains %d entries."),
            Index, Registry.Num());
        return false;
    }

    return SelectMap(Registry[Index].Tag);
}

void UGI_InfluenceMapSubsystem::ClearSelection()
{
    if (!ActiveTag.IsValid()) { return; }

    const int32 Index = FindEntryIndexByTag(ActiveTag);
    if (Index != INDEX_NONE)
    {
        const FGI_InfluenceMapEntry& Entry = Registry[Index];
        if (Entry.Visualiser != nullptr && Entry.Visualiser->IsActive())
        {
            Entry.Visualiser->Deactivate();
        }
    }

    ActiveTag = FGameplayTag::EmptyTag;
}

// -- Queries -----------------------------------------------------------------

FGI_InfluenceMapEntry UGI_InfluenceMapSubsystem::GetEntryByTag(FGameplayTag Tag) const
{
    const int32 Index = FindEntryIndexByTag(Tag);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::GetEntryByTag — Tag '%s' is not registered."),
            *Tag.ToString());
        return FGI_InfluenceMapEntry{};
    }

    return Registry[Index];
}

TArray<FGameplayTag> UGI_InfluenceMapSubsystem::GetRegisteredTags() const
{
    TArray<FGameplayTag> Out;
    Out.Reserve(Registry.Num());
    for (const FGI_InfluenceMapEntry& Entry : Registry)
    {
        Out.Add(Entry.Tag);
    }
    return Out;
}

TArray<FText> UGI_InfluenceMapSubsystem::GetRegisteredText() const
{
    TArray<FText> Out;
    Out.Reserve(Registry.Num());
    for (const FGI_InfluenceMapEntry& Entry : Registry)
    {
        Out.Add(Entry.DisplayName);
    }
    return Out;
}

// -- UI ----------------------------------------------------------------------

void UGI_InfluenceMapSubsystem::DestroyDebugUI()
{
    if (!ActiveWidget) { return; }

    HideDebugUI();
    
    ActiveWidget->OnSelectionChanged.RemoveDynamic(
        this, &UGI_InfluenceMapSubsystem::OnWidgetSelectionChanged);

    ActiveWidget->RemoveFromParent();
    ActiveWidget = nullptr;
}

void UGI_InfluenceMapSubsystem::CreateDebugWidget(
    const TSubclassOf<UGI_InfluenceMapWidget> WidgetClass, APlayerController* OwningPlayer)
{
    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::ShowDebugUI — WidgetClass is null. "
                 "Pass a valid UGI_InfluenceMapWidget subclass."));
        return;
    }

    if (!OwningPlayer)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::ShowDebugUI — OwningPlayer is null."));
        return;
    }
    
    DestroyDebugUI();

    ActiveWidget = CreateWidget<UGI_InfluenceMapWidget>(OwningPlayer, WidgetClass);
    if (!ActiveWidget)
    {
        UE_LOG(LogTemp, Error,
            TEXT("GI_InfluenceMapSubsystem::ShowDebugUI — Failed to create widget."));
        return;
    }

    // Wire the widget's selection delegate to the subsystem's handler
    ActiveWidget->OnSelectionChanged.AddDynamic(this, &UGI_InfluenceMapSubsystem::OnWidgetSelectionChanged);

    // Populate buttons from the current registry
    ActiveWidget->SetupButtons(GetRegisteredTags());

    ActiveWidget->AddToViewport();
    ActiveWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UGI_InfluenceMapSubsystem::ShowDebugUI() const
{
    if (!ActiveWidget) { return; }
    
    ActiveWidget->SetVisibility(ESlateVisibility::Visible);
}

void UGI_InfluenceMapSubsystem::HideDebugUI() const
{
    if (!ActiveWidget) { return; }

    ActiveWidget->SetVisibility(ESlateVisibility::Collapsed);
}

// -- Internal ----------------------------------------------------------------

void UGI_InfluenceMapSubsystem::OnWidgetSelectionChanged(FGameplayTag Tag, int32 Index)
{
    SelectMap(Tag);
}

int32 UGI_InfluenceMapSubsystem::FindEntryIndexByTag(FGameplayTag Tag) const
{
    return Registry.IndexOfByPredicate([&Tag](const FGI_InfluenceMapEntry& Entry)
    {
        return Entry.Tag == Tag;
    });
}
