#include "Misc/AutomationTest.h"
#include "Turn/RTMatchFormatData.h"
#include "Turn/RTMatchFormatLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Un formato STRUTTURALMENTE valido, che ogni test rompe in un punto solo: cosi' un errore di
	 * validazione dice quale campo lo ha causato, non "il formato e' sbagliato da qualche parte".
	 */
	URTMatchFormatData* MakeValidFormat()
	{
		URTMatchFormatData* Format = NewObject<URTMatchFormatData>();
		Format->FormatId = FName(TEXT("Format.Test2v2"));
		Format->RoundLimit = 12;
		Format->ExpectedRounds = 10;
		Format->ScoreToWin = 0;
		return Format;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatValidatorTest,
	"RefactorTactics.MatchFormat.ValidatorRejectsInvalidAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatValidatorTest::RunTest(const FString&)
{
	TestEqual(TEXT("il formato valido non produce errori"),
		URTMatchFormatLibrary::ValidateFormat(MakeValidFormat()).Num(), 0);

	{
		URTMatchFormatData* Format = MakeValidFormat();
		Format->RoundLimit = 0;
		TestEqual(TEXT("RoundLimit 0: una partita che non puo' finire per limite"),
			URTMatchFormatLibrary::ValidateFormat(Format).Num(), 1);
	}
	{
		URTMatchFormatData* Format = MakeValidFormat();
		Format->RoundLimit = -1;
		TestEqual(TEXT("RoundLimit negativo"),
			URTMatchFormatLibrary::ValidateFormat(Format).Num(), 1);
	}
	{
		// Contraddittorio E silenzioso: la partita finirebbe sistematicamente prima della durata attesa,
		// e nessun sintomo direbbe che il formato lo aveva gia' dichiarato.
		URTMatchFormatData* Format = MakeValidFormat();
		Format->ExpectedRounds = 20;
		Format->RoundLimit = 12;
		TestEqual(TEXT("round attesi oltre il limite"),
			URTMatchFormatLibrary::ValidateFormat(Format).Num(), 1);
	}
	{
		// Senza identita' il marcatore nell'header del TurnLog e' muto, ed e' l'unica ragione per cui esiste.
		URTMatchFormatData* Format = MakeValidFormat();
		Format->FormatId = NAME_None;
		TestEqual(TEXT("FormatId assente"),
			URTMatchFormatLibrary::ValidateFormat(Format).Num(), 1);
	}
	{
		URTMatchFormatData* Format = MakeValidFormat();
		Format->ScoreToWin = -1;
		TestEqual(TEXT("soglia di punteggio negativa"),
			URTMatchFormatLibrary::ValidateFormat(Format).Num(), 1);
	}
	{
		// Piu' errori insieme: il validator li elenca tutti, non si ferma al primo (stessa disciplina di
		// ValidateHeroes: chi corregge l'asset vuole la lista, non un errore per volta).
		URTMatchFormatData* Format = MakeValidFormat();
		Format->RoundLimit = 0;
		Format->FormatId = NAME_None;
		TestEqual(TEXT("due errori, due voci"),
			URTMatchFormatLibrary::ValidateFormat(Format).Num(), 2);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatRefusesWithoutFormatTest,
	"RefactorTactics.MatchFormat.LibraryRefusesWithoutFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatRefusesWithoutFormatTest::RunTest(const FString&)
{
	// La libreria pura non ripiega MAI: il ripiego e' una politica, e le politiche vivono nell'Actor
	// (stessa separazione di MakeDemoArena, che ritorna una mappa senza decidere di usarla).
	{
		FRTMatchRules Rules;
		Rules.RoundLimit = 7; // valore sentinella: un rifiuto non deve toccarlo
		FString Reason;
		TestFalse(TEXT("formato assente -> rifiuto"),
			URTMatchFormatLibrary::ResolveRules(nullptr, Rules, Reason));
		TestFalse(TEXT("il rifiuto dice perche'"), Reason.IsEmpty());
		TestEqual(TEXT("le regole in uscita non sono state toccate"), Rules.RoundLimit, 7);
	}
	{
		// Contenuto sbagliato: rifiuto anche qui. Il ripiego di D1 copre l'ASSENZA del formato, non un
		// formato presente e invalido — quello va corretto, non aggirato.
		URTMatchFormatData* Format = MakeValidFormat();
		Format->RoundLimit = 0;
		FRTMatchRules Rules;
		FString Reason;
		TestFalse(TEXT("formato invalido -> rifiuto"),
			URTMatchFormatLibrary::ResolveRules(Format, Rules, Reason));
		TestFalse(TEXT("il rifiuto dice perche'"), Reason.IsEmpty());
	}
	{
		URTMatchFormatData* Format = MakeValidFormat();
		FRTMatchRules Rules;
		FString Reason;
		TestTrue(TEXT("formato valido -> risolto"),
			URTMatchFormatLibrary::ResolveRules(Format, Rules, Reason));
		TestEqual(TEXT("il RoundLimit arriva dall'asset"), Rules.RoundLimit, 12);
		TestEqual(TEXT("l'identita' arriva dall'asset"), Rules.FormatId, FName(TEXT("Format.Test2v2")));
		TestEqual(TEXT("la soglia arriva dall'asset"), Rules.ScoreToWin, 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatShippedIsValidTest,
	"RefactorTactics.MatchFormat.ShippedSkirmish2v2IsValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatShippedIsValidTest::RunTest(const FString&)
{
	// Il formato canonico della v0.1 vive in C++ come eroi e azioni (#375), non in un `.uasset` che qualcuno
	// deve ricordarsi di creare: senza, il pacchettizzato gira sul ripiego, ed e' cio' che CP 12.5 ha misurato.
	URTMatchFormatData* Shipped =
		URTMatchFormatLibrary::FindShippedFormat(URTMatchFormatLibrary::Skirmish2v2FormatId);
	if (!TestNotNull(TEXT("Format.Skirmish2v2 e' spedito col gioco"), Shipped)) { return false; }

	TestEqual(TEXT("identita'"), Shipped->FormatId, URTMatchFormatLibrary::Skirmish2v2FormatId);
	TestNotEqual(TEXT("e NON e' l'identita' del ripiego"),
		Shipped->FormatId, URTMatchFormatLibrary::FallbackFormatId);
	// 12 e' il centro dei **10-14 in 2v2** che D-010 consolida. Era 5 fino al 2026-08-10: un valore da test
	// che contraddiceva quella decisione, falsificato dal primo playtest al PIE (partita chiusa in pareggio
	// allo scadere dei round con una squadra in vantaggio 2 contro 1).
	TestEqual(TEXT("RoundLimit 12, allineato a D-010"), Shipped->RoundLimit, 12);
	// I round ATTESI stanno sotto il limite, e non ci coincidono: con 12 la fine attesa torna a essere
	// l'eliminazione — 10 e' il dato misurato headless il 2026-08-06 — e il limite la rete dietro di essa.
	// Il validator rifiuta solo il caso opposto, round attesi OLTRE il limite.
	TestEqual(TEXT("ExpectedRounds 10, sotto il limite e non uguale"), Shipped->ExpectedRounds, 10);
	TestTrue(TEXT("i round attesi restano entro il limite"), Shipped->ExpectedRounds <= Shipped->RoundLimit);
	TestEqual(TEXT("due unita' per squadra: e' il 2v2 del vertical slice"), Shipped->UnitsPerTeam, 2);
	TestEqual(TEXT("classe di mappa Skirmish"), Shipped->MapClass, ERTMapClass::Skirmish);

	// La soglia e' ZERO perche' nessuno assegna punti: `AddTeamScore` non ha chiamanti runtime. Se un giorno
	// un obiettivo la alimentera', questo assert cade — ed e' il momento in cui la soglia va scelta davvero,
	// non prima. Fissarla a un numero adesso dichiarerebbe una via di vittoria irraggiungibile.
	TestEqual(TEXT("soglia obiettivo 0 finche' nessuno assegna punti"), Shipped->ScoreToWin, 0);

	// Deve superare lo STESSO validator degli asset dei designer: un formato spedito che non passa la propria
	// validazione e' un difetto di codice, e il gioco lo rifiuterebbe all'avvio.
	const TArray<FString> Errors = URTMatchFormatLibrary::ValidateFormat(Shipped);
	TestEqual(TEXT("il formato spedito e' valido secondo il suo validator"), Errors.Num(), 0);
	for (const FString& E : Errors) { AddError(E); }

	// Un id sconosciuto non produce un formato inventato: ritorna nullptr e decide il chiamante.
	TestNull(TEXT("un id sconosciuto non e' spedito"),
		URTMatchFormatLibrary::FindShippedFormat(FName(TEXT("Format.NonEsiste"))));
	TestEqual(TEXT("un solo formato spedito in v0.1"), URTMatchFormatLibrary::ShippedFormatIds().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatFallbackRulesAreValidTest,
	"RefactorTactics.MatchFormat.FallbackRulesAreValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatFallbackRulesAreValidTest::RunTest(const FString&)
{
	const FRTMatchRules Fallback = URTMatchFormatLibrary::MakeFallbackRules();

	// D1: il ripiego esiste ovunque, anche in build packaged. Proprio per questo l'onere si sposta tutto
	// sull'osservabilita': un ripiego che si presentasse con l'identita' di un formato reale (o senza
	// identita') farebbe attribuire i numeri del playtest a un formato fantasma.
	TestNotEqual(TEXT("il ripiego ha un'identita' dichiarata"), Fallback.FormatId, FName(NAME_None));
	TestEqual(TEXT("ed e' l'identita' riservata al ripiego"),
		Fallback.FormatId, URTMatchFormatLibrary::FallbackFormatId);
	TestNotEqual(TEXT("che nessun formato reale puo' usare"),
		Fallback.FormatId, MakeValidFormat()->FormatId);

	// Il ripiego resta una partita giocabile: il valore iniziale del 2v2 v0.1 (banda 10-14, spec §6).
	TestEqual(TEXT("RoundLimit di ripiego"), Fallback.RoundLimit, 12);

	// E deve superare lo stesso validator degli asset: un ripiego che non passa la propria validazione
	// sarebbe una regola che il gioco applica ma non accetterebbe da un designer.
	FString Reason;
	TestTrue(TEXT("il ripiego e' valido secondo le stesse regole"),
		URTMatchFormatLibrary::AreRulesUsable(Fallback, Reason));

	return true;
}

/**
 * CP 19.2 — la composizione e' un campo del FORMATO, non una costante dell'orchestratore.
 *
 * `Format.Skirmish2v2` dichiara 2. Il valore attraversa `ResolveRules` e arriva alle regole in vigore: senza
 * quel passaggio il campo esisterebbe sull'asset e nessuno lo leggerebbe, che e' il difetto ricorrente di
 * questo repository — il dato senza consumatore.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatDeclaresUnitsPerTeamTest,
	"RefactorTactics.MatchFormat.DeclaresUnitsPerTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatDeclaresUnitsPerTeamTest::RunTest(const FString&)
{
	URTMatchFormatData* Skirmish = MakeValidFormat();
	Skirmish->FormatId = FName(TEXT("Format.Skirmish2v2"));
	Skirmish->UnitsPerTeam = 2;

	FRTMatchRules Rules;
	FString Reason;
	TestTrue(TEXT("il formato si risolve"), URTMatchFormatLibrary::ResolveRules(Skirmish, Rules, Reason));
	TestEqual(TEXT("Format.Skirmish2v2 dichiara 2 unita' per squadra"), Rules.UnitsPerTeam, 2);

	// Un formato che ne dichiara quattro e' altrettanto legittimo: e' il 4v4 di E17, che smette di essere un
	// ramo del `GameMode` e diventa un dato. Se questo non passasse, «formato dichiarato» sarebbe una parola.
	URTMatchFormatData* Stress = MakeValidFormat();
	Stress->FormatId = FName(TEXT("Format.Stress4v4"));
	Stress->UnitsPerTeam = 4;
	TestTrue(TEXT("anche un 4v4 si risolve"), URTMatchFormatLibrary::ResolveRules(Stress, Rules, Reason));
	TestEqual(TEXT("con quattro unita' per squadra"), Rules.UnitsPerTeam, 4);

	// Zero non e' «nessun limite» come per `ScoreToWin`: e' una squadra vuota, e il validator lo rifiuta
	// nominando il campo.
	URTMatchFormatData* Empty = MakeValidFormat();
	Empty->UnitsPerTeam = 0;
	const TArray<FString> Errors = URTMatchFormatLibrary::ValidateFormat(Empty);
	TestTrue(TEXT("una composizione vuota e' un errore"), Errors.Num() > 0);

	bool bNamesTheField = false;
	for (const FString& E : Errors)
	{
		bNamesTheField = bNamesTheField || E.Contains(TEXT("UnitsPerTeam"));
	}
	TestTrue(TEXT("e l'errore nomina il campo colpevole"), bNamesTheField);

	return true;
}

/**
 * **Un numero di versione non torna su questa classe senza la macchina che lo regge** (`D-141`, #844).
 *
 * ## Perche' un test che pinna un'ASSENZA
 *
 * Questo test non difende un comportamento — non c'e' comportamento da difendere. Difende una **decisione**,
 * ed e' l'asimmetria che il panel di revisione ha trovato: #687 ha chiuso il proprio difetto lasciando tre
 * test e uno `static_assert`; `D-141` lo ha chiuso **rimuovendo** un campo e non lasciando niente. Rimuovere
 * sembra non richiedere protezione, e qui non e' vero: il giorno in cui qualcuno crea il formato 3v3 e pensa
 * *«serve una versione»*, `int32 FormatVersion = 1` rientra in una riga e nulla diventa rosso.
 *
 * `URTMatchFormatData` aveva quel campo con il **vocabolario** di una versione e nessuna macchina: niente
 * `CurrentFormatVersion`, niente `MigrateToCurrentFormat`, niente `PostLoad`, e nessuno che lo scrivesse.
 * Un numero di versione **da solo** non protegge niente — produce l'illusione di una rete, che e' cio' che
 * #687 ha pagato per otto versioni su `URTHexMapAsset`.
 *
 * ## Quando questo test va aggiornato, e come
 *
 * ⚠️ **Non e' un divieto perpetuo, ed e' deliberato che sia rosso e non un commento**: il giorno in cui un
 * formato **autorato come asset** dovra' sopravvivere a un campo che cambia significato, la versione torna —
 * ma torna **insieme** al meccanismo (`FCustomVersionRegistry`, come `Map/RTHexMapCustomVersion.h`, che ha
 * gia' il suo test a due binari) e al primo passo di migrazione. Allora questo test si aggiorna nello stesso
 * commit, che e' esattamente il punto: costringe chi lo riaggiunge a leggere **perche'** era stato tolto.
 *
 * E' lo stesso ruolo che #830 ha scelto per i default di `MapClass`/`HexSize`: il test non impedisce il
 * cambio, lo rende **rosso**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMatchFormatHasNoOrphanVersionTest,
	"RefactorTactics.MatchFormat.NoVersionFieldWithoutItsMachinery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMatchFormatHasNoOrphanVersionTest::RunTest(const FString&)
{
	const UClass* FormatClass = URTMatchFormatData::StaticClass();

	// Il nome si cerca per stringa **di proposito**: se fosse un riferimento simbolico, questo test non
	// compilerebbe nel momento stesso in cui deve diventare rosso.
	TestNull(
		TEXT("URTMatchFormatData non dichiara FormatVersion: un numero di versione senza migrazione ne' "
			 "lato-scrittura promette una rete che non esiste (D-141, #844) — se lo stai riaggiungendo, "
			 "portalo con FCustomVersionRegistry e il suo passo di migrazione, poi aggiorna questo test"),
		FormatClass->FindPropertyByName(TEXT("FormatVersion")));

	// La controprova che il test guarda la classe giusta e con lo strumento giusto: un campo che ESISTE
	// dev'essere trovato dalla stessa chiamata. Senza, un errore di nome o di classe renderebbe la riga
	// sopra verde per sempre — e sarebbe una guardia che misura se stessa.
	TestNotNull(TEXT("controprova: la reflection su questa classe funziona (FormatId esiste)"),
		FormatClass->FindPropertyByName(TEXT("FormatId")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
