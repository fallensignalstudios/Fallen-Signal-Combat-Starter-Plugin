// Copyright Fallen Signal Studios. All Rights Reserved.

#include "Data/GCSCombatConfig.h"

FPrimaryAssetId UGCSCombatConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("GCSCombatConfig"), GetFName());
}
