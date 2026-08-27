#include "Turn/RTMatchSetupLibrary.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Terrain/RTTerrainData.h"

namespace
{
	/**
	 * Cella della fixture showcase: superficie richiesta, costo dettato dal CATALOGO terreni.
	 * Il costo non si scrive qui, altrimenti ribilanciare `Rough` lascerebbe la showcase su un numero morto.
	 * (Nome prefissato per dominio: nella unity build questo file condivide l'unita' di traduzione con altri.)
	 */
	FRTHexCellData MakeShowcaseTerrainCell(const FRTCellId& Id, ERTHexSurface Surface)
	{
		FRTHexCellData Cell(Id);
		Cell.Surface = Surface;
		Cell.MoveCost = URTTerrainLibrary::FindTerrainDef(Surface).MoveCost;
		return Cell;
	}

	/**
	 * L'arena si costruisce in MEMORIA e si consegna all'asset in **una** revisione (`#1435`, [D-201]).
	 *
	 * `URTHexMapAsset::AddOrUpdateCell` e `AddTransition` fanno `++Revision` a ogni chiamata, quindi un
	 * builder che le chiama per cella consegna a `CurrentGraphRevision()` un numero a forma di **conteggio
	 * celle** — 91 per un raggio 5. Quel numero `AppendLogEntry` lo stampiglia in ogni voce di TurnLog, dove
	 * entra nell'hash ([D-067]) e nell'hash di stato partita. `Revision` significa «quante volte questo
	 * grafo e' cambiato»: un'arena appena costruita che ne dichiara 91 dice il falso.
	 *
	 * [D-196] lo aveva corretto per `MakeFlatArena` soltanto, rendendo quel builder **l'eccezione invece
	 * della regola** — e il prossimo builder avrebbe ereditato il pattern sbagliato. Questa e' la regola.
	 *
	 * ⚠️ `Set` **sostituisce** la cella con lo stesso id, come faceva `AddOrUpdateCell`: i builder
	 * stendono una base e poi la riscrivono a pezzi, e senza la sostituzione l'ordine delle passate
	 * cambierebbe l'arena.
	 */
	struct FRTArenaDraft
	{
		TArray<FRTHexCellData> Cells;
		TArray<FRTHexEdge> Transitions;
		TMap<FRTCellId, int32> Where;

		void Set(const FRTHexCellData& Cell)
		{
			if (const int32* Found = Where.Find(Cell.Id))
			{
				Cells[*Found] = Cell;
				return;
			}
			Where.Add(Cell.Id, Cells.Add(Cell));
		}

		/** La cella gia' stesa, per le passate che la MODIFICANO invece di rifarla. `nullptr` se non c'e'. */
		const FRTHexCellData* Find(const FRTCellId& Id) const
		{
			const int32* Found = Where.Find(Id);
			return Found ? &Cells[*Found] : nullptr;
		}

		/** Un arco fra layer. Bidirezionale come il default di `AddTransition`, e senza duplicati per verso. */
		void Link(const FRTCellId& From, const FRTCellId& To, int32 Cost,
			ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair, bool bBidirectional = true)
		{
			auto Upsert = [this](const FRTCellId& A, const FRTCellId& B, int32 InCost, ERTHexTransitionKind InKind)
			{
				for (FRTHexEdge& E : Transitions)
				{
					if (E.From == A && E.To == B) { E.Cost = InCost; E.Kind = InKind; return; }
				}
				Transitions.Add(FRTHexEdge(A, B, InCost, InKind));
			};
			Upsert(From, To, Cost, Kind);
			if (bBidirectional) { Upsert(To, From, Cost, Kind); }
		}

		/**
		 * Consegna e ordina. ⚠️ `ReplaceContent` con celle E transizioni insieme: consegnarle in due
		 * chiamate rimetterebbe la revisione a due, e due non e' uno.
		 */
		void CommitTo(URTHexMapAsset* Asset) const
		{
			if (!Asset) { return; }
			Asset->ReplaceContent(Cells, Transitions);
			Asset->SortCells();
		}
	};
}

