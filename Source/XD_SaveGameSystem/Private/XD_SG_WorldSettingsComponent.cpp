// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SG_WorldSettingsComponent.h"
#include "XD_SaveGameSystemBase.h"


// Sets default values for this component's properties
UXD_SG_WorldSettingsComponent::UXD_SG_WorldSettingsComponent()
	:bActiveAutoSave(true)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


void UXD_SG_WorldSettingsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (bActiveAutoSave)
	{
		ULevel* Level = GetOwner()->GetLevel();
		if (Level && Level->bIsBeingRemoved)
		{
			if (UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(this))
			{
				if (SaveGameSystem->bEnableAutoSave && SaveGameSystem->IsLevelInitCompleted(Level))
				{
					SaveGameSystem->SaveLevel(Level);
				}
			}
		}
	}
}
