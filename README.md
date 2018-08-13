# XD_SaveGameSystem
保存游戏全局信息插件

配置：
GameInstance去实现XD_SaveGame_GameInstanceInterface的GetSaveGameSystem函数

GameMode  BeginPlay时调用InitAutoSaveLoadSystem
          EndPlay时调用ShutDownAutoSaveLoadSystem
拓展：     
Actor只要使用XD_SaveGameInterface就会被自动保存，且属性的元信息中存在SaveGame的变量及Outer为该Actor的引用都会被保存，软引用也会记录下来。

使用：
InvokeSaveGame
InvokeLoadGame

详情查看UXD_SaveGameFunctionLibrary类
