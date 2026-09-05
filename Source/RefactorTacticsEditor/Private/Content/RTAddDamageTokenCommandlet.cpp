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
	 * 🔴 **Piu' grande della vita, e la misura viene da una seduta PIE, non da un gusto.**
	 *
	 * La prima stesura usava `18`, e la verifica del 2026-09-06 l'ha trovato **leggibile ma piu' debole del
	 * testo intorno**: a schermo `-24` risultava piu' piccolo di `60/90` e senza contorno, quindi opaco su
	 * un fondo chiaro. Il token e' l'unico elemento della sovrapposizione che dura **meno di un secondo e
	 * non torna**: deve farsi notare senza che l'occhio lo cerchi, mentre gli altri si leggono
	 * appoggiandocisi.
	 *
	 * ⚠️ La gerarchia la fa la **differenza** fra le dimensioni, non la dimensione in se' — e' la stessa
	 * ragione gia' scritta in `RTBuildPlaygroundPanel`.
	 */
	constexpr int32 TokenFontSize = 26;

	/**
	 * Il contorno, che e' cio' che rende il token leggibile su **qualunque** fondo.
	 *
	 * 🔴 **Senza, il canale fallisce proprio dove serve.** La board e' grigio chiaro e le celle in fiamme
	 * sono arancioni: un testo chiaro senza contorno sparisce sulla prima e si confonde sulla seconda. E'
	 * la stessa regola del secondo canale che [D-146] fissa per le celle — se l'unico canale fallisce, la
	 * lettura non degrada, **sparisce**.
	 */
	constexpr int32 TokenOutlineSize = 2;

	/** Stile del token: applicato sia quando nasce sia quando esiste gia', cosi' il comando e' RIPETIBILE. */
	void ApplyTokenStyle(UTextBlock* Token)
	{
		FSlateFontInfo Font = Token->GetFont();
		Font.Size = TokenFontSize;
		Font.OutlineSettings.OutlineSize = TokenOutlineSize;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
		Token->SetFont(Font);
		Token->SetJustification(ETextJustify::Center);

		// ⛔ **Il colore non e' il canale, e non deve diventarlo.** `#2453` vieta il colore come UNICO
		// canale: qui il senso lo portano gia' la cifra e il segno. Il bianco caldo serve solo a staccare
		// dal fondo, e chi non lo distingue legge comunque `-24`.
		Token->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.85f, 1.f)));
	}

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

		// Non e' un errore: il commandlet e' ripetibile. ⚠️ **Ma «ripetibile» non e' «inerte»**: lo stile si
		// RI-APPLICA, o una taratura decisa dopo una seduta PIE non raggiungerebbe mai un asset in cui
		// l'elemento e' gia' presente — e il comando direbbe «niente da fare» su un token illeggibile.
		UTextBlock* Presente = Cast<UTextBlock>(Existing);
		ApplyTokenStyle(Presente);
		Presente->SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogRTDamageToken, Display,
			TEXT("'%s' e' gia' nell'albero: stile riapplicato (corpo %d, contorno %d)"),
			TokenName, TokenFontSize, TokenOutlineSize);

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Overlay);
		FKismetEditorUtilities::CompileBlueprint(Overlay);
		Overlay->MarkPackageDirty();
		if (!SaveWidgetPackage(Overlay->GetOutermost(), Overlay))
		{
			UE_LOG(LogRTDamageToken, Error, TEXT("overlay NON salvato"));
			return 1;
		}
		UE_LOG(LogRTDamageToken, Display, TEXT("=== fatto ==="));
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

	ApplyTokenStyle(Token);

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
