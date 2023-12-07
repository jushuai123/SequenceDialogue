// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "MovieSceneDialogueTemplate.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
//#include "MovieSceneEvaluation.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "IMovieScenePlayer.h"
//#include "DialogueInterface.h"
#include "GameFramework/GameModeBase.h"

#include "Widgets/SViewport.h"

#include "Widgets/SOverlay.h"
#include "DialogueWidget.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_EDITOR
#include "LevelEditor.h"
//#include "ILevelViewport.h"
#include "Editor.h"
#include "Slate/SceneViewport.h"
#include "EditorStyleSet.h"

#endif

struct FDialogueSharedTrackData : IPersistentEvaluationData
{
	FDialogueSharedTrackData():bNeedExecute(false)
	{}

	~FDialogueSharedTrackData()
	{
	}

	bool HasAnythingToDo() const
	{
		return bNeedExecute;
	}

	void SetInfo(bool show,const FText& info, const FText& name)
	{
		bShow = show;
		DialogueInfo = info;
		SpeakerName = name;
		bNeedExecute = true;
	}

	void Apply(IMovieScenePlayer& Player)
	{
		bNeedExecute = false;

		UWorld* curWorld = nullptr;
		if (Player.GetPlaybackContext())
		{
			curWorld = Player.GetPlaybackContext()->GetWorld();
		}
		else
		{
			return;
		}

#if WITH_EDITOR
		if (GIsEditor && !curWorld->IsPlayInEditor())
		{
			//Editor mode;
			if (bShow)
			{
				if (!WBP_DialogueWidget)
				{
					FSceneViewport* SceneViewport = nullptr;
					FLevelEditorViewportClient* LevelEditorViewportClient = GEditor->GetLevelViewportClients()[0];
					const TIndirectArray<FWorldContext>& WorldContexts = GEditor->GetWorldContexts();
					//TODO: Handle multiple viewports?
					if (WorldContexts.Num() > 0)
					{
						UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
						FSceneViewport* EditorViewport = (FSceneViewport*)EditorEngine->GetActiveViewport();
						if (EditorViewport != nullptr)
						{
							SceneViewport = EditorViewport;
						}
						
						if (SceneViewport)
						{
							if (SceneViewport->GetViewportWidget().IsValid())
							{
								//Viewport = SceneViewport->GetViewportWidget();
								UWorld* World = WorldContexts[0].World();
								WBP_DialogueWidget = CreateDialogueWidget(World);
								TSharedRef<SWidget> DialogueSWidget = WBP_DialogueWidget->TakeWidget();
								ViewportWidget = SceneViewport->GetViewportWidget().Pin()->GetContent();
								SOverlay* Overlay = (SOverlay*)ViewportWidget.Pin().Get();
								if (Overlay)
								{
									Overlay->AddSlot()
									[
										DialogueSWidget
									];
								}
							}
						}
					}
				}

				if (WBP_DialogueWidget)
				{
					FText Delimiter = FText::FromString(TEXT(":"));
					WBP_DialogueWidget->SetSpeakerName(FText::Join(Delimiter, SpeakerName, FText::FromString(TEXT(""))));
					WBP_DialogueWidget->SetDialogueInfo(DialogueInfo);
				}
			}
			else
			{
				SOverlay* Overlay = (SOverlay*)ViewportWidget.Pin().Get();

				if (Overlay && WBP_DialogueWidget)
				{
					Overlay->RemoveSlot(WBP_DialogueWidget->TakeWidget());
				}

				if (WBP_DialogueWidget)
				{
					WBP_DialogueWidget = nullptr;
				}
			}
		}
		else
#endif
		{

			if (GEngine && GEngine->GameViewport)
			{
				if (bShow)
				{
					if (!WBP_DialogueWidget)
					{
						WBP_DialogueWidget = CreateDialogueWidget(GEngine->GameViewport->GetWorld());
						//WBP_DialogueWidget->AddToViewport();
						GEngine->GameViewport->AddViewportWidgetContent(WBP_DialogueWidget->TakeWidget());
					}
			
					if (WBP_DialogueWidget)
					{
						FText Delimiter = FText::FromString(TEXT(":"));
						WBP_DialogueWidget->SetSpeakerName(FText::Join(Delimiter, SpeakerName, FText::FromString(TEXT(""))));
						WBP_DialogueWidget->SetDialogueInfo(DialogueInfo);
					}
				}
				else
				{
					if (WBP_DialogueWidget)
					{
						//WBP_DialogueWidget->RemoveFromViewport();
						GEngine->GameViewport->RemoveViewportWidgetContent(WBP_DialogueWidget->TakeWidget());
					}
					
					if (WBP_DialogueWidget)
					{
						WBP_DialogueWidget = nullptr;
					}
				}
			}
		}
	}

