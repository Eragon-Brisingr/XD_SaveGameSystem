// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XD_SaveGameInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UXD_SaveGameInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class XD_SAVEGAMESYSTEM_API IXD_SaveGameInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "游戏|存档")
	bool NeedNotSave() const;
	virtual bool NeedNotSave_Implementation() const;

	static void GameInit(UObject* Object);

	//只有Actor会调用
	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenGameInit();
	virtual void WhenGameInit_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenLoad();
	virtual void WhenLoad_Implementation();
};
