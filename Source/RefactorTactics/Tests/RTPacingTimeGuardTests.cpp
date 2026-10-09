#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * LA GUARDIA DEL TEMPO — `#2516`: nessun ramo del resolver legge una DURATA.
 *
 * 🔴 **La promessa che questo test sostituisce era scritta in una issue, e una promessa non ferma
 * nessuno.** La DoD di `#2516` la formula cosi': la guardia *«sul nessun ramo legge il tempo»* richiede
 * una **prova che sappia fallire**, sul modello di `Privacy.GuardDetectsAPlantedLeak`. La ragione e'
 * concreta e sta nella issue: fra sei mesi qualcuno vorra' *«saltare la collection se costa troppo»*, e
 * quel giorno il percorso autoritativo dipenderebbe dall'orologio — cioe' dalla macchina, dal carico,
 * dal frame. Il determinismo e' l'invariante numero due di questo progetto.
 *
 * 🔑 **Due test, e il primo da solo non varrebbe.** Lo sweep di produzione e' verde oggi; un detector
 * cieco sarebbe verde identicamente. E' il secondo a dare significato al primo, e lo fa in **entrambe
 * le direzioni**: pianta un ramo vietato e pretende che venga trovato, pianta le forme lecite e pretende
 * che NON vengano segnalate. Un detector che urla su tutto e' inutile quanto uno che tace.
 *
 * ⚠️ **Il perimetro e' una scelta, e si dichiara.** Lo sweep gira su `Source/RefactorTactics/Turn/` meno
 * il **modulo di pacing** (`RTPacing.h`, `RTPacingLibrary`, `RTPacingRecorder`, `RTPacingConsole`), che
 * quei campi li possiede: il recorder li scrive, la libreria li riassume, la console li stampa, e tutti e
 * tre li leggono per mestiere. `Turn/` e' il perimetro giusto e non uno comodo: `Pacing` e' un membro di
 * `ARTTurnManager`, quindi **solo** i file di questa cartella possono raggiungere il registratore.
 *
 * ⛔ **Cosa questo test NON prova.** Non e' un parser C++: lavora sul testo. In particolare
 * **non vieta di ramificare su `.Num()`**, cioe' su QUANTI campioni esistono — ed e' deliberato, perche'
 * il registratore stesso deve farlo (`RTTurnManager_Movement.cpp`, l'accumulo del boundary). Cio' che
 * vieta e' leggere un **valore**: `[i]`, `.Last()` come rvalue, un confronto con una soglia. Il limite e'
 * reale e sta scritto qui invece che essere scoperto da chi si fidera' del verde.
 */

namespace RTPacingTimeGuard
{
	/**
	 * I campi che portano un TEMPO dentro `FRTPacingSample`.
	 *
	 * ⚠️ Pinnati per **nome** e non per conteggio: aggiungerne uno senza aggiungerlo qui e' esattamente
	 * il modo in cui questa guardia smetterebbe di coprire restando verde, e un totale in prosa non
	 * renderebbe la dimenticanza piu' visibile.
	 */
	static const TCHAR* CampiDiTempo[] = {
		TEXT("CandidateCollectionCpuMs"),
		TEXT("BoundaryCpuMs"),
		TEXT("MsToFirstInput"),
		TEXT("MsToLockIn"),
		TEXT("MsSinceLastInput"),
		TEXT("MsPlayback"),
	};

	/**
	 * Le forme LECITE, come suffisso immediatamente dopo il nome del campo.
	 *
	 * 🔑 **E' una whitelist, e la polarita' e' il punto**: una forma nuova e' una violazione finche'
	 * qualcuno non la dichiara qui. Una blacklist avrebbe la polarita' opposta — tutto lecito tranne
	 * cio' che qualcuno si e' ricordato di vietare — che e' come i gate diventano ciechi.
	 */
	static const TCHAR* FormeLecite[] = {
		TEXT(".Add("),        // scrittura: una misura entra nel campione
		TEXT(".Last() +="),   // accumulo: il boundary cresce attraverso le sospensioni
		TEXT(".Num()"),       // forma, non valore: quanti campioni, non quanto sono durati
		TEXT(".IsEmpty()"),   // idem
	};

	/**
	 * ⛔ **`" ="` NON e' in quella lista, e la tentazione di aggiungerlo e' il buco da non aprire.**
	 * Sembrerebbe innocuo — ammettere l'assegnazione a uno scalare, `MsToLockIn = ...`. Ma il confronto
	 * si scrive `MsToLockIn == ...`, e `StartsWith(" =")` lo accetta: ammettere la scrittura
	 * ammetterebbe **in silenzio** anche il confronto, che e' esattamente cio' che questa guardia esiste
	 * per vietare.
	 *
	 * 🔑 Costa nulla perche' la regola piu' stretta e' gia' vera: **fuori dal modulo di pacing nessuno
	 * scrive questi campi**, e un futuro `Sample.MsPlayback = X;` nel resolver diventa rosso. E' il
	 * verdetto giusto — quella scrittura appartiene al registratore.
	 */

	static bool EUnCarattereDiIdentificatore(TCHAR C)
	{
		return FChar::IsAlnum(C) || C == TEXT('_');
	}

	/** Vero se l'occorrenza a `Pos` sta in un commento: riga che inizia per `//`, `*`, `/*`, o `//` prima. */
	static bool DentroUnCommento(const FString& Testo, int32 Pos)
	{
		int32 InizioRiga = 0;
		for (int32 i = Pos - 1; i >= 0; --i)
		{
			if (Testo[i] == TEXT('\n')) { InizioRiga = i + 1; break; }
		}
		FString Prefisso = Testo.Mid(InizioRiga, Pos - InizioRiga);
		if (Prefisso.Contains(TEXT("//"))) { return true; }
		Prefisso.TrimStartInline();
		return Prefisso.StartsWith(TEXT("*")) || Prefisso.StartsWith(TEXT("/*"));
	}

	/** Il numero di riga 1-based dell'offset, per un messaggio che si possa aprire nell'editor. */
	static int32 RigaDi(const FString& Testo, int32 Pos)
	{
		int32 Riga = 1;
		for (int32 i = 0; i < Pos && i < Testo.Len(); ++i)
		{
			if (Testo[i] == TEXT('\n')) { ++Riga; }
		}
		return Riga;
	}

	/**
	 * IL DETECTOR. Ogni occorrenza di un campo di tempo e' una violazione, salvo le forme dichiarate
	 * lecite. `OutLecite` conta quelle consentite: serve all'anti-vacuita' del chiamante.
	 */
	static void TrovaLettureDiTempo(const FString& Testo, const FString& Etichetta,
		TArray<FString>& OutViolazioni, int32& OutLecite)
	{
		for (const TCHAR* Campo : CampiDiTempo)
		{
			const FString Nome(Campo);
			int32 Pos = Testo.Find(Nome, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
			while (Pos != INDEX_NONE)
			{
				const int32 Fine = Pos + Nome.Len();

				// ⚠️ Il carattere PRIMA decide se e' davvero questo campo: `MsToLockIn` e'
				// sottostringa di `MedianMsToLockIn`, e `BoundaryCpuMs` di `P90BoundaryCpuMs`. Senza
				// questo controllo la guardia segnalerebbe i campi del SOMMARIO, che sono un'altra cosa.
				const bool bConfineSinistro =
					Pos == 0 || !EUnCarattereDiIdentificatore(Testo[Pos - 1]);
				const bool bConfineDestro =
					Fine >= Testo.Len() || !EUnCarattereDiIdentificatore(Testo[Fine]);

				if (bConfineSinistro && bConfineDestro && !DentroUnCommento(Testo, Pos))
				{
					const FString Resto = Testo.Mid(Fine, 16);
					bool bLecita = false;
					for (const TCHAR* Forma : FormeLecite)
					{
						if (Resto.StartsWith(Forma, ESearchCase::CaseSensitive)) { bLecita = true; break; }
					}
					if (bLecita)
					{
						++OutLecite;
					}
					else
					{
						OutViolazioni.Add(FString::Printf(TEXT("%s:%d — `%s%s`"),
							*Etichetta, RigaDi(Testo, Pos), *Nome, *Resto.TrimEnd()));
					}
				}

				Pos = Testo.Find(Nome, ESearchCase::CaseSensitive, ESearchDir::FromStart, Fine);
			}
		}
	}
}

/**
 * LA MISURA. Nessun file del resolver legge una durata.
 *
 * Given i sorgenti di `Turn/` meno il modulo di pacing
 * When si cerca ogni occorrenza dei campi di tempo di `FRTPacingSample`
 * Then ognuna e' una forma di scrittura dichiarata, e nessuna e' la lettura di un valore
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTResolverNeverBranchesOnATimeTest,
	"RefactorTactics.Pacing.ResolverNeverBranchesOnATime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTResolverNeverBranchesOnATimeTest::RunTest(const FString&)
{
	using namespace RTPacingTimeGuard;

	const FString Radice = FPaths::Combine(FPaths::ProjectDir(),
		TEXT("Source"), TEXT("RefactorTactics"), TEXT("Turn"));

	TArray<FString> Files;
	for (const TCHAR* Ext : { TEXT("*.cpp"), TEXT("*.h") })
	{
		TArray<FString> Trovati;
		IFileManager::Get().FindFilesRecursive(Trovati, *Radice, Ext, /*Files=*/ true, /*Dirs=*/ false,
			/*bClearFileNames=*/ false);
		Files.Append(Trovati);
	}

	// ⛔ FAIL-LOUD, non fail-silent. Zero file letti significa che il percorso e' sbagliato, non che il
	// resolver e' pulito — e un gate che tace quando non puo' misurare produce un verde.
	if (!TestTrue(TEXT("lo sweep ha trovato i sorgenti di Turn/"), Files.Num() > 0))
	{
		AddError(FString::Printf(TEXT("radice cercata: %s"), *Radice));
		return false;
	}

	TArray<FString> Violazioni;
	int32 Lecite = 0;
	bool bVistoIlMovimento = false;
	bool bVistoIlResolverPuro = false;

	for (const FString& F : Files)
	{
		const FString Nome = FPaths::GetCleanFilename(F);

		// Il modulo di pacing POSSIEDE questi campi: li scrive, li riassume, li stampa. Escluderlo non e'
		// una concessione, e' la definizione del perimetro.
		if (Nome.StartsWith(TEXT("RTPacing"))) { continue; }

		if (Nome == TEXT("RTTurnManager_Movement.cpp")) { bVistoIlMovimento = true; }
		if (Nome == TEXT("RTHexSimLibrary.cpp")) { bVistoIlResolverPuro = true; }

		FString Testo;
		if (!FFileHelper::LoadFileToString(Testo, *F)) { continue; }
		TrovaLettureDiTempo(Testo, Nome, Violazioni, Lecite);
	}

	// ⛔ L'ANTI-VACUITA', e sono tre controlli perche' questo sweep puo' diventare cieco in tre modi
	// diversi: il percorso cambia, i due file che contano spariscono dal perimetro, oppure i campi
	// vengono rinominati e la ricerca non trova piu' niente restando verde.
	TestTrue(TEXT("il file che innesta i cronometri e' dentro il perimetro"), bVistoIlMovimento);
	TestTrue(TEXT("il resolver puro e' dentro il perimetro"), bVistoIlResolverPuro);
	TestTrue(TEXT("i campi di tempo esistono davvero nella superficie spazzata"), Lecite > 0);

	for (const FString& V : Violazioni)
	{
		AddError(FString::Printf(TEXT("il resolver legge un valore di tempo: %s"), *V));
	}
	TestEqual(TEXT("nessun ramo del resolver legge una durata"), Violazioni.Num(), 0);
	return true;
}

