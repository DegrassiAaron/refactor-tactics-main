#include "Misc/AutomationTest.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapSummary.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CHE COSA CONTIENE la mappa aperta (#1186).
 *
 * 🔑 **Questi test esistono perché la domanda è costata mezza giornata in seduta `U21`.** *«Quanti piani ha
 * questa mappa?»* ha prodotto in sequenza: una misura headless che rispondeva a un'altra domanda
 * (`rt.Arena.Check` riporta le celle e **non** i layer); un numero letto dal Play, che descriveva un'altra
 * mappa; una prenotazione emessa su una necessità inesistente; e un cambio di `MapAsset` della sandbox che
 * nessuno aveva notato.
 *
 * ⛔ **Ciò che il pannello non copre, dichiarato invece che lasciato scoprire**: che i readout si **vedano**
 * nel pannello del mode, e che si aggiornino sotto l'occhio. Nessun automation test di questo repository
 * apre un `UEdMode`. Quella metà è della seduta, e la voce PIE la nomina.
 */

namespace
{
	/** Nomi prefissati per file: namespace anonimo + unity build. */
	URTHexMapAsset* SummaryMakeMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}
}

/**
 * **AC 1** — 🔴 **pannello e libreria non possono divergere**, ed è il criterio 4 del DoD.
 *
 * Il divieto della issue è *«nessuna logica di conteggio nuova: si chiama `GetLayers()` e si legge
 * `Cells.Num()`»*. Un secondo conteggio è una seconda risposta alla stessa domanda, e prima o poi le due
 * si separano — che è il difetto che `U21` ha pagato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapSummaryDoesNotRecountTest,
	"RefactorTactics.Map.Summary.PanelAndLibraryCannotDiverge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapSummaryDoesNotRecountTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = SummaryMakeMap(2);
	if (!TestNotNull(TEXT("la mappa di prova esiste"), Map))
	{
		return false;
	}

	const FRTHexMapSummary S = URTHexMapSummaryLibrary::Summarise(Map, 0);
	TestEqual(TEXT("le celle sono quelle dell'asset, non un secondo conteggio"), S.Cells, Map->NumCells());
	// ⚠️ `TestEqual` NON ha un overload per `TArray` (misurato in `AutomationTest.h`: int32, float,
	// FString, FVector... e basta). Il confronto e' esplicito, e dice anche QUANTI erano.
	TestEqual(TEXT("quanti layer"), S.Layers.Num(), Map->GetLayers().Num());
	TestTrue(TEXT("e sono gli stessi, nello stesso ordine"), S.Layers == Map->GetLayers());
	TestTrue(TEXT("la mappa di prova ha delle celle, o il confronto non verifica niente"), S.Cells > 0);

	// Controllo POSITIVO: aggiungendo una cella su un layer NUOVO, le due fonti si muovono insieme.
	// Senza, un `Summarise` che restituisse costanti passerebbe le asserzioni qui sopra.
	FRTHexCellData Nuova(FRTCellId(0, 0, 3));
	Map->AddOrUpdateCell(Nuova);

	const FRTHexMapSummary Dopo = URTHexMapSummaryLibrary::Summarise(Map, 0);
	TestEqual(TEXT("dopo la scrittura le celle seguono ancora l'asset"), Dopo.Cells, Map->NumCells());
	TestEqual(TEXT("e i layer pure, come quantita'"), Dopo.Layers.Num(), Map->GetLayers().Num());
	TestTrue(TEXT("e come contenuto"), Dopo.Layers == Map->GetLayers());
	TestTrue(TEXT("il layer nuovo e' comparso davvero"), Dopo.Layers.Contains(3));
	TestTrue(TEXT("e ce n'e' uno in piu' di prima"), Dopo.Layers.Num() > S.Layers.Num());

	return true;
}

/**
 * **AC 2** — 🔴 **senza asset si dice che non c'è, non si mostra uno zero**, ed è il criterio 2 del DoD.
 *
 * ⚠️ Non è un caso limite: è la condizione in cui la sandbox si è trovata per un commit intero senza che
 * nessuno se ne accorgesse (`7db11313`, #937, che spostò il `MapAsset` da `DA_HexMap_Arena` a
 * `DA_Format_Scratch`). Uno zero è una misura, e lì non c'era niente da misurare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapSummaryNoAssetSaysSoTest,
	"RefactorTactics.Map.Summary.WithoutAnAssetItSaysSoInsteadOfShowingZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapSummaryNoAssetSaysSoTest::RunTest(const FString&)
{
	const FRTHexMapSummary Senza = URTHexMapSummaryLibrary::Summarise(nullptr, 0);

	TestFalse(TEXT("senza asset, `bHasAsset` lo dice"), Senza.bHasAsset);
	TestEqual(TEXT("e le celle NON si leggono come zero"),
		URTHexMapSummaryLibrary::DescriviCelle(Senza), FString(TEXT("—")));
	TestTrue(TEXT("l'asset si descrive a parole, non con una stringa vuota"),
		URTHexMapSummaryLibrary::DescriviAsset(Senza).Contains(TEXT("nessun asset")));
	TestEqual(TEXT("e nemmeno i layer si leggono come zero"),
		URTHexMapSummaryLibrary::DescriviLayer(Senza), FString(TEXT("—")));

	// 🔑 Il controllo che separa i due «vuoti»: un asset che ESISTE e non ha celle dice una cosa DIVERSA
	// da «non c'e' asset». Senza questo, «mappa vuota» e «nessuna mappa» si leggerebbero uguali — che e'
	// precisamente l'equivoco costato mezza giornata.
	URTHexMapAsset* Vuota = NewObject<URTHexMapAsset>(GetTransientPackage());
	if (!TestNotNull(TEXT("l'asset vuoto esiste"), Vuota))
	{
		return false;
	}
	const FRTHexMapSummary Zero = URTHexMapSummaryLibrary::Summarise(Vuota, 0);
	TestTrue(TEXT("un asset vuoto ha comunque un asset"), Zero.bHasAsset);
	TestEqual(TEXT("e le sue celle sono zero, perche' li' lo zero E' la misura"),
		URTHexMapSummaryLibrary::DescriviCelle(Zero), FString(TEXT("0")));
	TestTrue(TEXT("i due vuoti non si leggono uguali"),
		URTHexMapSummaryLibrary::DescriviLayer(Zero) != URTHexMapSummaryLibrary::DescriviLayer(Senza));

	return true;
}

/**
 * **AC 3** — il segnale su cui il pannello si riaggiorna **si muove davvero**, ed è il criterio 3 del DoD.
 *
 * ⛔ Che il pannello si riscriva lo può dire solo la seduta: nessun automation test apre un `UEdMode`.
 * Ciò che si può provare headless è l'altra metà, ed è quella che il DoD nomina — *«un test lo dimostra
 * sulla **revisione** dell'asset»*: se `Revision` non si muovesse, nessun meccanismo di aggiornamento
 * potrebbe accorgersi del cambiamento, per quanto ben scritto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapSummaryFollowsRevisionTest,
	"RefactorTactics.Map.Summary.TheRevisionMovesWhenTheMapChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapSummaryFollowsRevisionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = SummaryMakeMap(1);
	if (!TestNotNull(TEXT("la mappa di prova esiste"), Map))
	{
		return false;
	}

	const int32 RevPrima = Map->Revision;
	const FRTHexMapSummary Prima = URTHexMapSummaryLibrary::Summarise(Map, 0);

	// Dipingere una cella: la mutazione piu' ordinaria che il pannello deve seguire.
	FRTHexCellData Dipinta(FRTCellId(5, -5, 0));
	Map->AddOrUpdateCell(Dipinta);

	TestTrue(TEXT("la revisione si e' mossa: e' il segnale su cui il pannello si riaggiorna"),
		Map->Revision > RevPrima);

	const FRTHexMapSummary Dopo = URTHexMapSummaryLibrary::Summarise(Map, 0);
	TestTrue(TEXT("e cio' che il pannello mostrerebbe e' cambiato con lei"), Dopo.Cells > Prima.Cells);

	// Controllo NEGATIVO: una lettura che NON cambia nulla non deve muovere la revisione, o il pannello
	// si riscriverebbe di continuo e la chiave non servirebbe a niente.
	const int32 RevDopo = Map->Revision;
	(void)URTHexMapSummaryLibrary::Summarise(Map, 0);
	TestEqual(TEXT("leggere non muove la revisione"), Map->Revision, RevDopo);

	return true;
}

/**
 * **AC 4** — un layer attivo che **non esiste** si distingue da uno popolato.
 *
 * Non è un errore: un piano nuovo si comincia esattamente così. Ma chi guarda deve distinguerlo, o crede
 * di lavorare dove non c'è niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapSummaryActiveLayerTest,
	"RefactorTactics.Map.Summary.AnActiveLayerWithNoCellsSaysSo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapSummaryActiveLayerTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = SummaryMakeMap(1);
	if (!TestNotNull(TEXT("la mappa di prova esiste"), Map))
	{
		return false;
	}

	const FRTHexMapSummary Esiste = URTHexMapSummaryLibrary::Summarise(Map, 0);
	TestTrue(TEXT("il layer 0 esiste nella mappa piatta"), Esiste.bActiveLayerExists);
	TestEqual(TEXT("e si descrive col solo numero"),
		URTHexMapSummaryLibrary::DescriviLayerAttivo(Esiste), FString(TEXT("0")));

	const FRTHexMapSummary Non = URTHexMapSummaryLibrary::Summarise(Map, 7);
	TestFalse(TEXT("il layer 7 non esiste"), Non.bActiveLayerExists);
	TestTrue(TEXT("e la descrizione lo dice invece di tacerlo"),
		URTHexMapSummaryLibrary::DescriviLayerAttivo(Non).Contains(TEXT("nessuna cella")));

	return true;
}


/**
 * CHE COSA DICE IL VALIDATORE, e lo dice a chi sta autorando (#1864, casella 8).
 *
 * 🔴 **Il difetto: nel modulo Editor `ValidateMap` non era chiamata da nessuno.** Misurato il 2026-09-24 —
 * `grep -rn "ValidateMap" Source/RefactorTacticsEditor/` dava un test e cinque commenti, zero chiamate di
 * produzione. La casella 8 chiede *«si rifiuta il gesto **o si segnala**»*: il rifiuto tipizzato esiste
 * (`ERTMapEditOutcome`), il segnalare non aveva un canale.
 *
 * ⛔ **E un `UE_LOG(Warning)` non e' quel canale.** L'Output Log e' un pannello che chi disegna deve avere
 * aperto e scorrere; e' la lezione che il repo ha gia' pagato con `RTMapTemplateValidationHookTests`, dove
 * regole *scritte, testate e verdi* non le eseguiva nessuno. La sede giusta e' il readout del mode, che
 * `#1186` ha stabilito per la stessa ragione: *«un dato che si consulta guardando deve vedersi guardando»*.
 *
 * 🔑 **`ValidateMap()` e NON `ValidateMapDetailed()`, e la differenza e' un fattore sei.** Misurato:
 * `ValidateMap` porta ventitre `Error:` e tre `Warning:` propri **e in coda chiama `ValidateMapDetailed`**,
 * formattandone le voci con lo stesso schema — quindi e' il **superset**. `ValidateMapDetailed` da sola
 * porta cinque regole: un readout costruito su quella direbbe «nessuna segnalazione» su una mappa che
 * `ValidateMap` dichiara in errore per una delle altre.
 *
 * ⚠️ **Il conteggio per PREFISSO non e' un'euristica**: ogni riga che `ValidateMap` produce comincia per
 * `Error: ` o `Warning: `, comprese quelle che arrivano da `ValidateMapDetailed` — che vengono formattate
 * proprio cosi' (`Issue.bIsError ? TEXT("Error") : TEXT("Warning")`). E' la convenzione del validatore, non
 * una lettura di questo file, e `ConteggiCorrispondonoAValidateMap` la pinna.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapSummaryValidationCountsTest,
	"RefactorTactics.Map.Summary.ValidationCountsMatchValidateMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapSummaryValidationCountsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = SummaryMakeMap(2);
	if (!TestNotNull(TEXT("la mappa di prova esiste"), Map)) { return false; }

	// L'allestimento nasce VALIDO: senza questa premessa il test non distinguerebbe «non segnala niente»
	// da «segnala sempre».
	if (!TestEqual(TEXT("l'arena di prova nasce senza segnalazioni"), Map->ValidateMap().Num(), 0))
	{
		return false;
	}
	TestTrue(TEXT("e il readout lo dice a parole, non con uno zero"),
		URTHexMapSummaryLibrary::DescriviValidazione(Map).Contains(TEXT("nessuna")));

	// Un ERRORE: costo negativo su una cella — una delle regole che vivono in `ValidateMap` e che
	// `ValidateMapDetailed` NON porta. E' deliberato: e' il caso che distingue le due funzioni.
	{
		FRTHexCellData Rotta = *Map->FindCell(FRTCellId(0, 0, 0));
		Rotta.MoveCost = -1;
		Map->AddOrUpdateCell(Rotta);
	}

	const TArray<FString> Righe = Map->ValidateMap();
	if (!TestTrue(TEXT("la mappa ora ha almeno una segnalazione"), Righe.Num() > 0))
	{
		return false;
	}

	// 🔑 IL CUORE: i conteggi del readout sono QUELLI di `ValidateMap`, contati sulla sua uscita e non
	// ricavati da una seconda passata sulle regole. Un secondo conteggio e' una seconda risposta alla
	// stessa domanda, ed e' il difetto che `PanelAndLibraryCannotDiverge` gia' presidia per le celle.
	int32 Errori = 0;
	int32 Avvisi = 0;
	for (const FString& R : Righe)
	{
		if (R.StartsWith(TEXT("Error:"))) { ++Errori; }
		else if (R.StartsWith(TEXT("Warning:"))) { ++Avvisi; }
	}

	// CONTROPROVA sulla convenzione: ogni riga ha uno dei due prefissi. Se un giorno una regola ne
	// scrivesse una senza, il conteggio del readout mentirebbe per difetto — e questo test lo dice QUI.
	TestEqual(TEXT("ogni riga di ValidateMap porta il suo prefisso"), Errori + Avvisi, Righe.Num());
	TestTrue(TEXT("e almeno un errore c'e', o il confronto non verifica niente"), Errori > 0);

	const FString Readout = URTHexMapSummaryLibrary::DescriviValidazione(Map);
	TestTrue(TEXT("il readout nomina il numero di errori"),
		Readout.Contains(FString::Printf(TEXT("%d"), Errori)));
	TestFalse(TEXT("e non dice piu' che va tutto bene"), Readout.Contains(TEXT("nessuna")));

	return true;
}

/**
 * SENZA UN ASSET la validazione non risponde «zero»: dice che la domanda non si pone.
 *
 * ⛔ E' il criterio che `#1186` ha reso esplicito per tutti i readout di questo pannello — *«la sandbox si
 * e' trovata con una mappa staccata senza che nessuno se ne accorgesse, e uno zero sembrava una mappa
 * vuota»*. Uno «0 segnalazioni» su una mappa che non c'e' e' la stessa bugia: si legge come «tutto a posto».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMapSummaryValidationWithoutAssetTest,
	"RefactorTactics.Map.Summary.ValidationWithoutAnAssetSaysSoInsteadOfZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMapSummaryValidationWithoutAssetTest::RunTest(const FString&)
{
	const FString Senza = URTHexMapSummaryLibrary::DescriviValidazione(nullptr);

	TestFalse(TEXT("non e' vuoto"), Senza.IsEmpty());
	TestFalse(TEXT("e non dice «nessuna segnalazione», che si leggerebbe come «va tutto bene»"),
		Senza.Contains(TEXT("nessuna segnalazione")));

	// CONTROPROVA: su una mappa vera e valida la frase e' DIVERSA. Senza questa, un `DescriviValidazione`
	// che restituisse sempre la stessa stringa supererebbe entrambe le asserzioni qui sopra.
	URTHexMapAsset* Map = SummaryMakeMap(1);
	if (!TestNotNull(TEXT("la mappa di prova esiste"), Map)) { return false; }
	TestNotEqual(TEXT("una mappa valida si legge in modo diverso da una mappa assente"),
		URTHexMapSummaryLibrary::DescriviValidazione(Map), Senza);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
