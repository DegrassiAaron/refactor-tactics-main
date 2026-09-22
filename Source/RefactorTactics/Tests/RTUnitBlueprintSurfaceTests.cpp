// Il confine della superficie Blueprint di `ARTUnit`: ogni campo che un grafo puo' leggere e' censito, e il
// suo verdetto e' DERIVATO da un test invece che scelto.
//
// Nasce da #1500. `FRTKnowledgeView` e' la porta filtrata e funziona; accanto ha un cancello di servizio
// spalancato — chiunque tenga un `ARTUnit*` legge tutto — e cinque canali di presentazione hanno perso lo
// stesso fatto (la cella di un'unita') per cinque meccanismi diversi, con la suite verde. La causa non e' la
// disattenzione: il confine era scritto in cinque commenti locali coerenti fra loro
// (`Perception/RTKnowledgeVeilPresenter.cpp:66`, `UI/RTScreenHudWidgets.h:176-177`,
// `Debug/RTDebugConsole.cpp:154-155`, `Bot/RTBotPlanning.h:12`, `Perception/RTKnowledgeView.h:10`) e nessun
// gate lo misurava.
//
// 🔑 **La regola, ed e' meccanica.** Un `ARTUnit` non ha un «non c'e'»: il velo nasconde i COMPONENTI, non
// l'actor (`Unit/RTUnit.cpp:540`), quindi un'unita' velata resta nel mondo alla sua posizione vera e risponde
// a chiunque ne tenga il puntatore. Un campo puo' restare esposto **se e solo se l'Actor, rispondendo senza
// filtro, da' la RISPOSTA GIUSTA** — la stessa che un osservatore autorizzato riceverebbe da una porta.
//
// ⛔ **Questo test non toglie nessuna esposizione, e non deve diventare lo strumento per toglierle.** La
// colonna `Verdetto` e' un CENSIMENTO: dice cosa questo repository ha deciso di un campo, non cosa la
// compilazione impone. Cio' che il test impone e' che il censimento sia COMPLETO, COERENTE con la riflessione
// e DERIVATO — e che nessun campo nuovo nasca esposto senza che qualcuno abbia risposto alla domanda.
//
// Stessa forma di `RefactorTactics.Unit.HeroDataCrossesTheBoundary` (tabella bidirezionale, motivo
// obbligatorio, messaggio d'errore che e' un'istruzione) e di
// `RefactorTactics.Privacy.PlanningFieldsOnTheUnitAreNotReplicated` (controllo positivo piantato, soglia
// calcolata invece che scritta).

#include "Misc/AutomationTest.h"

