#include "BlueprintHelpers.h"
#include "Misc/ConfigCacheIni.h"

FString UBlueprintHelpers::GetProjectVersion()
{
	FString Version;

	GConfig->GetString(
		TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"),
		Version,
		GGameIni
	);

	return Version;
}
