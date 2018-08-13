// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XD_SG_WorldSettingsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class XD_SAVEGAMESYSTEM_API UXD_SG_WorldSettingsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UXD_SG_WorldSettingsComponent();

	uint8 bActiveAutoSave : 1;
	uint8 bIsLoadingLevel : 1;
	uint8 bIsInitingLevel : 1;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
