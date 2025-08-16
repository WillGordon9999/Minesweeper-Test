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

void FMinesweeperGameModule::GenerateGrid()
{
	uint32 currentMines = 0;
	Grid.Get()->ClearChildren();
	Mines.Empty();
	Slots.SetNum(Width);
	GridButtonText.Get()->SetText(FText::FromString(TEXT("Generate New Grid")));

	for (uint32 i = 0; i < Width; i++)
		Slots[i].SetNum(Height);


	for (uint32 row = 0; row < Width; row++)
	{
		//Slots.EmplaceAt(col, TArray<int32>());
		
		for (uint32 col = 0; col < Height; col++)
		{
			//bool createMine = false;
			int32 newNum = 0;
			if (currentMines < MineCount)
			{
				if (FMath::RandRange(1, 10) % 2 == 0)
				{
					Mines.Add(TTuple<int32, int32>(row, col));
					newNum = -1;
					currentMines++;
				}
			}

			//Slots[col].EmplaceAt(row, newNum);
			Slots[row][col] = newNum;
			
			TSharedPtr<STextBlock> text;
			TSharedPtr<SButton> button;
			FColor pressedColor = FColor(0.3, 0.3, 0.3, 0.8);
			auto slot = Grid.Get()->AddSlot(col, row);
			
			slot
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[				
				SAssignNew(button, SButton)
					//SNew(SButton)
					.ButtonColorAndOpacity(FColor(0.7, 0.7, 0.7, 0.8))
					.Text(FText::FromString(TEXT(" ")))					
				[
					SAssignNew(text, STextBlock)
					.Text(FText::FromString(" "))
					//.Text(FText::FromString(FString::FromInt(row) + " " + FString::FromInt(col)))
				]								
				.OnClicked_Lambda
				(
					[this, row, col, text]() -> FReply
					{
						if (Slots[row][col] >= 0)
						{							
							text.Get()->SetText(FText::FromString(FString::FromInt(Slots[row][col])));
							//button.Get()->SetColorAndOpacity(pressedColor);							
							//RevealNewTiles(); //Should Search Left Right then Up and down
						}
						else
						{
							//YOU DIED
							GridButtonText.Get()->SetText(FText::FromString("You Lose. Play Again?"));

							for (int32 i = 0; i < Mines.Num(); i++)
							{
								//Show Remaining Mines here
							}
						}

						return FReply::Handled();
					}
				)				
			];
		}
	}

	TArray<TTuple<int32, int32>> adjacents =
	{
		{ -1, -1 },
		{ -1, 0 },
		{ -1, 1 },
		{ 0, -1 },
		{ 0, 1 },
		{ 1, -1 },
		{ 1, 0 },
		{ 1, 1 }
	};

	for (int32 i = 0; i < Mines.Num(); i++)
	{
		int32 rowCenter = Mines[i].Key;
		int32 colCenter = Mines[i].Value;

		for (int j = 0; j < 8; j++)
		{
			int32 row = rowCenter + adjacents[j].Key;
			int32 col = colCenter + adjacents[j].Value;

			if (Slots.IsValidIndex(row) && Slots[row].IsValidIndex(col))
			{
				if (Slots[row][col] >= 0)
				{
					Slots[row][col]++;
				}
			}
			//if (row >= 0 && row < Slots.Num())
			//{
			//	if (col >= 0 && col < Slots[row].Num())
			//	{
			//		
			//	}
			//}
		}
	}
}

TSharedRef<SDockTab> FMinesweeperGameModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{	
	Slots = TArray<TArray<int32>>();

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
				//+SGridPanel::Slot(0, 0)
			]
		];

	GenerateGrid();
	return Tab;
}

void FMinesweeperGameModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(MinesweeperGameTabName);
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