TArray<FRTCellId> URTMatchSetupLibrary::PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer)
{
	TArray<FRTCellId> Result;
	if (!Map || NumPerTeam <= 0)
	{
		return Result;
	}

	// Celle percorribili del layer. CellsInLayer garantisce gia' l'ordine stabile (Layer, X, Y): nessuna
	// dipendenza dall'ordine di una TMap, quindi l'allestimento e' deterministico (invariante #4).
	TArray<FRTCellId> Walkable;
	for (const FRTCellId& Id : Map->CellsInLayer(Layer))
	{
		const FRTHexCellData* Data = Map->FindCell(Id);
		if (Data && !Data->bBlocksMovement)
		{
			Walkable.Add(Id);
		}
	}

	// Non si allestisce a meta': o ci stanno tutte le unita', o il chiamante non allestisce affatto.
	if (Walkable.Num() < NumPerTeam * 2)
	{
		return Result;
	}

	// Team 0 dall'inizio dell'ordine, team 1 dalla fine: le squadre partono agli estremi della mappa.
	Result.Reserve(NumPerTeam * 2);
	for (int32 i = 0; i < NumPerTeam; ++i)
	{
		Result.Add(Walkable[i]);
	}
	for (int32 i = 0; i < NumPerTeam; ++i)
	{
		Result.Add(Walkable[Walkable.Num() - 1 - i]);
	}
	return Result;
}

TMap<FRTCellId, int32> URTMatchSetupLibrary::BuildOccupancy(const TArray<FRTCellId>& Cells,
	const TArray<int32>& UnitIds, const TArray<bool>& Alive)
{
	TMap<FRTCellId, int32> Occupancy;
	if (Cells.Num() != UnitIds.Num() || Cells.Num() != Alive.Num())
	{
		return Occupancy;
	}

	// Le unita' non vive non occupano celle (stessa regola di FRTHexSnapshot::Occupancy).
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		if (Alive[i])
		{
			Occupancy.Add(Cells[i], UnitIds[i]);
		}
	}
	return Occupancy;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeDemoArena(UObject* Outer, int32 Radius)
{
	if (Outer == nullptr || Radius < 1)
	{
		return nullptr; // nessuna arena a meta': il chiamante decide cosa fare senza mappa
	}

	// Un'arena piatta e' un'arena piatta: la differenza con `MakeFlatArena` era solo la guardia sul raggio
	// (qui `< 1`, perche' una demo di raggio zero non e' una demo). Il corpo era identico, riga per riga.
	return MakeFlatArena(Outer, Radius);
}

