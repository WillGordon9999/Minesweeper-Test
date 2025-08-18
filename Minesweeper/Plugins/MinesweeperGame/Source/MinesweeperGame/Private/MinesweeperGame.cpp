// Copyright Epic Games, Inc. All Rights Reserved.

#include "MinesweeperGame.h"
#include "MinesweeperGameStyle.h"
#include "MinesweeperGameCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STileView.h"
#include "ToolMenus.h"

static const FName MinesweeperGameTabName("MinesweeperGame");

#define LOCTEXT_NAMESPACE "FMinesweeperGameModule"

void FMinesweeperGameModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FMinesweeperGameStyle::Initialize();
	FMinesweeperGameStyle::ReloadTextures();

	FMinesweeperGameCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FMinesweeperGameCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FMinesweeperGameModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMinesweeperGameModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MinesweeperGameTabName, FOnSpawnTab::CreateRaw(this, &FMinesweeperGameModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FMinesweeperGameTabTitle", "MinesweeperGame"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FMinesweeperGameModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FMinesweeperGameStyle::Shutdown();

	FMinesweeperGameCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MinesweeperGameTabName);
}

void FMinesweeperGameModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(MinesweeperGameTabName);
}

int32 FMinesweeperGameModule::GetTileIndex(int32 Row, int32 Column)
{
	return (Row * Width) + Column;
}

