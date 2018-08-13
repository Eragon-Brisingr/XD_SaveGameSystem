// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "XD_SaveGameSystemInterface.h"
#include "XD_SaveGame_WorldSettings.generated.h"

/**
 * 
 */
UCLASS()
class XD_SAVEGAMESYSTEM_API AXD_SaveGame_WorldSettings : public AWorldSettings
{
	GENERATED_BODY()
	
public:
	AXD_SaveGame_WorldSettings();

	class UXD_SG_WorldSettingsComponent* SaveGame_WorldSettingsComponent;
};
