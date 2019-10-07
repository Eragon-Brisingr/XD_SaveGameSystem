// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ReadArchive.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_LevelFunctionLibrary.h"
#include "XD_SaveGameInterface.h"
#include "XD_SaveGameSystemUtility.h"
#include "XD_SaveGameSystemBase.h"

template<typename T>
FString GetSubObjectName(const TSoftObjectPtr<T>& SoftObjectPtr)
{
	FString SubObjectName;
	SoftObjectPtr.GetUniqueID().GetSubPathString().Split(TEXT("."), nullptr, &SubObjectName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	return SubObjectName;
}

FXD_ReadArchive::FXD_ReadArchive(FArchive& InInnerArchive, class ULevel* Level, TArray<UObject*>& ObjectReferenceCollection, const FIntVector& OldWorldOrigin) :FXD_ProxyArchiveBase(InInnerArchive), Level(Level), ObjectReferenceCollection(ObjectReferenceCollection), OldWorldOrigin(OldWorldOrigin)
{
	ArIsSaveGame = true;

	SetIsLoading(true);

	SetIsPersistent(false);

	int32 CurVersion;
	*this << CurVersion;
	SetCustomVersion(FXD_SaveGameVersion::Guid, CurVersion, FXD_SaveGameVersion::FriendlyName);
}

FArchive& FXD_ReadArchive::operator<<(class UObject*& Obj)
{
	struct FXD_ReadArchiveHelper
	{
		static void DeserilizeActorSpecialInfo(FXD_ReadArchive& Ar, AActor* Actor)
		{
			Actor->Serialize(Ar);
			IXD_SaveGameInterface::WhenGameSerialize(Actor, Ar);

			uint8 ComponentNumber;
			Ar << ComponentNumber;
			for (int i = 0; i < ComponentNumber; ++i)
			{
				FXD_DynamicSaveData DynamicSaveData;
				FXD_DynamicSaveData::StaticStruct()->SerializeBin(Ar, &DynamicSaveData);
				FString& ComponentName = DynamicSaveData.Name;

				if (UClass* ComponentClass = DynamicSaveData.ClassPath.LoadSynchronous())
				{
					UActorComponent* Component = nullptr;

					Component = FindObject<UActorComponent>(Actor, *ComponentName);
					if (Component == nullptr)
					{
						Component = NewObject<UActorComponent>(Actor, ComponentClass, *ComponentName);
						Actor->AddOwnedComponent(Component);
						Component->RegisterComponent();
					}
					Ar.ObjectReferenceCollection.Add(Component);
					Component->Serialize(Ar);
					IXD_SaveGameInterface::WhenGameSerialize(Component, Ar);
				}
			}
		}
	};

	int32 ObjectIndex = 0;
	*this << ObjectIndex;
	if (ObjectIndex >= ObjectReferenceCollection.Num())
	{
		EObjectArchiveType ObjectArchiveType;
		*this << ObjectArchiveType;
		switch (ObjectArchiveType)
		{
		case EObjectArchiveType::NullObject:
			ObjectReferenceCollection.Add(nullptr);
			break;
		case EObjectArchiveType::Asset:
		{
			FXD_AssetSaveData AssetSaveData;
			FXD_AssetSaveData::StaticStruct()->SerializeBin(*this, &AssetSaveData);
			UObject* Asset = AssetSaveData.Path.LoadSynchronous();
			ObjectReferenceCollection.Add(Asset);

			if (Asset == nullptr)
			{
				SaveGameSystem_Error_Log("读取资源[%s]失败，路径为[%s]", *GetSubObjectName(AssetSaveData.Path), *AssetSaveData.Path.GetLongPackageName());
			}
			break;
		}
		case EObjectArchiveType::InPackageObject:
		{
			FXD_InPackageSaveData InPackageSaveData;
			FXD_InPackageSaveData::StaticStruct()->SerializeBin(*this, &InPackageSaveData);

			UObject* findObject = InPackageSaveData.Path.LoadSynchronous();
			ObjectReferenceCollection.Add(findObject);

			if (findObject == nullptr)
			{
				SaveGameSystem_Error_Log("读取Object[%s]失败，无法在[%s]中找到，原路径为[%s]", *GetSubObjectName(InPackageSaveData.Path), *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()), *InPackageSaveData.Path.GetLongPackageName());

				findObject = NewObject<UObject>(Level.Get(), InPackageSaveData.ClassPath.LoadSynchronous(), *GetSubObjectName(InPackageSaveData.Path));
				findObject->ConditionalBeginDestroy();
				findObject->MarkPendingKill();
			}

			findObject->Serialize(*this);
			IXD_SaveGameInterface::WhenGameSerialize(findObject, *this);
			break;
		}
		case EObjectArchiveType::DynamicObject:
		{
			FXD_DynamicSaveData DynamicSaveData;
			FXD_DynamicSaveData::StaticStruct()->SerializeBin(*this, &DynamicSaveData);
			FString& ObjectName = DynamicSaveData.Name;

			UClass* ObjectClass = DynamicSaveData.ClassPath.LoadSynchronous();

			int32 OuterIndex;
			*this << OuterIndex;
			UObject* Outer = nullptr;
			if (OuterIndex == INDEX_NONE)
			{
				Outer = GetTransientPackage();
			}
			else
			{
				//Index和当前数量相等表明后一个即为Outer
				if (OuterIndex == ObjectReferenceCollection.Num())
				{
					*this << Outer;
				}
				else
				{
					Outer = ObjectReferenceCollection[OuterIndex];
				}
			}

			UObject* Object = StaticFindObject(ObjectClass, Outer, *ObjectName, true);
			if (Object)
			{
				SaveGameSystem_Warning_Log("因动态生成的Object%s已存在于%s中，请检查原因", *UXD_DebugFunctionLibrary::GetDebugName(Object), *UXD_DebugFunctionLibrary::GetDebugName(Outer));
			}
			else
			{
				Object = NewObject<UObject>(Outer, ObjectClass, *ObjectName);
			}
			ObjectReferenceCollection.Add(Object);

			Object->Serialize(*this);
			IXD_SaveGameInterface::WhenGameSerialize(Object, *this);
			break;
		}
		case EObjectArchiveType::InPackageActor:
		{
			FXD_InPackageSaveData InPackageSaveData;
			FXD_InPackageSaveData::StaticStruct()->SerializeBin(*this, &InPackageSaveData);
			TSoftClassPtr<UObject>& ClassPath = InPackageSaveData.ClassPath;
			TSoftObjectPtr<UObject>& ActorPath = InPackageSaveData.Path;

			FXD_ActorExtraSaveData ActorExtraSaveData;
			FXD_ActorExtraSaveData::StaticStruct()->SerializeBin(*this, &ActorExtraSaveData);

			AActor* findActor = (AActor*)ActorPath.Get();
			ObjectReferenceCollection.Add(findActor);

			bool bIsInvalidActor = false;
			if (findActor == nullptr)
			{
				bIsInvalidActor = true;

				SaveGameSystem_Error_Log("读取Actor[%s]失败，无法在[%s]中找到，原路径为[%s]", *GetSubObjectName(ActorPath), *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()), *ActorPath.GetLongPackageName());

				FActorSpawnParameters ActorSpawnParameters;
				ActorSpawnParameters.Name = *GetSubObjectName(ActorPath);
				ActorSpawnParameters.OverrideLevel = Level.Get();
				ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(Level.Get());
				SaveGameSystem->StartSpawnActorWithoutInit();
				{
					findActor = Level->GetWorld()->SpawnActor<AActor>(ClassPath.LoadSynchronous());
				}
				SaveGameSystem->EndSpawnActorWithoutInit();
			}

			ActorExtraSaveData.LoadData(findActor, Level.Get(), OldWorldOrigin);

			FXD_ReadArchiveHelper::DeserilizeActorSpecialInfo(*this, findActor);

			if (bIsInvalidActor)
			{
				findActor->Destroy();
			}
			break;
		}
		case EObjectArchiveType::DynamicActor:
		{
			FXD_DynamicSaveData DynamicSaveData;
			FXD_DynamicSaveData::StaticStruct()->SerializeBin(*this, &DynamicSaveData);
			const FString& ActorName = DynamicSaveData.Name;

			FXD_ActorExtraSaveData ActorExtraSaveData;
			FXD_ActorExtraSaveData::StaticStruct()->SerializeBin(*this, &ActorExtraSaveData);

			UClass* ActorClass = DynamicSaveData.ClassPath.LoadSynchronous();

			AActor* Actor = Cast<AActor>(Obj);
			if (Actor == nullptr)
			{
				Actor = FindObject<AActor>(Level.Get(), *ActorName);
				if (Actor)
				{
					SaveGameSystem_Warning_Log("应Spawn的Actor%s存在于关卡%s中，请检查原因", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(Level.Get()));
				}
			}
			if (Actor)
			{
				if (!Actor->IsA(ActorClass))
				{
					SaveGameSystem_Error_Log("提供反序列化的Actor%s类型%s与存档%s不匹配", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(Actor->GetClass()), *UXD_DebugFunctionLibrary::GetDebugName(ActorClass));
				}
			}
			else
			{
				FActorSpawnParameters ActorSpawnParameters;
				ActorSpawnParameters.Name = *ActorName;
				ActorSpawnParameters.OverrideLevel = Level.Get();
				ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				UXD_SaveGameSystemBase* SaveGameSystem = UXD_SaveGameSystemBase::Get(Level.Get());
				SaveGameSystem->StartSpawnActorWithoutInit();
				{
					Actor = Level->GetWorld()->SpawnActor<AActor>(ActorClass, ActorSpawnParameters);
#if WITH_EDITOR
					Actor->SetActorLabel(ActorName);
#endif
				}
				SaveGameSystem->EndSpawnActorWithoutInit();
			}
			int32 AddIndex = ObjectReferenceCollection.Add(Actor);

			int32 OwnerIndex;
			*this << OwnerIndex;
			bool bHasOwner = OwnerIndex != INDEX_NONE;
			if (bHasOwner)
			{
				//Index和当前数量相等表明后一个即为Owner
				if (OwnerIndex == ObjectReferenceCollection.Num())
				{
					AActor* Owner;
					*this << Owner;
					Actor->SetOwner(Owner);
				}
				else
				{
					Actor->SetOwner(Cast<AActor>(ObjectReferenceCollection[OwnerIndex]));
				}
			}

			ActorExtraSaveData.LoadData(Actor, Level.Get(), OldWorldOrigin);
			FXD_ReadArchiveHelper::DeserilizeActorSpecialInfo(*this, Actor);

			//假如Owner没读取成功则销毁该Actor
			if (bHasOwner)
			{
				if (Actor->GetOwner() == nullptr || Actor->GetOwner()->IsPendingKillPending())
				{
					SaveGameSystem_Warning_Log("应Spawn的Actor%s在关卡%s中无法找到Owner，一般原因是Owner被删除", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(Level.Get()));
					Actor->Destroy();
					ObjectReferenceCollection[AddIndex] = nullptr;
				}
			}
			break;
		}
		}
	}
	Obj = ObjectReferenceCollection[ObjectIndex];

	return *this;

}