URTHexMapAsset* URTMatchSetupLibrary::MakeFlatArena(UObject* Outer, int32 Radius, const FRTCellId& Center)
{
	if (Outer == nullptr || Radius < 0)
	{
		return nullptr;
	}

	// **UNA revisione: generare un'arena e' un evento, non 127** ([D-196], `#1423`).
	//
	// `AddOrUpdateCell` fa `++Revision` a ogni chiamata, quindi un raggio 6 ne produceva 127 — e quel numero
	// finisce in `GraphRevision`, che `AppendLogEntry` stampiglia in ogni voce del TurnLog. Cambiarlo cambia
	// l'IDENTITA' delle tracce archiviate, ed e' la ragione per cui la correzione e' stata rimandata due
	// volte invece di essere fatta di passaggio: il corpus golden e' stato rigenerato nella stessa PR che
	// l'ha fatta, dichiarando il perche' come chiede il DoD di CP 12.6.
	//
	// `ReplaceContent` e non `UpdateCells`: l'asset e' appena stato creato ed e' provabilmente VUOTO, quindi
	// costruire una `Lookup` da N voci per sbagliare N `Find` prima di appendere e' lavoro per niente. Il
	// suo commento dichiara proprio questo caso — *«"rimpiazza tutto" finiva scritto a mano dai chiamanti,
	// con un `AddOrUpdateCell` per cella»* — ed e' il chiamante che gli mancava.
	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);
	FRTArenaDraft Draft;
	const TArray<FRTCellId> Ids = URTHexLibrary::HexArea(Center, Radius);
	Draft.Cells.Reserve(Ids.Num());
	for (const FRTCellId& Id : Ids)
	{
		Draft.Set(FRTHexCellData(Id));
	}
	Draft.CommitTo(Arena);
	return Arena;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeTestArena(UObject* Outer)
{
	if (Outer == nullptr)
	{
		return nullptr;
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);
	FRTArenaDraft Draft;

	// Base: esagono pieno di raggio 4 sul layer 0, pavimento a costo 1.
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4))
	{
		Draft.Set(FRTHexCellData(Id));
	}

	// Ostacoli al MOVIMENTO, sparsi. Le celle di partenza stanno agli estremi (q=-4 e q=+4, vedi
	// PickStartCells): gli ostacoli non le toccano, cosi' la partita si allestisce comunque.
	for (const FRTCellId& Id : { FRTCellId(-1, 2, 0), FRTCellId(1, -2, 0), FRTCellId(2, 1, 0) })
	{
		FRTHexCellData Cell(Id);
		Cell.bBlocksMovement = true;
		Draft.Set(Cell);
	}

	// Muro che blocca la VISTA lungo q=0: separa le due meta' del campo restando attraversabile. Copre r=-2..2
	// perche' una linea fra i due estremi deriva di qualche riga: un muro piu' corto la lascerebbe passare di lato.
	for (int32 R = -2; R <= 2; ++R)
	{
		FRTHexCellData Cell(FRTCellId(0, R, 0));
		Cell.bBlocksLineOfSight = true;
		Draft.Set(Cell);
	}

	// Fascia di fango a q=-2: costo 3, non un muro. Serve a vedere il budget mordere (una cella "costa" tre passi).
	for (int32 R = -1; R <= 1; ++R)
	{
		FRTHexCellData Cell(FRTCellId(-2, R, 0));
		Cell.Surface = ERTHexSurface::Rough;
		Cell.MoveCost = 3;
		Draft.Set(Cell);
	}

	// Piattaforma sul layer 1, sopra il quadrante destro.
	for (const FRTCellId& Id : { FRTCellId(2, -1, 1), FRTCellId(2, 0, 1), FRTCellId(3, -1, 1), FRTCellId(3, 0, 1) })
	{
		Draft.Set(FRTHexCellData(Id));
	}

	// UNA sola transizione terra->piattaforma: i layer si collegano solo con archi espliciti, quindi togliendola
	// la piattaforma torna irraggiungibile. E' cio' che rende verificabile "il path FALLISCE, non teletrasporta".
	Draft.Link(FRTCellId(1, 0, 0), FRTCellId(2, 0, 1), /*Cost=*/ 2);

	Draft.CommitTo(Arena);
	return Arena;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeShowcaseRelayLiteArena(UObject* Outer)
{
	if (Outer == nullptr)
	{
		return nullptr;
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);
	FRTArenaDraft Draft;

	// Base: esagono pieno di raggio 5 sul layer 0 -> 3*5*6 + 1 = 91 celle di pavimento.
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5))
	{
		Draft.Set(MakeShowcaseTerrainCell(Id, ERTHexSurface::Floor));
	}

	// Le superfici stanno in COPPIE SPECULARI (q,r) / (-q,-r): il centro (0,0) e' l'unico punto fisso.
	// La simmetria non e' estetica — e' cio' che rende un esito attribuibile alle scelte e non al lato.
	struct FShowcasePatch
	{
		FRTCellId Cell;
		ERTHexSurface Surface;
	};
	const FShowcasePatch Patches[] = {
		// Spina d'acqua al centro: applica `Wet` a chi entra e conduce (il payoff elettrico e' CP 8.3).
		{ FRTCellId( 0,  0, 0), ERTHexSurface::ShallowWater },
		{ FRTCellId( 0, -1, 0), ERTHexSurface::ShallowWater },
		{ FRTCellId( 0,  1, 0), ERTHexSurface::ShallowWater },
		// Conduttivo a contatto con l'acqua: la rete esiste gia' come dato, nessuna regola nuova.
		{ FRTCellId( 1, -1, 0), ERTHexSurface::Conductive },
		{ FRTCellId(-1,  1, 0), ERTHexSurface::Conductive },
		// Rough: vieta Dash/Charge su una via d'avvicinamento, cosi' la mobilita' rapida ha un prezzo.
		{ FRTCellId(-2, -1, 0), ERTHexSurface::Rough },
		{ FRTCellId( 2,  1, 0), ERTHexSurface::Rough },
		// Ice: chi TERMINA il Move qui scivola di una cella.
		{ FRTCellId(-2,  2, 0), ERTHexSurface::Ice },
		{ FRTCellId( 2, -2, 0), ERTHexSurface::Ice },
		// Fire: 10 danni + `Burning` a chi entra, dal catalogo terreni.
		{ FRTCellId( 0, -2, 0), ERTHexSurface::Fire },
		{ FRTCellId( 0,  2, 0), ERTHexSurface::Fire },
		// Smoke: cap del targeting a 2 celle attraverso la cella.
		{ FRTCellId(-1, -2, 0), ERTHexSurface::Smoke },
		{ FRTCellId( 1,  2, 0), ERTHexSurface::Smoke },
	};
	for (const FShowcasePatch& Patch : Patches)
	{
		Draft.Set(MakeShowcaseTerrainCell(Patch.Cell, Patch.Surface));
	}

	Draft.CommitTo(Arena);
	return Arena;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeShowcaseRelayBasinArena(UObject* Outer)
{
	if (Outer == nullptr)
	{
		return nullptr;
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);
	FRTArenaDraft Draft;

	// --- Forma: 45 celle, sette righe. Non e' un esagono pieno: e' un bacino, piu' largo al centro. --------
	// L'estensione per riga viene dalla specifica della showcase; la somma (3+5+7+9+9+7+5) e' fissata dal
	// test, cosi' una riga aggiunta per sbaglio non passa inosservata.
	struct FBasinRow { int32 R; int32 MinQ; int32 MaxQ; };
	static const FBasinRow Rows[] = {
		{ -3, -1, 1 }, { -2, -2, 2 }, { -1, -3, 3 }, { 0, -4, 4 }, { 1, -4, 4 }, { 2, -3, 3 }, { 3, -2, 2 }
	};
	for (const FBasinRow& Row : Rows)
	{
		for (int32 Q = Row.MinQ; Q <= Row.MaxQ; ++Q)
		{
			Draft.Set(MakeShowcaseTerrainCell(FRTCellId(Q, Row.R, 0), ERTHexSurface::Floor));
		}
	}

	// --- Superfici ----------------------------------------------------------------------------------------
	// Ogni cella compare UNA volta: e' la coerenza che la specifica chiede di verificare, e qui e' garantita
	// dalla forma della tabella invece che da un controllo da ricordare.
	//
	// ⚠️ **Questa tabella ha un secondo committente oltre allo scenario a otto turni** (#1267): `Conductive`,
	// `Ice`, `Smoke` e `HighGround` sono le quattro superfici che `D-183` chiede di distinguere per FORMA
	// (CP 47.3, #956), e Basin e' la sola fixture che le contenga tutte — le altre rendono `Floor` piu'
	// `Rough` e nient'altro. L'invariante e' **almeno una cella per ciascuna delle quattro**, non tutte e
	// nove: le due `HighGround` sono le insostituibili, perche' sono le uniche che `RelayLite` non ha.
	// Chi cambia una superficie qui guardi anche quel checkpoint, non solo il turno che stava sistemando.
	struct FBasinPatch { FRTCellId Cell; ERTHexSurface Surface; };
	static const FBasinPatch Patches[] = {
		// Corridoio ovest: Gadget ci passa al turno 1, `MistVeil` ne aggiunge al turno 5.
		{ FRTCellId(-3,  0, 0), ERTHexSurface::Smoke },
		{ FRTCellId(-2,  0, 0), ERTHexSurface::Smoke },
		// Lane d'acqua di Phase: conduttiva, ed e' cio' che rende possibile il payoff elettrico del turno 7.
		{ FRTCellId(-3,  1, 0), ERTHexSurface::ShallowWater },
		{ FRTCellId(-2,  1, 0), ERTHexSurface::ShallowWater },
		{ FRTCellId(-1,  1, 0), ERTHexSurface::ShallowWater },
		{ FRTCellId( 0,  1, 0), ERTHexSurface::ShallowWater },
		// Prosegue verso est: il `ConductiveNode` del turno 2 collega questa tratta all'acqua.
		{ FRTCellId( 1,  1, 0), ERTHexSurface::Conductive },
		{ FRTCellId( 2,  1, 0), ERTHexSurface::Conductive },
		// Sbarra la via diretta est->Relay ai movimenti lineari: invalida il `Ram` del turno 7.
		{ FRTCellId( 1,  0, 0), ERTHexSurface::Rough },
		{ FRTCellId( 2,  0, 0), ERTHexSurface::Rough },
		// Fascia di fuoco sull'approccio NORD al Relay: Wraith la attraversa al turno 3 scendendo dalla
		// cresta, e prende `Burning`. NON sta accanto agli spawn: messa a (3,0)/(3,1) murava Riktor nel suo
		// angolo — ogni sua uscita passava dal fuoco, e il turno 1 sarebbe stato una punizione, non una scelta.
		{ FRTCellId( 2, -1, 0), ERTHexSurface::Fire },
		{ FRTCellId( 1, -1, 0), ERTHexSurface::Fire },
		// Cresta nord-est: vantaggio GEOMETRICO, nessun bonus numerico (D-024).
		{ FRTCellId( 2, -2, 0), ERTHexSurface::HighGround },
		{ FRTCellId( 3, -1, 0), ERTHexSurface::HighGround },
		// Ripiano sud: chi TERMINA il Move qui scivola, in modo deterministico (turno 7).
		{ FRTCellId(-1,  2, 0), ERTHexSurface::Ice },
		{ FRTCellId( 0,  2, 0), ERTHexSurface::Ice },
		{ FRTCellId( 1,  2, 0), ERTHexSurface::Ice },
	};
	for (const FBasinPatch& Patch : Patches)
	{
		Draft.Set(MakeShowcaseTerrainCell(Patch.Cell, Patch.Surface));
	}

	// --- Elementi di bordo --------------------------------------------------------------------------------
	// Copertura bassa sull'approccio NORD al Relay: da quel lato ci si avvicina riparati, dagli altri no.
	// La direzione si CHIEDE alla libreria invece di scriverla a mano: se la convenzione dei sei lati
	// cambiasse, un valore inciso qui diventerebbe silenziosamente il bordo sbagliato.
	if (const FRTHexCellData* RelayCell = Draft.Find(FRTCellId(0, 0, 0)))
	{
		ERTHexDirection Edge;
		if (URTHexCoverLibrary::EdgeDirection(FRTCellId(0, 0, 0), FRTCellId(0, -1, 0), Edge))
		{
			FRTHexCellData Updated = *RelayCell;
			Updated.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low,
				FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
			Draft.Set(Updated);
		}
	}

	// Il gate della lane sud: una PORTA chiusa (CP 9.3), non un meccanismo nuovo. Aperta, la revisione della
	// mappa sale e un percorso che prima non esisteva diventa percorribile — ed e' cio' che
	// `Spec.Map.InteractOpensDoor` misura, su questa fixture e su questo bordo.
	// ⛔ Questa riga diceva «Riktor la apre al turno 5» come se fosse un fatto. Non lo e': il T5 di
	// `RT_Showcase_Relay_v01.json` NON dichiara quell'intento, e la sua `_nota_apertura_del_gate` dice
	// che cosa manca. La fixture spedisce il varco; chi lo apre, e quando, lo decide lo scenario.
	if (const FRTHexCellData* GateCell = Draft.Find(FRTCellId(0, 1, 0)))
	{
		ERTHexDirection Edge;
		if (URTHexCoverLibrary::EdgeDirection(FRTCellId(0, 1, 0), FRTCellId(1, 1, 0), Edge))
		{
			FRTHexCellData Updated = *GateCell;
			Updated.Doors.Add(FRTHexDoor(Edge, ERTHexDoorState::Closed));
			Draft.Set(Updated);
		}
	}

	Draft.CommitTo(Arena);
	return Arena;
}

