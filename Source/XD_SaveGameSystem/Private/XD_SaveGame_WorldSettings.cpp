// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGame_WorldSettings.h"



AXD_SaveGame_WorldSettings::AXD_SaveGame_WorldSettings()
{
	SaveGame_WorldSettingsComponent = CreateDefaultSubobject<UXD_SG_WorldSettingsComponent>(GET_MEMBER_NAME_CHECKED(AXD_SaveGame_WorldSettings, SaveGame_WorldSettingsComponent));
}
