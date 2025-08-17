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
	SlotsNew.Empty(Width * Height);
	MineIndices.Empty(MineCount);

	for (uint32 row = 0; row < Width; row++)
	{
		for (uint32 col = 0; col < Height; col++)
		{			
			int32 newNum = 0;
			if (currentMines < MineCount)
			{
				if (FMath::RandRange(1, Width * Height) % Width == 0)
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
						if (SlotsNew[index].bIsChecked)
							return FReply::Handled();

						if (SlotsNew[index].Type >= 0)
						{
							if (SlotsNew[index].Type > 0)
							{
								text.Get()->SetText(FText::FromString(FString::FromInt(SlotsNew[index].Type)));
							}

							SlotsNew[index].Button.Get()->SetColorAndOpacity(PressedColor);
							SlotsNew[index].Button.Get()->SetBorderBackgroundColor(PressedColor);

							bFirstClick = false;
							SlotsNew[index].bIsChecked = true;
							RevealTileLoop(row, col);							
						}

						else
						{							
							if (bFirstClick)
							{
								SlotsNew[index].bIsChecked = true;
								SlotsNew[index].Button.Get()->SetColorAndOpacity(PressedColor);
								SlotsNew[index].Button.Get()->SetBorderBackgroundColor(PressedColor);								

								//Move this mine somewhere else
								int32 newIndex = index + Width;
								newIndex = FMath::Clamp(newIndex, 0, (Width * Height) - 1);
								MineIndices.Remove(index);
								MineIndices.Add(newIndex);
								CalculateAdjacentTiles(row, col, -1);
								TTuple<int32, int32> newMine = GetRowAndColumn(newIndex);
								CalculateAdjacentTiles(newMine.Key, newMine.Value, 1);
								bFirstClick = false;
								//RevealAdjacentTiles(row, col);
								RevealTileLoop(row, col);
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
			SlotsNew[index].Button = button;
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
	for (int32 i = 0; i < Adjacents.Num(); i++)
	{
		int32 adjRow = Row + Adjacents[i].Key;
		int32 adjCol = Column + Adjacents[i].Value;

		if (adjRow < 0 || adjRow > (int32)Width - 1) //was >= and Dimension - 1
			continue;

		if (adjCol < 0 || adjCol > (int32)Height - 1) //was >= and Dimension - 1
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

	for (int32 i = 0; i < Adjacents.Num(); i++)
	{
		multiplier = 1;

		while (true)
		{			
			int32 adjRow = Row + Adjacents[i].Key * multiplier;
			int32 adjCol = Column + Adjacents[i].Value * multiplier;

			if (adjRow < 0 || adjRow >= (int32)Width - 1)
				break;

			if (adjCol < 0 || adjCol >= (int32)Height - 1)
				break;

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

void FMinesweeperGameModule::RevealAdjacentTilesNew(int32 Row, int32 Column)
{
	for (int32 i = 0; i < Adjacents.Num(); i++)
	{
		int32 adjRow = Row + Adjacents[i].Key;
		int32 adjCol = Column + Adjacents[i].Value;

		if (adjRow < 0 || adjRow > (int32)Width - 1) //was >= and Dimension - 1
			continue;

		if (adjCol < 0 || adjCol > (int32)Height - 1) //was >= and Dimension - 1
			continue;

		int32 newIndex = GetTileIndex(adjRow, adjCol);

		if (!SlotsNew.IsValidIndex(newIndex))
			continue;
		
		if (SlotsNew[newIndex].Type < 0) 
			continue;

		SlotsNew[newIndex].Button.Get()->SetColorAndOpacity(PressedColor);
		SlotsNew[newIndex].Button.Get()->SetBorderBackgroundColor(PressedColor);

		if (SlotsNew[newIndex].bIsChecked)
			continue;

		SlotsNew[newIndex].bIsChecked = true;

		if (SlotsNew[newIndex].Type > 0)
		{
			SlotsNew[newIndex].Text.Get()->SetText(FText::FromString(FString::FromInt(SlotsNew[newIndex].Type)));
			continue;
		}

		if (SlotsNew[newIndex].Type == 0)
		{
			SearchList.Add(SlotsNew[newIndex]);
		}
	}
}

TSharedRef<SDockTab> FMinesweeperGameModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{		
	SlotsNew = TArray<FTileData>();
	SearchList = TArray<FTileData>();
	
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
			SNew(SVerticalBox) //Main List			
			//Width Height Mine Count Entry
			+SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.MaxHeight(30.0f)			
			//.FillHeight(0.01f)
			[
				SNew(SHorizontalBox) 
				+SHorizontalBox::Slot()
				.Padding(5, 0)
				.AutoWidth()
				//.FillWidth(0.1f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Width")))
				]

				+SHorizontalBox::Slot()
				.Padding(0, 0)
				//.FillWidth(0.1f)
				.AutoWidth()
				.HAlign(HAlign_Center)
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
				//.FillWidth(0.1f)
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Height")))
				]

				+SHorizontalBox::Slot()
				.Padding(0, 0)
				//.FillWidth(0.1f)
				.AutoWidth()
				.HAlign(HAlign_Center)
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
				//.FillWidth(0.1f)				
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Number of Mines")))
				]

				+SHorizontalBox::Slot()
				.Padding(0, 0)
				//.FillWidth(0.1f)
				.AutoWidth()
				.HAlign(HAlign_Center)
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

			//=============== END OF NUMBER ROW =================

			+SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.MaxHeight(20.0f)
			.Padding(0.0, 5.0, 0.0, 0.0)
			//.AutoHeight()						
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