namespace
{
	/**
	 * **L'elenco delle fixture, in UN posto solo** (`#1459`).
	 *
	 * Era scritto a mano in tre punti — questa if-chain, la doc di `MakeFixtureArena` e il messaggio d'errore
	 * di `GenerateFixtureIntoAsset` — e nessuno dei tre coincideva: due nominavano `DemoArena`, che non aveva
	 * un ramo, e omettevano `ArenaV01`, che ce l'aveva. Chi chiedeva `DemoArena` riceveva «fixture
	 * sconosciuta» seguito da un elenco che la conteneva.
	 *
	 * ⚠️ **`DemoArena` non torna**: `MakeDemoArena` vuole un raggio, e l'interfaccia per NOME non ha modo
	 * di fornirlo. Sceglierne uno qui sarebbe una decisione di contenuto presa di passaggio — chi vuole
	 * un'arena piatta chiama `MakeFlatArena`, che il raggio lo chiede.
	 *
	 * ⚠️ Il commento di `MakeFixtureArena` dichiara che l'elenco chiuso e' una scelta contro un registry
	 * a runtime, per non avere «una fixture registrata da qualche parte e non da un'altra». Quel difetto e'
	 * arrivato lo stesso, per la strada della prosa: la tabella lo chiude davvero.
	 */
	struct FFixtureBuilder
	{
		const TCHAR* Id;
		URTHexMapAsset* (*Make)(UObject*);
	};

