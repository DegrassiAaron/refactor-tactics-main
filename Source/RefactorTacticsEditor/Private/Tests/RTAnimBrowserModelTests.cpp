#include "Misc/AutomationTest.h"

#include "RTAnimBrowserModel.h"
#include "Content/RTBuildAnimBindingsCommandlet.h"
#include "Unit/RTAnimCatalogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString PathDi(const TCHAR* Pack, const TCHAR* Clip)
	{
		return FString::Printf(
			TEXT("/Game/FabAsset/Paragon/Paragon%s/Characters/Heroes/%s/Animations/%s.%s"),
			Pack, Pack, Clip, Clip);
	}

	/** Un modello con quattro clip: due di Gadget, due di Wraith, stati assortiti. */
	FRTAnimBrowserModel ModelloDiProva()
	{
		FRTAnimCatalog Catalog;
		Catalog.NextId = 5;

		auto Aggiungi = [&Catalog](const TCHAR* Id, const TCHAR* Pack, const TCHAR* Clip,
			ERTAnimClipStatus Status)
		{
			FRTAnimCatalogEntry E;
			E.Id = FName(Id);
			E.Derived.AssetPath = PathDi(Pack, Clip);
			E.Derived.AssetName = Clip;
			E.Authored.Status = Status;
			Catalog.Entries.Add(MoveTemp(E));
		};

		Aggiungi(TEXT("AV_0001"), TEXT("Gadget"), TEXT("Idle"),     ERTAnimClipStatus::Promoted);
		Aggiungi(TEXT("AV_0002"), TEXT("Gadget"), TEXT("Run_Fwd"),  ERTAnimClipStatus::Unreviewed);
		Aggiungi(TEXT("AV_0003"), TEXT("Wraith"), TEXT("Idle_NonCombat"), ERTAnimClipStatus::Rejected);
		Aggiungi(TEXT("AV_0004"), TEXT("Wraith"), TEXT("Jog_Fwd"),  ERTAnimClipStatus::Promoted);

		// Si passa dal JSON invece di iniettare la struct: cosi' il test attraversa anche la
		// serializzazione, ed e' l'unico modo in cui il pannello vedra' davvero questi dati.
		FString Json;
		URTAnimCatalogLibrary::SaveToString(Catalog, Json);

		FRTAnimBrowserModel Modello;
		FRTAnimCatalog Riletto;
		FString Errore;
		URTAnimCatalogLibrary::LoadFromString(Json, Riletto, Errore);

		// `LoadFrom` vuole un file; per i test si costruisce il modello dal round-trip via file temporaneo.
		const FString Temp = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Test_AnimBrowser.json"));
		FFileHelper::SaveStringToFile(Json, *Temp);
		Modello.LoadFrom(Temp, Errore);
		IFileManager::Get().Delete(*Temp);
		return Modello;
	}
}

// ─── Il pack si legge dal path ───────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBrowserPackFromPathTest,
	"RefactorTactics.Anim.Browser.PackFromPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBrowserPackFromPathTest::RunTest(const FString&)
{
	TestEqual(TEXT("Gadget"),
		FRTAnimBrowserModel::PackFromAssetPath(PathDi(TEXT("Gadget"), TEXT("Idle"))), FString(TEXT("Gadget")));
	TestEqual(TEXT("Wraith"),
		FRTAnimBrowserModel::PackFromAssetPath(PathDi(TEXT("Wraith"), TEXT("Jog_Fwd"))), FString(TEXT("Wraith")));

	// ⛔ Un path che non nomina un pack da' vuoto, non un pack inventato: dedurre produrrebbe un dato che
	// sembra misurato e non lo e'.
	TestEqual(TEXT("path estraneo -> vuoto"),
		FRTAnimBrowserModel::PackFromAssetPath(TEXT("/Game/RT/Anim/Qualcosa.Qualcosa")), FString());
	TestEqual(TEXT("stringa vuota -> vuoto"),
		FRTAnimBrowserModel::PackFromAssetPath(FString()), FString());
	return true;
}

