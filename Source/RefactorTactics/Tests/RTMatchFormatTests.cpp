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
		Format->FormatVersion = 1;
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
		Format->FormatVersion = 0;
		TestEqual(TEXT("FormatVersion non dichiarata"),
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

#endif // WITH_DEV_AUTOMATION_TESTS