	const FFixtureBuilder GFixtures[] = {
		{ TEXT("RelayBasin"), &URTMatchSetupLibrary::MakeShowcaseRelayBasinArena },
		{ TEXT("RelayLite"),  &URTMatchSetupLibrary::MakeShowcaseRelayLiteArena  },
		{ TEXT("TestArena"),  &URTMatchSetupLibrary::MakeTestArena               },
		{ TEXT("ArenaV01"),   &URTMatchSetupLibrary::MakeArenaV01                },
		{ TEXT("CoverYard"),  &URTMatchSetupLibrary::MakeCoverYardArena          },
	};
}

TArray<FString> URTMatchSetupLibrary::KnownFixtureIds()
{
	TArray<FString> Ids;
	Ids.Reserve(UE_ARRAY_COUNT(GFixtures));
	for (const FFixtureBuilder& Entry : GFixtures)
	{
		Ids.Add(Entry.Id);
	}
	return Ids;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeFixtureArena(UObject* Outer, const FString& FixtureId)
{
	if (Outer == nullptr || FixtureId.IsEmpty())
	{
		return nullptr;
	}

	for (const FFixtureBuilder& Entry : GFixtures)
	{
		if (FixtureId.Equals(Entry.Id, ESearchCase::IgnoreCase))
		{
			return Entry.Make(Outer);
		}
	}

	// Sconosciuta: nessuna arena. Inventarne una vuota farebbe girare la partita e produrrebbe un fallimento
	// che parla di unita' fuori mappa invece che del nome sbagliato.
	return nullptr;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeCoverYardArena(UObject* Outer)
{
	if (Outer == nullptr)
	{
		return nullptr;
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);
	FRTArenaDraft Draft;

	// Esagono pieno di raggio 3, tutto pavimento: 3*3*6 + 1 = 37 celle. Nessuna superficie, di proposito —
	// qui si studiano i BORDI, e un terreno che cambia costo o visibilita' offrirebbe una seconda spiegazione
	// a ogni esito.
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 3))
	{
		Draft.Set(MakeShowcaseTerrainCell(Id, ERTHexSurface::Floor));
	}

	// I due bordi. La direzione si CHIEDE alla libreria invece di scriverla a mano: se la convenzione dei sei
	// lati cambiasse, un valore inciso qui diventerebbe silenziosamente il bordo sbagliato (stessa disciplina
	// del Relay Basin).
	struct FYardCover
	{
		FRTCellId From;
		FRTCellId To;
		ERTHexCoverType Type;
	};
	static const FYardCover Covers[] = {
		// Barriera ALTA sulla riga centrale: nega vista, passo e proiettili nei due versi (integrita' 50).
		{ FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), ERTHexCoverType::High },
		// Copertura BASSA una riga sotto, sulla stessa direzione: si passa e si vede, ma il danno diretto dal
		// lato riparato cala di 10. Il confronto fra le due sta tutto in una cella di differenza.
		{ FRTCellId(0, 1, 0), FRTCellId(1, 1, 0), ERTHexCoverType::Low },
	};
	for (const FYardCover& Cover : Covers)
	{
		const FRTHexCellData* Cell = Draft.Find(Cover.From);
		ERTHexDirection Edge;
		if (Cell && URTHexCoverLibrary::EdgeDirection(Cover.From, Cover.To, Edge))
		{
			FRTHexCellData Updated = *Cell;
			Updated.Covers.Add(FRTHexCover(Edge, Cover.Type, FRTHexCover::DefaultIntegrity(Cover.Type)));
			Draft.Set(Updated);
		}
	}

	Draft.CommitTo(Arena);
	return Arena;
}

