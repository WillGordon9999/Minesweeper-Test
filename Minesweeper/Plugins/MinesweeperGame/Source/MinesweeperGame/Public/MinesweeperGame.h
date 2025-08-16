// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

struct FTileData
{
	uint32 Index;
	uint32 MineCount;
	TSharedPtr<STextBlock> Text;
};

class FMinesweeperGameModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void GenerateGrid();
	void PluginButtonClicked();
	
private:

	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	uint32 Width = 4;
	uint32 Height = 4;
	uint32 MineCount = 4;
	TSharedPtr<class SGridPanel> Grid;
	TSharedPtr<class STextBlock> GridButtonText;
	TArray<TArray<int32>> Slots; //Where < 0 is a mine, 0 is empty, > 0 is number of adjacent mines
	TArray<TTuple<int32, int32>> Mines;
	TSharedPtr<class FUICommandList> PluginCommands;
};