// ─── I tre filtri, e la loro combinazione ────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBrowserFiltersCombineTest,
	"RefactorTactics.Anim.Browser.FiltersCombine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBrowserFiltersCombineTest::RunTest(const FString&)
{
	FRTAnimBrowserModel M = ModelloDiProva();

	// ⛔ Anti-vacuita': senza righe ogni filtro sotto restituirebbe zero e passerebbe per il motivo
	// sbagliato — «filtra bene» e «non c'e' niente da filtrare» darebbero lo stesso numero.
	if (!TestEqual(TEXT("il catalogo di prova ha quattro voci"), M.TotalRowCount(), 4)) { return false; }
	if (!TestEqual(TEXT("senza filtri si vedono tutte"), M.VisibleRows().Num(), 4)) { return false; }

	M.SetPackFilter(TEXT("Gadget"));
	TestEqual(TEXT("solo Gadget"), M.VisibleRows().Num(), 2);

	M.SetStatusFilter(ERTAnimClipStatus::Promoted);
	TestEqual(TEXT("Gadget + Promoted"), M.VisibleRows().Num(), 1);

	// 🔑 La COMBINAZIONE, che e' il caso che un test per filtro singolo non copre: tre filtri in AND, e
	// il terzo esclude cio' che i primi due lasciavano passare.
	M.SetSearchText(TEXT("Run"));
	TestEqual(TEXT("Gadget + Promoted + 'Run' -> nessuna (Idle e' promossa, Run_Fwd no)"),
		M.VisibleRows().Num(), 0);

	// E il controllo positivo che rende non vacuo lo zero qui sopra: rilassando UN filtro riappare.
	M.SetStatusFilter(TOptional<ERTAnimClipStatus>());
	TestEqual(TEXT("Gadget + 'Run', senza filtro di stato -> una"), M.VisibleRows().Num(), 1);

	// La ricerca guarda anche l'`AV_ID`.
	M.SetPackFilter(FString());
	M.SetSearchText(TEXT("AV_0003"));
	TestEqual(TEXT("ricerca per AV_ID"), M.VisibleRows().Num(), 1);
	return true;
}

// ─── Il vincolo non negoziabile ─────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBrowserOnlyUserWritesStatusTest,
	"RefactorTactics.Anim.Browser.OnlyUserWritesStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBrowserOnlyUserWritesStatusTest::RunTest(const FString&)
{
	FRTAnimBrowserModel M = ModelloDiProva();

	auto StatoDi = [&M](const TCHAR* Id) -> ERTAnimClipStatus
	{
		for (const FRTAnimBrowserRow& R : M.VisibleRows())
		{
			if (R.Id == FName(Id)) { return R.Status; }
		}
		return ERTAnimClipStatus::Rejected;   // valore che nessun caso sotto si aspetta
	};

	if (!TestEqual(TEXT("premessa: AV_0002 e' Unreviewed"),
			static_cast<int32>(StatoDi(TEXT("AV_0002"))), static_cast<int32>(ERTAnimClipStatus::Unreviewed)))
	{
		return false;
	}

	// 🔑 **Il controllo POSITIVO viene prima**: senza un caso in cui lo stato cambia davvero, le
	// invarianze qui sotto sarebbero verdi anche se `ApplyUserStatus` non facesse niente.
	TestTrue(TEXT("il comando utente scrive"), M.ApplyUserStatus(FName(TEXT("AV_0002")), ERTAnimClipStatus::Promoted));
	TestEqual(TEXT("ed e' diventata Promoted"),
		static_cast<int32>(StatoDi(TEXT("AV_0002"))), static_cast<int32>(ERTAnimClipStatus::Promoted));

	// ⛔ Nessun altro percorso lo tocca. `BindToRole`, `MakeActive` e `Unbind` sono gli unici altri
	// comandi che scrivono, e nessuno dei tre puo' cambiare uno `Status`.
	M.BindToRole(FName(TEXT("AV_0002")), FName(TEXT("Hero.Gadget")), ERTPresentationRole::Move);
	M.MakeActive(FName(TEXT("AV_0002")), FName(TEXT("Hero.Gadget")), ERTPresentationRole::Move);
	M.Unbind(FName(TEXT("AV_0002")), FName(TEXT("Hero.Gadget")), ERTPresentationRole::Move);
	TestEqual(TEXT("bind/active/unbind non cambiano lo Status"),
		static_cast<int32>(StatoDi(TEXT("AV_0002"))), static_cast<int32>(ERTAnimClipStatus::Promoted));

	// Nemmeno i filtri, che sono la via piu' innocua e quindi quella che nessuno controllerebbe.
	M.SetSearchText(TEXT("Run"));
	M.SetStatusFilter(ERTAnimClipStatus::Candidate);
	M.SetStatusFilter(TOptional<ERTAnimClipStatus>());
	M.SetSearchText(FString());
	TestEqual(TEXT("i filtri non cambiano lo Status"),
		static_cast<int32>(StatoDi(TEXT("AV_0002"))), static_cast<int32>(ERTAnimClipStatus::Promoted));

	// Un id inesistente non e' un crash e non scrive niente.
	TestFalse(TEXT("id inesistente"), M.ApplyUserStatus(FName(TEXT("AV_9999")), ERTAnimClipStatus::Promoted));
	return true;
}