TTuple<int32, int32> FMinesweeperGameModule::GetRowAndColumn(int32 Index)
{
	if (Tiles.IsValidIndex(Index))
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

FTileData* FMinesweeperGameModule::GetTile(int32 Row, int32 Column)
{
	int32 index = GetTileIndex(Row, Column);

	if (Tiles.IsValidIndex(index))
		return &Tiles[index];
	else
		return nullptr;
}

void FMinesweeperGameModule::CalculateAdjacentTiles(int32 Row, int32 Column, int32 Value)
{
	for (int32 i = 0; i < Adjacents.Num(); i++)
	{
		int32 adjRow = Row + Adjacents[i].Key;
		int32 adjCol = Column + Adjacents[i].Value;
		
		if (adjRow < 0 || adjRow > (int32)Height - 1)
			continue;
		
		if (adjCol < 0 || adjCol > (int32)Width - 1)
			continue;

		if (FTileData* Tile = GetTile(adjRow, adjCol))
		{
			if (Value >= 0)
			{
				if (Tile->Type >= 0)
				{
					Tile->Type += Value;
				}
			}

			//Primarily for handling the first click mine swap to properly update surrounding tiles
			else 
			{
				if (Tile->Type > 0)
				{
					Tile->Type += Value;
				}
			}
		}
	}
}

void FMinesweeperGameModule::RevealAdjacentTiles(int32 Row, int32 Column)
{
	for (int32 i = 0; i < Adjacents.Num(); i++)
	{
		int32 adjRow = Row + Adjacents[i].Key;
		int32 adjCol = Column + Adjacents[i].Value;
		 
		if (adjRow < 0 || adjRow > (int32)Height - 1) 
			continue;
		
		if (adjCol < 0 || adjCol > (int32)Width - 1)
			continue;

		int32 newIndex = GetTileIndex(adjRow, adjCol);

		if (!Tiles.IsValidIndex(newIndex))
			continue;

		if (Tiles[newIndex].Type < 0)
			continue;

		Tiles[newIndex].Button.Get()->SetColorAndOpacity(PressedColor);
		Tiles[newIndex].Button.Get()->SetBorderBackgroundColor(PressedColor);

		if (Tiles[newIndex].bIsChecked)
			continue;

		Tiles[newIndex].bIsChecked = true;

		if (Tiles[newIndex].Type > 0)
		{
			Tiles[newIndex].Text.Get()->SetText(FText::FromString(FString::FromInt(Tiles[newIndex].Type)));
			continue;
		}

		if (Tiles[newIndex].Type == 0)
		{
			SearchList.Add(Tiles[newIndex]);
		}
	}
}

void FMinesweeperGameModule::RevealTileLoop(int32 Row, int32 Column)
{
	RevealAdjacentTiles(Row, Column);

	while (!SearchList.IsEmpty())
	{
		FTileData Tile = SearchList.Pop(true);
		RevealAdjacentTiles(Tile.Row, Tile.Column);
	}
}

void FMinesweeperGameModule::GenerateGrid()
{
	uint32 currentMines = 0;	
	Grid.Get()->ClearChildren();
	GridButtonText.Get()->SetText(FText::FromString(TEXT("Generate New Grid")));
	bFirstClick = true;
	
	Tiles.Empty(Width * Height);
	ShuffleIndices.Empty(Width * Height);
	MineIndices.Empty(MineCount);

	if (Width == 0)
	{
		Width = PrevWidth;
		WidthText.Get()->SetText(FText::FromString(FString::FromInt(Width)));
	}

	if (Height == 0)
	{
		Height = PrevHeight;
		HeightText.Get()->SetText(FText::FromString(FString::FromInt(Height)));
	}

	if (MineCount > (Width * Height) - 1 || MineCount == 0)
	{
		MineCount = PrevMineCount;
		MineCountText.Get()->SetText(FText::FromString(FString::FromInt(MineCount)));
	}
		
	for (uint32 row = 0; row < Height; row++)
	{		
		for (uint32 col = 0; col < Width; col++)
		{			
			int32 newNum = 0;
			ShuffleIndices.Add(GetTileIndex(row, col));
						
			int32 index = Tiles.Add(FTileData());
			Tiles[index].Index = index;
			Tiles[index].Row = row;
			Tiles[index].Column = col;
			Tiles[index].Type = newNum;

			TSharedPtr<STextBlock> text;						
			TSharedPtr<SButton> button;
			auto slot = Grid.Get()->AddSlot(col, row);			
			slot
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)			
			[				
				SAssignNew(button, SButton)
				.ButtonColorAndOpacity(TileColor)								
				.Text(FText::FromString(TEXT(" ")))	
				[
					SAssignNew(text, STextBlock)					
					.Text(FText::FromString(" "))					
				]
				.OnClicked_Lambda
				(
					[this, row, col, text, button, index]() -> FReply
					{
						if (Tiles[index].bIsChecked)
							return FReply::Handled();

						//Save the player from instantly losing if they first click on a mine
						if (bFirstClick)
						{
							ShuffleIndices.RemoveAt(index);

							for (int32 i = ShuffleIndices.Num() - 1; i > 0; i--)
							{
								int32 rand = FMath::RandRange(0, i);
								ShuffleIndices.SwapMemory(i, rand);
							}

							for (uint32 i = 0; i < MineCount; i++)
							{
								int32 newIndex = ShuffleIndices[i];
								Tiles[newIndex].Type = -1;
								MineIndices.Add(newIndex);
							}

							for (int32 i = 0; i < MineIndices.Num(); i++)
							{
								TTuple<int32, int32> minePos = GetRowAndColumn(MineIndices[i]);

								int32 rowCenter = minePos.Key;
								int32 colCenter = minePos.Value;

								CalculateAdjacentTiles(rowCenter, colCenter, 1);
							}

							bFirstClick = false;
						}


						if (Tiles[index].Type >= 0)
						{
							if (Tiles[index].Type > 0)
							{
								text.Get()->SetText(FText::FromString(FString::FromInt(Tiles[index].Type)));
							}

							Tiles[index].Button.Get()->SetColorAndOpacity(PressedColor);
							Tiles[index].Button.Get()->SetBorderBackgroundColor(PressedColor);

							//bFirstClick = false;
							Tiles[index].bIsChecked = true;
							RevealTileLoop(row, col);							
						}

						else
						{
							//Save the player from instantly losing if they first click on a mine
							//if (bFirstClick)
							//{
							//	Tiles[index].bIsChecked = true;
							//	Tiles[index].Button.Get()->SetColorAndOpacity(PressedColor);
							//	Tiles[index].Button.Get()->SetBorderBackgroundColor(PressedColor);								
							//
							//	//Move this mine somewhere else
							//	int32 newIndex = index + Width;
							//	newIndex = FMath::Clamp(newIndex, 0, (Width * Height) - 1);
							//	
							//	MineIndices.Remove(index);
							//	CalculateAdjacentTiles(row, col, -1); //Remove 1 from removed position
							//	MineIndices.Add(newIndex);
							//
							//	TTuple<int32, int32> newMine = GetRowAndColumn(newIndex);
							//	CalculateAdjacentTiles(newMine.Key, newMine.Value, 1);
							//	bFirstClick = false;
							//
							//	RevealTileLoop(row, col);
							//}

							//else
							{
								GridButtonText.Get()->SetText(FText::FromString("You Lose. Play Again?"));

								for (int32 i = 0; i < MineIndices.Num(); i++)
								{
									if (Tiles.IsValidIndex(MineIndices[i]))
										Tiles[MineIndices[i]].Text.Get()->SetText(FText::FromString("X"));
								}
							}							
						}

						return FReply::Handled();
					}
				)				
			];

			Tiles[index].Text = text;
			Tiles[index].Button = button;
		}
	}

	PrevWidth = Width;
	PrevHeight = Height;
	PrevMineCount = MineCount;

	//for (int32 i = ShuffleIndices.Num() - 1; i > 0; i--)
	//{
	//	int32 rand = FMath::RandRange(0, i);
	//	ShuffleIndices.SwapMemory(i, rand);
	//}
	//
	//for (uint32 i = 0; i < MineCount; i++)
	//{
	//	int32 index = ShuffleIndices[i];
	//	Tiles[index].Type = -1;
	//	MineIndices.Add(index);
	//}

	//for (int32 i = 0; i < MineIndices.Num(); i++)
	//{
	//	TTuple<int32, int32> minePos = GetRowAndColumn(MineIndices[i]);
	//	
	//	int32 rowCenter = minePos.Key;
	//	int32 colCenter = minePos.Value;
	//
	//	CalculateAdjacentTiles(rowCenter, colCenter, 1);
	//}
}

