// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XD_ProxyArchiveBase.h"

/**
 * 
 */
struct XD_SAVEGAMESYSTEM_API FXD_WriteArchive : public FXD_ProxyArchiveBase
{
	FXD_WriteArchive(FArchive& InInnerArchive, class ULevel* Level, TArray<UObject*>& ObjectReferenceCollection)
		:FXD_ProxyArchiveBase(InInnerArchive), Level(Level), ObjectReferenceCollection(ObjectReferenceCollection)
	{
		ArIsSaveGame = true;

		SetIsSaving(true);

		SetIsPersistent(false);
	}

	TArray<TWeakObjectPtr<class AActor>> TopActor;

	TWeakObjectPtr<class ULevel> Level;

	TArray<UObject*>& ObjectReferenceCollection;

	FString GetArchiveName() const { return TEXT("FXD_WriteArchive"); }

	virtual FArchive& operator<<(class UObject*& Obj) override;

#if WITH_EDITOR
	void CheckActorError(AActor* Actor);

	void CheckDynamicObjectError(const UObject* Object) const;
#endif

};
