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

	//项目中自行声明Type作为项目使用的Type
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

	// 只有Actor与Component会调用的函数
public:
	// 一般情况要求Actor不存在Owner才能保存
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "游戏|存档")
	bool NeedSave() const;
	virtual bool NeedSave_Implementation() const;
	static bool NeedSave(UObject* Obj) { return Execute_NeedSave(Obj); }

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenGameInit();
	virtual void WhenGameInit_Implementation();
	static void GameInit(UObject* Obj);

	// 只有Actor会调用的函数，初始化和加载时越大优先级越高，存档时相反
	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	int32 GetActorSerializePriority() const;
	virtual int32 GetActorSerializePriority_Implementation() const { return -1; }
	static int32 GetActorSerializePriority(UObject* Obj) { return Execute_GetActorSerializePriority(Obj); };
public:
	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenPostLoad();
	virtual void WhenPostLoad_Implementation();
	static void WhenPostLoad(UObject* Obj) { Execute_WhenPostLoad(Obj); }

	UFUNCTION(BlueprintNativeEvent, Category = "游戏|读档")
	void WhenPreSave();
	virtual void WhenPreSave_Implementation() {}
	static void WhenPreSave(UObject* Obj) { Execute_WhenPreSave(Obj); }

	// Native可实作的函数，可塞入定制化的数据
	virtual void WhenGameSerialize(FArchive& Archive) {}
	static void WhenGameSerialize(UObject* Obj, FArchive& Archive) { if (IXD_SaveGameInterface* SaveGameInterface = Cast<IXD_SaveGameInterface>(Obj)) SaveGameInterface->WhenGameSerialize(Archive); }
public:
	static int32 GetSaveGameVersion(FArchive& Archive);
};
