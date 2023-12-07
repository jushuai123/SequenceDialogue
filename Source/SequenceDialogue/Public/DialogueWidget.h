// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "DialogueWidget.generated.h"

/**
 * 
 */
UCLASS()
class SEQUENCEDIALOGUE_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	bool SetDialogueInfo(const FText& InText);
	bool SetSpeakerName(const FText& InText);
};
