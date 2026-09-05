#include "Content/RTAddDamageTokenCommandlet.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTDamageToken, Log, All);

namespace
{
	const TCHAR* OverlayPath = TEXT("/Game/RT/UI/Match/WBP_RT_UnitOverlay.WBP_RT_UnitOverlay");

	/** Il nome e' la MANIGLIA del `BindWidgetOptional`: cambiarlo qui scollega il C++ in silenzio. */
	const TCHAR* TokenName = TEXT("DamageTokenText");

	/** Il contenitore verticale che l'asset gia' porta, con dentro nome, vita, scudo e stati. */
	const TCHAR* RootBoxName = TEXT("RootBox");

	/**
	 * 🔴 **Piu' grande della vita, e non e' vezzo grafico.** `HealthText` mostra `60/100` e si legge
	 * *appoggiandocisi*; il token deve farsi notare **senza** che l'occhio lo cerchi, perche' dura meno di
	 * un secondo e non torna. La gerarchia la fa la differenza fra le dimensioni, non la dimensione in se'
	 * — e' la stessa ragione scritta in `RTBuildPlaygroundPanel`.
	 */
	constexpr int32 TokenFontSize = 18;

	/** Il salvataggio, nella stessa forma di `RTRemoveEnergyBar` e `RTBuildPlaygroundPanel`. */
	bool SaveWidgetPackage(UPackage* Package, UBlueprint* Blueprint)
	{
		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *FileName, SaveArgs);
	}
}