// ─── Bind, Make Active, Unbind ──────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBrowserBindingRulesTest,
	"RefactorTactics.Anim.Browser.BindingRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBrowserBindingRulesTest::RunTest(const FString&)
{
	FRTAnimBrowserModel M = ModelloDiProva();
	const FName Gadget(TEXT("Hero.Gadget"));
	const FName Idle(TEXT("AV_0001"));      // Promoted
	const FName Run(TEXT("AV_0002"));       // Unreviewed

	// ⛔ Non si lega cio' che nessuno ha guardato.
	TestFalse(TEXT("una clip Unreviewed non si lega"),
		M.BindToRole(Run, Gadget, ERTPresentationRole::Move));

	// Il controllo positivo: una Promoted si lega.
	TestTrue(TEXT("una clip Promoted si lega"),
		M.BindToRole(Idle, Gadget, ERTPresentationRole::Idle));
	TestFalse(TEXT("legarla due volte non duplica"),
		M.BindToRole(Idle, Gadget, ERTPresentationRole::Idle));

	auto Attiva = [&M](const FName& Id, const FName& Hero, ERTPresentationRole Role) -> bool
	{
		for (const FRTAnimCatalogEntry& E : M.GetCatalog().Entries)
		{
			if (E.Id != Id) { continue; }
			for (const FRTAnimBinding& B : E.Authored.Bindings)
			{
				if (B.HeroId == Hero && B.Role == Role) { return B.bActive; }
			}
		}
		return false;
	};

	// 🔑 Entra INATTIVA anche se e' la prima del ruolo.
	TestFalse(TEXT("la prima variante legata non e' attiva"), Attiva(Idle, Gadget, ERTPresentationRole::Idle));

	TestTrue(TEXT("Make Active riesce"), M.MakeActive(Idle, Gadget, ERTPresentationRole::Idle));
	TestTrue(TEXT("ed e' attiva"), Attiva(Idle, Gadget, ERTPresentationRole::Idle));

	// Una seconda clip promossa sullo stesso ruolo: legandola, l'attiva NON cambia.
	M.ApplyUserStatus(Run, ERTAnimClipStatus::Promoted);
	TestTrue(TEXT("la seconda si lega"), M.BindToRole(Run, Gadget, ERTPresentationRole::Idle));
	TestTrue(TEXT("il bind non ha spostato l'attiva"), Attiva(Idle, Gadget, ERTPresentationRole::Idle));
	TestFalse(TEXT("e la nuova e' inattiva"), Attiva(Run, Gadget, ERTPresentationRole::Idle));

	// L'atomicita': attivando la seconda, la prima si spegne nello stesso passo.
	TestTrue(TEXT("Make Active sulla seconda"), M.MakeActive(Run, Gadget, ERTPresentationRole::Idle));
	TestTrue(TEXT("la seconda e' attiva"), Attiva(Run, Gadget, ERTPresentationRole::Idle));
	TestFalse(TEXT("la prima non lo e' piu'"), Attiva(Idle, Gadget, ERTPresentationRole::Idle));

	// E il catalogo resta valido: due attive sullo stesso ruolo sarebbero rosse.
	TestEqual(TEXT("il catalogo e' valido dopo lo scambio"),
		URTAnimCatalogLibrary::ValidateCatalog(&M.GetCatalog()).Num(), 0);

	// Rimuovere l'attiva lascia il ruolo SENZA attiva.
	TestTrue(TEXT("unbind dell'attiva"), M.Unbind(Run, Gadget, ERTPresentationRole::Idle));
	TestFalse(TEXT("nessuna sostituta eletta"), Attiva(Idle, Gadget, ERTPresentationRole::Idle));
	return true;
}

