// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

struct FTileData
{
	int32 Index;
	int32 Row;
	int32 Column;
	int8 Type; //Where < 0 is a mine, 0 is empty, > 0 is number of adjacent mines
	bool bIsChecked;
	TSharedPtr<STextBlock> Text;
};

class FMinesweeperGameModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */	
	void PluginButtonClicked();
	
private:

	int32 GetTileIndex(int32 Row, int32 Column)
	{
		return (Row * Width) + Column;
	}

	TTuple<int32, int32> GetRowAndColumn(int32 Index)
	{
		if (SlotsNew.IsValidIndex(Index))
		{
			int32 row = Index / Width;
			int32 column = Index % Width;
			return TTuple<int32, int32>(row, column);
		}

		else
		{
			return TTuple<int32, int32>(-1, -1);
		}
	}

	FTileData* GetTile(int32 Row, int32 Column)
	{				
		int32 index = GetTileIndex(Row, Column);

		if (SlotsNew.IsValidIndex(index))		
			return &SlotsNew[index];		
		else		
			return nullptr;		
	}

	void GenerateGrid();
	void CalculateAdjacentTiles(int32 Row, int32 Column, int32 value);
	void RevealAdjacentTiles(int32 Row, int32 Column);
	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	uint32 Width = 4;
	uint32 Height = 4;
	uint32 MineCount = 4;
	TSharedPtr<class SGridPanel> Grid;
	TSharedPtr<class STextBlock> GridButtonText;
	TArray<TArray<int32>> Slots; 
	TArray<TTuple<int32, int32>> Mines;

	TArray<FTileData> SlotsNew;
	TArray<int32> MineIndices;
	FColor TileColor;
	bool bFirstClick = true; //If player's first click is on a mine, move that mine elsewhere
	TSharedPtr<class FUICommandList> PluginCommands;

	TArray<TTuple<int32, int32>> adjacents; 
};
