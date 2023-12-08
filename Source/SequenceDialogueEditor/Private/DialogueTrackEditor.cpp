#include "DialogueTrackEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "SequencerUtilities.h"
#include "MovieSceneDialogueTrack.h"
#include "MovieSceneDialogueSection.h"
#include "DialogueSection.h"
#include "IDetailsView.h"
//#include "ModuleManager.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"

//#include "MovieSceneAudioTrack.h"
#include "Tracks/MovieSceneAudioTrack.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "DialogueTrackEditor.h"

FDialogueTrackEditor::FDialogueTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{ }


TSharedRef<ISequencerTrackEditor> FDialogueTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FDialogueTrackEditor( InSequencer ) );
}

bool FDialogueTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	return Type == UMovieSceneDialogueTrack::StaticClass();
}

const FSlateBrush* FDialogueTrackEditor::GetIconBrush() const
{
	return FAppStyle::GetBrush("Sequencer.Tracks.LevelVisibility");
}


TSharedRef<ISequencerSection> FDialogueTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding )
{
	UMovieSceneDialogueSection* dialogueSection = Cast<UMovieSceneDialogueSection>(&SectionObject);
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) && dialogueSection != nullptr );

	return MakeShareable( new FDialogueSection( *dialogueSection) );
}

void FDialogueTrackEditor::BuildAddTrackMenu( FMenuBuilder& MenuBuilder )
{
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();

	if ( ( RootMovieSceneSequence == nullptr ) || ( RootMovieSceneSequence->GetClass()->GetName() != TEXT( "LevelSequence" ) ) )
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddDialogueTrack", "Dialogue" ),
		LOCTEXT("AddDialogueToolTip", "Dialogue Track." ),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Tracks.LevelVisibility"),
		FUIAction( FExecuteAction::CreateRaw( this, &FDialogueTrackEditor::OnAddTrack ) ) );
}


TSharedPtr<SWidget> FDialogueTrackEditor::BuildOutlinerEditWidget( const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params )
{
	// Create a container edit box
	return FSequencerUtilities::MakeAddButton( 
		LOCTEXT( "AddDialogueTrigger", "Add Dialogue" ),
		FOnGetContent::CreateSP( this, &FDialogueTrackEditor::BuildAddVisibilityTriggerMenu, Track ),
		Params.NodeIsHovered,
		GetSequencer());
}


void FDialogueTrackEditor::BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track)
{
	UMovieSceneDialogueTrack* DialogueTrack = Cast<UMovieSceneDialogueTrack>(Track);

	class FDialogueTrackCustomization : public IDetailCustomization
	{
	public:
		FDialogueTrackCustomization()
		{
		}

		~FDialogueTrackCustomization()
		{
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			DetailBuilder.HideCategory("Track");
			DetailBuilder.HideCategory("General");
		}

	};

	auto SubMenuDelegate = [DialogueTrack](FMenuBuilder& InMenuBuilder, UMovieSceneDialogueTrack* InDialogueTrack)
	{
		FDetailsViewArgs DetailsViewArgs;
		{
			DetailsViewArgs.bAllowSearch = false;
			DetailsViewArgs.bCustomFilterAreaLocation = true;
			DetailsViewArgs.bCustomNameAreaLocation = true;
			DetailsViewArgs.bHideSelectionTip = true;
			DetailsViewArgs.bLockable = false;
			DetailsViewArgs.bSearchInitialKeyFocus = true;
			DetailsViewArgs.bUpdatesFromSelection = false;
			DetailsViewArgs.bShowOptions = false;
			DetailsViewArgs.bShowModifiedPropertiesOption = false;
			//DetailsViewArgs.NotifyHook = &DetailsNotifyWrapper.Get();
		}

		TArray<TWeakObjectPtr<UObject>> Tracks;
		{
			Tracks.Add(DialogueTrack);
		}

		TSharedRef<IDetailsView> DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);

		FOnGetDetailCustomizationInstance CreateInstance = FOnGetDetailCustomizationInstance::CreateLambda([=] { return MakeShared<FDialogueTrackCustomization>(); });
		DetailsView->RegisterInstancedCustomPropertyLayout(UMovieSceneDialogueTrack::StaticClass(), CreateInstance);
		{
			DetailsView->SetObjects(Tracks);
		}

		InMenuBuilder.AddWidget(DetailsView, FText::GetEmpty(), true);
	};

	const bool bInOpenSubMenuOnClick = false;

	MenuBuilder.AddSubMenu(
		LOCTEXT("Properties", "Properties"),
		LOCTEXT("PropertiesTooltip", ""),
		FNewMenuDelegate::CreateLambda(SubMenuDelegate, DialogueTrack),
		bInOpenSubMenuOnClick);
}