// ─── Il validator difende l'invariante anche su un file scritto a mano ──────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsTwoActivePerRoleTest,
	"RefactorTactics.Anim.Catalog.RejectsTwoActivePerRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsTwoActivePerRoleTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	Catalog.NextId = 3;

	auto Aggiungi = [&Catalog](const TCHAR* Id, const TCHAR* Clip, bool bActive)
	{
		FRTAnimCatalogEntry E;
		E.Id = FName(Id);
		E.Derived.AssetPath = PathDi(TEXT("Gadget"), Clip);
		E.Authored.Status = ERTAnimClipStatus::Promoted;
		FRTAnimBinding B;
		B.HeroId = FName(TEXT("Hero.Gadget"));
		B.Role = ERTPresentationRole::Move;
		B.bActive = bActive;
		E.Authored.Bindings.Add(B);
		Catalog.Entries.Add(MoveTemp(E));
	};

	// Il controllo positivo: una sola attiva e' valida. Senza, il rosso sotto non distinguerebbe
	// «due attive» da «il validator si lamenta comunque».
	Aggiungi(TEXT("AV_0001"), TEXT("Run_Fwd"), true);
	Aggiungi(TEXT("AV_0002"), TEXT("Run_Bwd"), false);
	TestEqual(TEXT("una sola attiva: valido"),
		URTAnimCatalogLibrary::ValidateCatalog(&Catalog).Num(), 0);

	// 🔴 Il caso che il testo rende rappresentabile e il runtime no: due `"active": true`.
	Catalog.Entries[1].Authored.Bindings[0].bActive = true;
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&Catalog);
	TestTrue(TEXT("due attive sullo stesso ruolo sono un errore"), Errori.Num() > 0);

	bool bNominaEntrambe = false;
	for (const FString& E : Errori)
	{
		if (E.Contains(TEXT("AV_0001")) && E.Contains(TEXT("AV_0002"))) { bNominaEntrambe = true; }
	}
	// Il messaggio deve dire QUALI due: «catalogo non valido» non si aziona.
	TestTrue(TEXT("la riga nomina entrambe le clip in conflitto"), bNominaEntrambe);
	return true;
}