URTAddDamageTokenCommandlet::URTAddDamageTokenCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URTAddDamageTokenCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	const bool bDryRun = Switches.Contains(TEXT("DryRun"));

	UE_LOG(LogRTDamageToken, Display, TEXT("=== RTAddDamageToken%s ==="),
		bDryRun ? TEXT(" [DryRun]") : TEXT(""));

	UWidgetBlueprint* Overlay = LoadObject<UWidgetBlueprint>(nullptr, OverlayPath);
	if (!Overlay || !Overlay->WidgetTree)
	{
		UE_LOG(LogRTDamageToken, Error, TEXT("overlay non caricato: %s"), OverlayPath);
		return 1;
	}

	// ------------------------------------------------------------------ 1. c'e' gia'?
	if (UWidget* Existing = Overlay->WidgetTree->FindWidget(FName(TokenName)))
	{
		if (!Cast<UTextBlock>(Existing))
		{
			// Stessa guardia di `RTRemoveEnergyBar`: se quel nome designasse un altro widget, sostituirlo
			// alla cieca cancellerebbe una cosa diversa da quella che questa issue ha misurato.
			UE_LOG(LogRTDamageToken, Error,
				TEXT("'%s' esiste ma e' una %s, non un UTextBlock — non lo tocco"),
				TokenName, *Existing->GetClass()->GetName());
			return 1;
		}

		// Non e' un errore: il commandlet e' ripetibile, e la seconda volta non c'e' niente da fare.
		UE_LOG(LogRTDamageToken, Display,
			TEXT("'%s' e' gia' nell'albero — niente da fare"), TokenName);
		return 0;
	}

	// ------------------------------------------------------------------ 2. dove va
	UVerticalBox* RootBox = Cast<UVerticalBox>(Overlay->WidgetTree->FindWidget(FName(RootBoxName)));
	if (!RootBox)
	{
		// ⚠️ Fail-closed, e con il motivo scritto: senza il contenitore atteso non si INVENTA una gerarchia
		// nuova. Un token appeso alla radice sbagliata si disegnerebbe in un posto che nessuno ha deciso.
		UE_LOG(LogRTDamageToken, Error,
			TEXT("'%s' non trovato (o non e' una UVerticalBox): l'albero dell'asset non e' quello atteso"),
			RootBoxName);
		return 1;
	}

	if (bDryRun)
	{
		UE_LOG(LogRTDamageToken, Display,
			TEXT("[DryRun] aggiungerei '%s' (UTextBlock, corpo %d, nascosto a riposo) in cima a '%s', "
			     "che oggi ha %d figli, e ne registrerei il GUID"),
			TokenName, TokenFontSize, RootBoxName, RootBox->GetChildrenCount());
		UE_LOG(LogRTDamageToken, Display, TEXT("=== fatto [DryRun: nulla e' stato scritto] ==="));
		return 0;
	}

	// ------------------------------------------------------------------ 3. si costruisce
	UTextBlock* Token = Overlay->WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), FName(TokenName));
	if (!Token)
	{
		UE_LOG(LogRTDamageToken, Error, TEXT("costruzione di '%s' fallita"), TokenName);
		return 1;
	}

	FSlateFontInfo Font = Token->GetFont();
	Font.Size = TokenFontSize;
	Token->SetFont(Font);
	Token->SetJustification(ETextJustify::Center);

	// 🔴 **Nasce NASCOSTO, e il default dell'asset conta quanto il codice.** `SetOverlayView` riafferma il
	// riposo a ogni fotogramma, ma il primo disegno avviene **prima** di quel giro: un elemento che nasce
	// visibile mostrerebbe una riga vuota sopra ogni unita' per un fotogramma. E' la stessa scelta con cui
	// `ARTUnit` fa nascere spento l'intero `OverlayWidget` — si accende chi ha qualcosa da dire.
	//
	// ⚠️ `Collapsed` e non `Hidden`: dentro una `UVerticalBox` un `Hidden` occuperebbe comunque il suo
	// spazio, e la sovrapposizione a riposo sarebbe piu' alta di quanto e' oggi.
	Token->SetVisibility(ESlateVisibility::Collapsed);

	// Un testo di partenza che NON e' un numero: se qualcuno lo vedesse a schermo saprebbe subito che sta
	// guardando il default dell'asset e non un colpo. ⛔ Mai `0` o `-0`, che si leggerebbero come un esito.
	Token->SetText(FText::FromString(TEXT("--")));

	// 🔑 **Senza questo il `BindWidgetOptional` non lo trova.** Il binding cerca una VARIABILE del
	// Blueprint con quel nome; un widget non-variabile vive nell'albero e resta invisibile al C++, che
	// continuerebbe a posare il token su un puntatore nullo — cioe' il difetto di partenza, intatto.
	Token->bIsVariable = true;

	// 🔴 **Un widget AGGIUNTO deve portarsi il GUID, o la compilazione ASSERISCE.**
	// `ValidateAndFixUpVariableGuids` (`WidgetBlueprintCompiler.cpp`) pretende che ogni widget-variabile
	// sia in `WidgetVariableNameToGuidMap`: senza, esce *«Widget was added but did not get a GUID»* e il
	// commandlet fallisce **dopo** aver fatto il lavoro giusto.
	//
	// ⚠️ `FindOrAdd` e **non** `Add`: sovrascrivere il GUID di chi ce l'ha gia' romperebbe i riferimenti
	// che quel GUID serve a riparare dopo un rename. E' la meta' speculare della regola che
	// `RTRemoveEnergyBar` documenta per la rimozione — la mappa e l'albero si muovono insieme.
	Overlay->WidgetVariableNameToGuidMap.FindOrAdd(Token->GetFName(), FGuid::NewGuid());

	// ------------------------------------------------------------------ 4. in CIMA, non in coda
	//
	// 🔑 Il numero di un colpo si legge **sopra** l'unita', non sotto le icone di stato: e' la posizione
	// che l'occhio associa al danno fluttuante. In coda finirebbe sotto `StatusBox`, cioe' nel posto in cui
	// si guarda per sapere *cosa* si ha addosso, non *cosa e' appena successo*.
	//
	// ⚠️ A riposo non sposta niente: `Collapsed` in una `UVerticalBox` occupa zero, quindi la
	// sovrapposizione ha esattamente l'ingombro di prima finche' nessuno incassa.
	RootBox->InsertChildAt(0, Token);

	UE_LOG(LogRTDamageToken, Display,
		TEXT("'%s' aggiunto in cima a '%s' (ora %d figli), variabile, GUID registrato"),
		TokenName, RootBoxName, RootBox->GetChildrenCount());

	// ------------------------------------------------------------------ 5. ricompila e salva
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Overlay);
	FKismetEditorUtilities::CompileBlueprint(Overlay);
	Overlay->MarkPackageDirty();

	if (!SaveWidgetPackage(Overlay->GetOutermost(), Overlay))
	{
		UE_LOG(LogRTDamageToken, Error, TEXT("overlay NON salvato"));
		return 1;
	}

	UE_LOG(LogRTDamageToken, Display, TEXT("overlay ricompilato e salvato"));
	UE_LOG(LogRTDamageToken, Display, TEXT("=== fatto ==="));
	return 0;
}
