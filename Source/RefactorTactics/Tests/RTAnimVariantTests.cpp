#include "Misc/AutomationTest.h"

#include "Unit/RTUnitAnimInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Una clip finta: qui conta l'IDENTITA' del path, non che risolva. Nessun pack Paragon richiesto. */
	TSoftObjectPtr<UAnimSequenceBase> ClipFinta(const TCHAR* Nome)
	{
		return TSoftObjectPtr<UAnimSequenceBase>(
			FSoftObjectPath(FString::Printf(TEXT("/Game/Test/Anim/%s.%s"), Nome, Nome)));
	}

	/** Un ruolo con `Quante` varianti `AV_1..AV_n`, tutte inattive. */
	FRTAnimRoleClips ConVarianti(int32 Quante)
	{
		FRTAnimRoleClips Ruolo;
		for (int32 i = 1; i <= Quante; ++i)
		{
			const FName Id(*FString::Printf(TEXT("AV_%d"), i));
			Ruolo.AddVariant(Id, NAME_None, ClipFinta(*FString::Printf(TEXT("Clip%d"), i)));
		}
		return Ruolo;
	}

	FName IdAttiva(const FRTAnimRoleClips& Ruolo)
	{
		const FRTAnimVariant* Attiva = Ruolo.FindActive();
		return Attiva ? Attiva->VariantId : NAME_None;
	}
}

// ─── Make Active ────────────────────────────────────────────────────────────────────────────────────
//
// 🔴 **Il controllo POSITIVO viene prima, e non e' cerimonia.** I tre test qui sotto asseriscono in gran
// parte che qualcosa **non** cambia, e un'asserzione di invarianza e' vacua se nello stesso file non
// esiste un caso in cui quella stessa cosa cambia davvero. Senza questo test, `MakeActive` potrebbe non
// fare niente e tutti gli altri resterebbero verdi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimMakeActiveIsAtomicTest,
	"RefactorTactics.Anim.MakeActiveIsAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimMakeActiveIsAtomicTest::RunTest(const FString&)
{
	FRTAnimRoleClips Ruolo = ConVarianti(3);

	// Anti-vacuita': senza varianti ogni asserzione sotto guarderebbe il vuoto.
	if (!TestEqual(TEXT("tre varianti legate"), Ruolo.Variants.Num(), 3)) { return false; }
	TestTrue(TEXT("nessuna attiva prima di sceglierla"), Ruolo.ActiveClipVariant.IsNone());

	TestTrue(TEXT("MakeActive su un id esistente riesce"), Ruolo.MakeActive(FName(TEXT("AV_2"))));
	TestEqual(TEXT("l'attiva e' quella scelta"), IdAttiva(Ruolo), FName(TEXT("AV_2")));

	// 🔑 L'ATOMICITA': dopo il secondo `MakeActive` la precedente non e' piu' attiva. Lo stato e' uno
	// solo, quindi non esiste un istante con due attive — ed e' proprio la ragione per cui e' un `FName`
	// e non un `bool` per variante.
	TestTrue(TEXT("MakeActive di un'altra riesce"), Ruolo.MakeActive(FName(TEXT("AV_3"))));
	TestEqual(TEXT("l'attiva e' la nuova"), IdAttiva(Ruolo), FName(TEXT("AV_3")));
	TestNotEqual(TEXT("la vecchia attiva non lo e' piu'"), IdAttiva(Ruolo), FName(TEXT("AV_2")));

	// Un id inesistente NON deve azzerare l'attiva: sarebbe una disattivazione travestita da errore.
	TestFalse(TEXT("MakeActive su un id inesistente fallisce"), Ruolo.MakeActive(FName(TEXT("AV_99"))));
	TestEqual(TEXT("e non ha toccato l'attiva"), IdAttiva(Ruolo), FName(TEXT("AV_3")));
	return true;
}

