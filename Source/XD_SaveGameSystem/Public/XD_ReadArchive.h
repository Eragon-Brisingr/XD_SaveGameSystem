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
	FXD_ReadArchive(FArchive& InInnerArchive, class ULevel* Level, TArray<UObject*>& ObjectReferenceCollection, const FIntVector& OldWorldOrigin)
		:FXD_ProxyArchiveBase(InInnerArchive), Level(Level), ObjectReferenceCollection(ObjectReferenceCollection), OldWorldOrigin(OldWorldOrigin)
	{
		ArIsSaveGame = true;

		SetIsLoading(true);

		SetIsPersistent(false);
	}

	TWeakObjectPtr<class ULevel> Level;

	TArray<UObject*>& ObjectReferenceCollection;

	FIntVector OldWorldOrigin;

	FString GetArchiveName() const override { return TEXT("FXD_ReadArchive"); }

	virtual FArchive& operator<<(class UObject*& Obj) override;
};
