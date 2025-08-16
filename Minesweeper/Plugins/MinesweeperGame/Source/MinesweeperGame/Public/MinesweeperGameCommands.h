// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "MinesweeperGameStyle.h"

class FMinesweeperGameCommands : public TCommands<FMinesweeperGameCommands>
{
public:

	FMinesweeperGameCommands()
		: TCommands<FMinesweeperGameCommands>(TEXT("MinesweeperGame"), NSLOCTEXT("Contexts", "MinesweeperGame", "MinesweeperGame Plugin"), NAME_None, FMinesweeperGameStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenPluginWindow;
};