#include "Unit/RTUnit.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** La risposta che l'Actor da' senza filtro cambia con l'osservatore? E' IL test della decisione. */
	enum class ERTRispostaPerOsservatore : uint8
	{
		/**
		 * No: due osservatori qualsiasi che possano vedere il soggetto ricevono lo stesso valore. La fuga,
		 * se c'e', e' nel SOGGETTO e non nel campo — e il gate che la chiude e' quello sul gesto di
		 * acquisizione, non un elenco di campi.
		 */
		Invariante,
		/** Si': l'Actor risponde a tutti allo stesso modo, e per qualcuno quella risposta e' sbagliata. */
		Varia,
	};

	/** Cosa questo repository ha deciso del campo. ⚠️ Un censimento, non un'imposizione del compilatore. */
	enum class ERTVerdetto : uint8
	{
		Ammesso,
		AmmessoConRiserva,
		DaRitirare,
	};

	/**
	 * Una riga del censimento.
	 *
	 * ⚠️ Questa tabella e' la SPECIFICA del confine, non una fotografia del codice. `Porta` non e' un
	 * commento: e' il DTO che consegna lo stesso fatto gia' filtrato per osservatore, e `nullptr` significa
	 * che **non esiste** — cioe' che il debito e' la porta mancante, non lo specificatore.
	 *
	 * 🔴 **`Scrivibile` non e' una ridichiarazione di `BlueprintReadWrite`: e' un'asserzione contro i flag
	 * dell'engine, verificata nei due versi.** E' la parte del gate che prende il giorno stesso chi giri un
	 * `BlueprintReadOnly` in `BlueprintReadWrite` su un'unita', che e' la modifica piu' pericolosa che questo
	 * censimento nomina — precedente gia' pagato in code review su `GuardReduction` (`RTUnit.h:109-114`).
	 */
	struct FRTUnitFieldRule
	{
		const TCHAR* Campo;
		ERTRispostaPerOsservatore Risposta;
		bool bScrivibile;
		const TCHAR* Porta;   // il DTO che consegna lo stesso fatto filtrato; nullptr = non esiste
		ERTVerdetto Verdetto;
		const TCHAR* Motivo;  // sempre obbligatorio, anche su una riga «ammesso»
	};

	/**
	 * Il verdetto DERIVA dalle altre tre colonne, in quest'ordine.
	 *
	 * 🔑 **Senza questa funzione la tabella sarebbe una raccolta di opinioni.** Le tre stesure precedenti del
	 * censimento avevano ciascuna una manciata di campi il cui verdetto non seguiva dalla regola dichiarata —
	 * ed e' il difetto che rende un confine inutilizzabile come specifica: due autori in buona fede
	 * classificano lo stesso campo nuovo in due modi diversi. Qui la regola PRODUCE il verdetto, e il test
	 * rifiuta una riga in cui i due non coincidono.
	 *
	 * ⚠️ La scrittura viene prima della conoscenza, e non e' un ordine arbitrario: un grafo che scrive un
	 * campo che il resolver legge e' una **seconda autorita' su una regola competitiva**, e lo e' anche se
	 * quel campo non rivelasse niente a nessuno.
	 */
	ERTVerdetto VerdettoAtteso(const FRTUnitFieldRule& R)
	{
		if (R.bScrivibile)
		{
			return ERTVerdetto::DaRitirare;
		}
		if (R.Risposta == ERTRispostaPerOsservatore::Invariante)
		{
			return ERTVerdetto::Ammesso;
		}
		// La risposta cambia. Se una porta la consegna gia' filtrata, la lettura sull'Actor e' una seconda
		// copia NON filtrata di qualcosa che funziona. Se non la consegna nessuno, ritirare oggi lascerebbe
		// un consumatore senza sorgente: il debito e' la porta, e la riserva la nomina.
		return (R.Porta != nullptr) ? ERTVerdetto::DaRitirare : ERTVerdetto::AmmessoConRiserva;
	}

	const TCHAR* NomeVerdetto(ERTVerdetto V)
	{
		switch (V)
		{
		case ERTVerdetto::Ammesso:           return TEXT("ammesso");
		case ERTVerdetto::AmmessoConRiserva: return TEXT("ammesso-con-riserva");
		default:                             return TEXT("da-ritirare-in-futuro");
		}
	}

	// Scorciatoie, solo per tenere le 65 righe leggibili in una schermata.
	constexpr ERTRispostaPerOsservatore Inv = ERTRispostaPerOsservatore::Invariante;
	constexpr ERTRispostaPerOsservatore Var = ERTRispostaPerOsservatore::Varia;
	constexpr ERTVerdetto Amm = ERTVerdetto::Ammesso;
	constexpr ERTVerdetto Ris = ERTVerdetto::AmmessoConRiserva;
	constexpr ERTVerdetto Rit = ERTVerdetto::DaRitirare;

	const FRTUnitFieldRule Campi[] = {
	// ─── 1. IL PIANO — sedici campi che un grafo SCRIVE ────────────────────────────────────────────────
	// 🔴 Il difetto qui non e' di lettura: `CLIENT PROPOSES -> SERVER VALIDATES` perde il primo passo. La
	// porta esiste ed e' quella che il repo chiama «il checkpoint intero»
	// (`Debug/RTDebugReportLibrary.cpp:33-38`): un avversario non rivelato non e' «una voce vuota», e'
	// NESSUNA voce (`Turn/RTIntentPrivacyLibrary.cpp:49`), e waypoint, reazione e rotazione dichiarata
	// restano di soli alleati (`:70-77`).
	// ➕ Lo stesso predicato che li riconosce — `IsPlanningFieldName`, `Tests/RTServerOnlyGuardTests.cpp:274`
	// — copre TRE campi di piano che ai Blueprint non sono esposti affatto (`PlannedReactionCondition`,
	// `bMovePlanRejectedByOccupant`, `RejectedMoveDestination`). Quando qualcuno si e' posto la domanda, la
	// risposta e' gia' stata tenerli fuori: il difetto non e' una scelta diversa, e' una scelta mai fatta.
	{ TEXT("PlannedCell"),                Var, true,  TEXT("FRTIntentView"), Rit, TEXT("intento privato della squadra che lo dichiara; un grafo lo SCRIVE") },
	{ TEXT("PlannedPath"),                Var, true,  TEXT("FRTIntentView"), Rit, TEXT("il percorso e' intento: `Reveal` lo espone, la porta decide a chi") },
	{ TEXT("PlannedWaypoints"),           Var, true,  TEXT("FRTIntentView"), Rit, TEXT("copiati SOLO per un alleato, rivelato o meno (RTIntentPrivacyLibrary.cpp:70-77)") },
	{ TEXT("PlannedMovementProfileId"),   Var, true,  TEXT("FRTIntentView"), Rit, TEXT("profilo del movimento pianificato: intento") },
	{ TEXT("PlannedAbilityIndex"),        Var, true,  TEXT("FRTIntentView"), Rit, TEXT("RTMatchStateHash.h:102-104 — «l'intento di usare un'abilita' ... e' privato della squadra che lo dichiara»") },
	{ TEXT("PlannedAttackTarget"),        Var, true,  TEXT("FRTIntentView"), Rit, TEXT("unico TObjectPtr<ARTUnit> dell'header: da un'unita' si raggiunge un'altra unita' COMPLETA senza enumerare") },
	{ TEXT("PlannedAttackCell"),          Var, true,  TEXT("FRTIntentView"), Rit, TEXT("bersaglio dichiarato: intento") },
	{ TEXT("bAttackTargetsCell"),         Var, true,  TEXT("FRTIntentView"), Rit, TEXT("quale delle due forme di bersaglio: intento") },
	{ TEXT("PlannedDashAbility"),         Var, true,  TEXT("FRTIntentView"), Rit, TEXT("lo scatto dichiarato: intento") },
	{ TEXT("PlannedDashCell"),            Var, true,  TEXT("FRTIntentView"), Rit, TEXT("destinazione dello scatto: intento") },
	{ TEXT("PlannedFacing"),              Var, true,  TEXT("FRTIntentView"), Rit, TEXT("RTMatchStateHash.h:70-72 — «E' il facing REALE, non `PlannedFacing`. Quello e' un intento»") },
	{ TEXT("bDeclaresPlannedFacing"),     Var, true,  TEXT("FRTIntentView"), Rit, TEXT("che ci sia una rotazione dichiarata e' gia' informazione di piano") },
	{ TEXT("PlannedCoverEdge"),           Var, true,  TEXT("FRTIntentView"), Rit, TEXT("copertura scelta: intento") },
	{ TEXT("bHasPlannedCoverEdge"),       Var, true,  TEXT("FRTIntentView"), Rit, TEXT("che una copertura sia stata scelta e' gia' informazione di piano") },
	{ TEXT("PlannedReactionAbility"),     Var, true,  TEXT("FRTIntentView"), Rit, TEXT("`ReactionName` si copia SOLO per un alleato") },
	{ TEXT("PlannedCleansePriority"),     Var, true,  TEXT("FRTIntentView"), Rit, TEXT("priorita' di bonifica dichiarata: intento") },

	// ─── 2. LA CELLA ───────────────────────────────────────────────────────────────────────────────────
	// 🔑 L'UNICO dei 65 la cui risposta e' sbagliata anche per un osservatore AUTORIZZATO. Per gli altri
	// quattro campi che `FRTKnowledgeEntry` porta il valore e' identico a quello dell'Actor: cio' che la
	// porta decide per loro e' se la voce ESISTA. Per `Cell` no — su un `Remembered` la porta consegna la
	// cella dell'ultimo contatto, «Mai la posizione vera» (`Perception/RTKnowledgeView.h:48`, `:83`).
	// ⚠️ Il campo resta pubblico: e' la posizione AUTOREVOLE, entra nel digest
	// (`Turn/RTMatchStateHash.cpp:224`) e dall'esterno la scrive solo il resolver
	// (`Turn/RTTurnManager.cpp:2031`, `:4739`). Cio' che si ritira e' lo specificatore, e non tocca nessuno
	// dei due — e' il punto (1) del costo misurato nella voce di decisione.
	{ TEXT("Cell"),                       Var, false, TEXT("FRTKnowledgeEntry::Cell"), Rit, TEXT("il fatto che i cinque canali hanno perso, e l'unico il cui confine era gia' scritto, eseguibile e verde") },

	// ─── 3. I TELL DEL TURNO, E LA CHIUSURA DEL PIANO — nessuna porta li consegna ──────────────────────
	// Non dicono dov'e' ne' cosa fara': dicono che sta facendo qualcosa ADESSO, cioe' cio' che il velo esiste
	// per chiudere. ⚠️ La riserva non e' un'ammissione: e' il debito nominato. Ritirarli oggi lascerebbe un
	// consumatore senza sorgente, e il difetto e' che nessun DTO li consegna filtrati.
	{ TEXT("bIsMovingVisually"),          Var, false, nullptr, Ris, TEXT("su un'unita' velata dice «si sta muovendo ora»; il grafo lavora gia' sulla COPIA (RTUnitAnimInstance.cpp:246-250) e il canale e' dichiarato in FRTPresentationBinding") },
	{ TEXT("ReactionActivationsThisTurn"), Var, false, nullptr, Ris, TEXT("«ha reagito» su un'unita' che non vedi; nato per rendere D-092 osservabile (RTUnit.h:655-670), nessuna vista lo espone") },
	{ TEXT("SelectedAbilityIndex"),       Var, false, nullptr, Ris, TEXT("l'armamento, cioe' un tell PRIMA della dichiarazione; e' anche la FORMA giusta — RO + ARTUnit::SelectAbility che valida e non e' UFUNCTION") },
	{ TEXT("LastDamageToken"),            Var, false, nullptr, Ris, TEXT("«ha appena incassato N»; e' gia' un DTO e Transient fuori da ogni hash (RTUnit.h:1717-1721), ma su un velato resta un tell") },
	{ TEXT("bTurnPlanDeclared"),          Var, false, nullptr, Ris, TEXT("RTUnit.h:578-581 — «leggerebbe il ritmo del tuo planning», e la stessa riga dichiara la porta mancante: «una proiezione da progettare in RTIntentPrivacyLibrary»") },

	// ─── 4. LE DUE LEVE DI SCRITTURA CHE NON SONO PIANO ────────────────────────────────────────────────
	// 🔴 Non e' privacy, e' autorita'. Precedente gia' pagato in code review: `GuardReduction` fu declassato
	// da `BlueprintReadWrite` perche' «sarebbe stato un canale per cui un Blueprint — anche di presentazione
	// — muta un numero che `ARTTurnManager::ResolveCombatPasses` legge per decidere il danno ... e non
	// lascerebbe traccia nel TurnLog» (`RTUnit.h:109-114`).
	{ TEXT("ActiveVariantId"),            Inv, true,  nullptr, Rit, TEXT("ZERO scrittori di produzione e un solo lettore, RTTurnManager.cpp:3789, che da li' sceglie i Parameters della variante: un grafo che lo scrive cambia il danno") },
	{ TEXT("bIsBotControlled"),           Inv, true,  nullptr, Rit, TEXT("decide CHI comanda l'unita'. ⛔ Si ritira il BlueprintReadWrite, NON l'EditAnywhere: e' cosi' che una mappa dichiara un'unita' da bot") },

	// ─── 5. LA CONDIZIONE — la porta la RIFIUTA per decisione ──────────────────────────────────────────
	// ⛔ E nessun'altra la trasporta. `URTHudViewModel::BuildUnitCard` sembra la porta e non lo e': prende
	// `const ARTUnit*` IN FIRMA e nel corpo non c'e' un solo controllo di conoscenza
	// (`UI/RTHudViewModel.cpp:114-139`). Che gli HP nemici oggi non si vedano dipende dal CHIAMANTE
	// (`ResolveObserverTeamIds`, `:743-746`), non dalla porta.
	{ TEXT("Health"),                     Var, false, nullptr, Ris, TEXT("RTBotPlanningLibrary.cpp:417-420 — «gli HP correnti sarebbero la fuga esatta che il canary deve prendere»; RTKnowledgeView.h:53-55 la rifiuta per decisione") },
	{ TEXT("Shield"),                     Var, false, nullptr, Ris, TEXT("stessa disciplina degli HP; entra nel digest (RTMatchStateHash.cpp:226) e nessuna vista filtrata lo porta") },
	{ TEXT("TemporaryShield"),            Var, false, nullptr, Ris, TEXT("l'asimmetria LETTERALE della issue: private con AllowPrivateAccess, il C++ passa da GetTemporaryShield() (:831), il Blueprint legge il campo") },

	// ─── 6. L'IDENTITA' — la porta la consegna con lo STESSO valore ────────────────────────────────────
	// 🔑 La differenza col gruppo 2 e' il punto di tutto il censimento: qui la porta non cambia il VALORE,
	// decide se la voce ESISTA. Il repo lo dice gia' di `TeamId` (`Perception/RTKnowledgeView.h:64-71`), e il
	// bot lo dice del ricordo: «Cio' che la squadra conosce di un ricordo e' l'IDENTITA' ... non la
	// condizione» (`Bot/RTBotPlanningLibrary.cpp:418-419`). ∴ la fuga e' nel SOGGETTO, e il gate che la
	// chiude e' quello sul gesto di acquisizione.
	{ TEXT("StableUnitId"),               Inv, false, TEXT("FRTKnowledgeEntry"), Amm, TEXT("la porta lo consegna identico; e' anche la chiave con cui un ricordo risale a eroe e catalogo") },
	{ TEXT("TeamId"),                     Inv, false, TEXT("FRTKnowledgeEntry"), Amm, TEXT("RTKnowledgeView.h:64-71 — «Non e' informazione nuova ... una voce che esiste e' una voce autorizzata»") },
	{ TEXT("HeroId"),                     Inv, false, TEXT("FRTKnowledgeEntry"), Amm, TEXT("la porta lo consegna identico; e' anche l'ordine stabile del roster (RTScreenHudWidgets.cpp:146-149)") },
	{ TEXT("HeroDisplayName"),            Inv, false, TEXT("FRTKnowledgeEntry"), Amm, TEXT("la porta lo consegna identico") },
	{ TEXT("ControlGroup"),               Inv, false, nullptr, Amm, TEXT("fatto dell'ALLESTIMENTO, non del turno: «quale persona seduta in questa squadra comanda questa unita'» (RTUnit.h:71-82). La porta non lo porta perche' nessun consumatore lo chiede, non perche' lo rifiuti") },

	// ─── 7. IL CATALOGO — dato che il progetto tratta come PUBBLICO, e legge non filtrato apposta ───────
	// ⚠️ L'argomento NON e' «sono funzione di HeroId»: sarebbe falso per tre di essi, perche' `EquipLoadout`
	// duplica `Abilities[0]`, applica la variante d'arma e RICALCOLA i due specchi
	// (`Unit/RTUnit.cpp:1662-1681`). L'argomento vero e' gia' eseguito: nel punto in cui il bot filtra TUTTO
	// il resto per conoscenza, la portata la prende non filtrata e dichiara perche' —
	// «gittate e forme sono catalogo, cioe' dato pubblico» (`Bot/RTBotPlanningLibrary.cpp:385`, `:396-397`).
	{ TEXT("MaxHealth"),                  Inv, false, nullptr, Amm, TEXT("e' il valore PUBBLICO con cui il bot SOSTITUISCE gli HP che non ha diritto di sapere (RTBotPlanningLibrary.cpp:427)") },
	{ TEXT("MoveRange"),                  Inv, false, nullptr, Amm, TEXT("catalogo: destinazione dichiarata di Routes[] (RTHeroDataBoundaryTests.cpp)") },
	{ TEXT("AttackRange"),                Inv, false, nullptr, Amm, TEXT("il bot lo legge NON filtrato da un nemico e lo dichiara pubblico (RTBotPlanningLibrary.cpp:385)") },
	{ TEXT("AttackPower"),                Inv, false, nullptr, Amm, TEXT("stesso specchio di AttackRange, ricalcolato da Abilities[0] dentro l'unita' (RTUnit.cpp:1676-1681)") },
	{ TEXT("VisionRange"),                Inv, false, nullptr, Amm, TEXT("catalogo: destinazione dichiarata di Routes[]") },
	{ TEXT("HearingThreshold"),           Inv, false, nullptr, Amm, TEXT("catalogo: destinazione dichiarata di Routes[]") },
	{ TEXT("PushResistance"),             Inv, false, nullptr, Amm, TEXT("catalogo: destinazione dichiarata di Routes[]") },
	{ TEXT("GuardReduction"),             Inv, false, nullptr, Amm, TEXT("catalogo [D-408]; la sua riga porta gia' il precedente sul perche' NON e' BlueprintReadWrite") },
	{ TEXT("MoveEndPivotMaxSteps"),       Inv, false, nullptr, Amm, TEXT("catalogo ADR-0008 §1: destinazione dichiarata di Routes[]") },
	{ TEXT("DashEndPivotMaxSteps"),       Inv, false, nullptr, Amm, TEXT("catalogo ADR-0008 §1: destinazione dichiarata di Routes[]") },
	{ TEXT("Affinity"),                   Inv, false, nullptr, Amm, TEXT("catalogo: destinazione dichiarata di Routes[]") },
	{ TEXT("Weakness"),                   Inv, false, nullptr, Amm, TEXT("catalogo: destinazione dichiarata di Routes[]") },
	{ TEXT("ReactionProfileId"),          Inv, false, nullptr, Amm, TEXT("catalogo E14.7 [D-047]: destinazione dichiarata di Routes[]") },
	{ TEXT("Abilities"),                  Inv, false, nullptr, Amm, TEXT("il kit e' pubblico quanto l'eroe. ⚠️ In produzione il campo non si legge: si passa da NumAbilities()/GetAbility() (RTTurnManager.cpp:973, :977)") },

	// ─── 8. LA POSA PUBBLICA E LE COSTANTI DI RESA ─────────────────────────────────────────────────────
	// ⚠️ `Facing` E' stato autorevole ed ENTRA nel digest (`RTMatchStateHash.cpp:230`) — e sta qui lo stesso,
	// perche' l'appartenenza al digest e' una domanda di DETERMINISMO, non di conoscenza, e le due sono
	// ortogonali. Questo censimento risponde a una sola delle due, e va detto invece che aggirato.
	// Le altre nove sono costanti autorate: `grep -rn -- "->NOME" Source/` fuori da `Unit/RTUnit.*`, da
	// `Tests/` e dalle righe di commento risponde ZERO per ciascuna.
	{ TEXT("Facing"),                     Inv, false, TEXT("FRTIntentView"), Amm, TEXT("due permessi scritti: «posa attuale: si vede guardando l'unita'» (RTIntentPrivacyLibrary.cpp:64) e la lettura non filtrata del bot (RTBotPlanningLibrary.cpp:445-454)") },
	{ TEXT("VisualZOffset"),              Inv, false, nullptr, Amm, TEXT("«SOLO l'offset di presentazione» (RTUnit.cpp:1161-1167); costante per eroe") },
	{ TEXT("VisualRunSpeed"),             Inv, false, nullptr, Amm, TEXT("costante di resa autorata; zero siti fuori da RTUnit.*") },
	{ TEXT("MeshYawOffset"),              Inv, false, nullptr, Amm, TEXT("costante di resa autorata; zero siti fuori da RTUnit.*") },
	{ TEXT("UnitAnimClass"),              Inv, false, nullptr, Amm, TEXT("costante di resa autorata; zero siti fuori da RTUnit.*") },
	{ TEXT("ContactGhostAnimClass"),      Inv, false, nullptr, Amm, TEXT("costante di resa autorata; zero siti fuori da RTUnit.*") },
	{ TEXT("bFaceMovementDirection"),     Inv, false, nullptr, Amm, TEXT("dichiarato alla riga di Facing come cio' che «resta presentazione»; l'autorevole e' l'altro campo") },
	{ TEXT("ForcedMeshLOD"),              Inv, false, nullptr, Amm, TEXT("taratura visiva autorata; zero siti fuori da RTUnit.*") },
	{ TEXT("bShowFacingArrow"),           Inv, false, nullptr, Amm, TEXT("taratura visiva autorata; zero siti fuori da RTUnit.*") },
	{ TEXT("OverlayWidgetClass"),         Inv, false, nullptr, Amm, TEXT("⛔ protected (RTUnit.h:1866): il BlueprintReadOnly e' l'unica porta esistente, e la sottoclasse Blueprint e' il consumatore previsto") },

	// ─── 9. LE MANIGLIE AI COMPONENT ───────────────────────────────────────────────────────────────────
	// ⚠️ OTTO dei nove sono protected (`:1750, 1753, 1767, 1784, 1799, 1803, 1841, 1857`, dopo il
	// `protected:` di `:1724`). `FacingArrow` e' PUBBLICO: sta a `:1137`, dentro il `public:` aperto a
	// `:1019`. Per gli otto il BlueprintReadOnly e' letteralmente l'unica porta (`grep -c friend` -> 0,
	// nessuna sottoclasse C++), ed e' l'asimmetria della issue in forma misurabile.
	// 🔑 Restano perche' ritirarle non comprerebbe niente: la trasformata di un componente e' la trasformata
	// dell'Actor, e `AActor::K2_GetActorLocation` e' ereditata e BlueprintCallable. Il loro rischio si chiude
	// sul GESTO che procura l'Actor, mai su un elenco di campi.
	{ TEXT("SceneRoot"),                  Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	{ TEXT("Mesh"),                       Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	{ TEXT("TeamRing"),                   Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	{ TEXT("SelectionRing"),              Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	{ TEXT("LeftArm"),                    Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	{ TEXT("RightArm"),                   Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	{ TEXT("ContactGhost"),               Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	{ TEXT("FacingArrow"),                Inv, false, nullptr, Amm, TEXT("⚠️ l'unica delle nove che e' PUBBLICA (RTUnit.h:1137): per lei il BlueprintReadOnly non e' l'unica porta") },
	{ TEXT("OverlayWidget"),              Inv, false, nullptr, Amm, TEXT("maniglia a un componente: non consegna nulla che AActor non consegni gia'") },
	};

	const FRTUnitFieldRule* TrovaRiga(const FString& Nome)
	{
		for (const FRTUnitFieldRule& R : Campi)
		{
			if (Nome == R.Campo)
			{
				return &R;
			}
		}
		return nullptr;
	}

	/**
	 * Le `UFUNCTION` di `ARTUnit` raggiungibili da un grafo.
	 *
	 * 🔴 **Questa seconda tabella nasce da un buco trovato rileggendo la prima.** Un censimento delle sole
	 * `UPROPERTY` dichiara chiusa una porta che non lo e': `RechargeBaseShield` e' `BlueprintCallable` e
	 * SCRIVE `Shield` (`Unit/RTUnit.cpp:422`), che e' campo del digest (`Turn/RTMatchStateHash.cpp:226`). E'
	 * il caso di `GuardReduction` in forma di funzione invece che di campo, e nessun ritiro di specificatore
	 * lo chiude.
	 *
	 * ⚠️ Copre SOLO le funzioni che `ARTUnit` dichiara — cinque. Il canale funzione EREDITATO
	 * (`AActor::K2_GetActorLocation`) e quello di LIBRERIA (`URTPlanValidationLibrary::MakePlanFor`,
	 * `ARTPlayerController::GetInspectedUnit`) restano fuori portata di un gate costruito su questa classe, e
	 * la voce di decisione li dichiara per nome.
	 */
	struct FRTUnitFunctionRule
	{
		const TCHAR* Funzione;
		bool bScriveStatoCanonico;
		ERTVerdetto Verdetto;
		const TCHAR* Motivo;
	};

	const FRTUnitFunctionRule Funzioni[] = {
	{ TEXT("PivotBudget"),        false, Amm, TEXT("BlueprintPure che compone due valori di catalogo (RTUnit.h:207-211): non legge stato di turno") },
	{ TEXT("RechargeBaseShield"), true,  Rit, TEXT("🔴 BlueprintCallable che SCRIVE Shield (RTUnit.cpp:422), campo del digest: seconda autorita' su stato canonico, raggiungibile da un grafo") },
	{ TEXT("PlayAttackMontage"),  false, Amm, TEXT("BlueprintImplementableEvent: il C++ chiama DENTRO il grafo con la clip gia' risolta, il grafo non legge l'unita'") },
	{ TEXT("PlayHitMontage"),     false, Amm, TEXT("BlueprintImplementableEvent: il C++ chiama DENTRO il grafo con la clip gia' risolta") },
	{ TEXT("PlayDefeatMontage"),  false, Amm, TEXT("BlueprintImplementableEvent: il C++ chiama DENTRO il grafo con la clip gia' risolta") },
	};

	const FRTUnitFunctionRule* TrovaFunzione(const FString& Nome)
	{
		for (const FRTUnitFunctionRule& F : Funzioni)
		{
			if (Nome == F.Funzione)
			{
				return &F;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitBlueprintSurfaceTest,
	"RefactorTactics.Unit.BlueprintSurfaceIsCensused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitBlueprintSurfaceTest::RunTest(const FString&)
{
	const UClass* Unita = ARTUnit::StaticClass();

	// --- 0. CONTROLLO POSITIVO, e non e' opzionale -----------------------------------------------------
	// ⛔ Senza questo blocco tutto il resto sarebbe verde a vuoto: un filtro che non vede MAI niente e un
	// filtro rotto producono lo stesso output. Si chiede al rilevatore di VEDERE una proprieta' che deve
	// vedere, prima di credergli quando dice di non vederne — stessa forma di
	// `Tests/RTServerOnlyGuardTests.cpp:333-350`, che pianta un leak vero.
	{
		const FProperty* Visibile = Unita->FindPropertyByName(FName(TEXT("Cell")));
		if (TestNotNull(TEXT("premessa: ARTUnit dichiara `Cell`"), Visibile))
		{
			TestTrue(TEXT("premessa: `Cell` E' visibile ai Blueprint — se cade qui, il filtro non misura niente"),
				Visibile->HasAnyPropertyFlags(CPF_BlueprintVisible));
		}

		// ⚠️ **Controprova della PREMESSA.** `CPF_BlueprintVisible` e non un conteggio di specificatori nel
		// testo dell'header: la distinzione e' stata MISURATA e non prevista. La prima stesura del gate
		// gemello dell'HUD iterava tutte le `UPROPERTY` e segnalava campi riflessi per il GC —
		// «Il criterio sbagliato non era «troppo severo»: diceva una cosa falsa»
		// (`Tests/RTScreenHudWidgetTests.cpp:528-536`). Questi quattro sono `UPROPERTY()` nude e private
		// (`RTUnit.h:975-991`): nessun grafo li raggiunge, e il censimento non deve elencarli.
		for (const TCHAR* Nome : { TEXT("StatusTurns"), TEXT("MarkedByTeam"),
								   TEXT("CellBoundStatuses"), TEXT("AbilityCooldowns") })
		{
			const FProperty* Prop = Unita->FindPropertyByName(FName(Nome));
			if (TestNotNull(*FString::Printf(TEXT("premessa: ARTUnit dichiara `%s`"), Nome), Prop))
			{
				TestFalse(*FString::Printf(
					TEXT("`%s` NON e' visibile ai Blueprint: e' UPROPERTY() nuda, e il digest la legge per accessore"), Nome),
					Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));
			}
		}
	}

	// --- 1. OGNI CAMPO VISIBILE E' CENSITO, E IL SUO VERDETTO E' DERIVATO -------------------------------
	// `ExcludeSuper`: il confine riguarda i campi che questo progetto dichiara, non la meccanica di `AActor`
	// (`bReplicates`, `NetPriority`, `Tags`, `RootComponent`...), che nessuno qui puo' classificare in modo
	// sensato e che cambierebbe a ogni aggiornamento dell'engine.
	// ⚠️ **Ed e' anche il limite da dichiarare**: `AActor::K2_GetActorLocation` e' `BlueprintCallable`, e con
	// `URTHexLibrary::WorldToCellId` (`BlueprintPure`) due nodi ricostruiscono `FRTCellId` senza che nessuna
	// `UPROPERTY` di `ARTUnit` sia toccata. Questo test non lo vede, e la voce di decisione lo dichiara.
	int32 Ispezionate = 0;
	for (TFieldIterator<FProperty> It(Unita, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_BlueprintVisible))
		{
			continue;
		}
		++Ispezionate;

		const FString Nome = It->GetName();
		const FRTUnitFieldRule* Riga = TrovaRiga(Nome);
		if (Riga == nullptr)
		{
			AddError(FString::Printf(
				TEXT("ARTUnit::%s e' visibile ai Blueprint e NON e' censito. Aggiungi la riga in Campi[] di ")
				TEXT("questo test, rispondendo a tre domande: la risposta che l'Actor da' senza filtro cambia ")
				TEXT("con l'osservatore? un grafo puo' scriverlo? quale porta consegna gia' lo stesso fatto ")
				TEXT("filtrato (nullptr se non esiste)? Il verdetto lo deriva il test, non lo scegli tu."), *Nome));
			continue; // non si conta due volte lo stesso difetto: il numero di errori dice quanti campi sono scoperti
		}

		// 🔴 **La colonna `Scrivibile` contro i flag dell'engine, nei due versi.** Non e' coerenza fra due
		// tabelle: e' l'unica asserzione di questo test che misura la realta' invece del censimento, e prende
		// il giorno stesso chi giri un `BlueprintReadOnly` in `BlueprintReadWrite` su un'unita'.
		const bool bScrivibileDaGrafo = !It->HasAnyPropertyFlags(CPF_BlueprintReadOnly);
		if (bScrivibileDaGrafo != Riga->bScrivibile)
		{
			AddError(FString::Printf(
				TEXT("ARTUnit::%s: la riflessione dice scrivibile=%s, Campi[] dichiara %s. Se hai appena ")
				TEXT("aperto la scrittura, fermati: un grafo che scrive un campo che il resolver legge e' una ")
				TEXT("SECONDA AUTORITA' su una regola competitiva e non lascia traccia nel TurnLog — il ")
				TEXT("precedente e' GuardReduction, RTUnit.h:109-114. Se il cambio e' voluto, aggiorna la riga: ")
				TEXT("il verdetto diventera' «da-ritirare-in-futuro» da solo."),
				*Nome, bScrivibileDaGrafo ? TEXT("SI'") : TEXT("NO"),
				Riga->bScrivibile ? TEXT("SI'") : TEXT("NO")));
			continue;
		}

		// ⚠️ Il motivo si verifica NON VUOTO, non buono: `TEXT("x")` lo soddisfa. La riga rende il
		// silenziamento *esplicito*, non *difficile* — chi rivede la PR e' l'unico che puo' respingerlo.
		// ⚠️ **E su 65 righe la barra bassa pesa piu' che su 14**: e' la differenza di scala che la voce di
		// decisione dichiara invece di ereditare. La contromisura non e' alzare la barra del motivo, e' che
		// il VERDETTO non sia libero — il blocco qui sotto.
		TestTrue(*FString::Printf(TEXT("la riga di ARTUnit::%s dichiara un motivo"), *Nome),
			Riga->Motivo != nullptr && FCString::Strlen(Riga->Motivo) > 0);

		const ERTVerdetto Atteso = VerdettoAtteso(*Riga);
		if (Atteso != Riga->Verdetto)
		{
			AddError(FString::Printf(
				TEXT("ARTUnit::%s: dalle risposte segue «%s», la riga dichiara «%s». Cambia una delle due, non ")
				TEXT("lasciarle in disaccordo — un censimento in cui il verdetto non segue dalla regola e' una ")
				TEXT("raccolta di opinioni, e il prossimo autore classifichera' diversamente lo stesso campo."),
				*Nome, NomeVerdetto(Atteso), NomeVerdetto(Riga->Verdetto)));
		}
	}

	// ⛔ **Anti-vacuita', e si CALCOLA quanto si e' guardato invece di darlo per scontato.** Senza questa
	// riga un `ExcludeSuper` su una classe sbagliata, o un filtro che non accende mai, darebbero lo stesso
	// «zero violazioni» di un confine rispettato.
	TestTrue(TEXT("l'iterazione ha davvero guardato delle superfici"), Ispezionate > 0);

	// --- 2. LA TABELLA NON DESCRIVE CAMPI CHE NON ESISTONO PIU' ----------------------------------------
	// Un instradamento orfano e' una regola che nessuno applica, e nasconde il fatto che il campo e' stato
	// rimosso o rinominato. Sono DUE difetti distinti e meritano due messaggi distinti: «non esiste» chiede
	// di aggiornare o togliere; «esiste ma non e' piu' esposto» dice che il ritiro E' AVVENUTO, ed e'
	// l'istruzione che arriva a chi lo sta facendo, nella sua stessa PR.
	for (const FRTUnitFieldRule& R : Campi)
	{
		const FProperty* Prop = Unita->FindPropertyByName(FName(R.Campo));
		if (Prop == nullptr)
		{
			AddError(FString::Printf(
				TEXT("Campi[] cita ARTUnit::%s, che non esiste. Se e' stato RINOMINATO aggiorna il nome qui ")
				TEXT("(non togliere la riga: toglierla perde il verdetto gia' deciso); se e' stato RIMOSSO, ")
				TEXT("togli la riga."), R.Campo));
			continue;
		}
		if (!Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
		{
			AddError(FString::Printf(
				TEXT("ARTUnit::%s esiste ma NON e' piu' visibile ai Blueprint: il ritiro e' avvenuto. Togli la ")
				TEXT("riga da Campi[] e, se il suo verdetto era «da-ritirare-in-futuro», registra la chiusura ")
				TEXT("nel Decision Log accanto alla voce di #1500."), R.Campo));
		}
	}

	// 🔑 **La pinza si chiude qui.** I due versi qui sopra, presi da soli, lascerebbero passare una tabella
	// con una riga duplicata. Il conteggio la prende, e prende anche il caso in cui qualcuno aggiunga una
	// riga «per sicurezza» senza che ci sia il campo dietro.
	TestEqual(TEXT("ogni campo visibile ai Blueprint ha esattamente una riga in Campi[]"),
		Ispezionate, static_cast<int32>(UE_ARRAY_COUNT(Campi)));

	// --- 3. IL CANALE FUNZIONE DI `ARTUnit`, con la stessa pinza ---------------------------------------
	// ⚠️ Solo le funzioni raggiungibili da un grafo sono una superficie: una `UFUNCTION()` nuda esiste per il
	// delegate binding, non per l'autore di un `.uasset`. Stessa maschera del gate dell'HUD
	// (`Tests/RTScreenHudWidgetTests.cpp:541-542`).
	static constexpr uint64 RaggiungibileDaBlueprint = FUNC_BlueprintCallable | FUNC_BlueprintEvent;

	int32 FunzioniIspezionate = 0;
	for (TFieldIterator<UFunction> Fn(Unita, EFieldIteratorFlags::ExcludeSuper); Fn; ++Fn)
	{
		if ((Fn->FunctionFlags & RaggiungibileDaBlueprint) == 0)
		{
			continue;
		}
		++FunzioniIspezionate;

		const FString Nome = Fn->GetName();
		const FRTUnitFunctionRule* Riga = TrovaFunzione(Nome);
		if (Riga == nullptr)
		{
			AddError(FString::Printf(
				TEXT("ARTUnit::%s() e' raggiungibile da un grafo e non e' censita. Aggiungi la riga in ")
				TEXT("Funzioni[]: scrive stato che il resolver legge, oppure no? Una BlueprintCallable che ")
				TEXT("scrive un campo del digest e' una seconda autorita' tanto quanto un BlueprintReadWrite ")
				TEXT("— e' il buco che ha fatto nascere questa seconda tabella (RechargeBaseShield)."), *Nome));
			continue;
		}

		const ERTVerdetto AttesoFn = Riga->bScriveStatoCanonico ? ERTVerdetto::DaRitirare : ERTVerdetto::Ammesso;
		TestEqual(*FString::Printf(TEXT("il verdetto di ARTUnit::%s() segue dalla sua riga"), *Nome),
			static_cast<int32>(Riga->Verdetto), static_cast<int32>(AttesoFn));
		TestTrue(*FString::Printf(TEXT("la riga di ARTUnit::%s() dichiara un motivo"), *Nome),
			Riga->Motivo != nullptr && FCString::Strlen(Riga->Motivo) > 0);
	}

	for (const FRTUnitFunctionRule& F : Funzioni)
	{
		const UFunction* Fn = Unita->FindFunctionByName(FName(F.Funzione), EIncludeSuperFlag::ExcludeSuper);
		if (Fn == nullptr)
		{
			AddError(FString::Printf(
				TEXT("Funzioni[] cita ARTUnit::%s(), che ARTUnit non dichiara piu'. Aggiorna il nome se e' un ")
				TEXT("rinomina, togli la riga se e' una rimozione."), F.Funzione));
			continue;
		}
		if ((Fn->FunctionFlags & RaggiungibileDaBlueprint) == 0)
		{
			AddError(FString::Printf(
				TEXT("ARTUnit::%s() non e' piu' raggiungibile da un grafo: il ritiro e' avvenuto. Togli la riga ")
				TEXT("da Funzioni[] e registralo nel Decision Log."), F.Funzione));
		}
	}

	TestTrue(TEXT("l'iterazione ha davvero guardato delle funzioni"), FunzioniIspezionate > 0);
	TestEqual(TEXT("ogni UFUNCTION raggiungibile da un grafo ha esattamente una riga in Funzioni[]"),
		FunzioniIspezionate, static_cast<int32>(UE_ARRAY_COUNT(Funzioni)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