// ─── La traduzione catalogo → CDO ───────────────────────────────────────────────────────────────────
//
// 🔑 **E' l'unica parte del commandlet che si puo' provare headless, ed e' l'unica che decide qualcosa.**
// Il resto — aprire un file, creare un package, salvarlo — non ha alternative da sbagliare. Provare il
// commandlet per intero richiederebbe un catalogo con dei legami, e un legame richiede una clip
// `Promoted`, che **solo una persona puo' scrivere**: il test sarebbe rimasto impossibile per costruzione.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindingsMapToCdoTest,
	"RefactorTactics.Anim.Bindings.MapToCdo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindingsMapToCdoTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	Catalog.NextId = 4;

	auto Aggiungi = [&Catalog](const TCHAR* Id, const TCHAR* Clip, const TCHAR* Hero,
		ERTPresentationRole Role, bool bActive, const TCHAR* Label)
	{
		FRTAnimCatalogEntry E;
		E.Id = FName(Id);
		E.Derived.AssetPath = PathDi(TEXT("Gadget"), Clip);
		E.Derived.AssetName = Clip;
		E.Authored.Status = ERTAnimClipStatus::Promoted;
		E.Authored.Label = Label;
		FRTAnimBinding B;
		B.HeroId = FName(Hero);
		B.Role = Role;
		B.bActive = bActive;
		E.Authored.Bindings.Add(B);
		Catalog.Entries.Add(MoveTemp(E));
	};

	Aggiungi(TEXT("AV_0001"), TEXT("Run_Fwd"), TEXT("Hero.Gadget"), ERTPresentationRole::Move, true,  TEXT("A"));
	Aggiungi(TEXT("AV_0002"), TEXT("Run_Bwd"), TEXT("Hero.Gadget"), ERTPresentationRole::Move, false, TEXT("B"));
	Aggiungi(TEXT("AV_0003"), TEXT("Idle"),    TEXT("Hero.Wraith"), ERTPresentationRole::Idle, true,  TEXT("A"));

	int32 Legami = 0;
	const TMap<FName, FRTHeroPresentationClips> PerEroe =
		URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Catalog, Legami);

	// Anti-vacuita': senza legami tradotti ogni asserzione sotto guarderebbe mappe vuote.
	if (!TestEqual(TEXT("tre legami tradotti"), Legami, 3)) { return false; }
	if (!TestEqual(TEXT("due eroi"), PerEroe.Num(), 2)) { return false; }

	const FRTHeroPresentationClips* Gadget = PerEroe.Find(FName(TEXT("Hero.Gadget")));
	if (!TestNotNull(TEXT("Gadget c'e'"), (const void*)Gadget)) { return false; }
	const FRTAnimRoleClips* Move = Gadget->FindRole(ERTPresentationRole::Move);
	if (!TestNotNull(TEXT("il ruolo Move c'e'"), (const void*)Move)) { return false; }

	TestEqual(TEXT("due varianti sullo stesso ruolo"), Move->Variants.Num(), 2);

	// 🔑 L'attiva e' quella che il catalogo dichiarava, e le altre restano inattive.
	const FRTAnimVariant* Attiva = Move->FindActive();
	if (!TestNotNull(TEXT("c'e' un'attiva"), (const void*)Attiva)) { return false; }
	TestEqual(TEXT("l'attiva e' AV_0001"), Attiva->VariantId, FName(TEXT("AV_0001")));

	// L'`AV_ID` diventa il `VariantId`: non si conia una seconda identita'.
	TestNotNull(TEXT("AV_0002 e' fra le varianti"), (const void*)Move->FindVariant(FName(TEXT("AV_0002"))));

	// E il path della clip attraversa intatto: e' il dato che il cook dovra' seguire.
	TestEqual(TEXT("il path arriva al CDO"),
		Attiva->Clip.ToSoftObjectPath().ToString(), PathDi(TEXT("Gadget"), TEXT("Run_Fwd")));

	// Un eroe diverso non finisce nella stessa voce: la mappa e' per eroe, non globale.
	const FRTHeroPresentationClips* Wraith = PerEroe.Find(FName(TEXT("Hero.Wraith")));
	if (TestNotNull(TEXT("Wraith c'e'"), (const void*)Wraith))
	{
		TestNull(TEXT("Wraith non ha il ruolo Move"), (const void*)Wraith->FindRole(ERTPresentationRole::Move));
		TestNotNull(TEXT("Wraith ha il ruolo Idle"), (const void*)Wraith->FindRole(ERTPresentationRole::Idle));
	}

	// ⛔ Un binding senza eroe non produce una voce fantasma.
	FRTAnimCatalog Sporco;
	Sporco.NextId = 2;
	FRTAnimCatalogEntry Orfana;
	Orfana.Id = FName(TEXT("AV_0001"));
	Orfana.Derived.AssetPath = PathDi(TEXT("Gadget"), TEXT("Idle"));
	FRTAnimBinding SenzaEroe;   // HeroId resta NAME_None
	Orfana.Authored.Bindings.Add(SenzaEroe);
	Sporco.Entries.Add(MoveTemp(Orfana));

	int32 LegamiSporchi = 0;
	const TMap<FName, FRTHeroPresentationClips> Vuota =
		URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Sporco, LegamiSporchi);
	TestEqual(TEXT("un binding senza eroe non si traduce"), LegamiSporchi, 0);
	TestEqual(TEXT("e non crea eroi"), Vuota.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
