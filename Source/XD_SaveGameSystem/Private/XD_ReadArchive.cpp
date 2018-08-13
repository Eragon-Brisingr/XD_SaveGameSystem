// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ReadArchive.h"
#include "XD_DebugFunctionLibrary.h"
#include "XD_LevelFunctionLibrary.h"
#include "XD_SaveGameInterface.h"
#include "XD_SaveGameSystemUtility.h"


FArchive& FXD_ReadArchive::operator<<(class UObject*& Obj)
{
	struct FXD_ReadArchiveHelper
	{
		static void DeserilizeActorSpecialInfo(FXD_ReadArchive& Ar, AActor* Actor)
		{
			uint8 ComponentNumber;
			Ar << ComponentNumber;
			for (int i = 0; i < ComponentNumber; ++i)
			{
				FString ComponentClassPath;
				FString ComponentName;
				Ar << ComponentClassPath << ComponentName;
				if (UClass* ComponentClass = ConstructorHelpersInternal::FindOrLoadObject<UClass>(ComponentClassPath))
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

					if (Component->Implements<UXD_SaveGameInterface>())
					{
						IXD_SaveGameInterface::Execute_WhenLoad(Component);
					}
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
			FString AssetPath;
			*this << AssetPath;
			UObject* Asset = ConstructorHelpersInternal::FindOrLoadObject<UObject>(AssetPath);
			ObjectReferenceCollection.Add(Asset);

			if (Asset == nullptr)
			{
				SaveGameSystem_Error_Log("读取资源[%s]失败", *AssetPath);
			}
		}
		break;
		case EObjectArchiveType::InPackageObject:
		{
			FString ClassPath;
			*this << ClassPath;
			FString ObjectPath;
			*this << ObjectPath;

			UObject* findObject = ConstructorHelpersInternal::FindOrLoadObject<UObject>(ObjectPath);
			ObjectReferenceCollection.Add(findObject);

			if (findObject == nullptr)
			{
				SaveGameSystem_Error_Log("读取Object[%s]失败，无法在[%s]中找到", *ObjectPath, *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()));

				findObject = NewObject<UObject>(Level.Get(), ConstructorHelpersInternal::FindOrLoadClass(ClassPath, UObject::StaticClass()), *ObjectPath);
				findObject->MarkPendingKill();
			}

			findObject->Serialize(*this);

			//未知原因被加到了根集上
			findObject->RemoveFromRoot();

		}
		break;
		case EObjectArchiveType::DynamicObject:
		{
			FString ClassPath;
			*this << ClassPath;
			FString ObjectName;
			*this << ObjectName;

			UClass* ObjectClass = ConstructorHelpersInternal::FindOrLoadClass(ClassPath, UObject::StaticClass());
			UObject* Object = NewObject<UObject>(GetTransientPackage(), ObjectClass);
			ObjectReferenceCollection.Add(Object);

			int32 OuterIndex;
			*this << OuterIndex;
			UObject* Outer = nullptr;
			if (OuterIndex == INDEX_NONE)
			{
				Outer = GetTransientPackage();
			}
			else
			{
				if (OuterIndex >= ObjectReferenceCollection.Num())
				{
					*this << Outer;
				}
				else
				{
					Outer = ObjectReferenceCollection[OuterIndex];
				}
			}

			//假如存在同名Object，销毁
			if (UObject* ExistingObject = StaticFindObject(ObjectClass, Outer, *ObjectName, true))
			{
				//Dirty hack，Rename会找到这个Object，标记为RF_NewerVersionExists才不会找到
				ExistingObject->SetFlags(RF_NewerVersionExists);
				ExistingObject->MarkPendingKill();
			}

			Object->Rename(*ObjectName, Outer);

			Object->Serialize(*this);

		}
		break;
		case EObjectArchiveType::InPackageActor:
		{
			FString ClassPath;
			*this << ClassPath;
			FString ActorPath;
			*this << ActorPath;

			AActor* findActor = ConstructorHelpersInternal::FindOrLoadObject<AActor>(ActorPath);
			ObjectReferenceCollection.Add(findActor);

			FTransform ActorTransForm;
			*this << ActorTransForm;

			ActorTransForm.SetLocation(UXD_LevelFunctionLibrary::GetFixedWorldLocation(Level.Get(), OldWorldOrigin, ActorTransForm.GetLocation()));

			if (findActor == nullptr)
			{
				SaveGameSystem_Error_Log("读取Actor[%s]失败，无法在[%s]中找到", *ActorPath, *UXD_LevelFunctionLibrary::GetLevelName(Level.Get()));

				FActorSpawnParameters ActorSpawnParameters;
				int32 NameStartIndex;
				ActorPath.FindLastChar(TEXT('.'), NameStartIndex);
				ActorSpawnParameters.Name = *ActorPath.Right(ActorPath.Len() - NameStartIndex - 1);
				ActorSpawnParameters.OverrideLevel = Level.Get();
				ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				findActor = Level->GetWorld()->SpawnActor<AActor>(ConstructorHelpersInternal::FindOrLoadClass(ClassPath, UObject::StaticClass()));

				findActor->Destroy();
			}

			findActor->SetActorTransform(ActorTransForm, false, nullptr, ETeleportType::TeleportPhysics);
			findActor->Serialize(*this);

			FXD_ReadArchiveHelper::DeserilizeActorSpecialInfo(*this, findActor);

			//未知原因被加到了根集上
			findActor->RemoveFromRoot();
		}
		break;
		case EObjectArchiveType::DynamicActor:
		{
			FString ClassPath;
			*this << ClassPath;
			FString ActorName;
			*this << ActorName;
			FTransform ActorTransForm;
			*this << ActorTransForm;

			ActorTransForm.SetLocation(UXD_LevelFunctionLibrary::GetFixedWorldLocation(Level.Get(), OldWorldOrigin, ActorTransForm.GetLocation()));

			UClass* ActorClass = ConstructorHelpersInternal::FindOrLoadClass(ClassPath, UObject::StaticClass());

			AActor* Actor = Cast<AActor>(Obj);
			if (Actor == nullptr)
			{
				Actor = FindObject<AActor>(Level.Get(), *ActorName);
				if (Actor)
				{
					SaveGameSystem_Warning_LOG("应Spawn的Actor%s存在于关卡%s中，请检查原因", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(Level.Get()));
				}
			}
			if (Actor)
			{
				if (Actor->IsA(ActorClass))
				{
					Actor->SetActorTransform(ActorTransForm);
				}
				else
				{
					SaveGameSystem_Warning_LOG("提供反序列化的Actor%s类型[%s]与存档[%s]不匹配", *UXD_DebugFunctionLibrary::GetDebugName(Actor->GetClass()), *UXD_DebugFunctionLibrary::GetDebugName(ActorClass));
				}
			}
			else
			{
				FActorSpawnParameters ActorSpawnParameters;
				ActorSpawnParameters.Name = *ActorName;
				ActorSpawnParameters.OverrideLevel = Level.Get();
				ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

// 				UARPG_SaveGameSystem* SaveGameSystem = UARPG_SaveGameFunctionLibrary::GetSaveGameSystem(Level.Get());
// 				SaveGameSystem->StartSpawnActorWithoutInit();
// 				{
// 					Actor = Level->GetWorld()->SpawnActor<AActor>(ActorClass, ActorTransForm, ActorSpawnParameters);
// 				}
// 				SaveGameSystem->EndSpawnActorWithoutInit();
			}
			int32 AddIndex = ObjectReferenceCollection.Add(Actor);

			int32 OwnerIndex;
			*this << OwnerIndex;
			if (OwnerIndex != INDEX_NONE)
			{
				if (OwnerIndex >= ObjectReferenceCollection.Num())
				{
					AActor* Owner;
					*this << Owner;
					Actor->SetOwner(Owner);
				}
				else
				{
					Actor->SetOwner(Cast<AActor>(ObjectReferenceCollection[OwnerIndex]));
				}

				if (Actor->GetOwner() == nullptr || Actor->GetOwner()->IsPendingKillPending())
				{
					//假如Owner没读取成功则销毁该Actor
					SaveGameSystem_Warning_LOG("应Spawn的Actor%s在关卡%s中无法找到Owner，一般原因是Owner被删除", *UXD_DebugFunctionLibrary::GetDebugName(Actor), *UXD_DebugFunctionLibrary::GetDebugName(Level.Get()));
					Actor->Destroy();
					ObjectReferenceCollection[AddIndex] = nullptr;
				}
			}

			Actor->Serialize(*this);

			FXD_ReadArchiveHelper::DeserilizeActorSpecialInfo(*this, Actor);

		}
		break;
		case EObjectArchiveType::InPackageComponent:
		{

		}
		break;
		case EObjectArchiveType::DynamicComponent:
		{

		}
		break;
		}
	}
	Obj = ObjectReferenceCollection[ObjectIndex];

	//最后呼叫WhenLoad
	if (Obj && Obj->Implements<UXD_SaveGameInterface>())
	{
		IXD_SaveGameInterface::Execute_WhenLoad(Obj);
	}

	return *this;

}
