// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueWidget.h"

bool UDialogueWidget::SetDialogueInfo(const FText& InText)
{
	if (UTextBlock* DialogueInfoTextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("TB_DialogueInfo"))))
	{
		DialogueInfoTextBlock->SetText(InText);
		return true;
	}
	return false;
}

bool UDialogueWidget::SetSpeakerName(const FText& InText)
{
	if (UTextBlock* SpeakerNameTextBlock = Cast<UTextBlock>(GetWidgetFromName(TEXT("TB_SpeakerName"))))
	{
		SpeakerNameTextBlock->SetText(InText);
		return true;
	}
	return false;
}