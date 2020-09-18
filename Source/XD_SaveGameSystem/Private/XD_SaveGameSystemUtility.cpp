// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveGameSystemUtility.h"

DEFINE_LOG_CATEGORY(XD_SaveGameSystemLog);

FString SaveGameSystemUtility::GetLevelName(ULevel* Level)
{
	FString LevelFullName = Level->GetOutermost()->GetName();
	int32 Index;
	if (LevelFullName.FindLastChar(TEXT('/'), Index))
	{
		return LevelFullName.Right(LevelFullName.Len() - Index - 1);
	}
	return LevelFullName;
}

AWorldSettings* SaveGameSystemUtility::GetCurrentLevelWorldSettings(ULevel* Level)
{
	return CastChecked<UWorld>(Level->GetOuter())->GetWorldSettings();
}
