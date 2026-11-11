// Il gate di catalogo: nessuno scenario dichiara una voce PIE che il registro non ha.
//
// Senza questo, `verifies` diventa un campo di testo libero: un id sbagliato di una lettera comporrebbe
// una coda che nomina una voce inesistente, e chi giudica non la ritroverebbe nel registro dove deve
// scrivere l'esito. E' lo stesso mestiere che `RefactorTactics.Icon.*` fa per il catalogo icone.

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PieSession/RTPieSessionPlaylist.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * Gli ID delle righe-voce del registro.
	 *
	 * ⚠️ Solo le righe che APRONO con `| **PIE-`: il file nomina le voci anche in prosa, nei verbali
	 * datati e nei segmenti ritirati, e contarle tutte farebbe passare un id che non ha una riga sua.
	 */
	TSet<FString> PieSessionVociDelRegistro()
	{
		TSet<FString> Voci;

		FString Registro;
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("docs"), TEXT("technical"),
			TEXT("test-manuali-pie.md"));
		if (!FFileHelper::LoadFileToString(Registro, *Path))
		{
			return Voci;
		}

		TArray<FString> Righe;
		Registro.ParseIntoArrayLines(Righe, /*CullEmpty=*/ false);

		const FString Apertura = TEXT("| **");
		for (const FString& Riga : Righe)
		{
			if (!Riga.StartsWith(TEXT("| **PIE-")))
			{
				continue;
			}
			const int32 Inizio = Apertura.Len();
			const int32 Chiusura = Riga.Find(TEXT("**"), ESearchCase::CaseSensitive,
				ESearchDir::FromStart, Inizio);
			if (Chiusura == INDEX_NONE)
			{
				continue;
			}
			Voci.Add(Riga.Mid(Inizio, Chiusura - Inizio).TrimStartAndEnd());
		}
		return Voci;
	}

	TArray<FString> PieSessionFileScenario()
	{
		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files,
			*FPaths::Combine(FPaths::ProjectDir(), TEXT("Scenarios")), TEXT("*.json"),
			/*Files=*/ true, /*Directories=*/ false);
		return Files;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionRegistryIsReadableTest,
	"RefactorTactics.PieSession.CatalogRegistryIsReadable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionRegistryIsReadableTest::RunTest(const FString&)
{
	// Il controllo positivo del gate qui sotto: se il registro non si leggesse, `Contains` sarebbe
	// falso per tutti e il gate diventerebbe rosso per la ragione sbagliata — oppure, peggio, se il
	// gate fosse scritto al contrario, verde su un insieme vuoto.
	const TSet<FString> Voci = PieSessionVociDelRegistro();
	TestTrue(TEXT("il registro si legge e porta delle voci"), Voci.Num() > 100);
	TestTrue(TEXT("e ne contiene una nota"), Voci.Contains(TEXT("PIE-VIS-SIGHTWALL")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionCatalogNoInventedIdTest,
	"RefactorTactics.PieSession.CatalogDeclaresOnlyRealItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionCatalogNoInventedIdTest::RunTest(const FString&)
{
	const TSet<FString> Registro = PieSessionVociDelRegistro();
	if (Registro.Num() == 0)
	{
		AddError(TEXT("il registro non si legge: il gate non puo' dire niente"));
		return false;
	}

	int32 Dichiarate = 0;
	for (const FString& File : PieSessionFileScenario())
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *File))
		{
			continue;
		}

		TArray<FString> Voci;
		if (!URTPieSessionPlaylist::ReadVerifies(Json, Voci))
		{
			continue;
		}

		for (const FString& Voce : Voci)
		{
			++Dichiarate;
			if (!Registro.Contains(Voce))
			{
				AddError(FString::Printf(
					TEXT("%s dichiara '%s', che nel registro non ha una riga"),
					*FPaths::GetCleanFilename(File), *Voce));
			}
		}
	}

	// ⛔ Senza questa riga il gate sarebbe verde anche su un corpus che non dichiara NIENTE, cioe'
	// proprio nello stato in cui non prova nulla.
	TestTrue(TEXT("almeno uno scenario dichiara delle voci"), Dichiarate > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPieSessionCatalogAmbiguityIsVisibleTest,
	"RefactorTactics.PieSession.CatalogAmbiguityIsVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPieSessionCatalogAmbiguityIsVisibleTest::RunTest(const FString&)
{
	// Una voce dichiarata da due scenari non e' vietata — due allestimenti possono giudicarla — ma la
	// coda non puo' sceglierne uno in silenzio. Qui si rende visibile; a comporre, ferma la coda.
	TMap<FString, TArray<FString>> Dichiaranti;

	for (const FString& File : PieSessionFileScenario())
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *File))
		{
			continue;
		}
		TArray<FString> Voci;
		if (!URTPieSessionPlaylist::ReadVerifies(Json, Voci))
		{
			continue;
		}
		for (const FString& Voce : Voci)
		{
			Dichiaranti.FindOrAdd(Voce).Add(FPaths::GetCleanFilename(File));
		}
	}

	for (const TPair<FString, TArray<FString>>& Coppia : Dichiaranti)
	{
		if (Coppia.Value.Num() > 1)
		{
			AddInfo(FString::Printf(TEXT("%s e' dichiarata da: %s"),
				*Coppia.Key, *FString::Join(Coppia.Value, TEXT(", "))));
		}
	}
	return true;
}

#endif
