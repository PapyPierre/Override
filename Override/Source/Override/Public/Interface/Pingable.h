// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Pingable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UPingable : public UInterface
{
	GENERATED_BODY()
};

class OVERRIDE_API IPingable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void Ping();
};
