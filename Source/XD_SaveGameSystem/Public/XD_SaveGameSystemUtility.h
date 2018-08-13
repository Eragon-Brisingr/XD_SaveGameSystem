// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(XD_SaveGameSystemLog, Log, All);
#define SaveGameSystem_Display_Log(Format, ...) UE_LOG(XD_SaveGameSystemLog, Log, TEXT(Format), ##__VA_ARGS__)
#define SaveGameSystem_Warning_LOG(Format, ...) UE_LOG(XD_SaveGameSystemLog, Warning, TEXT(Format), ##__VA_ARGS__)
#define SaveGameSystem_Error_Log(Format, ...) UE_LOG(XD_SaveGameSystemLog, Error, TEXT(Format), ##__VA_ARGS__)
