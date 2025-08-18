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

	int32 GetTileIndex(int32 Row, int32 Column);
	TTuple<int32, int32> GetRowAndColumn(int32 Index);	
	FTileData* GetTile(int32 Row, int32 Column);
	
	void CalculateAdjacentTiles(int32 Row, int32 Column, int32 value);
	void RevealAdjacentTiles(int32 Row, int32 Column);
	void RevealTileLoop(int32 Row, int32 Column);

	void GenerateGrid();

private:
	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	uint32 Width = 8;
	uint32 Height = 8;
	uint32 MineCount = 8;
	
	uint32 PrevWidth;
	uint32 PrevHeight;
	uint32 PrevMineCount;

	TSharedPtr<class SGridPanel> Grid;
	TSharedPtr<class STextBlock> GridButtonText;	 	
	TSharedPtr<class SEditableTextBox> WidthText;	 	
	TSharedPtr<class SEditableTextBox> HeightText;	 	
	TSharedPtr<class SEditableTextBox> MineCountText;	 	
	TSharedPtr<class FUICommandList> PluginCommands;

	TArray<FTileData> Tiles;
	TArray<int32> MineIndices;
	TArray<int32> ShuffleIndices;
	FLinearColor TileColor;
	FLinearColor PressedColor;	
	bool bFirstClick = true; //If player's first click is on a mine, move that mine elsewhere
	TArray<TTuple<int32, int32>> Adjacents;
	TArray<FTileData> SearchList;	
};
