#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameMode/MatchData.h"
#include "HttpRequester.generated.h"

UINTERFACE()
class UHttpRequester : public UInterface
{
	GENERATED_BODY()
};

class OVERRIDE_API IHttpRequester
{
	GENERATED_BODY()
	
public:
	
	virtual void OnMatchesDataReceived(TArray<FMatchData> MatchesData) = 0;
};
