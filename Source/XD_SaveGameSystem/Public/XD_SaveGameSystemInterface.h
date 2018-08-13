// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XD_SaveGameSystemInterface.generated.h"

UINTERFACE(MinimalAPI)
class UXD_SaveGame_GameInstanceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class XD_SAVEGAMESYSTEM_API IXD_SaveGame_GameInstanceInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "存档系统")
	class UXD_SaveGameSystemBase* GetSaveGameSystem() const;
	virtual class UXD_SaveGameSystemBase* GetSaveGameSystem_Implementation() const { return nullptr; }

};
