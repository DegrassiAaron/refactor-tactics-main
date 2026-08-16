#include "UI/RTIconLibrary.h"

#include "RefactorTactics.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "GameplayTagsManager.h"
#include "Turn/RTTurnRules.h"

const TCHAR* URTIconLibrary::IconIdPrefix = TEXT("UI.Icon.");

FName URTIconLibrary::MakeIconId(const FName& SemanticPath)
{
	if (SemanticPath.IsNone())
	{
		return NAME_None;
	}
	return FName(*FString::Printf(TEXT("%s%s"), IconIdPrefix, *SemanticPath.ToString()));
}

FString URTIconLibrary::CategoryName(ERTIconCategory Category)
{
	const UEnum* Enum = StaticEnum<ERTIconCategory>();
	return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Category)) : FString();
}

TArray<FName> URTIconLibrary::RequiredIconIds()
{
	TArray<FName> Ids;

	// FASI — le quattro volontarie, nell'ordine del turno. `Planning` e `Cleanup` non sono fasi in cui il
	// giocatore agisce, e la reaction non e' una quinta fase: mostrarla qui la trasformerebbe in una.
	if (const UEnum* PhaseEnum = StaticEnum<ERTMatchPhase>())
	{
		const ERTMatchPhase VoluntaryPhases[] = {
			ERTMatchPhase::Prep, ERTMatchPhase::Dash, ERTMatchPhase::Blast, ERTMatchPhase::Move };

		for (const ERTMatchPhase Phase : VoluntaryPhases)
		{
			const FString PhaseName = PhaseEnum->GetNameStringByValue(static_cast<int64>(Phase));
			Ids.AddUnique(MakeIconId(FName(*FString::Printf(
				TEXT("%s.%s"), *CategoryName(ERTIconCategory::Phase), *PhaseName))));
		}
	}

	// AZIONI — quelle che il catalogo generico dichiara davvero. Non la lista canonica dei documenti: se
	// `Action.Overwatch` non e' ancora in codice (arriva con E14), pretenderne l'icona significherebbe
	// chiedere un disegno per qualcosa che nessuno puo' pianificare.
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (!Def.ActionId.IsNone())
		{
			Ids.AddUnique(MakeIconId(Def.ActionId));
		}
	}

	// STATI — i tag registrati sotto `Status.` (`Core/RTGameplayTags.cpp`). E' la sorgente giusta perche' e'
	// la stessa che il gameplay applica: un tag nuovo senza icona fa cadere la copertura il giorno in cui
	// viene definito, non il giorno in cui qualcuno se ne accorge a schermo.
	const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(
		FName(*CategoryName(ERTIconCategory::Status)), /*ErrorIfNotFound*/ false);

	if (StatusRoot.IsValid())
	{
		TArray<FGameplayTag> StatusTags;
		UGameplayTagsManager::Get().RequestGameplayTagChildren(StatusRoot).GetGameplayTagArray(StatusTags);

		TArray<FName> StatusIds;
		for (const FGameplayTag& Tag : StatusTags)
		{
			StatusIds.AddUnique(MakeIconId(Tag.GetTagName()));
		}

		// L'ordine dei tag restituiti dal manager non e' garantito: senza questo sort due esecuzioni potrebbero
		// produrre due liste diverse, e un confronto fra cataloghi diventerebbe rumore.
		StatusIds.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
		Ids.Append(StatusIds);
	}

	// CERTEZZA — i tre livelli di CP 11.2, che il resolver classifica e la UI mostra senza ricalcolarli.
	//
	// ⏱️ **Il tipo ADESSO esiste** — `ERTIntentCertainty`, introdotto da #859 e consumato da `ClassifyPlan`.
	// Questo commento diceva *«il giorno in cui CP 11.2 introduce l'enum, questo elenco diventa la sua
	// immagine»*: quel giorno e' arrivato, la condizione e' scattata e nessuno se n'era accorto (trovato
	// dalla code review di #964).
	//
	// 🔴 **E la conclusione si ROVESCIA: l'elenco resta scritto a mano, per una ragione che prima non
	// esisteva.** L'enum ha **quattro** valori e questa categoria ne pretende **tre**: `Unknown = 0`
	// significa «mai calcolato» e non ha una resa — un'icona lo farebbe sembrare uno stato previsto invece
	// del difetto a monte che e'. Derivare da `StaticEnum<ERTIntentCertainty>()` produrrebbe quindi una
	// quarta chiave richiesta, un asset da disegnare e un test verde su una cosa sbagliata.
	// ⚠️ Le chiavi di questa categoria sono i livelli **disegnabili**, non i valori del tipo, e le due cose
	// hanno smesso di coincidere il 2026-08-16. Se un giorno un quinto livello sara' disegnabile, si
	// aggiunge **qui** e il test lo pretende — la sincronia si paga con una riga, non con una derivazione
	// che importa anche cio' che non si disegna.
	for (const TCHAR* Level : { TEXT("Confirmed"), TEXT("Predicted"), TEXT("Uncertain") })
	{
		Ids.AddUnique(MakeIconId(FName(*FString::Printf(
			TEXT("%s.%s"), *CategoryName(ERTIconCategory::Certainty), Level))));
	}

	// IDENTITA' — i quattro eroi del roster, piu' la relazione di squadra.
	//
	// ⚠️ **Il prefisso cambia, e non e' un dettaglio**: gli `HeroId` sono `Hero.Flux`, non `Flux`, quindi
	// `MakeIconId(HeroId)` darebbe `UI.Icon.Hero.Flux` — il cui segmento di categoria (`Hero`) non combacia
	// con `Identity`, e il validator di CP 20.1 lo rifiuterebbe. La derivazione tiene il NOME e sostituisce
	// il prefisso con la categoria: `Hero.Flux` -> `Identity.Flux`. E' l'unico punto in cui la regola «la
	// chiave si deriva dall'identificatore che esiste» ha bisogno di una traduzione, ed e' scritta qui una
	// volta invece che in quattro righe a mano.
	for (const FName& HeroId : URTHeroCatalogLibrary::GetHeroIds())
	{
		FString HeroName = HeroId.ToString();
		int32 DotIndex = INDEX_NONE;
		if (HeroName.FindLastChar(TEXT('.'), DotIndex))
		{
			HeroName = HeroName.RightChop(DotIndex + 1);
		}

		if (!HeroName.IsEmpty())
		{
			Ids.AddUnique(MakeIconId(FName(*FString::Printf(
				TEXT("%s.%s"), *CategoryName(ERTIconCategory::Identity), *HeroName))));
		}
	}

	// Alleato e avversario: il consumatore esiste gia' — `ARTHUD` colora gli intenti per squadra leggendo
	// `View.bIsAlly` (`RTHUD.cpp:285`). Sono relazione, non personaggi: restano due chiavi anche se il roster
	// cresce.
	for (const TCHAR* Relation : { TEXT("Ally"), TEXT("Enemy") })
	{
		Ids.AddUnique(MakeIconId(FName(*FString::Printf(
			TEXT("%s.%s"), *CategoryName(ERTIconCategory::Identity), Relation))));
	}

	return Ids;
}

