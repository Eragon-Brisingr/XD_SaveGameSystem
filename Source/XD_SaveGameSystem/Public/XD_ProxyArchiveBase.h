// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <ArchiveProxy.h>
#include "XD_ProxyArchiveBase.generated.h"

/**
 * 
 */

enum class EObjectArchiveType : uint8
{
	//空指针
	NullObject,
	//资源
	Asset,
	//在Package中的非资源非Actor对象
	InPackageObject,
	//运行中生成的非Actor对象
	DynamicObject,
	//在Package中的Actor
	InPackageActor,
	//运行中生成的Actor
	DynamicActor
};

USTRUCT()
struct XD_SAVEGAMESYSTEM_API FXD_AssetSaveData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	TSoftObjectPtr<UObject> Path;
};

USTRUCT()
struct XD_SAVEGAMESYSTEM_API FXD_InPackageSaveData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	TSoftObjectPtr<UObject> Path;

	UPROPERTY(SaveGame)
	TSoftClassPtr<UObject> ClassPath;
};

USTRUCT()
struct XD_SAVEGAMESYSTEM_API FXD_DynamicSaveData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FString Name;

	UPROPERTY(SaveGame)
	TSoftClassPtr<UObject> ClassPath;
};

USTRUCT()
struct XD_SAVEGAMESYSTEM_API FXD_ActorExtraSaveData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame)
	FTransform Transform;
};

struct XD_SAVEGAMESYSTEM_API FXD_ProxyArchiveBase : public FArchiveProxy
{
	FXD_ProxyArchiveBase(FArchive& InInnerArchive)
		:FArchiveProxy(InInnerArchive)
	{

	}

	using FArchive::operator<<; // For visibility of the overloads we don't override

	//~ Begin FArchive Interface
	virtual FArchive& operator<<(FLazyObjectPtr& Value) override;
	virtual FArchive& operator<<(FSoftObjectPtr& Value) override;
	virtual FArchive& operator<<(FSoftObjectPath& Value) override;
	virtual FArchive& operator<<(FWeakObjectPtr& Value) override;
	//~ End FArchive Interface
};