void FDialogueTrackEditor::AddNewSection( UMovieScene* MovieScene, UMovieSceneTrack* dialogueTrack)
{
	const FScopedTransaction Transaction( LOCTEXT( "AddDialogueSection_Transaction", "Add Dialogue Trigger" ) );
	dialogueTrack->Modify();

	FFrameNumber startSliderTime = MovieScene->GetPlaybackRange().GetLowerBoundValue();

	FFrameNumber LastSectionEndTime = startSliderTime;
	for (auto Section : dialogueTrack->GetAllSections())
	{
		TRange<FFrameNumber> SectionRange = Section->GetRange();
		LastSectionEndTime = SectionRange.GetUpperBoundValue() > LastSectionEndTime ? SectionRange.GetUpperBoundValue() : LastSectionEndTime;
	}

	UMovieSceneDialogueSection* dialogueSection = CastChecked<UMovieSceneDialogueSection>(dialogueTrack->CreateNewSection() );
	if (dialogueTrack->GetAllSections().Num() > 0)
	{
		UMovieSceneDialogueSection* LastSection = Cast<UMovieSceneDialogueSection>(dialogueTrack->GetAllSections().Last());
		dialogueSection->SetDialogueSpeakerName(LastSection->GetDialogueSpeakerName());
	}

	startSliderTime = LastSectionEndTime;
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameNumber SectionLength = (0.5 * TickResolution).FloorToFrame();
	FFrameNumber endTime = startSliderTime + SectionLength;
	if (endTime < startSliderTime)
	{
		FFrameNumber Temp = startSliderTime;
		startSliderTime = endTime;
		endTime = Temp;
	}

	dialogueSection->SetRange(TRange<FFrameNumber>(startSliderTime, TRangeBound<FFrameNumber>::Exclusive(endTime)));

	dialogueSection->SetRowIndex(0);

	dialogueTrack->AddSection( *dialogueSection);
}

void FDialogueTrackEditor::OnAddTrack()
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if ( FocusedMovieScene == nullptr )
	{
		return;
	}

	const FScopedTransaction Transaction( LOCTEXT( "AddDialogueTrack_Transaction", "Add Dialogue Track" ) );
	FocusedMovieScene->Modify();

	//UMovieSceneDialogueTrack* NewTrack = FocusedMovieScene->AddMasterTrack<UMovieSceneDialogueTrack>();
	UMovieSceneDialogueTrack* NewTrack = FocusedMovieScene->AddTrack<UMovieSceneDialogueTrack>();
	checkf( NewTrack != nullptr, TEXT("Failed to create new dialogue track.") );

	//AddNewSection( FocusedMovieScene, NewTrack);

	GetSequencer()->NotifyMovieSceneDataChanged( EMovieSceneDataChangeType::MovieSceneStructureItemAdded );
}


TSharedRef<SWidget> FDialogueTrackEditor::BuildAddVisibilityTriggerMenu( UMovieSceneTrack* dialogueTrack )
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT( "AddDialogueTrigger", "Dialogue" ),
		LOCTEXT( "AddDialogueTriggerToolTip", "Add a Dialogue." ),
		FSlateIcon(),
		FUIAction( FExecuteAction::CreateSP(
			this, &FDialogueTrackEditor::OnAddNewSection, dialogueTrack) ) );

	return MenuBuilder.MakeWidget();
}


void FDialogueTrackEditor::OnAddNewSection( UMovieSceneTrack* dialogueTrack)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if ( FocusedMovieScene == nullptr )
	{
		return;
	}

	AddNewSection( FocusedMovieScene, dialogueTrack);
	GetSequencer()->NotifyMovieSceneDataChanged( EMovieSceneDataChangeType::MovieSceneStructureItemAdded );
}

#undef LOCTEXT_NAMESPACE