TSharedRef<SDockTab> FMinesweeperGameModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{		
	Tiles = TArray<FTileData>();
	SearchList = TArray<FTileData>();
	ShuffleIndices = TArray<int32>();

	TileColor = FLinearColor::Green;
	PressedColor = FLinearColor::Yellow;
	
	Adjacents =
	{
		{ -1, -1 },	//TOP LEFT
		{ -1, 0 },	//TOP MIDDLE
		{ -1, 1 },	//TOP RIGHT
		{ 0, -1 },	//MIDDLE LEFT
		{ 0, 1 },	//MIDDLE RIGHT
		{ 1, -1 },	//BOTTOM LEFT
		{ 1, 0 },	//BOTTOM MIDDLE
		{ 1, 1 }	//BOTTOM RIGHT
	};

	auto Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			//Main List
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.MaxHeight(30.0f)			
			[
				//Width, Height, Mine Count Entry
				SNew(SHorizontalBox) 
				+SHorizontalBox::Slot()
				.Padding(5, 0)
				.AutoWidth()				
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Width")))
				]

				+SHorizontalBox::Slot()
				.Padding(0, 0)				
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(WidthText, SEditableTextBox)
					.Text(FText::FromString(FString::FromInt(Width)))
					.OnTextCommitted_Lambda
					(
						[&](const FText& Text, ETextCommit::Type)
						{
							Width = FCString::Atoi(*Text.ToString());
						}
					)
				]

				+SHorizontalBox::Slot()
				.Padding(5, 0)				
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Height")))
				]

				+SHorizontalBox::Slot()
				.Padding(0, 0)				
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(HeightText, SEditableTextBox)
					.Text(FText::FromString(FString::FromInt(Height)))
					.OnTextCommitted_Lambda
					(
						[&](const FText& Text, ETextCommit::Type)
						{
							Height = FCString::Atoi(*Text.ToString());
						}
					)
				]

				+SHorizontalBox::Slot()
				.Padding(5, 0)								
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Number of Mines")))
				]

				+SHorizontalBox::Slot()
				.Padding(0, 0)				
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(MineCountText, SEditableTextBox)
					.Text(FText::FromString(FString::FromInt(MineCount)))
					.OnTextCommitted_Lambda
					(
						[&](const FText& Text, ETextCommit::Type)
						{
							MineCount = FCString::Atoi(*Text.ToString());
						}
					)
				]
			]

			//=============== END OF NUMBER ROW =================

			+SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.MaxHeight(20.0f)
			.Padding(0.0, 5.0, 0.0, 0.0)									
			[
				SNew(SButton)
				.OnClicked_Lambda
				(
					[&]() -> FReply
					{
						GenerateGrid();
						return FReply::Handled();
					}
				)					
				[
					SAssignNew(GridButtonText, STextBlock)
					.Text(FText::FromString(TEXT("Generate New Grid")))					
				]
			]

			//Actual Grid
			+SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoHeight()
			.Padding(0.0, 5.0, 0.0, 0.0)
			[
				SAssignNew(Grid, SGridPanel)				
			]
		];

	GenerateGrid();
	return Tab;
}

void FMinesweeperGameModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FMinesweeperGameCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FMinesweeperGameCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMinesweeperGameModule, MinesweeperGame)