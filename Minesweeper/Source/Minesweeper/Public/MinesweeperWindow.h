#include "CoreMinimal.h"
#include "Widgets/Docking/SDockTab.h"
#include "MinesweeperWindow.generated.h"

UCLASS()
class SMinesweeperWindow : public SDockTab
{
public:
    SMinesweeperWindow() { }

    TSharedRef<SDockTab> OnSpawnTab(const FSpawnTabArgs& SpawnTabArgs);
};