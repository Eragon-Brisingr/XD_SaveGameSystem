// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XD_ProxyArchiveBase.h"

/**
 * 
 */
struct XD_SAVEGAMESYSTEM_API FXD_ReadArchive : public FXD_ProxyArchiveBase
{
public:
	FXD_ReadArchive(FArchive& InInnerArchive, class ULevel* Level, TArray<UObject*>& ObjectReferenceCollection, TArray<UObject*>& ObjectExecuteWhenLoadOrder, const FIntVector& OldWorldOrigin);

	TWeakObjectPtr<class ULevel> Level;

	TArray<UObject*>& ObjectReferenceCollection;

	// 需要记录下Object的WhenLoad执行顺序，Outer拥有的Object初始化完毕再初始化Outer
	TArray<UObject*>& ObjectExecuteWhenLoadOrder;

	FIntVector OldWorldOrigin;

	FString GetArchiveName() const override { return TEXT("FXD_ReadArchive"); }

	virtual FArchive& operator<<(class UObject*& Obj) override;
};