TArray<FName> URTIconLibrary::FindMissingRequiredIcons(const URTIconCatalogData* Catalog)
{
	const TArray<FName> Required = RequiredIconIds();
	if (!Catalog)
	{
		// Nessun catalogo non e' «zero mancanze»: e' la mancanza totale.
		return Required;
	}

	TSet<FName> Present;
	for (const FRTIconDef& Icon : Catalog->Icons)
	{
		// Una chiave dichiarata con asset nullo NON copre: e' esattamente il widget vuoto che CP 20.1 vieta.
		if (!Icon.IconId.IsNone() && !Icon.Asset.IsNull())
		{
			Present.Add(Icon.IconId);
		}
	}

	TArray<FName> Missing;
	for (const FName& Id : Required)
	{
		if (!Present.Contains(Id))
		{
			Missing.Add(Id);
		}
	}
	return Missing;
}

TArray<FString> URTIconLibrary::ValidateIconCatalog(const URTIconCatalogData* Catalog)
{
	TArray<FString> Errors;

	if (!Catalog)
	{
		Errors.Add(TEXT("catalogo nullo"));
		return Errors;
	}

	if (Catalog->MissingIcon.IsNull())
	{
		Errors.Add(TEXT("MissingIcon non impostata: una chiave sconosciuta non avrebbe nulla da mostrare"));
	}

	TSet<FName> Seen;
	for (const FRTIconDef& Icon : Catalog->Icons)
	{
		if (Icon.IconId.IsNone())
		{
			Errors.Add(TEXT("voce senza IconId"));
			continue;
		}

		const FString Id = Icon.IconId.ToString();

		if (!Id.StartsWith(IconIdPrefix))
		{
			Errors.Add(FString::Printf(TEXT("'%s': IconId senza il prefisso '%s'"), *Id, IconIdPrefix));
		}
		else
		{
			// La categoria dichiarata e il segmento dentro l'ID devono dire la stessa cosa. Senza questo
			// controllo `UI.Icon.Status.Wet` potrebbe dichiararsi `Warning` e nessuno se ne accorgerebbe
			// finche' un filtro per categoria non restituisce la lista sbagliata.
			const FString Expected = FString::Printf(TEXT("%s%s."), IconIdPrefix, *CategoryName(Icon.Category));
			if (!Id.StartsWith(Expected))
			{
				Errors.Add(FString::Printf(TEXT("'%s': categoria dichiarata '%s', ma l'ID non la nomina"),
					*Id, *CategoryName(Icon.Category)));
			}
		}

		bool bAlreadySeen = false;
		Seen.Add(Icon.IconId, &bAlreadySeen);
		if (bAlreadySeen)
		{
			Errors.Add(FString::Printf(TEXT("'%s': IconId duplicato"), *Id));
		}

		if (Icon.Asset.IsNull())
		{
			Errors.Add(FString::Printf(TEXT("'%s': asset nullo"), *Id));
		}
	}

	return Errors;
}

FRTIconResolution URTIconLibrary::ResolveIcon(const URTIconCatalogData* Catalog, const FName& IconId,
	const FName& Consumer)
{
	if (Catalog)
	{
		// Scansione lineare: il catalogo core sta nelle decine di voci, e una TMap costruita a ogni chiamata
		// costerebbe piu' della ricerca. Se un giorno le icone diventassero migliaia, l'indice va costruito
		// UNA volta nel catalogo, non qui.
		for (const FRTIconDef& Icon : Catalog->Icons)
		{
			if (Icon.IconId == IconId && !Icon.Asset.IsNull())
			{
				return FRTIconResolution(Icon.Asset, /*bResolved*/ true);
			}
		}
	}

	UE_LOG(LogRT, Warning, TEXT("Icona non risolta: '%s' richiesta da '%s' — mostrata l'icona di fallback"),
		*IconId.ToString(), *Consumer.ToString());

	return FRTIconResolution(Catalog ? Catalog->MissingIcon : TSoftObjectPtr<UTexture2D>(), /*bResolved*/ false);
}