	UDialogueWidget* CreateDialogueWidget(UWorld* OwningObject)
	{
		UClass* DialogueWidgetClass = StaticLoadClass(UDialogueWidget::StaticClass(), nullptr, TEXT("/SequenceDialogue/WBP_Dialogue.WBP_Dialogue_C"));
		if (!DialogueWidgetClass || !OwningObject)
		{
			return nullptr;
		}

		return CreateWidget<UDialogueWidget>(OwningObject, DialogueWidgetClass);
	}

private:
	bool bNeedExecute;
	bool bShow;

	FText DialogueInfo;
	FText SpeakerName;

#if WITH_EDITOR
	TWeakPtr<SWidget> ViewportWidget;
	//TWeakPtr<ILevelViewport> Viewport;
#endif
	UPROPERTY()
	UDialogueWidget* WBP_DialogueWidget = nullptr;
};


struct FDialogueExecutionToken : IMovieSceneSharedExecutionToken
{
	virtual void Execute(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	//virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		FDialogueSharedTrackData* TrackData = PersistentData.Find<FDialogueSharedTrackData>(FMovieSceneDialogueSharedTrack::GetSharedDataKey());
		if (TrackData)
		{
#if WITH_EDITOR
			if (GIsEditor && !Player.GetPlaybackContext()->GetWorld()->IsPlayInEditor())
			{
				if (EnvMode == EEnvMode::WithEditor)
				{
					TrackData->Apply(Player);
				}
			}
			else
#endif
			{
				if (EnvMode == EEnvMode::WithRuntime)
				{
					TrackData->Apply(Player);
				}
			}
			
		}
	}

public:
	enum EEnvMode
	{
		None = 0,
		WithEditor = 1,
		WithRuntime = 2
	};

	EEnvMode EnvMode = EEnvMode::None;
};

FMovieSceneDialogueSectionTemplate::FMovieSceneDialogueSectionTemplate(const UMovieSceneDialogueSection& Section)
	: DialogueInfo(Section.GetDialogueInfo()),SpeakerName(Section.GetDialogueSpeakerName())
{
}

void FMovieSceneDialogueSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	FMovieSceneSharedDataId shared_id = FMovieSceneDialogueSharedTrack::GetSharedDataKey().UniqueId;
	const FDialogueSharedTrackData* TrackData = PersistentData.Find<FDialogueSharedTrackData>(FMovieSceneDialogueSharedTrack::GetSharedDataKey());
	if (TrackData && TrackData->HasAnythingToDo() && !ExecutionTokens.FindShared(shared_id))
	{
		FDialogueExecutionToken DialogueExecutionToken = FDialogueExecutionToken();
#if WITH_EDITOR
		if (GIsEditor && !GIsPlayInEditorWorld)
		{
			DialogueExecutionToken.EnvMode = FDialogueExecutionToken::EEnvMode::WithEditor;
		}
		else
#endif
		{
			DialogueExecutionToken.EnvMode = FDialogueExecutionToken::EEnvMode::WithRuntime;
		}

		ExecutionTokens.AddShared(shared_id, DialogueExecutionToken);
	}
}

void FMovieSceneDialogueSectionTemplate::Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	FDialogueSharedTrackData& TrackData = PersistentData.GetOrAdd<FDialogueSharedTrackData>(FMovieSceneDialogueSharedTrack::GetSharedDataKey());
	TrackData.SetInfo(true, DialogueInfo,SpeakerName);
}

void FMovieSceneDialogueSectionTemplate::TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	FDialogueSharedTrackData& TrackData = PersistentData.GetOrAdd<FDialogueSharedTrackData>(FMovieSceneDialogueSharedTrack::GetSharedDataKey());
	TrackData.SetInfo(false, DialogueInfo, SpeakerName);
	TrackData.Apply(Player);
}

FSharedPersistentDataKey FMovieSceneDialogueSharedTrack::GetSharedDataKey()
{
	static FMovieSceneSharedDataId DataId(FMovieSceneSharedDataId::Allocate());
	return FSharedPersistentDataKey(DataId, FMovieSceneEvaluationOperand());
}

void FMovieSceneDialogueSharedTrack::TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
/*
	FDialogueSharedTrackData* TrackData = PersistentData.Find<FDialogueSharedTrackData>(GetSharedDataKey());
	if (TrackData)
	{
		FText noused;
		TrackData->SetInfo(false, noused,noused);
		TrackData->Apply(Player);

		PersistentData.Reset(GetSharedDataKey());
	}
*/
}

void FMovieSceneDialogueSharedTrack::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
/*
	const FDialogueSharedTrackData* TrackData = PersistentData.Find<FDialogueSharedTrackData>(GetSharedDataKey());
	if (TrackData && TrackData->HasAnythingToDo())
	{
		ExecutionTokens.Add(FDialogueExecutionToken());
	}
*/
}