/**
 * IL CONTROLLO POSITIVO. Senza questo test, il precedente non distingue «nessuna lettura» da
 * «detector cieco» — ed e' la prova che `#2516` chiede esplicitamente.
 *
 * Given un testo che contiene sia le forme lecite sia due rami vietati
 * When lo si passa allo STESSO detector dello sweep di produzione
 * Then trova esattamente i due rami vietati, li NOMINA, e lascia stare le forme lecite
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTimeGuardDetectsAPlantedBranchTest,
	"RefactorTactics.Pacing.TimeGuardDetectsAPlantedBranch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTimeGuardDetectsAPlantedBranchTest::RunTest(const FString&)
{
	using namespace RTPacingTimeGuard;

	// Il ramo che la issue teme, scritto per esteso: «saltare la collection se costa troppo».
	// Accanto, le forme lecite che il registratore usa davvero — perche' il controllo deve provare
	// ENTRAMBE le direzioni, non solo che il detector sa urlare.
	const FString Piantato = FString::Join(TArray<FString>{
		TEXT("void Finto()"),
		TEXT("{"),
		TEXT("\tPacing.Current().CandidateCollectionCpuMs.Add(RaccoltaMs);"),
		TEXT("\tif (bMisura && Pacing.Current().BoundaryCpuMs.Num() > 0)"),
		TEXT("\t{"),
		TEXT("\t\tPacing.Current().BoundaryCpuMs.Last() += Delta;"),
		TEXT("\t}"),
		TEXT("\t// BoundaryCpuMs in un commento non e' un ramo, e non deve contare."),
		TEXT("\tif (Pacing.Current().CandidateCollectionCpuMs.Last() > 2.0)"),
		TEXT("\t{"),
		TEXT("\t\treturn; // saltare la collection perche' costa troppo"),
		TEXT("\t}"),
		TEXT("\tconst double Costo = Sample.BoundaryCpuMs[0];"),
		TEXT("}"),
	}, TEXT("\n"));

	TArray<FString> Violazioni;
	int32 Lecite = 0;
	TrovaLettureDiTempo(Piantato, TEXT("Piantato.cpp"), Violazioni, Lecite);

	// Direzione 1: il detector VEDE i due rami vietati.
	if (!TestEqual(TEXT("la guardia trova entrambi i rami piantati"), Violazioni.Num(), 2))
	{
		for (const FString& V : Violazioni) { AddInfo(FString::Printf(TEXT("trovata: %s"), *V)); }
		return false;
	}

	// E li NOMINA: una violazione che dice solo «esiste» non e' azionabile.
	TestTrue(TEXT("la prima violazione nomina il confronto con la soglia"),
		Violazioni[0].Contains(TEXT("CandidateCollectionCpuMs.Last() >")));
	TestTrue(TEXT("la seconda violazione nomina l'indicizzazione"),
		Violazioni[1].Contains(TEXT("BoundaryCpuMs[0]")));

	// Direzione 2: il detector LASCIA STARE le forme lecite — `.Add(`, `.Num()` dentro un `if`,
	// `.Last() +=` e la menzione in un commento. Senza questa meta', una guardia che segnala tutto
	// passerebbe il test sopra e renderebbe lo sweep di produzione impossibile da tenere verde.
	TestEqual(TEXT("le forme lecite sono riconosciute, non segnalate"), Lecite, 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