// ─── Active non cambia da solo ──────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindDoesNotChangeActiveTest,
	"RefactorTactics.Anim.BindDoesNotChangeActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindDoesNotChangeActiveTest::RunTest(const FString&)
{
	// (1) La PRIMA variante legata entra inattiva. «E' l'unica, quindi sara' lei» e' esattamente la
	//     deduzione che l'autore non ha chiesto, ed e' il caso in cui e' piu' facile scivolare.
	FRTAnimRoleClips Vergine;
	Vergine.AddVariant(FName(TEXT("AV_1")), NAME_None, ClipFinta(TEXT("Clip1")));
	TestEqual(TEXT("una sola variante legata"), Vergine.Variants.Num(), 1);
	TestTrue(TEXT("la prima variante legata NON diventa attiva"), Vergine.ActiveClipVariant.IsNone());

	// (2) Con un'attiva gia' scelta, legarne un'altra non la sposta.
	FRTAnimRoleClips Ruolo = ConVarianti(2);
	Ruolo.MakeActive(FName(TEXT("AV_1")));
	if (!TestEqual(TEXT("premessa: AV_1 e' attiva"), IdAttiva(Ruolo), FName(TEXT("AV_1")))) { return false; }

	Ruolo.AddVariant(FName(TEXT("AV_9")), NAME_None, ClipFinta(TEXT("Clip9")));
	TestEqual(TEXT("la variante nuova e' entrata"), Ruolo.Variants.Num(), 3);
	TestEqual(TEXT("il bind non ha cambiato l'attiva"), IdAttiva(Ruolo), FName(TEXT("AV_1")));

	// (3) Nemmeno un riordino dell'array: l'attiva e' nominata per id, non per indice. Se fosse un
	//     indice, questo `Swap` la sposterebbe in silenzio.
	Ruolo.Variants.Swap(0, 2);
	TestEqual(TEXT("il riordino non ha cambiato l'attiva"), IdAttiva(Ruolo), FName(TEXT("AV_1")));
	return true;
}

// ─── Rimozione ──────────────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimRemoveActiveClearsItTest,
	"RefactorTactics.Anim.RemoveActiveClearsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimRemoveActiveClearsItTest::RunTest(const FString&)
{
	FRTAnimRoleClips Ruolo = ConVarianti(3);
	Ruolo.MakeActive(FName(TEXT("AV_2")));
	if (!TestEqual(TEXT("premessa: AV_2 e' attiva"), IdAttiva(Ruolo), FName(TEXT("AV_2")))) { return false; }

	// (1) Rimuovere una NON attiva non tocca l'attiva.
	TestTrue(TEXT("rimozione di AV_1 riesce"), Ruolo.RemoveVariant(FName(TEXT("AV_1"))));
	TestEqual(TEXT("l'attiva e' ancora AV_2"), IdAttiva(Ruolo), FName(TEXT("AV_2")));

	// (2) 🔑 Rimuovere l'ATTIVA porta a `NAME_None`, e NON elegge una sostituta. `AV_3` e' ancora li' e
	//     deve restare inattiva: eleggerla toglierebbe all'autore una scelta che e' sua, in silenzio.
	TestTrue(TEXT("rimozione di AV_2 riesce"), Ruolo.RemoveVariant(FName(TEXT("AV_2"))));
	TestTrue(TEXT("nessuna attiva dopo aver rimosso l'attiva"), Ruolo.ActiveClipVariant.IsNone());
	TestEqual(TEXT("una variante e' rimasta, e non e' stata eletta"), Ruolo.Variants.Num(), 1);
	TestNull(TEXT("FindActive non trova niente"), (const void*)Ruolo.FindActive());

	// (3) Un `ActiveClipVariant` che nomina una variante inesistente si legge come «nessuna», non crasha.
	FRTAnimRoleClips Incoerente = ConVarianti(1);
	Incoerente.ActiveClipVariant = FName(TEXT("AV_fantasma"));
	TestNull(TEXT("un id attivo inesistente da' nessuna attiva"), (const void*)Incoerente.FindActive());
	return true;
}

// ─── Etichette neutre ───────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimNeutralLabelFillsHolesTest,
	"RefactorTactics.Anim.NeutralLabelFillsHoles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimNeutralLabelFillsHolesTest::RunTest(const FString&)
{
	FRTAnimRoleClips Ruolo;
	Ruolo.AddVariant(FName(TEXT("AV_1")), NAME_None, ClipFinta(TEXT("C1")));
	Ruolo.AddVariant(FName(TEXT("AV_2")), NAME_None, ClipFinta(TEXT("C2")));
	Ruolo.AddVariant(FName(TEXT("AV_3")), NAME_None, ClipFinta(TEXT("C3")));

	TestEqual(TEXT("le tre automatiche sono A, B, C"),
		FString::Printf(TEXT("%s%s%s"),
			*Ruolo.Variants[0].Label.ToString(),
			*Ruolo.Variants[1].Label.ToString(),
			*Ruolo.Variants[2].Label.ToString()),
		FString(TEXT("ABC")));

	// 🔑 **Il buco.** Tolta `B`, la prossima automatica deve essere `B` e non `D`: contare le varianti
	// darebbe `D`, e la differenza fra le due implementazioni si vede SOLO con un buco in mezzo.
	TestTrue(TEXT("rimozione di AV_2 (etichetta B)"), Ruolo.RemoveVariant(FName(TEXT("AV_2"))));
	TestEqual(TEXT("la prima libera e' B, non D"),
		Ruolo.PrimaEtichettaNeutraLibera(), FName(TEXT("B")));

	Ruolo.AddVariant(FName(TEXT("AV_4")), NAME_None, ClipFinta(TEXT("C4")));
	TestEqual(TEXT("la nuova ha preso B"), Ruolo.Variants.Last().Label, FName(TEXT("B")));

	// Un'etichetta scritta dall'autore non viene sostituita.
	Ruolo.AddVariant(FName(TEXT("AV_5")), FName(TEXT("Heavy")), ClipFinta(TEXT("C5")));
	TestEqual(TEXT("l'etichetta esplicita resta"), Ruolo.Variants.Last().Label, FName(TEXT("Heavy")));
	return true;
}

