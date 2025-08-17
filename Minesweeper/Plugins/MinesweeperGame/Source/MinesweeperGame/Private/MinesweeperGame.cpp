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

void FMinesweeperGameModule::GenerateGrid()
{
	uint32 currentMines = 0;	
	Grid.Get()->ClearChildren();
	GridButtonText.Get()->SetText(FText::FromString(TEXT("Generate New Grid")));
	bFirstClick = true;

	Mines.Empty();
	//Slots.SetNum(Width);
	//for (uint32 i = 0; i < Width; i++)
	//	Slots[i].SetNum(Height);

	SlotsNew.Empty(Width * Height);
	MineIndices.Empty(MineCount);

	for (uint32 row = 0; row < Width; row++)
	{
		for (uint32 col = 0; col < Height; col++)
		{			
			int32 newNum = 0;
			if (currentMines < MineCount)
			{
				if (FMath::RandRange(1, 10) % 2 == 0)
				{
					Mines.Add(TTuple<int32, int32>(row, col));
					MineIndices.Add(GetTileIndex(row, col));
					newNum = -1;
					currentMines++;
				}
			}
			
			//Slots[row][col] = newNum;
			int32 index = SlotsNew.Add(FTileData());
			SlotsNew[index].Index = index;
			SlotsNew[index].Row = row;
			SlotsNew[index].Column = col;
			SlotsNew[index].Type = newNum;

			TSharedPtr<STextBlock> text;						
			auto slot = Grid.Get()->AddSlot(col, row);
			
			slot
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[				
				SNew(SButton)
				.ButtonColorAndOpacity(TileColor)
				.Text(FText::FromString(TEXT(" ")))	
				[
					SAssignNew(text, STextBlock)
					.Text(FText::FromString(" "))
				]
				.OnClicked_Lambda
				(
					[this, row, col, text, index]() -> FReply
					{
						if (SlotsNew[index].Type >= 0)
						{
							text.Get()->SetText(FText::FromString(FString::FromInt(SlotsNew[index].Type)));
							bFirstClick = false;
							SlotsNew[index].bIsChecked = true;
							RevealAdjacentTiles(row, col);
						}

						else
						{							
							if (bFirstClick)
							{
								//Move this mine somewhere else
								SlotsNew[index].bIsChecked = true;
								int32 newIndex = index + Width;
								newIndex = FMath::Clamp(newIndex, 0, (Width * Height) - 1);
								MineIndices.Remove(index);
								MineIndices.Add(newIndex);
								CalculateAdjacentTiles(row, col, -1);
								TTuple<int32, int32> newMine = GetRowAndColumn(newIndex);
								CalculateAdjacentTiles(newMine.Key, newMine.Value, 1);
								bFirstClick = false;
								RevealAdjacentTiles(row, col);
							}

							else
							{
								GridButtonText.Get()->SetText(FText::FromString("You Lose. Play Again?"));

								for (int32 i = 0; i < MineIndices.Num(); i++)
								{
									SlotsNew[MineIndices[i]].Text.Get()->SetText(FText::FromString("X"));
								}
							}							
						}

						return FReply::Handled();
					}
				)				
			];

			SlotsNew[index].Text = text;
		}
	}

	for (int32 i = 0; i < Mines.Num(); i++)
	{
		int32 rowCenter = Mines[i].Key;
		int32 colCenter = Mines[i].Value;

		CalculateAdjacentTiles(rowCenter, colCenter, 1);
	}
}

void FMinesweeperGameModule::CalculateAdjacentTiles(int32 Row, int32 Column, int32 Value)
{		
	for (int32 i = 0; i < adjacents.Num(); i++)
	{
		int32 adjRow = Row + adjacents[i].Key;
		int32 adjCol = Column + adjacents[i].Value;

		if (FTileData* Tile = GetTile(adjRow, adjCol))
		{
			if (Value >= 0)
			{
				if (Tile->Type >= 0)
				{
					Tile->Type += Value;
				}
			}

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
	int32 multiplier = 1;

	for (int32 i = 0; i < adjacents.Num(); i++)
	{
		multiplier = 1;

		while (true)
		{
			int32 adjRow = Row + adjacents[i].Key * multiplier;
			int32 adjCol = Column + adjacents[i].Value * multiplier;

			int32 newIndex = GetTileIndex(adjRow, adjCol);

			if (!SlotsNew.IsValidIndex(newIndex))
				break;

			if (SlotsNew[newIndex].bIsChecked) //Stop at a tile that has previously been checked
				break;

			SlotsNew[newIndex].bIsChecked = true;

			if (SlotsNew[newIndex].Type < 0) //Make sure mines don't get marked
				break;
			
			SlotsNew[newIndex].Text.Get()->SetText(FText::FromString(FString::FromInt(SlotsNew[newIndex].Type)));

			if (SlotsNew[newIndex].Type > 0) //Stop at a tile with adjacent mines							
				break;
			
			multiplier++;
		}
	}
}

TSharedRef<SDockTab> FMinesweeperGameModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{	
	//Slots = TArray<TArray<int32>>();
	SlotsNew = TArray<FTileData>();
	TileColor = FColor(0.7, 0.7, 0.7, 0.8);

	adjacents =
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
			SNew(SVerticalBox) //Main List
			+SVerticalBox::Slot() //Width/Height entry
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox) 
				+SHorizontalBox::Slot()
				.Padding(5, 0)
				.FillWidth(0.1f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Width")))
				]

				+SHorizontalBox::Slot()
				.Padding(0, 0)
				.FillWidth(0.25f)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
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
				.FillWidth(0.1f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Height")))
				]
				+SHorizontalBox::Slot()
				.Padding(0, 0)
				.FillWidth(0.25f)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
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
				.FillWidth(0.1f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Number of Mines")))
				]
				+SHorizontalBox::Slot()
				.Padding(0, 0)
				.FillWidth(0.25f)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
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
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
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
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
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