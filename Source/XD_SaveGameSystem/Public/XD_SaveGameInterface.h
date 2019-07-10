// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XD_SaveGameInterface.generated.h"

struct XD_SAVEGAMESYSTEM_API FXD_SaveGameVersion
{
	static const FGuid Guid;

	static const FName FriendlyName;

	static int32 Version;

	enum VersionType
	{
		Original,
	};
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UXD_SaveGameInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 实现该接口的Actor和Component会获得自动存档的能力，其被标记为SaveGame的属性也会被保存，Actor的Owner为该Actor或者Object的Outer为该Actor也会被保存
 */
class XD_SAVEGAMESYSTEM_API IXD_SaveGameInterface
{
	GENERATED_BODY()

		// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	//只有Actor与Component会调用，一般情况要求Actor不存在Owner才能保存
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "游戏|存档")
	bool NeedSave() const;
	virtual bool NeedSave_Implementation() const;
	static bool NeedSave(UObject* Obj) { return IXD_SaveGameInterface::Execute_NeedSave(Obj); }

	//只有Actor与Component会调用
	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenGameInit();
	virtual void WhenGameInit_Implementation();
	static void GameInit(UObject* Obj);

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenPostLoad();
	virtual void WhenPostLoad_Implementation();
	static void WhenPostLoad(UObject* Obj) { IXD_SaveGameInterface::Execute_WhenPostLoad(Obj); }

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenPreSave();
	virtual void WhenPreSave_Implementation() {}
	static void WhenPreSave(UObject* Obj) { IXD_SaveGameInterface::Execute_WhenPreSave(Obj); }

	virtual void WhenGameSerialize(FArchive& Archive) {}
	static void WhenGameSerialize(UObject* Obj, FArchive& Archive) { if (IXD_SaveGameInterface* SaveGameInterface = Cast<IXD_SaveGameInterface>(Obj)) SaveGameInterface->WhenGameSerialize(Archive); }

	static int32 GetSaveGameVersion(FArchive& Archive);
};
