#include "CustomAssetManager.h"
#include "AbilitySystemGlobals.h"

UCustomAssetManager& UCustomAssetManager::Get()
{
	UCustomAssetManager* Singleton = Cast<UCustomAssetManager>(GEngine->AssetManager);

	if (Singleton)
	{
		return *Singleton;
	}
	else
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid AssetManager in DefaultEngine.ini, must be GDAssetManager!"));
		return *NewObject<UCustomAssetManager>();	 // never calls this
	}
}

void UCustomAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	UAbilitySystemGlobals::Get().InitGlobalData();
}
