#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTLegacyIdentity
{
	/** Un'identita' ritirata, col tetto di file che oggi la nominano. */
	struct FRitirata
	{
		const TCHAR* Id;             // il token, **col prefisso `Hero.`**
		const TCHAR* Erede;
		int32 TettoSource;           // file `.cpp`/`.h` sotto `Source/`
		int32 TettoScenari;          // file `.json` sotto `Scenarios/`
	};

	/**
	 * 🔴 **I tetti sono una MISURA del 2026-09-09, non un obiettivo**, e scendono a ogni fetta del rename.
	 * Vanno abbassati **insieme** alla fetta che li fa scendere, mai prima: un tetto piu' basso del reale e'
	 * un rosso che non nomina un difetto.
	 */
	const FRitirata Ritirate[] = {
		{ TEXT("Hero.Gadget"), TEXT("Hero.Aevik"),  60, 97 },
		// ✅ **Fetta eseguita il 2026-09-09**: il tetto è sceso a ZERO, e per questa identità il ratchet
		// **è già** l'oracolo secco che [D-341] chiede — qualunque ricomparsa fallisce.
		// ⚠️ Zero e non quattro come `Hero.Riktor`: quelle quattro sono commenti sul rename e un test che ne
		// verifica la scomparsa. Qui non esistono ancora — se qualcuno li scrivesse, il tetto va alzato
		// **con loro**, non prima.
		{ TEXT("Hero.Wraith"), TEXT("Hero.Ivrin"),   0,  0 },
		{ TEXT("Hero.Phase"),  TEXT("Hero.Muiren"), 35, 44 },
		// ⌫ Gia' rinominata (`D-334`), e il suo tetto **non e' zero**: le quattro occorrenze residue sono
		// menzioni LEGITTIME — tre commenti che spiegano il rename e un test che verifica che l'identita' non
		// si risolva piu'. E' la misura di cosa resta quando una fetta e' completa.
		{ TEXT("Hero.Riktor"), TEXT("Hero.Branth"),  4,  0 },
	};

	/**
	 * Conta, in **una sola passata**, quanti file di una radice contengono ciascun token.
	 *
	 * 🔴 **Una passata e non una per identita', ed e' una correzione misurata.** La prima stesura leggeva
	 * l'intero albero una volta **per token**: con quattro identita' significava aprire ogni `.cpp` e ogni `.h`
	 * di `Source/` quattro volte, e la suite completa ha superato i **dieci minuti** invece dei soliti
	 * quaranta secondi. Un test che misura il debito non deve diventarne uno.
	 */
	void ContaTokenPerFile(const FString& Radice, const TArray<FString>& Estensioni,
		const TArray<FString>& Tokens, const FString& DaEscludere, TArray<int32>& OutConteggi)
	{
		OutConteggi.Init(0, Tokens.Num());

		TArray<FString> Files;
		for (const FString& Ext : Estensioni)
		{
			TArray<FString> Trovati;
			IFileManager::Get().FindFilesRecursive(Trovati, *Radice, *Ext, /*Files=*/ true, /*Dirs=*/ false,
				/*bClearFileNames=*/ false);
			Files.Append(Trovati);
		}

		for (const FString& F : Files)
		{
			if (!DaEscludere.IsEmpty() && F.Contains(DaEscludere)) { continue; }
			FString Testo;
			if (!FFileHelper::LoadFileToString(Testo, *F)) { continue; }
			for (int32 i = 0; i < Tokens.Num(); ++i)
			{
				if (Testo.Contains(Tokens[i], ESearchCase::CaseSensitive)) { ++OutConteggi[i]; }
			}
		}
	}
}