// ─── Migrazione del roster ──────────────────────────────────────────────────────────────────────────
//
// 🔑 **Gli otto path devono essere identici a prima della migrazione a ruoli.** I nomi sono MISURATI sul
// disco (§AS.3b: sei caselle su venti divergono), quindi qui sono trascritti come letterali: derivarli
// dallo stesso codice che si sta verificando renderebbe il test vacuo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimRosterMigrationKeepsPathsTest,
	"RefactorTactics.Anim.RosterMigrationKeepsPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimRosterMigrationKeepsPathsTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO del grafo di animazione"), Cdo)) { return false; }

	struct FAtteso { const TCHAR* Eroe; const TCHAR* Pack; const TCHAR* Idle; const TCHAR* Move; };
	const FAtteso Attesi[] = {
		{ TEXT("Hero.Gadget"), TEXT("Gadget"), TEXT("Idle"),           TEXT("Run_Fwd") },
		{ TEXT("Hero.Phase"),  TEXT("Phase"),  TEXT("Idle"),           TEXT("Jog_Fwd") },
		{ TEXT("Hero.Branth"), TEXT("Riktor"), TEXT("Idle"),           TEXT("Jog_Fwd") },
		{ TEXT("Hero.Wraith"), TEXT("Wraith"), TEXT("Idle_NonCombat"), TEXT("Jog_Fwd") },
	};

	if (!TestEqual(TEXT("il roster ha quattro eroi"), Cdo->ClipsPerHero.Num(), 4)) { return false; }

	int32 PathVisti = 0;
	for (const FAtteso& A : Attesi)
	{
		const FName Eroe(A.Eroe);
		const FString Radice = FString::Printf(
			TEXT("/Game/FabAsset/Paragon/Paragon%s/Characters/Heroes/%s/Animations/"), A.Pack, A.Pack);

		const TCHAR* Nomi[] = { A.Idle, A.Move };
		const ERTPresentationRole Ruoli[] = { ERTPresentationRole::Idle, ERTPresentationRole::Move };

		for (int32 i = 0; i < 2; ++i)
		{
			const FString Visto = Cdo->ActiveClipFor(Eroe, Ruoli[i]).ToSoftObjectPath().ToString();
			const FString Atteso = FString::Printf(TEXT("%s%s.%s"), *Radice, Nomi[i], Nomi[i]);
			TestEqual(*FString::Printf(TEXT("%s ruolo %d"), A.Eroe, i), Visto, Atteso);
			if (!Visto.IsEmpty()) { ++PathVisti; }
		}
	}

	// ⛔ Anti-vacuita': se `ActiveClipFor` restituisse sempre vuoto, gli otto `TestEqual` sopra
	// cadrebbero — ma se un giorno l'atteso venisse derivato dal codice, tutto tornerebbe verde a vuoto.
	// Questo conteggio e' indipendente da quel confronto.
	TestEqual(TEXT("gli otto path del roster sono tutti presenti"), PathVisti, 8);

	// Ogni ruolo del roster ha UNA variante, e quella variante e' attiva: e' la forma che la migrazione
	// doveva produrre, ed e' il presupposto del gate di packaging.
	for (const FAtteso& A : Attesi)
	{
		const FRTHeroPresentationClips* Eroe = Cdo->FindClipsFor(FName(A.Eroe));
		if (!TestNotNull(*FString::Printf(TEXT("voce di %s"), A.Eroe), (const void*)Eroe)) { continue; }
		TestEqual(*FString::Printf(TEXT("%s: due ruoli popolati"), A.Eroe), Eroe->PerRole.Num(), 2);

		for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Eroe->PerRole)
		{
			TestEqual(*FString::Printf(TEXT("%s: una variante nel ruolo"), A.Eroe),
				Ruolo.Value.Variants.Num(), 1);
			TestNotNull(*FString::Printf(TEXT("%s: la variante del roster e' attiva"), A.Eroe),
				(const void*)Ruolo.Value.FindActive());
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
