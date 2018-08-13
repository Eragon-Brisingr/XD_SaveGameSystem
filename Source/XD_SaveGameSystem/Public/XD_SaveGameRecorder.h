// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XD_SaveGameRecorder.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct XD_SAVEGAMESYSTEM_API FARPG_Recorder
{
	GENERATED_BODY()
public:
	FARPG_Recorder() = default;
	FARPG_Recorder(const TArray<uint8>& Data)
		:Data(Data)
	{}

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<uint8> Data;
};

