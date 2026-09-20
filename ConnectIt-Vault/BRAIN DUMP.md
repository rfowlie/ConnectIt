
- Action UI getting the correct values from player state
	- Should call one method and get an array of action states. States describe
		- UI name
		- Permenant / numbered uses (how many uses per turn + how many uses this turn)
		- Whether action is viable to use (greyed out otherwise)

Calls each and then we create or adjust the UI element to setup depending on the struct. Should we create matching UI elements? Or just build differently according to which one?

UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")  
TArray<FPermanentActionRuntimeEntry> GetPermanentActionState() const { return PermanentActionState; }  
  
UFUNCTION(BlueprintPure, Category = "Turn Based|Actions")  
TArray<FNumberedActionRuntimeEntry> GetNumberedActionState() const { return NumberedActionState; }