TArray<FRTShowcaseSpawn> URTMatchSetupLibrary::GetShowcaseRelayBasinSpawns()
{
	// Estremi opposti del bacino, sulle due righe centrali. Celle di pavimento: nessuna squadra comincia
	// dentro un terreno che la penalizza al primo passo.
	return {
		FRTShowcaseSpawn(TEXT("Hero.Gadget"),    /*TeamId=*/ 0, FRTCellId(-4, 0, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Phase"),    /*TeamId=*/ 0, FRTCellId(-4, 1, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Riktor"), /*TeamId=*/ 1, FRTCellId( 4, 0, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Wraith"),  /*TeamId=*/ 1, FRTCellId( 4, 1, 0)),
	};
}

TArray<FRTShowcaseSpawn> URTMatchSetupLibrary::GetShowcaseRelayLiteSpawns()
{
	// Estremi opposti dell'arena, in coppie speculari come le superfici. Celle di pavimento: nessuna squadra
	// comincia dentro un terreno che la penalizza al primo passo.
	return {
		FRTShowcaseSpawn(TEXT("Hero.Gadget"),    /*TeamId=*/ 0, FRTCellId(-5,  2, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Phase"),    /*TeamId=*/ 0, FRTCellId(-5,  3, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Riktor"), /*TeamId=*/ 1, FRTCellId( 5, -2, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Wraith"),  /*TeamId=*/ 1, FRTCellId( 5, -3, 0)),
	};
}

URTHexMapAsset* URTMatchSetupLibrary::MakeArenaV01(UObject* Outer)
{
	if (Outer == nullptr)
	{
		return nullptr;
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);
	FRTArenaDraft Draft;

	// Esagono pieno di raggio 4: 61 celle. Gli spawn li derivera' PickStartCells dai due estremi dell'ordine
	// stabile (X, Y), quindi cadranno su (-4,0) e (4,0): l'asse su cui e' costruito tutto il resto.
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4))
	{
		Draft.Set(MakeShowcaseTerrainCell(Id, ERTHexSurface::Floor));
	}

	// ⚠️ Legge dal DRAFT e non dall'asset: l'asset resta vuoto fino a `CommitTo`, quindi `ContainsCell`
	// direbbe sempre di no e questa passata non farebbe niente (`#1435`).
	auto SetCell = [&Draft](const FRTCellId& Id, bool bBlocksMovement, bool bBlocksSight, ERTHexSurface Surface)
	{
		const FRTHexCellData* Existing = Draft.Find(Id);
		if (!Existing) { return; }
		FRTHexCellData Cell = *Existing;
		Cell.bBlocksMovement = bBlocksMovement;
		Cell.bBlocksLineOfSight = bBlocksSight;
		Cell.Surface = Surface;
		Cell.MoveCost = URTTerrainLibrary::FindTerrainDef(Surface).MoveCost;
		Draft.Set(Cell);
	};

	// 1. Barriera verticale con DUE SOLE PORTE. E' la struttura che rende la mappa una scelta invece di un
	//    campo aperto: senza, il pathfinding trova due varianti della stessa strada a nord e la meta' sud non
	//    la percorre nessuno — misurato, non supposto.
	const int32 NorthGate = -3;
	const int32 SouthGate = 3;
	for (int32 R = -4; R <= 4; ++R)
	{
		if (R == NorthGate || R == SouthGate) { continue; }
		SetCell(FRTCellId(0, R, 0), /*Move=*/ true, /*Sight=*/ true, ERTHexSurface::Floor);
	}

	// 2. Spalle della barriera sull'asse degli spawn: portano a due il numero di celle che bloccano la vista
	//    sul segmento, che e' cio' che il criterio della copertura pretende (una sola non basta).
	SetCell(FRTCellId(-1, 0, 0), /*Move=*/ true, /*Sight=*/ true, ERTHexSurface::Floor);
	SetCell(FRTCellId( 1, 0, 0), /*Move=*/ true, /*Sight=*/ true, ERTHexSurface::Floor);

	// 3. Fango su ENTRAMBE le porte: qualunque via si scelga, il budget morde. Una zona costosa che si aggira
	//    senza rinunciare a nulla non e' una scelta, e l'A* la eviterebbe rendendola invisibile al criterio.
	SetCell(FRTCellId(0, NorthGate, 0), /*Move=*/ false, /*Sight=*/ false, ERTHexSurface::Rough);
	SetCell(FRTCellId(0, SouthGate, 0), /*Move=*/ false, /*Sight=*/ false, ERTHexSurface::Rough);

	// 4. Schermo davanti alla via MERIDIONALE: blocca la VISTA e non il passo.
	//
	//    E' la correzione di un errore misurato: con lo schermo che bloccava anche il movimento, il corridoio
	//    sud-orientale spariva e restava una rotta sola. Una cella che blocca solo la vista si attraversa — ed
	//    e' esattamente cio' che serve a una rotta coperta ma percorribile.
	//
	//    Nasconde il TRATTO CENTRALE della via, non il suo sbocco: le ultime celle prima dello spawn avversario
	//    saranno viste comunque, e va bene — l'esposizione si misura in frazione, non sull'ultima cella.
	for (int32 Q = 2; Q <= 3; ++Q)
	{
		SetCell(FRTCellId(Q, 1, 0), /*Move=*/ false, /*Sight=*/ true, ERTHexSurface::Floor);
	}

	// 5. Terreno accidentato sulla via meridionale, quella che lo schermo tiene nascosta.
	//    Mette le due rotte nel verso giusto: chi vuole restare coperto paga, chi ha fretta si espone. Senza,
	//    la via coperta sarebbe anche la piu' economica — e due rotte di cui una domina non sono un trade-off.
	for (int32 Q = 1; Q <= 2; ++Q)
	{
		SetCell(FRTCellId(Q, 2, 0), /*Move=*/ false, /*Sight=*/ false, ERTHexSurface::Rough);
	}

	// 6. Piattaforma sul layer 1, collegata da UNA SOLA transizione.
	//
	//    Stava nei passi di U1 e in nessun criterio: il `done_when` verificava i passi 3, 4 e 7 e saltava il 5,
	//    quindi l'arena poteva passare tutti i controlli senza avere una piattaforma. `CheckReachability` chiude
	//    quel buco.
	//
	//    **Una sola** transizione, come chiede U1: con una la piattaforma e' un vicolo cieco e non altera le
	//    rotte; con due diventerebbe una scorciatoia, e i tre criteri cambierebbero sotto i piedi. E' anche
	//    cio' che la rende un banco vero per `PIE-HEX-MODE-E/H`, che pretendono celle su >=2 layer.
	//
	//    Sta a nord-est, fuori da entrambe le rotte: una piattaforma sul percorso sposterebbe il pathfinding.
	const FRTCellId PlatformAnchor(3, -3, 0);
	for (const FRTCellId& Id : { FRTCellId(3, -3, 1), FRTCellId(2, -3, 1), FRTCellId(3, -2, 1) })
	{
		Draft.Set(MakeShowcaseTerrainCell(Id, ERTHexSurface::Floor));
	}
	Draft.Link(PlatformAnchor, FRTCellId(3, -3, 1), /*Cost=*/ 2);

	Draft.CommitTo(Arena);
	return Arena;
}
