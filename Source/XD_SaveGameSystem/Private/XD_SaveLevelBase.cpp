﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_SaveLevelBase.h"
#include "XD_SaveGameInterface.h"

bool UXD_SaveLevelBase::SaveLevel(ULevel* OuterLevel)
{
	if (OuterLevel)
	{
		OldWorldOrigin = UGameplayStatics::GetWorldOriginLocation(OuterLevel);

		TArray<UObject*> ObjectReferenceCollection;

		for (AActor* Actor : OuterLevel->Actors)
		{
			if (Actor && Actor->Implements<UXD_SaveGameInterface>() && IXD_SaveGameInterface::NeedSave(Actor))
			{
				ActorRecorders.Add(UXD_SaveGameFunctionLibrary::SerializeObject(Actor, OuterLevel, ObjectReferenceCollection));
			}
		}

		bool Result = UGameplayStatics::SaveGameToSlot(this, UXD_SaveGameSystemBase::MakeLevelSlotName(OuterLevel), UXD_SaveGameSystemBase::Get(OuterLevel)->UserIndex);

		//LOG
		{
			FString SavedObjectsDesc;
			for (UObject* Object : ObjectReferenceCollection)
			{
				SavedObjectsDesc += UXD_DebugFunctionLibrary::GetDebugName(Object) + " | ";
			}

			if (Result)
			{
				SaveGameSystem_Display_Log("--------------------------------------保存关卡[%s]成功--------------------------------------", *UXD_LevelFunctionLibrary::GetLevelName(OuterLevel));

				SaveGameSystem_Display_Log("保存的Object列表：%s", *SavedObjectsDesc);
			}
			else
			{
				SaveGameSystem_Error_Log("\n保存关卡[%s]失败", *UXD_LevelFunctionLibrary::GetLevelName(OuterLevel));
			}
		}

		return Result;
	}
	return false;
}

void UXD_SaveLevelBase::LoadLevel(ULevel* OuterLevel, const bool SplitFrameLoadActors)
{
	if (OuterLevel)
	{
		struct FSplitFrameLoadActorHelper
		{
			UXD_SaveGameSystemBase::FLoadLevelGuard LoadLevelGuard;

			FSplitFrameLoadActorHelper(ULevel* Level, UXD_SaveLevelBase* SaveLevelBase, bool IsNoSplitFrameLoadUse = false)
				:Level(Level), LoadLevelGuard(Level), SaveLevelBase(SaveLevelBase), IsNoSplitFrameLoadUse(IsNoSplitFrameLoadUse)
			{
				if (IsNoSplitFrameLoadUse)
				{
					TimeLimit = TNumericLimits<double>::Max();
				}
				else
				{
					TimeLimit = UXD_SaveGameSystemBase::Get(Level)->SplitFrameLoadActorLimitSeconds;
				}

				SaveLevelBase->SetFlags(RF_StrongRefOnFrame);

				//记录下需要被读取的Actor
				for (AActor* Actor : Level->Actors)
				{
					if (Actor && Actor->Implements<UXD_SaveGameInterface>() && IXD_SaveGameInterface::NeedSave(Actor))
					{
						NeedBeLoadActors.Add(Actor);
					}
				}
			}

			~FSplitFrameLoadActorHelper()
			{
				if (SaveLevelBase.IsValid())
				{
					SaveLevelBase->ClearFlags(RF_StrongRefOnFrame);
				}
			}

			bool IsNoSplitFrameLoadUse;

			double TimeLimit;

			TWeakObjectPtr<ULevel> Level;
			TWeakObjectPtr<UXD_SaveLevelBase> SaveLevelBase;

			TSet<TWeakObjectPtr<AActor>> NeedBeLoadActors;

			TSet<TWeakObjectPtr<AActor>> RemainExistActors;

			TArray<UObject*> ObjectReferenceCollection;

			int32 LoadDataIdx = 0;
			int32 ExecutePostLoadIdx = 0;

			bool LoadActorsTick(float DeltaSceonds)
			{
				if (Level.IsValid() == false || SaveLevelBase.IsValid() == false)
				{
					if (!IsNoSplitFrameLoadUse)
					{
						delete this;
					}
					return false;
				}

				double StartTime = FPlatformTime::Seconds();

				TArray<FXD_SaveGameRecorder>& ActorRecorders = SaveLevelBase->ActorRecorders;
				for (;LoadDataIdx < ActorRecorders.Num(); ++LoadDataIdx)
				{
					FXD_SaveGameRecorder& XD_Recorder = ActorRecorders[LoadDataIdx];
					UXD_SaveGameFunctionLibrary::DeserializeObject(XD_Recorder, Level.Get(), ObjectReferenceCollection, SaveLevelBase->OldWorldOrigin);

					if (FPlatformTime::Seconds() - StartTime > TimeLimit)
					{
						return true;
					}
				}

				//分帧执行读取后的事件
				for (; ExecutePostLoadIdx < ObjectReferenceCollection.Num(); ++ExecutePostLoadIdx)
				{
					UObject* Object = ObjectReferenceCollection[ExecutePostLoadIdx];
					if (Object && Object->Implements<UXD_SaveGameInterface>())
					{
						IXD_SaveGameInterface::WhenPostLoad(Object);
					}

					if (FPlatformTime::Seconds() - StartTime > TimeLimit)
					{
						return true;
					}
				}

				//------------------当读取完毕

				//过滤出被读取的Actor
				for (UObject* Object : ObjectReferenceCollection)
				{
					if (AActor* Actor = Cast<AActor>(Object))
					{
						RemainExistActors.Add(Actor);
					}
				}

				TSet<TWeakObjectPtr<AActor>> NeedDestroyActors(NeedBeLoadActors.Difference(RemainExistActors));
				//LOG
				{
					FString LoadedObjectsDesc;
					for (UObject* Object : ObjectReferenceCollection)
					{
						LoadedObjectsDesc += UXD_DebugFunctionLibrary::GetDebugName(Object) + TEXT(" | ");
					}

					FString DestroyedActorsDesc;
					for (const TWeakObjectPtr<AActor>& Actor : NeedDestroyActors)
					{
						if (Actor.IsValid())
						{
							DestroyedActorsDesc += UXD_DebugFunctionLibrary::GetDebugName(Actor.Get()) + TEXT(" | ");
						}
					}

					SaveGameSystem_Display_Log("--------------------------------------读取关卡[%s]--------------------------------------", *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()));

					SaveGameSystem_Display_Log("读取的Object列表：%s", *LoadedObjectsDesc);

					SaveGameSystem_Display_Log("删除的Actor列表：%s", *DestroyedActorsDesc);
				}

				//删除没被读取的Actor
				for (const TWeakObjectPtr<AActor>& Actor : NeedDestroyActors)
				{
					if (Actor.IsValid())
					{
						Actor->Destroy();
					}
				}

				//最后删除自己防止内存泄露
				if (!IsNoSplitFrameLoadUse)
				{
					delete this;
				}
				return false;
			}
		};

		if (SplitFrameLoadActors)
		{
			FSplitFrameLoadActorHelper* SplitFrameLoadHelper = new FSplitFrameLoadActorHelper(OuterLevel, this);

			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(SplitFrameLoadHelper, &FSplitFrameLoadActorHelper::LoadActorsTick));
		}
		else
		{
			FSplitFrameLoadActorHelper SplitFrameLoadHelper(OuterLevel, this, true);
			SplitFrameLoadHelper.LoadActorsTick(0.f);
		}
	}

}



