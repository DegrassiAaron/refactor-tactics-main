#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

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

	/** Un modello con quattro clip: due di Aevik, due di Ivrin, stati assortiti. */
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

		Aggiungi(TEXT("AV_0001"), TEXT("Aevik"), TEXT("Idle"),     ERTAnimClipStatus::Promoted);
		Aggiungi(TEXT("AV_0002"), TEXT("Aevik"), TEXT("Run_Fwd"),  ERTAnimClipStatus::Unreviewed);
		Aggiungi(TEXT("AV_0003"), TEXT("Ivrin"), TEXT("Idle_NonCombat"), ERTAnimClipStatus::Rejected);
		Aggiungi(TEXT("AV_0004"), TEXT("Ivrin"), TEXT("Jog_Fwd"),  ERTAnimClipStatus::Promoted);

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

	/**
	 * Un catalogo VALIDO con una voce `Promoted` e un binding ATTIVO per Aevik (#3563). `ActionId` nullo = binding
	 * di ruolo. E' la base dei test sulla terna: ogni rosso sotto parte da un verde misurato.
	 */
	FRTAnimCatalog CatalogoConLegame(ERTPresentationRole Role, const TCHAR* ActionId)
	{
		FRTAnimCatalog Catalog;
		Catalog.NextId = 2;
		FRTAnimCatalogEntry E;
		E.Id = FName(TEXT("AV_0001"));
		E.Derived.AssetPath = PathDi(TEXT("Gadget"), TEXT("Throw_Ready"));
		E.Derived.AssetName = TEXT("Throw_Ready");
		E.Authored.Status = ERTAnimClipStatus::Promoted;
		FRTAnimBinding B;
		B.HeroId = FName(TEXT("Hero.Aevik"));
		B.Role = Role;
		B.ActionId = ActionId ? FName(ActionId) : NAME_None;
		B.bActive = true;
		E.Authored.Bindings.Add(B);
		Catalog.Entries.Add(MoveTemp(E));
		return Catalog;
	}

	/**
	 * Aggiunge al catalogo una voce `Promoted` con UN binding per Aevik, e tiene `NextId` dominante.
	 * ⚠️ `Clip` distinta per voce: due voci sullo stesso path sono gia' un errore di `ValidateCatalog` (il controllo sul path in `ValidateCatalog`).
	 */
	void AggiungiLegame(FRTAnimCatalog& Catalog, const TCHAR* Id, const TCHAR* Clip, ERTPresentationRole Role,
		const TCHAR* ActionId, bool bActive)
	{
		FRTAnimCatalogEntry E;
		E.Id = FName(Id);
		E.Derived.AssetPath = PathDi(TEXT("Gadget"), Clip);
		E.Derived.AssetName = Clip;
		E.Authored.Status = ERTAnimClipStatus::Promoted;
		FRTAnimBinding B;
		B.HeroId = FName(TEXT("Hero.Aevik"));
		B.Role = Role;
		B.ActionId = ActionId ? FName(ActionId) : NAME_None;
		B.bActive = bActive;
		E.Authored.Bindings.Add(B);
		Catalog.Entries.Add(MoveTemp(E));
		Catalog.NextId = Catalog.Entries.Num() + 1;
	}

	bool QualcheRigaContiene(const TArray<FString>& Righe, const TCHAR* Frammento)
	{
		return Righe.ContainsByPredicate([Frammento](const FString& R) { return R.Contains(Frammento); });
	}

	/** Un catalogo JSON minimo con UN binding Cast di Aevik; `ActionIdJson` e' il frammento della chiave, o vuoto. */
	FString JsonConUnLegame(int32 Versione, const TCHAR* ActionIdJson)
	{
		return FString::Printf(TEXT(R"({ "formatVersion": %d, "nextId": 2, "entries": [ { "id": "AV_0001", )")
			TEXT(R"("derived": { "assetPath": "/Game/A.A" }, "authored": { "status": "Promoted", "bindings": [ )")
			TEXT(R"({ "hero": "Hero.Aevik", "role": "Cast", %s"active": true } ] } } ] })"), Versione, ActionIdJson);
	}

	/** Un pool di una variante attiva, per costruire a mano le mappe della fusione (#3563). */
	FRTAnimRoleClips PoolDiFusione(const TCHAR* Path)
	{
		FRTAnimRoleClips Pool;
		Pool.AddVariant(FName(TEXT("AV_Fusione")), FName(TEXT("A")), TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path)));
		Pool.MakeActive(FName(TEXT("AV_Fusione")));
		return Pool;
	}

	FString PathAttivoDi(const FRTAnimRoleClips* Pool)
	{
		const FRTAnimVariant* Attiva = Pool ? Pool->FindActive() : nullptr;
		return Attiva ? Attiva->Clip.ToSoftObjectPath().ToString() : FString();
	}
}

