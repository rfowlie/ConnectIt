# GI Influence Maps — Composite Maps Guide

## Overview

The GI Influence Map plugin is intentionally minimal. It defines contracts via interfaces
and leaves implementation details to the project. Composite maps — influence maps that
internally manage and combine multiple other maps — are a natural extension of this design
and require no special plugin support. This document outlines two patterns for constructing
them.

---

## The Interface Contract

Every influence map, whether a leaf map or a composite, satisfies IGI_InfluenceMap:

```cpp
UINTERFACE()
class UGI_InfluenceMap : public UInterface { GENERATED_BODY() };

class IGI_InfluenceMap
{
    GENERATED_BODY()
public:
    virtual FGameplayTag  GetTag()         const = 0;
    virtual FText         GetDisplayName() const = 0;

    // Bind the delegate that determines when this map updates.
    // The map stores and binds to this delegate internally.
    // When the delegate fires, the map executes its update logic.
    virtual void SetUpdateDelegate(FSimpleDelegate InUpdateDelegate) = 0;
};
```

The plugin does not call Update() directly. The map binds to whatever delegate is passed
into SetUpdateDelegate and responds when that delegate fires. When to fire the delegate is
entirely the caller's concern — a tick, a timer, a gameplay event, or never.

---

## Pattern 1 — Managed Construction

The composite map constructs its internal child maps itself and passes its own update
delegate down to each child. All children update together when the composite updates.

This is the simpler pattern and suits cases where all child maps share the same update
cadence.

```cpp
// Project-specific composite map
// Does not live in the plugin — lives in the project or grid mechanics plugin
class UMyGame_PriorityMap : public UObject, public IGI_InfluenceMap
{
public:

    virtual void SetUpdateDelegate(FSimpleDelegate InUpdateDelegate) override
    {
        // Construct children internally
        ThreatMap    = NewObject<UMyGame_ThreatMap>(this);
        ResourceMap  = NewObject<UMyGame_ResourceMap>(this);

        // Bind the composite's own update logic
        FSimpleDelegate CompositeDelegate = FSimpleDelegate::CreateUObject(
            this, &UMyGame_PriorityMap::OnUpdate);

        // Pass the same delegate cadence down to children
        // Children will update whenever the composite updates
        ThreatMap->SetUpdateDelegate(InUpdateDelegate);
        ResourceMap->SetUpdateDelegate(InUpdateDelegate);

        // Composite itself also binds so it can combine results after children update
        InUpdateDelegate.BindUObject(this, &UMyGame_PriorityMap::OnCombine);
    }

private:

    UPROPERTY()
    TObjectPtr<UMyGame_ThreatMap>   ThreatMap;

    UPROPERTY()
    TObjectPtr<UMyGame_ResourceMap> ResourceMap;

    void OnCombine()
    {
        // Children have already updated via their own bindings.
        // Query their current state and combine into this map's output.
    }
};
```

### Sequencing Note

When multiple objects bind to the same delegate, firing order is not guaranteed. If the
composite's OnCombine must run after children have updated, either:

- Bind OnCombine to a separate downstream delegate that children fire when complete, or
- Have the composite call child update logic directly in OnCombine rather than relying
  on delegate ordering.

---

## Pattern 2 — Dependency Injection

The composite map accepts already-constructed child maps. Each child has its own update
delegate set independently before being passed in. The composite does not set or override
child update delegates.

This pattern suits cases where children update at different cadences — for example a
threat map that updates every tick and a resource map that updates every five seconds.

```cpp
class UMyGame_PriorityMap : public UObject, public IGI_InfluenceMap
{
public:

    // Children are injected already constructed and already bound to their own
    // update delegates. The composite does not touch their update timing.
    void InjectMaps(
        TScriptInterface<IGI_InfluenceMap> InThreatMap,
        TScriptInterface<IGI_InfluenceMap> InResourceMap)
    {
        ThreatMap   = InThreatMap;
        ResourceMap = InResourceMap;
    }

    virtual void SetUpdateDelegate(FSimpleDelegate InUpdateDelegate) override
    {
        // The composite only binds its own combination step.
        // Children update independently on their own schedules.
        // The composite combines whatever their current state is when it fires.
        InUpdateDelegate.BindUObject(this, &UMyGame_PriorityMap::OnCombine);
    }

private:

    TScriptInterface<IGI_InfluenceMap> ThreatMap;
    TScriptInterface<IGI_InfluenceMap> ResourceMap;

    void OnCombine()
    {
        // Children may have updated more or less recently than this call.
        // Query their current state and combine. This is intentional —
        // each child reflects the most recent state at its own update rate.
    }
};
```

### Construction at the Call Site

```cpp
// Construct and configure each child independently
UMyGame_ThreatMap* Threat = NewObject<UMyGame_ThreatMap>(this);
Threat->SetUpdateDelegate(
    FSimpleDelegate::CreateUObject(this, &AMyAIDirector::OnTickFastUpdate));

UMyGame_ResourceMap* Resources = NewObject<UMyGame_ResourceMap>(this);
Resources->SetUpdateDelegate(
    FSimpleDelegate::CreateUObject(this, &AMyAIDirector::OnTickSlowUpdate));

// Construct composite and inject
UMyGame_PriorityMap* Priority = NewObject<UMyGame_PriorityMap>(this);
Priority->InjectMaps(Threat, Resources);
Priority->SetUpdateDelegate(
    FSimpleDelegate::CreateUObject(this, &AMyAIDirector::OnTickFastUpdate));

// Register the composite with the subsystem — children are internal detail
Subsystem->RegisterMap(Priority, PriorityVisualiser);
```

The subsystem only sees the composite. Children are invisible to it. This is intentional —
the subsystem does not need to know how a map produces its data, only that it satisfies
the interface.

---

## Choosing Between Patterns

| | Managed Construction | Dependency Injection |
|---|---|---|
| Child update cadence | Same as composite | Independent per child |
| Child lifetime | Owned by composite | Owned externally |
| Construction complexity | Simple | More setup at call site |
| Child reuse across composites | Not natural | Straightforward |
| Testability | Composite harder to isolate | Children testable independently |

Use managed construction when children exist solely to serve the composite and share its
update timing. Use dependency injection when children have independent lifetimes, update
at different rates, or are shared across multiple composites.

---

## What the Plugin Does Not Prescribe

- How maps store or expose their data. A float grid, an enumeration map, a tag map — all
  are equally valid. The interface says nothing about data shape.

- How composites combine child data. Weighted sum, max value, boolean AND — this is
  entirely the project's concern.

- Whether composite children are registered with the subsystem. In most cases they should
  not be — only register maps you want visible in the debug UI. Children of a composite
  are an implementation detail unless you specifically want them debuggable independently.

- Update ordering between children. If ordering matters, the composite is responsible for
  enforcing it, not the plugin.