/**
 * **La superficie del rename non deve crescere mentre la migrazione aspetta.**
 *
 * 🔑 **Questo e' l'oracolo che [D-341] chiede, con la soglia che parte da dove siamo.** La decisione lo
 * formula come *«un oracolo che fallisca se un'identita' legacy **ricompare** in `Source/` o `Scenarios/`»* —
 * ma scritto cosi' sarebbe **rosso oggi**, perche' le identita' ritirate non sono ricomparse: **non se ne
 * sono ancora andate**. Un oracolo di quella forma puo' nascere solo DOPO il rename, e quindi non
 * proteggerebbe niente **proprio nella finestra** in cui `D-341` (6) avverte che *«ogni lavoro eseguito prima
 * le aggiunge superficie»*.
 *
 * Questo test e' lo **stesso** oracolo con una soglia mobile:
 *
 * - **oggi** impedisce che la superficie si allarghi mentre il rename aspetta;
 * - **durante** il rename ogni fetta abbassa il proprio tetto, e la discesa e' una **misura** registrata nel
 *   commit che la produce, non una promessa;
 * - **a fine rename** i tetti sono al minimo e il ratchet **diventa** l'oracolo di `D-341`, senza riscritture.
 *
 * ⚠️ **Il minimo non e' zero, ed e' gia' misurato.** `Hero.Riktor` e' rinominata da `D-334` e nomina ancora
 * **quattro** file: tre commenti che spiegano il rename e un test che verifica che quell'identita' non si
 * risolva piu'. Sono menzioni **legittime** e devono restare — un tetto a zero le vieterebbe, cioe' vieterebbe
 * di documentare il proprio passato.
 *
 * ⛔ **Col prefisso `Hero.`, e non senza**, ed e' la trappola che `D-337` e `D-341` hanno gia' pagato due
 * volte: `Gadget.Sprinkler` e' **equipaggiamento** — il namespace che `D-120` ha separato — e un selettore
 * senza prefisso lo conterebbe come identita'. Nel corpus golden quella svista fa la differenza fra 8 file e 7.
 *
 * ⛔ **Cosa questo test NON fa**: non verifica che il rename sia corretto, ne' che gli eredi siano usati bene.
 * Verifica una sola cosa — che il debito non cresca.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLegacyIdentityRatchetTest,
	"RefactorTactics.Meta.LegacyHeroIdentitiesDoNotSpread",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLegacyIdentityRatchetTest::RunTest(const FString&)
{
	using namespace RTLegacyIdentity;

	const FString RadiceSource    = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"));
	const FString RadiceScenari   = FPaths::Combine(FPaths::ProjectDir(), TEXT("Scenarios"));
	// Questo file nomina le identita' ritirate per mestiere: contarlo sarebbe contare il termometro.
	const FString QuestoFile      = TEXT("RTLegacyIdentityRatchetTests");

	const TArray<FString> EstSource  = { TEXT("*.cpp"), TEXT("*.h") };
	const TArray<FString> EstScenari = { TEXT("*.json") };

	TArray<FString> Tokens;
	for (const FRitirata& R : Ritirate) { Tokens.Add(R.Id); }

	TArray<int32> InSource, InScenari;
	ContaTokenPerFile(RadiceSource,  EstSource,  Tokens, QuestoFile, InSource);
	ContaTokenPerFile(RadiceScenari, EstScenari, Tokens, FString(), InScenari);

	bool bQualcosaMisurato = false;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Ritirate); ++i)
	{
		const FRitirata& R = Ritirate[i];
		bQualcosaMisurato |= (InSource[i] + InScenari[i]) > 0;

		AddInfo(FString::Printf(TEXT("%s -> %s : Source %d/%d, Scenari %d/%d"),
			R.Id, R.Erede, InSource[i], R.TettoSource, InScenari[i], R.TettoScenari));

		if (InSource[i] > R.TettoSource)
		{
			AddError(FString::Printf(
				TEXT("`%s` e' un'identita' RITIRATA (erede: `%s`) e la sua superficie in `Source/` e' CRESCIUTA: ")
				TEXT("%d file contro un tetto di %d. Il rename di [D-341] costa di piu' a ogni riferimento nuovo. ")
				TEXT("Usa `%s`; se l'aumento e' voluto, abbassa il tetto NELLA STESSA PR e di' perche'."),
				R.Id, R.Erede, InSource[i], R.TettoSource, R.Erede));
		}
		if (InScenari[i] > R.TettoScenari)
		{
			AddError(FString::Printf(
				TEXT("`%s` e' un'identita' RITIRATA (erede: `%s`) e la sua superficie in `Scenarios/` e' ")
				TEXT("CRESCIUTA: %d file contro un tetto di %d. Usa `%s`."),
				R.Id, R.Erede, InScenari[i], R.TettoScenari, R.Erede));
		}

		// 🔵 Il ratchet ha un secondo mestiere: dire quando un tetto e' STANTIO. Un conteggio sceso sotto il
		// proprio tetto non e' un difetto — e' una fetta di rename avvenuta e non registrata qui.
		if (InSource[i] < R.TettoSource || InScenari[i] < R.TettoScenari)
		{
			AddInfo(FString::Printf(
				TEXT("il tetto di `%s` e' SCESO (Source %d<%d, Scenari %d<%d): una fetta e' avvenuta. ")
				TEXT("Abbassare i tetti qui rende la discesa irreversibile."),
				R.Id, InSource[i], R.TettoSource, InScenari[i], R.TettoScenari));
		}
	}

	// ⚠️ La premessa: se la scansione non trovasse **nulla**, il verde direbbe che il rename e' finito mentre
	// direbbe soltanto che il test non ha letto niente. E' la stessa guardia anti-degenere di
	// `Meta.PlayerMoveDecayOnTheShowcase`.
	TestTrue(TEXT("premessa: la scansione ha letto qualcosa (un verde a zero file sarebbe vacuo)"),
		bQualcosaMisurato);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