// ─── Il pack si legge dal path ───────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBrowserPackFromPathTest,
	"RefactorTactics.Anim.Browser.PackFromPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBrowserPackFromPathTest::RunTest(const FString&)
{
	TestEqual(TEXT("Aevik"),
		FRTAnimBrowserModel::PackFromAssetPath(PathDi(TEXT("Aevik"), TEXT("Idle"))), FString(TEXT("Aevik")));
	TestEqual(TEXT("Ivrin"),
		FRTAnimBrowserModel::PackFromAssetPath(PathDi(TEXT("Ivrin"), TEXT("Jog_Fwd"))), FString(TEXT("Ivrin")));

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

	M.SetPackFilter(TEXT("Aevik"));
	TestEqual(TEXT("solo Aevik"), M.VisibleRows().Num(), 2);

	M.SetStatusFilter(ERTAnimClipStatus::Promoted);
	TestEqual(TEXT("Aevik + Promoted"), M.VisibleRows().Num(), 1);

	// 🔑 La COMBINAZIONE, che e' il caso che un test per filtro singolo non copre: tre filtri in AND, e
	// il terzo esclude cio' che i primi due lasciavano passare.
	M.SetSearchText(TEXT("Run"));
	TestEqual(TEXT("Aevik + Promoted + 'Run' -> nessuna (Idle e' promossa, Run_Fwd no)"),
		M.VisibleRows().Num(), 0);

	// E il controllo positivo che rende non vacuo lo zero qui sopra: rilassando UN filtro riappare.
	M.SetStatusFilter(TOptional<ERTAnimClipStatus>());
	TestEqual(TEXT("Aevik + 'Run', senza filtro di stato -> una"), M.VisibleRows().Num(), 1);

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
	M.BindToRole(FName(TEXT("AV_0002")), FName(TEXT("Hero.Aevik")), ERTPresentationRole::Move);
	M.MakeActive(FName(TEXT("AV_0002")), FName(TEXT("Hero.Aevik")), ERTPresentationRole::Move);
	M.Unbind(FName(TEXT("AV_0002")), FName(TEXT("Hero.Aevik")), ERTPresentationRole::Move);
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
	const FName Aevik(TEXT("Hero.Aevik"));
	const FName Idle(TEXT("AV_0001"));      // Promoted
	const FName Run(TEXT("AV_0002"));       // Unreviewed

	// ⛔ Non si lega cio' che nessuno ha guardato.
	TestFalse(TEXT("una clip Unreviewed non si lega"),
		M.BindToRole(Run, Aevik, ERTPresentationRole::Move));

	// Il controllo positivo: una Promoted si lega.
	TestTrue(TEXT("una clip Promoted si lega"),
		M.BindToRole(Idle, Aevik, ERTPresentationRole::Idle));
	TestFalse(TEXT("legarla due volte non duplica"),
		M.BindToRole(Idle, Aevik, ERTPresentationRole::Idle));

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
	TestFalse(TEXT("la prima variante legata non e' attiva"), Attiva(Idle, Aevik, ERTPresentationRole::Idle));

	TestTrue(TEXT("Make Active riesce"), M.MakeActive(Idle, Aevik, ERTPresentationRole::Idle));
	TestTrue(TEXT("ed e' attiva"), Attiva(Idle, Aevik, ERTPresentationRole::Idle));

	// Una seconda clip promossa sullo stesso ruolo: legandola, l'attiva NON cambia.
	M.ApplyUserStatus(Run, ERTAnimClipStatus::Promoted);
	TestTrue(TEXT("la seconda si lega"), M.BindToRole(Run, Aevik, ERTPresentationRole::Idle));
	TestTrue(TEXT("il bind non ha spostato l'attiva"), Attiva(Idle, Aevik, ERTPresentationRole::Idle));
	TestFalse(TEXT("e la nuova e' inattiva"), Attiva(Run, Aevik, ERTPresentationRole::Idle));

	// L'atomicita': attivando la seconda, la prima si spegne nello stesso passo.
	TestTrue(TEXT("Make Active sulla seconda"), M.MakeActive(Run, Aevik, ERTPresentationRole::Idle));
	TestTrue(TEXT("la seconda e' attiva"), Attiva(Run, Aevik, ERTPresentationRole::Idle));
	TestFalse(TEXT("la prima non lo e' piu'"), Attiva(Idle, Aevik, ERTPresentationRole::Idle));

	// E il catalogo resta valido: due attive sullo stesso ruolo sarebbero rosse.
	TestEqual(TEXT("il catalogo e' valido dopo lo scambio"),
		URTAnimCatalogLibrary::ValidateCatalog(&M.GetCatalog()).Num(), 0);

	// Rimuovere l'attiva lascia il ruolo SENZA attiva.
	TestTrue(TEXT("unbind dell'attiva"), M.Unbind(Run, Aevik, ERTPresentationRole::Idle));
	TestFalse(TEXT("nessuna sostituta eletta"), Attiva(Idle, Aevik, ERTPresentationRole::Idle));
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
		E.Derived.AssetPath = PathDi(TEXT("Aevik"), Clip);
		E.Authored.Status = ERTAnimClipStatus::Promoted;
		FRTAnimBinding B;
		B.HeroId = FName(TEXT("Hero.Aevik"));
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

// ─── La chiave `actionId` (#3563, spec «la clip per abilita'» §2.3) ──────────────────────────────────────────

/**
 * Il JSON con `actionId` fa round-trip, e un catalogo v1 che guadagna un `actionId` si RISALVA come v2.
 * ✅ Validato per mutazione (P5): il writer che scrive `Catalog.FormatVersion` invece di `CurrentFormatVersion`
 * fa rifiutare la rilettura («un actionId esiste solo da v2») → cade «la rilettura riesce».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogActionIdRoundTripsTest,
	"RefactorTactics.Anim.Catalog.ActionIdRoundTrips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogActionIdRoundTripsTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	Catalog.FormatVersion = 1;   // 🔑 un catalogo letto da un file v1, a cui l'autore aggiunge un binding d'azione
	AggiungiLegame(Catalog, TEXT("AV_0001"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	AggiungiLegame(Catalog, TEXT("AV_0002"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);

	FString Json;
	TestTrue(TEXT("scrittura riuscita"), URTAnimCatalogLibrary::SaveToString(Catalog, Json));
	TestTrue(TEXT("il file scritto dichiara formatVersion 2"), Json.Contains(TEXT("\"formatVersion\": 2")));
	TestTrue(TEXT("il binding d'azione porta actionId"), Json.Contains(TEXT("\"actionId\": \"Hero.Aevik.Overload\"")));
	int32 Occorrenze = 0;
	for (int32 Da = Json.Find(TEXT("\"actionId\"")); Da != INDEX_NONE;
		Da = Json.Find(TEXT("\"actionId\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Da + 1))
	{
		++Occorrenze;
	}
	TestEqual(TEXT("il binding di ruolo NON porta la chiave"), Occorrenze, 1);

	FRTAnimCatalog Riletto;
	FString Errore;
	if (!TestTrue(TEXT("🔴 la rilettura riesce"), URTAnimCatalogLibrary::LoadFromString(Json, Riletto, Errore)))
	{
		AddInfo(Errore);
		return false;
	}
	TestEqual(TEXT("round-trip: l'azione del primo binding"),
		Riletto.Entries[0].Authored.Bindings[0].ActionId, FName(TEXT("Hero.Aevik.Overload")));
	TestEqual(TEXT("round-trip: il secondo resta di ruolo"),
		Riletto.Entries[1].Authored.Bindings[0].ActionId, FName(NAME_None));
	return true;
}

/**
 * Un `actionId` che non e' un'azione conosciuta e' un ERRORE, col suo nome — `Ruling` di §2.3.
 * ✅ Validato per mutazione (5): il controllo dell'azione ignota tolto → cade «un refuso e' un errore».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsUnknownActionIdTest,
	"RefactorTactics.Anim.Catalog.RejectsUnknownActionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsUnknownActionIdTest::RunTest(const FString&)
{
	const FRTAnimCatalog Buono = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"));
	TestEqual(TEXT("controllo positivo: un'abilita' del kit e' valida"),
		URTAnimCatalogLibrary::ValidateCatalog(&Buono).Num(), 0);
	const FRTAnimCatalog Generica = CatalogoConLegame(ERTPresentationRole::Attack, TEXT("Action.BasicAttack"));
	TestEqual(TEXT("controllo positivo: una generica core e' valida"),
		URTAnimCatalogLibrary::ValidateCatalog(&Generica).Num(), 0);

	const FRTAnimCatalog Refuso = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overlaod"));
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&Refuso);
	TestTrue(TEXT("🔴 un refuso e' un errore"), Errori.Num() > 0);
	TestTrue(TEXT("e la riga nomina l'azione"), QualcheRigaContiene(Errori, TEXT("Hero.Aevik.Overlaod")));
	return true;
}

/**
 * Al piu' una attiva per POOL: due sulla stessa terna sono errore; una di ruolo e una d'azione sullo stesso
 * `(eroe, ruolo)` sono due pool, e convivono (Review Focus (b), meta' «entrambi validi»).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsTwoActivePerActionRoleTest,
	"RefactorTactics.Anim.Catalog.RejectsTwoActivePerActionRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsTwoActivePerActionRoleTest::RunTest(const FString&)
{
	FRTAnimCatalog Pool;
	AggiungiLegame(Pool, TEXT("AV_0001"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);
	AggiungiLegame(Pool, TEXT("AV_0002"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	TestEqual(TEXT("🔑 una attiva di ruolo e una d'azione sullo stesso (eroe, ruolo): valido"),
		URTAnimCatalogLibrary::ValidateCatalog(&Pool).Num(), 0);

	AggiungiLegame(Pool, TEXT("AV_0003"), TEXT("Ability_Q_Target"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&Pool);
	TestTrue(TEXT("🔴 due attive sulla stessa terna sono un errore"), Errori.Num() > 0);
	bool bNominaEntrambe = false;
	for (const FString& E : Errori)
	{
		if (E.Contains(TEXT("AV_0002")) && E.Contains(TEXT("AV_0003")) && E.Contains(TEXT("Hero.Aevik.Overload")))
		{
			bNominaEntrambe = true;
		}
	}
	TestTrue(TEXT("la riga nomina le due clip e l'azione"), bNominaEntrambe);

	// 🔑 L'altra meta' di «un pool per terna»: azioni DIVERSE sullo stesso (eroe, ruolo) sono pool diversi, e
	// convivono — attive entrambe, e anche insieme a quella di ruolo. Una chiave piu' debole della terna cade qui.
	FRTAnimCatalog Distinti;
	AggiungiLegame(Distinti, TEXT("AV_0001"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);
	AggiungiLegame(Distinti, TEXT("AV_0002"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	AggiungiLegame(Distinti, TEXT("AV_0003"), TEXT("Ability_Q_Target"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.ArcPulse"), true);
	TestEqual(TEXT("🔑 due azioni diverse attive sullo stesso (eroe, ruolo), e quella di ruolo: valido"),
		URTAnimCatalogLibrary::ValidateCatalog(&Distinti).Num(), 0);
	return true;
}

/**
 * `formatVersion: 1` con un `actionId` e' rifiutato: una build vecchia lo leggerebbe come binding di RUOLO, cioe'
 * una clip sbagliata e attiva (spec §2.3, §4). Lo stesso testo dichiarato v2 si legge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogFormatVersion2IsRequiredTest,
	"RefactorTactics.Anim.Catalog.FormatVersion2IsRequired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogFormatVersion2IsRequiredTest::RunTest(const FString&)
{
	FRTAnimCatalog Letto;
	FString Errore;
	TestFalse(TEXT("🔴 v1 con actionId: rifiutato"),
		URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(1, TEXT(R"("actionId": "Hero.Aevik.Overload", )")), Letto, Errore));
	TestTrue(TEXT("e il messaggio nomina actionId"), Errore.Contains(TEXT("actionId")));

	FRTAnimCatalog LettoV2;
	FString ErroreV2;
	TestTrue(TEXT("controllo positivo: lo stesso testo v2 si legge"),
		URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(2, TEXT(R"("actionId": "Hero.Aevik.Overload", )")), LettoV2, ErroreV2));
	return true;
}

/**
 * Un `actionId` PRESENTE ma non stringa e' un errore di lettura: ignorarlo farebbe del binding un binding di RUOLO
 * attivo, il guasto che il bump di versione esiste per evitare (review Task 4, M1).
 * ✅ Validato per mutazione: il controllo sul tipo del valore tolto → cade «🔴 actionId non stringa: rifiutato».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsNonStringActionIdTest,
	"RefactorTactics.Anim.Catalog.RejectsNonStringActionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsNonStringActionIdTest::RunTest(const FString&)
{
	// Il controllo positivo: lo stesso testo con la stringa si legge, quindi il rifiuto sotto e' del TIPO.
	FRTAnimCatalog Positivo;
	FString ErrorePositivo;
	TestTrue(TEXT("controllo positivo: actionId stringa si legge"),
		URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(2, TEXT(R"("actionId": "Hero.Aevik.Overload", )")), Positivo, ErrorePositivo));

	const TCHAR* NonStringhe[] = { TEXT("7"), TEXT("null"), TEXT("{}"), TEXT("[]"), TEXT("true") };
	for (const TCHAR* Valore : NonStringhe)
	{
		FRTAnimCatalog Letto;
		FString Errore;
		const FString Frammento = FString::Printf(TEXT("\"actionId\": %s, "), Valore);
		TestFalse(*FString::Printf(TEXT("🔴 actionId non stringa: rifiutato (%s)"), Valore),
			URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(2, *Frammento), Letto, Errore));
		TestTrue(*FString::Printf(TEXT("e il messaggio nomina il campo e l'eroe (%s)"), Valore),
			Errore.Contains(TEXT("actionId")) && Errore.Contains(TEXT("Hero.Aevik")));
	}
	return true;
}

/** Review Focus (c): un catalogo v2 di soli binding di RUOLO si legge, ed e' valido. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogFormatVersion2WithoutActionIdLoadsTest,
	"RefactorTactics.Anim.Catalog.FormatVersion2WithoutActionIdLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogFormatVersion2WithoutActionIdLoadsTest::RunTest(const FString&)
{
	FRTAnimCatalog Letto;
	FString Errore;
	if (!TestTrue(TEXT("🔴 v2 senza actionId: si legge"),
			URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(2, TEXT("")), Letto, Errore)))
	{
		AddInfo(Errore);
		return false;
	}
	TestEqual(TEXT("il binding e' di ruolo"), Letto.Entries[0].Authored.Bindings[0].ActionId, FName(NAME_None));
	TestEqual(TEXT("ed e' valido"), URTAnimCatalogLibrary::ValidateCatalog(&Letto).Num(), 0);
	FRTAnimCatalog LettoV1;
	TestTrue(TEXT("e un v1 senza actionId si legge ancora"),
		URTAnimCatalogLibrary::LoadFromString(JsonConUnLegame(1, TEXT("")), LettoV1, Errore));
	return true;
}

/**
 * Un `actionId` VALIDO su un ruolo che non propaga l'azione (`Move`) e' un errore: non suonerebbe mai (D3).
 * ✅ Validato per mutazione (9): il controllo del ruolo tolto → cade il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogRejectsActionIdOnNonPropagatingRoleTest,
	"RefactorTactics.Anim.Catalog.RejectsActionIdOnNonPropagatingRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogRejectsActionIdOnNonPropagatingRoleTest::RunTest(const FString&)
{
	const FRTAnimCatalog SuMove = CatalogoConLegame(ERTPresentationRole::Move, TEXT("Hero.Aevik.Overload"));
	const TArray<FString> Errori = URTAnimCatalogLibrary::ValidateCatalog(&SuMove);
	TestTrue(TEXT("🔴 actionId valido su Move: errore"), Errori.Num() > 0);
	TestTrue(TEXT("e la riga nomina il ruolo"), QualcheRigaContiene(Errori, TEXT("Move")));

	const FRTAnimCatalog SuCast = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"));
	TestEqual(TEXT("controllo positivo: la stessa azione su Cast e' valida"),
		URTAnimCatalogLibrary::ValidateCatalog(&SuCast).Num(), 0);
	return true;
}

/**
 * `Ruling` R10: un'azione concessa dall'equipaggiamento porta l'id del PEZZO (`MakeEquipmentAction`,
 * in `URTCatalogLibrary`) e si attiva come le altre: `Gadget.Sprinkler` su `Cast` e' accettato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimCatalogAcceptsEquipmentActionIdTest,
	"RefactorTactics.Anim.Catalog.AcceptsEquipmentActionId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimCatalogAcceptsEquipmentActionIdTest::RunTest(const FString&)
{
	const FRTAnimCatalog Gadget = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Gadget.Sprinkler"));
	TestEqual(TEXT("🔴 l'azione di un gadget e' un'azione conosciuta"),
		URTAnimCatalogLibrary::ValidateCatalog(&Gadget).Num(), 0);
	// Il controllo positivo del rifiuto: senza, «accettato» non distinguerebbe «conosciuta» da «nessun controllo».
	const FRTAnimCatalog Refuso = CatalogoConLegame(ERTPresentationRole::Cast, TEXT("Gadget.Sprinklr"));
	TestTrue(TEXT("e un pezzo inesistente resta un errore"), URTAnimCatalogLibrary::ValidateCatalog(&Refuso).Num() > 0);
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
		E.Derived.AssetPath = PathDi(TEXT("Aevik"), Clip);
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

	Aggiungi(TEXT("AV_0001"), TEXT("Run_Fwd"), TEXT("Hero.Aevik"), ERTPresentationRole::Move, true,  TEXT("A"));
	Aggiungi(TEXT("AV_0002"), TEXT("Run_Bwd"), TEXT("Hero.Aevik"), ERTPresentationRole::Move, false, TEXT("B"));
	Aggiungi(TEXT("AV_0003"), TEXT("Idle"),    TEXT("Hero.Ivrin"), ERTPresentationRole::Idle, true,  TEXT("A"));

	int32 Legami = 0;
	const TMap<FName, FRTHeroPresentationClips> PerEroe =
		URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Catalog, Legami);

	// Anti-vacuita': senza legami tradotti ogni asserzione sotto guarderebbe mappe vuote.
	if (!TestEqual(TEXT("tre legami tradotti"), Legami, 3)) { return false; }
	if (!TestEqual(TEXT("due eroi"), PerEroe.Num(), 2)) { return false; }

	const FRTHeroPresentationClips* Aevik = PerEroe.Find(FName(TEXT("Hero.Aevik")));
	if (!TestNotNull(TEXT("Aevik c'e'"), (const void*)Aevik)) { return false; }
	const FRTAnimRoleClips* Move = Aevik->FindRole(ERTPresentationRole::Move);
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
		Attiva->Clip.ToSoftObjectPath().ToString(), PathDi(TEXT("Aevik"), TEXT("Run_Fwd")));

	// Un eroe diverso non finisce nella stessa voce: la mappa e' per eroe, non globale.
	const FRTHeroPresentationClips* Ivrin = PerEroe.Find(FName(TEXT("Hero.Ivrin")));
	if (TestNotNull(TEXT("Ivrin c'e'"), (const void*)Ivrin))
	{
		TestNull(TEXT("Ivrin non ha il ruolo Move"), (const void*)Ivrin->FindRole(ERTPresentationRole::Move));
		TestNotNull(TEXT("Ivrin ha il ruolo Idle"), (const void*)Ivrin->FindRole(ERTPresentationRole::Idle));
	}

	// ⛔ Un binding senza eroe non produce una voce fantasma.
	FRTAnimCatalog Sporco;
	Sporco.NextId = 2;
	FRTAnimCatalogEntry Orfana;
	Orfana.Id = FName(TEXT("AV_0001"));
	Orfana.Derived.AssetPath = PathDi(TEXT("Aevik"), TEXT("Idle"));
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

// ─── Il pool d'azione nel CDO (#3563, spec «la clip per abilita'» §2.3) ──────────────────────────────────────

/**
 * Un binding con `actionId` va in `PerAction[azione].PerRole[ruolo]`, uno senza in `PerRole` — due pool.
 * ✅ Validato per mutazione (P6): lo smistamento sostituito da `Eroe.PerRole.FindOrAdd(Binding.Role)` → cade.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindingsMapToCdoPerActionTest,
	"RefactorTactics.Anim.Bindings.MapToCdoPerAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindingsMapToCdoPerActionTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	AggiungiLegame(Catalog, TEXT("AV_0001"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);
	AggiungiLegame(Catalog, TEXT("AV_0002"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	AggiungiLegame(Catalog, TEXT("AV_0003"), TEXT("Ability_Q_Target"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), false);

	int32 Legami = 0;
	const TMap<FName, FRTHeroPresentationClips> PerEroe = URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Catalog, Legami);
	if (!TestEqual(TEXT("tre legami tradotti"), Legami, 3)) { return false; }
	const FRTHeroPresentationClips* Aevik = PerEroe.Find(FName(TEXT("Hero.Aevik")));
	if (!TestNotNull(TEXT("Aevik c'e'"), (const void*)Aevik)) { return false; }

	const FRTAnimRoleClips* Ruolo = Aevik->FindRole(ERTPresentationRole::Cast);
	if (!TestNotNull(TEXT("il pool di ruolo Cast c'e'"), (const void*)Ruolo)) { return false; }
	TestEqual(TEXT("🔴 il pool di ruolo ha SOLO il binding di ruolo"), Ruolo->Variants.Num(), 1);
	TestEqual(TEXT("ed e' AV_0001, attiva"), Ruolo->ActiveClipVariant, FName(TEXT("AV_0001")));

	const FRTActionPresentationClips* Azione = Aevik->PerAction.Find(FName(TEXT("Hero.Aevik.Overload")));
	const FRTAnimRoleClips* PoolAzione = Azione ? Azione->PerRole.Find(ERTPresentationRole::Cast) : nullptr;
	if (!TestNotNull(TEXT("🔴 il pool d'azione Overload/Cast c'e'"), (const void*)PoolAzione)) { return false; }
	TestEqual(TEXT("con le due varianti d'azione"), PoolAzione->Variants.Num(), 2);
	TestEqual(TEXT("attiva quella dichiarata, AV_0002"), PoolAzione->ActiveClipVariant, FName(TEXT("AV_0002")));
	return true;
}

/**
 * Review Focus (b): un binding d'azione attivo e uno di ruolo attivo per lo stesso `(eroe, ruolo)` sono VALIDI, e a
 * risolvere vince l'azione; senza azione resta il ruolo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindingsActionAndRoleBindingsCoexistTest,
	"RefactorTactics.Anim.Bindings.ActionAndRoleBindingsCoexist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindingsActionAndRoleBindingsCoexistTest::RunTest(const FString&)
{
	FRTAnimCatalog Catalog;
	AggiungiLegame(Catalog, TEXT("AV_0001"), TEXT("Cast"), ERTPresentationRole::Cast, nullptr, true);
	AggiungiLegame(Catalog, TEXT("AV_0002"), TEXT("Throw_Ready"), ERTPresentationRole::Cast, TEXT("Hero.Aevik.Overload"), true);
	if (!TestEqual(TEXT("⛔ premessa: le due attive sono valide"), URTAnimCatalogLibrary::ValidateCatalog(&Catalog).Num(), 0))
	{
		return false;
	}
	int32 Legami = 0;
	const TMap<FName, FRTHeroPresentationClips> PerEroe = URTBuildAnimBindingsCommandlet::BuildClipsPerHero(Catalog, Legami);

	// Il CDO e' l'unico `ActiveClipFor` disponibile: si scrive e si ripristina, come `ConfiguraVariante`.
	URTUnitAnimInstance* Cdo = GetMutableDefault<URTUnitAnimInstance>();
	const TMap<FName, FRTHeroPresentationClips> Salvato = Cdo->ClipsPerHero;
	ON_SCOPE_EXIT{ Cdo->ClipsPerHero = Salvato; };
	Cdo->ClipsPerHero = PerEroe;

	const FName Aevik(TEXT("Hero.Aevik"));
	TestEqual(TEXT("🔴 con l'azione: vince la clip d'azione"),
		Cdo->ActiveClipFor(Aevik, ERTPresentationRole::Cast, FName(TEXT("Hero.Aevik.Overload")), NAME_None).ToSoftObjectPath().ToString(),
		PathDi(TEXT("Gadget"), TEXT("Throw_Ready")));
	TestEqual(TEXT("senza azione: la clip di ruolo"),
		Cdo->ActiveClipFor(Aevik, ERTPresentationRole::Cast).ToSoftObjectPath().ToString(),
		PathDi(TEXT("Gadget"), TEXT("Cast")));
	return true;
}

/**
 * La fusione per pool (`Ruling` di §2.3, ➕ rev2.): eroi e pool che il catalogo non nomina tengono il default.
 * ✅ Validato per mutazione (8): `MergeClipsPerHero` ridotta a `return PerEroe;` → cade «eroe assente».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBindingsMergeKeepsDefaultPoolsTest,
	"RefactorTactics.Anim.Bindings.MergeKeepsDefaultPools",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBindingsMergeKeepsDefaultPoolsTest::RunTest(const FString&)
{
	const FName Aevik(TEXT("Hero.Aevik"));
	const FName Ivrin(TEXT("Hero.Ivrin"));
	const FName Overload(TEXT("Hero.Aevik.Overload"));

	TMap<FName, FRTHeroPresentationClips> Base;
	FRTHeroPresentationClips& BaseAevik = Base.Add(Aevik);
	BaseAevik.PerRole.Add(ERTPresentationRole::Move, PoolDiFusione(TEXT("/Game/Prova/DefMove.DefMove")));
	BaseAevik.PerRole.Add(ERTPresentationRole::Cast, PoolDiFusione(TEXT("/Game/Prova/DefCast.DefCast")));
	Base.Add(Ivrin).PerRole.Add(ERTPresentationRole::Idle, PoolDiFusione(TEXT("/Game/Prova/DefIdle.DefIdle")));

	TMap<FName, FRTHeroPresentationClips> PerEroe;
	FRTHeroPresentationClips& DalCatalogo = PerEroe.Add(Aevik);
	DalCatalogo.PerRole.Add(ERTPresentationRole::Cast, PoolDiFusione(TEXT("/Game/Prova/CatCast.CatCast")));
	DalCatalogo.PerAction.FindOrAdd(Overload).PerRole.Add(ERTPresentationRole::Cast,
		PoolDiFusione(TEXT("/Game/Prova/CatOverload.CatOverload")));

	const TMap<FName, FRTHeroPresentationClips> Fuso = URTBuildAnimBindingsCommandlet::MergeClipsPerHero(Base, PerEroe);

	// 1. Eroe assente dal catalogo: tutti i pool del default.
	const FRTHeroPresentationClips* FusoIvrin = Fuso.Find(Ivrin);
	if (!TestNotNull(TEXT("🔴 eroe assente dal catalogo: resta"), (const void*)FusoIvrin)) { return false; }
	TestEqual(TEXT("con il suo Idle di default"), PathAttivoDi(FusoIvrin->FindRole(ERTPresentationRole::Idle)),
		FString(TEXT("/Game/Prova/DefIdle.DefIdle")));

	// 2. Eroe con solo Cast nel catalogo: Move del default, Cast del catalogo.
	const FRTHeroPresentationClips* FusoAevik = Fuso.Find(Aevik);
	if (!TestNotNull(TEXT("Aevik c'e'"), (const void*)FusoAevik)) { return false; }
	TestEqual(TEXT("🔴 il Move che il catalogo non nomina resta quello di default"),
		PathAttivoDi(FusoAevik->FindRole(ERTPresentationRole::Move)), FString(TEXT("/Game/Prova/DefMove.DefMove")));
	TestEqual(TEXT("il Cast che il catalogo nomina e' quello del catalogo"),
		PathAttivoDi(FusoAevik->FindRole(ERTPresentationRole::Cast)), FString(TEXT("/Game/Prova/CatCast.CatCast")));

	// 3. Il pool d'azione si aggiunge a PerAction senza toccare PerRole.
	const FRTActionPresentationClips* Azione = FusoAevik->PerAction.Find(Overload);
	TestEqual(TEXT("il pool d'azione del catalogo e' entrato"),
		PathAttivoDi(Azione ? Azione->PerRole.Find(ERTPresentationRole::Cast) : nullptr),
		FString(TEXT("/Game/Prova/CatOverload.CatOverload")));
	TestEqual(TEXT("e PerRole ha ancora i suoi due ruoli"), FusoAevik->PerRole.Num(), 2);
	return true;
}

/**
 * I predicati del modello distinguono `(eroe, ruolo)` da `(eroe, ruolo, azione)`: attivare un binding di ruolo non
 * spegne quello d'azione, e viceversa (spec §2.3, il quarto predicato).
 * ✅ Validato per mutazione (7): il ciclo atomico di `MakeActive` senza `&& Binding.ActionId == ActionId` → cade
 * «attivare il ruolo non spegne l'azione».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimBrowserBindingRulesPerActionTest,
	"RefactorTactics.Anim.Browser.BindingRulesPerAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimBrowserBindingRulesPerActionTest::RunTest(const FString&)
{
	FRTAnimBrowserModel M = ModelloDiProva();
	const FName Aevik(TEXT("Hero.Aevik"));
	const FName Overload(TEXT("Hero.Aevik.Overload"));
	const FName Idle(TEXT("AV_0001"));    // Promoted
	const FName Jog(TEXT("AV_0004"));     // Promoted

	auto Attiva = [&M](const FName& Id, const FName& Hero, ERTPresentationRole Role, const FName& ActionId) -> bool
	{
		for (const FRTAnimCatalogEntry& E : M.GetCatalog().Entries)
		{
			if (E.Id != Id) { continue; }
			for (const FRTAnimBinding& B : E.Authored.Bindings)
			{
				if (B.HeroId == Hero && B.Role == Role && B.ActionId == ActionId) { return B.bActive; }
			}
		}
		return false;
	};

	TestTrue(TEXT("lega Idle al ruolo Cast"), M.BindToRole(Idle, Aevik, ERTPresentationRole::Cast));
	TestTrue(TEXT("🔑 la STESSA clip si lega anche all'azione: e' un altro pool, non un duplicato"),
		M.BindToRole(Idle, Aevik, ERTPresentationRole::Cast, Overload));
	TestTrue(TEXT("lega Jog all'azione"), M.BindToRole(Jog, Aevik, ERTPresentationRole::Cast, Overload));

	TestTrue(TEXT("attiva Jog sull'azione"), M.MakeActive(Jog, Aevik, ERTPresentationRole::Cast, Overload));
	TestTrue(TEXT("attiva Idle sul ruolo"), M.MakeActive(Idle, Aevik, ERTPresentationRole::Cast));
	TestTrue(TEXT("🔴 attivare il ruolo non spegne l'azione"), Attiva(Jog, Aevik, ERTPresentationRole::Cast, Overload));
	TestTrue(TEXT("e il ruolo e' attivo"), Attiva(Idle, Aevik, ERTPresentationRole::Cast, NAME_None));
	TestFalse(TEXT("Idle sull'azione resta inattiva"), Attiva(Idle, Aevik, ERTPresentationRole::Cast, Overload));
	TestEqual(TEXT("il catalogo e' valido: una attiva per pool"), URTAnimCatalogLibrary::ValidateCatalog(&M.GetCatalog()).Num(), 0);

	TestTrue(TEXT("unbind del ruolo"), M.Unbind(Idle, Aevik, ERTPresentationRole::Cast));
	TestTrue(TEXT("🔴 non tocca il binding d'azione della stessa clip"),
		M.MakeActive(Idle, Aevik, ERTPresentationRole::Cast, Overload));

	// ⛔ Un pool mai legato non si attiva: Idle ha un binding (Aevik, Cast) ma d'azione Overload, non di questa.
	TestFalse(TEXT("🔴 MakeActive su un (eroe, ruolo, azione) mai legato torna false"),
		M.MakeActive(Idle, Aevik, ERTPresentationRole::Cast, FName(TEXT("Hero.Aevik.MaiLegata"))));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
