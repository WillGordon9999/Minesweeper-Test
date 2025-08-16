// Copyright Epic Games, Inc. All Rights Reserved.

#include "MinesweeperGameCommands.h"

#define LOCTEXT_NAMESPACE "FMinesweeperGameModule"

void FMinesweeperGameCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "MinesweeperGame", "Bring up MinesweeperGame window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
