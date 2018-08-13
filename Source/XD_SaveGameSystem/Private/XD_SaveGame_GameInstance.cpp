// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGame_GameInstance.h"




UXD_SaveGame_GameInstance::UXD_SaveGame_GameInstance()
{
	SaveGameSystem = CreateDefaultSubobject<UXD_SaveGameSystemBase>(GET_MEMBER_NAME_CHECKED(UXD_SaveGame_GameInstance, SaveGameSystem));
}
