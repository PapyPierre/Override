#include "Hacks/GameplayHackTargetData.h"

bool FGameplayHackTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	int32 NumTargets = Targets.Num();
	Ar << NumTargets;

	if (Ar.IsLoading())
	{
		Targets.SetNum(NumTargets);
	}

	for (int32 i = 0; i < NumTargets; i++)
	{
		UObject* Actor = nullptr;
        
		if (Ar.IsSaving())
		{
			Actor = Targets[i].Get();
		}
        
		bool bMapped = Map->SerializeObject(Ar, AActor::StaticClass(), Actor);
        
		if (Ar.IsLoading())
		{
			Targets[i] =  Cast<AActor>(Actor);;
		}
        
		if (!bMapped)
		{
			bOutSuccess = false;
			return false;
		}
	}

	bOutSuccess = true;
	return true;
}