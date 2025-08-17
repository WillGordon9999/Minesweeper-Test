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
	TSharedPtr<SButton> Button;
};

class FMinesweeperGameModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */	
	void PluginButtonClicked();

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
	void RevealAdjacentTilesNew(int32 Row, int32 Column);

	void RevealTileLoop(int32 Row, int32 Column)
	{
		RevealAdjacentTilesNew(Row, Column);

		while (!SearchList.IsEmpty())
		{
			FTileData Tile = SearchList.Pop(true);
			RevealAdjacentTilesNew(Tile.Row, Tile.Column);
		}
	}

private:
	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	uint32 Width = 8;
	uint32 Height = 8;
	uint32 MineCount = 8;
	TSharedPtr<class SGridPanel> Grid;
	TSharedPtr<class STextBlock> GridButtonText;
	//TArray<TArray<int32>> Slots; 
	TArray<TTuple<int32, int32>> Mines;

	TArray<FTileData> SlotsNew;
	TArray<int32> MineIndices;
	FLinearColor TileColor;
	FLinearColor PressedColor;	
	bool bFirstClick = true; //If player's first click is on a mine, move that mine elsewhere
	TSharedPtr<class FUICommandList> PluginCommands;

	TArray<TTuple<int32, int32>> Adjacents;
	TArray<FTileData> SearchList;	
};
