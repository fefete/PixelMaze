#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PMEHUD.generated.h"

UCLASS()
class PIXELMAZEESCAPE_API APMEHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawPanel(float X, float Y, float Width, float Height);
	void DrawLabel(const FText& Text, float X, float Y, float Scale = 1.0f);
};
