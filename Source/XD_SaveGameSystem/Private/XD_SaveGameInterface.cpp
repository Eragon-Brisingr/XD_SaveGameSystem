// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGameInterface.h"
#include "XD_SaveGameSystemUtility.h"
#include "XD_DebugFunctionLibrary.h"


// Add default functionality here for any IXD_SaveGameInterface functions that are not pure virtual.

//Init的时候可能会SpawnActor，不能用bool
int32 WhenGameInitIsOverride;

void IXD_SaveGameInterface::GameInit(UObject* Object)
{
	WhenGameInitIsOverride += 1;
	int32 ChachWhenGameInitIsOverride = WhenGameInitIsOverride;

	IXD_SaveGameInterface::Execute_WhenGameInit(Object);

	if (ChachWhenGameInitIsOverride == WhenGameInitIsOverride)
	{
		SaveGameSystem_Display_Log("初始化%s", *UXD_DebugFunctionLibrary::GetDebugName(Object));
		WhenGameInitIsOverride -= 1;
	}
}

void IXD_SaveGameInterface::WhenGameInit_Implementation()
{
	WhenGameInitIsOverride -= 1;
}

bool IXD_SaveGameInterface::NeedNotSave_Implementation() const
{
	if (const AActor* Actor = Cast<AActor>(this))
	{
		return Actor->GetOwner() ? true : false;
	}
	else
	{
		return false;
	}
}

void IXD_SaveGameInterface::WhenLoad_Implementation()
{

}