#include "BlueprintHelpers.h"
#include "Version.h"
#include "Misc/ConfigCacheIni.h"

FString UBlueprintHelpers::GetProjectVersion()
{
	return TEXT(GAME_VERSION);
}
