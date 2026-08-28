#include "Ability/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Ability/RTActionData.h" // MakeGenericActions crea le istanze accodate al kit
#include "Ability/RTEquipmentData.h"
#include "Combat/RTCombatLibrary.h" // DeflectDamageReduction: il numero della riduzione resta uno solo
#include "Map/RTHexCellData.h"     // ERTHexDoorState: `Action.Interact` dichiara lo stato che chiede (D-151)

TArray<FRTReactionProfileDef> URTCatalogLibrary::GetReactionProfileCatalog()
{
	// I tre profili di `spec-reaction-clash-e14.md` §2.5, e **tre e' il numero giusto**: Riktor non ne ha uno
	// perche' `Hold Ground` fa gia' cio' che il suo `ANCHOR` avrebbe fatto. Vedi il commento della
	// dichiarazione — qui non si ripete, si esegue.
	//
	// I token NON portano il prefisso d'eroe: `Profile.Sidestep` e non `Phase.Sidestep`. E' cio' che permette
	// di riassegnare un profilo quando il roster cresce, senza che il rename tocchi un dato di gioco.
	//
	// ⚠️ **`static const`, e non e' micro-ottimizzazione**: questi dati sono costanti di compilazione, e la
	// funzione e' sulla strada calda di tre chiamanti diversi — il loader la interroga **per decisione** (due
	// volte sul ramo d'errore, per comporre il messaggio), l'harness **per finestra**, e il resolver **per
	// unita' in `Brace` per Blast**. Senza lo `static` ogni giro ricostruiva tre `FRTReactionProfileDef` con
	// i loro `TArray` annidati e le `FString`. Misurato da una code review, non supposto.
	//
	// 🔴 **Gli EFFETTI ci sono per uno solo dei tre, e le due assenze sono un registro.** `spec-reaction-clash-e14.md`
	// §2.5 e [D-132] dichiarano aperti «Charge del `Grounding`» e «ampiezza della deviazione»: sono
	// bilanciamento, e inventarli qui sarebbe deciderli di nascosto in un file di catalogo. Finche' restano
	// aperti, `BraceExecutableResponses` non offre quelle due risposte — la cardinalita' DICHIARATA di [D-132]
	// (2/2/3) non si muove di un valore, e il resolver non apre una finestra su una scelta che non sa
	// applicare. Chi chiude una delle due voci aggiunge gli effetti qui, e non serve altro.
	static const TArray<FRTReactionProfileDef> Catalog = {
		{ FName(TEXT("Profile.Grounding")), { FRTReactionResponseDef(TEXT("GROUND"), {}) } },

		// `SIDESTEP` si esprime con `SelfReposition`, e NON e' una primitiva scelta per comodita': `BAS-4`
		// decide che questo profilo «risponde al **Forced Movement**» nella stessa forma di
		// `Hero.Phase.FlowReaction`, cioe' `Reposition 1`. La stessa che `Reaction.EmergencyDash` e
		// `Reaction.HazardEscape` gia' usano — quindi lo spostamento passa dai dieci passi di
		// `ApplyForcedDisplacement` (causa nel TurnLog, hazard attraversati, facing, piano che segue) invece
		// di essere un `SetActorLocation` di questa feature.
		//
		// ⚠️ **`1` non e' un numero nuovo**: e' l'`Amount` che quelle due reazioni portano da D-093. La
		// «ampiezza della deviazione» che §2.5 lascia aperta riguarda `GLANCE`, non questo.
		//
		// 🔴 **La GEOMETRIA e' cambiata il 2026-08-19, e questo commento diceva il contrario.** Sosteneva che
		// ci si allontana lungo la linea di chi spinge, e che «la scelta vera esiste contro le spinte di 2».
		// Una code review ha misurato che e' falso: il ramo `Status.Braced` blocca la spinta a QUALUNQUE
		// distanza, quindi contro la spinta di 2 `Hold Ground` tiene la cella e `SIDESTEP` la cedeva — a
		// danno identico. Era una risposta **strettamente dominata**, cioe' un prompt che non compra niente.
		// Ora lo scarto ESCE dalla linea: `URTReactionLibrary::FindSidestepCell`, spec §2.5-bis.
		//
		// ⚠️ **L'`Amount` non e' piu' una distanza e resta `1` di proposito**: uno scarto e' di una cella per
		// definizione — «esci dalla linea» non ha un multiplo. Il valore serve al resolver per DISTINGUERE una
		// risposta che sposta da una che non sposta, ed e' l'unico modo in cui il catalogo puo' dirlo senza un
		// ramo per token. La primitiva resta quella di D-093, non ne nasce una nuova.
		{ FName(TEXT("Profile.Sidestep")),  { FRTReactionResponseDef(TEXT("SIDESTEP"),
			{ FRTActionEffectSpec(ERTActionEffect::SelfReposition, 1) }) } },

		{ FName(TEXT("Profile.Glance")),    { FRTReactionResponseDef(TEXT("GLANCE LEFT"),  {}),
		                                      FRTReactionResponseDef(TEXT("GLANCE RIGHT"), {}) } }
	};
	return Catalog;
}

FRTReactionProfileDef URTCatalogLibrary::FindReactionProfile(const FName& ProfileId)
{
	if (!ProfileId.IsNone())
	{
		for (const FRTReactionProfileDef& Def : GetReactionProfileCatalog())
		{
			if (Def.ProfileId == ProfileId)
			{
				return Def;
			}
		}
	}

	// Profilo base: id vuoto e nessuna risposta extra. Ci si arriva da due strade — chi non ne dichiara uno
	// (Riktor) e chi ne dichiara uno che il catalogo non conosce — e l'esito e' lo stesso di proposito:
	// cardinalita' 1, nessuna finestra. Inventare risposte per un profilo assente aprirebbe un boundary su
	// scelte che il gioco non ha.
	return FRTReactionProfileDef();
}

TArray<FString> URTCatalogLibrary::BraceAllowedResponses(const FName& ProfileId)
{
	// `Hold Ground` PRIMA e sempre: e' la risposta universale, ed e' anche il fallback che §9 assegna al
	// difensore allo scadere della finestra. La sua posizione non e' estetica — un `AllowedResponses` il cui
	// primo elemento non fosse la scelta sicura renderebbe l'ordine dell'array una regola implicita.
	TArray<FString> Responses = { TEXT("Hold Ground") };
	for (const FRTReactionResponseDef& Extra : FindReactionProfile(ProfileId).ExtraResponses)
	{
		// Il TOKEN, non gli effetti: questa funzione risponde a «cosa dichiara il profilo», ed e' la
		// cardinalita' che [D-132] ha deciso. Filtrare qui per effetti disponibili farebbe dire al catalogo
		// che Wraith ha una risposta sola, cioe' cambierebbe un contenuto deciso per una lacuna di runtime.
		Responses.Add(Extra.Response);
	}
	return Responses;
}

TArray<FString> URTCatalogLibrary::BraceExecutableResponses(const FName& ProfileId)
{
	// `Hold Ground` c'e' sempre e per prima, come sopra: e' la risposta universale ed e' il fallback che §9
	// assegna al difensore allo scadere della finestra. Non ha effetti dichiarati e non e' un'eccezione a
	// questa funzione — il suo esito e' il ramo `Status.Braced` del resolver, che gira da CP 5.2.
	TArray<FString> Responses = { TEXT("Hold Ground") };
	for (const FRTReactionResponseDef& Extra : FindReactionProfile(ProfileId).ExtraResponses)
	{
		if (Extra.Effects.Num() > 0)
		{
			Responses.Add(Extra.Response);
		}
	}
	return Responses;
}

TArray<FString> URTCatalogLibrary::AllReactionProfileResponses()
{
	// `static const` per la stessa ragione di `GetReactionProfileCatalog`: il loader interroga questa funzione
	// **per ogni decisione**, e sul ramo d'errore una seconda volta per comporre il messaggio.
	static const TArray<FString> All = []
	{
		// Si parte da `Hold Ground` perche' e' universale: appartiene a ogni profilo, compreso quello base, e
		// nessuna voce del catalogo la elenca — per costruzione, come dichiara `FRTReactionProfileDef`.
		TArray<FString> Responses = BraceAllowedResponses(NAME_None);

		for (const FRTReactionProfileDef& Profile : GetReactionProfileCatalog())
		{
			for (const FRTReactionResponseDef& Extra : Profile.ExtraResponses)
			{
				// `AddUnique` e non `Add`: due profili possono offrire la stessa risposta — oggi non capita, e
				// il giorno in cui capitasse un elenco con un duplicato farebbe stampare due volte lo stesso
				// token nel messaggio d'errore del loader, che e' il posto in cui si va a leggere quando
				// qualcosa non torna.
				Responses.AddUnique(Extra.Response);
			}
		}
		return Responses;
	}();

	return All;
}

bool URTCatalogLibrary::IsKnownReactionProfileResponse(const FString& Response)
{
	// Confronto esatto e case-sensitive, come `URTReactionOpportunityLibrary::IsResponseAllowed`: le due
	// domande sono diverse — «esiste nel catalogo» contro «e' legale in questa finestra» — ma se una fosse
	// piu' tollerante dell'altra ci sarebbe una risposta accettata dal loader e rifiutata dal resolver, cioe'
	// uno scenario che si carica e poi non decide.
	return AllReactionProfileResponses().ContainsByPredicate([&Response](const FString& Known)
	{
		return Known.Equals(Response, ESearchCase::CaseSensitive);
	});
}

TArray<FRTActionEffectSpec> URTCatalogLibrary::BraceResponseEffects(const FName& ProfileId,
	const FString& Response)
{
	for (const FRTReactionResponseDef& Extra : FindReactionProfile(ProfileId).ExtraResponses)
	{
		// Confronto ESATTO e case-sensitive, come `URTReactionOpportunityLibrary::IsResponseAllowed`: la
		// risposta arriva da `AllowedResponses`, che questo stesso catalogo ha costruito, e una tolleranza qui
		// significherebbe eseguire una risposta con un nome che il profilo non ha mai offerto.
		if (Extra.Response.Equals(Response, ESearchCase::CaseSensitive))
		{
			return Extra.Effects;
		}
	}
	return {};
}

ERTMatchPhase URTCatalogLibrary::MapResolutionPhase(ERTResolutionPhase Phase)
{
	// Funzione TOTALE: un caso per ogni valore dell'enum, nessun `default` che nasconda una fase dimenticata
	// (aggiungerne una senza mapparla diventa un errore di compilazione, non un bug silenzioso a runtime).
	switch (Phase)
	{
	case ERTResolutionPhase::Snapshot:       return ERTMatchPhase::Planning; // congelamento a fine pianificazione
	case ERTResolutionPhase::Preparation:    return ERTMatchPhase::Prep;
	case ERTResolutionPhase::FastMovement:   return ERTMatchPhase::Dash;     // la mobilita' rapida precede il Blast
	case ERTResolutionPhase::NormalMovement: return ERTMatchPhase::Move;     // il percorso normale lo segue
	case ERTResolutionPhase::Control:        return ERTMatchPhase::Blast;    // il controllo non e' una macro-fase
	case ERTResolutionPhase::Attack:         return ERTMatchPhase::Blast;
	case ERTResolutionPhase::Environment:    return ERTMatchPhase::Cleanup;  // dopo il Move
	case ERTResolutionPhase::Cleanup:        return ERTMatchPhase::Cleanup;
	}
	return ERTMatchPhase::Cleanup;
}

bool URTCatalogLibrary::IsFastMovement(const FRTActionDef& Def)
{
	return MapResolutionPhase(Def.ResolutionPhase) == ERTMatchPhase::Dash;
}

int32 URTCatalogLibrary::FirstDamage(const FRTActionDef& Def)
{
	for (const FRTActionEffectSpec& Spec : Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { return Spec.Amount; }
	}
	return 0;
}

int32 URTCatalogLibrary::ResolutionPhaseCode(ERTResolutionPhase Phase)
{
	switch (Phase)
	{
	case ERTResolutionPhase::Snapshot:       return 0;
	case ERTResolutionPhase::Preparation:    return 10;
	case ERTResolutionPhase::FastMovement:   return 20; // stesso codice del movimento normale: il 20 si sdoppia
	case ERTResolutionPhase::NormalMovement: return 20;
	case ERTResolutionPhase::Control:        return 30;
	case ERTResolutionPhase::Attack:         return 40;
	case ERTResolutionPhase::Environment:    return 50;
	case ERTResolutionPhase::Cleanup:        return 60;
	}
	return 60;
}

TArray<FString> URTCatalogLibrary::ValidateActions(const TArray<FRTActionDef>& Actions)
{
	TArray<FString> Errors;
	TSet<FName> Seen;

	for (int32 i = 0; i < Actions.Num(); ++i)
	{
		const FRTActionDef& Action = Actions[i];
		// Nome per i messaggi: l'ID se c'e', altrimenti la posizione (un errore che non dice DOVE e' inutile).
		const FString Where = Action.ActionId.IsNone()
			? FString::Printf(TEXT("azione #%d"), i)
			: Action.ActionId.ToString();

		if (Action.ActionId.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: ActionId mancante (l'ID e' la chiave stabile del TurnLog)"), *Where));
		}
		else if (Seen.Contains(Action.ActionId))
		{
			Errors.Add(FString::Printf(TEXT("%s: ActionId duplicato"), *Where));
		}
		else
		{
			Seen.Add(Action.ActionId);
		}

		if (Action.Priority < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: priorita' negativa (%d)"), *Where, Action.Priority));
		}
		if (Action.RangeCells < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: portata negativa (%d)"), *Where, Action.RangeCells));
		}
		if (Action.CostMP < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: costo negativo (%d)"), *Where, Action.CostMP));
		}
		if (Action.CooldownTurns < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: cooldown negativo (%d)"), *Where, Action.CooldownTurns));
		}

		if (Action.PropagationLimit < 0)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: propagazione senza limite (usa 0 per 'non propaga', N>0 per fermarsi a N celle)"), *Where));
		}

		if (Action.ResolutionPhase == ERTResolutionPhase::Snapshot)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: nessuna azione risolve nello Snapshot (fase di congelamento dello stato)"), *Where));
		}

		// Regola del vertical slice: un movimento bloccato si ferma nell'ultima cella valida. Un fallback
		// diverso (annullare, attaccare) renderebbe imprevedibile il movimento simultaneo.
		const bool bIsMovement = Action.ResolutionPhase == ERTResolutionPhase::FastMovement
			|| Action.ResolutionPhase == ERTResolutionPhase::NormalMovement;
		if (bIsMovement && Action.Fallback != ERTActionFallback::Stop)
		{
			Errors.Add(FString::Printf(TEXT("%s: azione di movimento con fallback diverso da Stop"), *Where));
		}
	}

	return Errors;
}

// Soglie di [D-087], `PROPOSED FOR PLAYTEST` (`WV-2`). Sono qui e non in una costante di configurazione
// perche' il catalogo e' l'owner del NUMERO e il codice dell'APPLICAZIONE: una ritaratura tocca questa
// riga e la riga del catalogo, e nessun test cade.
ERTAttackDamageBand URTCatalogLibrary::DamageBandOf(int32 BaseDamage)
{
	if (BaseDamage >= 19) { return ERTAttackDamageBand::High; }
	if (BaseDamage >= 11) { return ERTAttackDamageBand::Medium; }
	return ERTAttackDamageBand::Low;
}

int32 URTCatalogLibrary::DeclaredDamage(const FRTActionDef& Action)
{
	for (const FRTActionEffectSpec& Spec : Action.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { return Spec.Amount; }
	}
	return 0;
}

URTEquipmentData* URTCatalogLibrary::MakePortableCoverGadget()
{
	URTEquipmentData* Cover = NewObject<URTEquipmentData>();
	Cover->EquipmentId = TEXT("Gadget.PortableCover");
	Cover->DisplayName = FText::FromString(TEXT("Copertura portatile"));
	Cover->Slot = ERTEquipmentSlot::Gadget;
	Cover->Advantage = FText::FromString(
		TEXT("erige una copertura bassa su un bordo, anche per chi non e' Riktor"));

	// Lo svantaggio e' **obbligatorio** (regola di prodotto: senza, l'equipaggiamento e' una scelta verticale),
	// e il catalogo equipaggiamento non ne dichiara uno specifico per questo gadget. Invece di inventare un
	// numero si dichiara quello che i cataloghi gia' dicono: **cooldown 3** per ogni gadget, contro il 2 del
	// pannello d'eroe, e l'unico slot gadget occupato. Chi non e' Riktor puo' erigere pannelli, ma piu' di
	// rado e rinunciando a medkit, isolante o sensore.
	Cover->Drawback = FText::FromString(
		TEXT("ricarica 3 turni invece dei 2 del pannello d'eroe, e occupa l'unico slot gadget"));
	Cover->CooldownTurns = 3; // catalogo equipaggiamento §2: «tutti i gadget hanno cooldown 3»
	Cover->GrantedActionId = TEXT("Action.CreateCover");
	return Cover;
}

namespace
{
	/** Scheletro comune delle sei varianti: cambia solo cio' che ciascuna compra e cio' con cui lo paga. */
	/** Porta un delta unico sulle tre fasce, senza tararle.
	 *
	 *  ⚠️ E' la forma con cui #509 ha migrato il campo: la struttura per fascia esiste, i VALORI no.
	 *  Tre valori uguali sono la dichiarazione che la taratura non e' stata fatta — non una svista — e
	 *  conservano esattamente il comportamento che il gioco aveva col vecchio intero unico.
	 *  I numeri si chiudono con una partita (`WV-2`), non qui. */
	void SetFlatDelta(URTEquipmentData* V, int32 Delta)
	{
		V->DamageDeltaByBand.Add(ERTAttackDamageBand::Low, Delta);
		V->DamageDeltaByBand.Add(ERTAttackDamageBand::Medium, Delta);
		V->DamageDeltaByBand.Add(ERTAttackDamageBand::High, Delta);
	}

	URTEquipmentData* WeaponVariant(const TCHAR* Id, const TCHAR* Nome, const TCHAR* Vantaggio,
		const TCHAR* Svantaggio)
	{
		URTEquipmentData* V = NewObject<URTEquipmentData>();
		V->EquipmentId = Id;
		V->DisplayName = FText::FromString(Nome);
		V->Slot = ERTEquipmentSlot::WeaponVariant;
		V->Advantage = FText::FromString(Vantaggio);
		V->Drawback = FText::FromString(Svantaggio);
		// Una variante non e' un oggetto che si usa: modifica l'attacco base, che ha il cooldown dell'arma.
		// Il `CooldownDeltaTurns` e' un'altra cosa e vive nei delta.
		V->CooldownTurns = 0;
		return V;
	}
}

TArray<URTEquipmentData*> URTCatalogLibrary::MakeWeaponVariants()
{
	TArray<URTEquipmentData*> Variants;

	// I numeri vengono da `docs/balance/RT_EquipmentCatalog_v0.1.md` §1, che e' l'owner. Ripeterli qui e'
	// inevitabile — il C++ non legge il markdown — ma il test `WeaponVariantHasTradeoff` verifica la REGOLA
	// (ogni variante paga qualcosa), non i singoli valori, cosi' una ritaratura del catalogo non fa cadere
	// sei test che non c'entrano.

	// Precisione — +1 portata, −4 danni. La variante del tiratore che vuole restare fuori dalla mischia.
	URTEquipmentData* Precision = WeaponVariant(TEXT("Weapon.Precision"), TEXT("Precisione"),
		TEXT("+1 cella di portata"), TEXT("-4 danni"));
	Precision->RangeDeltaCells = 1;
	SetFlatDelta(Precision, -4);  // stesso valore sulle tre fasce: la taratura per fascia e' `WV-2`
	Variants.Add(Precision);

	// Impatto — spinge di 1, −1 portata. Comprare uno spostamento costa avvicinarsi.
	URTEquipmentData* Impact = WeaponVariant(TEXT("Weapon.Impact"), TEXT("Impatto"),
		TEXT("l'attacco base respinge di 1 cella"), TEXT("-1 cella di portata"));
	Impact->RangeDeltaCells = -1;
	// Zero DICHIARATO su tutte e tre le fasce, non omesso: `Impact` paga in portata e in `Push`, non
	// in danno. Il validator rifiuta una fascia mancante proprio perche' «non dichiarata» e «zero» sono
	// cose diverse — la prima e' un'omissione, la seconda una scelta. Trovato dal validator stesso
	// durante #509: questa riga mancava.
	SetFlatDelta(Impact, 0);
	Impact->AddedEffects.Add(FRTActionEffectSpec(ERTActionEffect::Push, 1));
	Variants.Add(Impact);

	// Sovraccarico — +6 danni, +2 turni di ricarica. E' l'unica che paga in TEMPO invece che in numeri
	// dell'attacco: il colpo e' migliore, ma l'attacco base smette di essere disponibile ogni turno.
	//
	// 🔴 **Il costo e' `2` e non `1`, e la differenza NON e' di taratura**: `TickCooldowns()` gira nel
	// Cleanup dello STESSO turno in cui l'attacco e' stato usato, quindi un `1` si azzera subito e
	// significa «ogni turno» — cioe' nessuna pausa. [D-090] lo ha misurato e ha scartato la traduzione
	// letterale del catalogo («+1 turno di ricarica») proprio per questo, fissando `+2`.
	// ⚠️ Questa riga ha portato `1` dal 2026-08-11 al 2026-08-25 (#1387): il codice e' stato scritto alle
	// 09:57 e la decisione presa alle 15:23 dello stesso giorno, e nessuno le ha riavvicinate. Il test
	// `Equipment.WeaponVariantHasTradeoff` non poteva accorgersene — chiede `CooldownDeltaTurns > 0`, e
	// l'`1` lo soddisfaceva restando gratis. Lo pinna `Equipment.OverchargeCostSurvivesTheCleanup`.
	// ➕ Il `+6` di danno resta un intero unico ed e' un'ALTRA issue: #509 lo porta alle fasce (D-087).
	URTEquipmentData* Overcharge = WeaponVariant(TEXT("Weapon.Overcharge"), TEXT("Sovraccarico"),
		TEXT("+6 danni"), TEXT("+1 turno di ricarica: l'attacco base non e' piu' disponibile ogni turno"));
	SetFlatDelta(Overcharge, 6);  // stesso valore sulle tre fasce: la taratura per fascia e' `WV-2`
	Overcharge->CooldownDeltaTurns = 2;
	Variants.Add(Overcharge);

	// Multiplo — un bersaglio in piu', −6 danni. ⚠️ Meta' dichiarata e non consumata: vedi `ExtraTargets`.
	URTEquipmentData* Split = WeaponVariant(TEXT("Weapon.Split"), TEXT("Multiplo"),
		TEXT("un bersaglio aggiuntivo (dichiarato: il motore v0.1 non ha cardinalita' dei bersagli)"),
		TEXT("-6 danni"));
	SetFlatDelta(Split, -6);  // stesso valore sulle tre fasce: la taratura per fascia e' `WV-2`
	Split->ExtraTargets = 1;
	Variants.Add(Split);

	// Soppressione — applica `Slow`, −5 danni. Stesso mestiere dell'attacco base di Riktor (ADR-0007), che
	// e' la prova che lo `Slow` su un attacco base e' gia' rappresentabile e gia' osservato in partita.
	URTEquipmentData* Suppressive = WeaponVariant(TEXT("Weapon.Suppressive"), TEXT("Soppressione"),
		TEXT("l'attacco base applica Status.Slow per 1 turno"), TEXT("-5 danni"));
	SetFlatDelta(Suppressive, -5);  // stesso valore sulle tre fasce: la taratura per fascia e' `WV-2`
	Suppressive->AddedEffects.Add(FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Slow, /*Turni*/ 1));
	Variants.Add(Suppressive);

	// Ambientale — hazard migliorati, −5 danni diretti. Il vantaggio resta **prosa**: «migliorare un hazard»
	// non e' un delta dell'attacco base ma una modifica di come l'ambiente reagisce, e nessun campo del
	// catalogo lo esprime. Si dichiara cio' che e' vero — lo svantaggio, che e' numerico — invece di
	// inventare un modificatore che nessuno applicherebbe.
	URTEquipmentData* Environmental = WeaponVariant(TEXT("Weapon.Environmental"), TEXT("Ambientale"),
		TEXT("migliora gli hazard prodotti (dichiarato: nessun campo del catalogo lo esprime in v0.1)"),
		TEXT("-5 danni diretti"));
	SetFlatDelta(Environmental, -5);  // stesso valore sulle tre fasce: la taratura per fascia e' `WV-2`
	Variants.Add(Environmental);

	return Variants;
}

TArray<URTEquipmentData*> URTCatalogLibrary::MakeGadgets()
{
	TArray<URTEquipmentData*> Gadgets;

	auto Gadget = [](const TCHAR* Id, const TCHAR* Nome, const TCHAR* CoreAction, const TCHAR* Vantaggio,
		const TCHAR* Svantaggio, const TArray<FRTActionEffectSpec>& Effects)
	{
		URTEquipmentData* G = NewObject<URTEquipmentData>();
		G->EquipmentId = Id;
		G->DisplayName = FText::FromString(Nome);
		G->Slot = ERTEquipmentSlot::Gadget;
		G->Advantage = FText::FromString(Vantaggio);
		G->Drawback = FText::FromString(Svantaggio);
		G->GrantedActionId = CoreAction;
		G->GrantedEffects = Effects;
		G->CooldownTurns = 3; // catalogo equipaggiamento §2: «tutti i gadget hanno cooldown 3»
		return G;
	};

	// `Gadget.Medkit` — cura 18 su `Action.Heal`, che ne cura 20: il gadget e' la versione portatile di una
	// capacita' che il catalogo core ha gia', e paga due turni di ricarica in piu' (3 contro 1).
	Gadgets.Add(Gadget(TEXT("Gadget.Medkit"), TEXT("Medkit"), TEXT("Action.Heal"),
		TEXT("cura 18 punti a un alleato entro 3 celle, senza essere un curatore"),
		TEXT("cura 18 invece dei 20 di Action.Heal, e si ricarica in 3 turni invece che in 1"),
		{ FRTActionEffectSpec(ERTActionEffect::Heal, 18) }));

	// `Gadget.BreachCharge` — 35 a una struttura. Costruito su `Action.HeavyAttack`, che e' l'unica azione
	// core a dichiarare `DamageStructure`, ma ne SOSTITUISCE gli effetti: la carica da sfondamento apre muri,
	// non ferisce persone. Il numero 35 non e' inventato qui — il commento di `HeavyAttack` lo dichiara gia'
	// («un colpo pesante scalfisce un muro meno di una carica da sfondamento dedicata, 35 a struttura, E7»),
	// e questa e' la riga che lo rende vero invece che promesso.
	Gadgets.Add(Gadget(TEXT("Gadget.BreachCharge"), TEXT("Carica da breccia"), TEXT("Action.HeavyAttack"),
		TEXT("35 danni a una struttura: apre coperture e porte che il fuoco normale scalfisce appena"),
		TEXT("non fa alcun danno alle unita', e occupa l'unico slot gadget"),
		{ FRTActionEffectSpec(ERTActionEffect::DamageStructure, 35) }));

	// `Gadget.Sprinkler` — acqua raggio 1, che e' **esattamente** cio' che `Action.CreateWater` gia' fa
	// (`SurfaceRadius = 1`, catalogo azioni §6). Nessun effetto proprio: il suo esito e' una superficie, che
	// `FRTActionEffectSpec` non sa esprimere — dichiararne uno qui significherebbe fraintenderlo.
	Gadgets.Add(Gadget(TEXT("Gadget.Sprinkler"), TEXT("Sprinkler"), TEXT("Action.CreateWater"),
		TEXT("allaga un'area di raggio 1: prepara le combo elettriche e spegne il fuoco"),
		TEXT("bagna anche i propri, e l'acqua resta dopo che il turno e' passato"),
		{}));

	// `Gadget.PortableCover` — gia' costruito in CP 9.5, non riscritto qui.
	Gadgets.Add(MakePortableCoverGadget());

	// ⛔ I QUATTRO ASSENTI, con la ragione ciascuno — sono quattro ragioni diverse, non «mancano».
	//
	// - `Gadget.SmokeEmitter` (fumo raggio 1): **nessuna azione core crea `ERTHexSurface::Smoke`**. L'unica
	//   che lo fa e' `Phase.MistVeil`, che e' d'eroe e risolve in `Preparation` — e la fase e' proprio il
	//   difetto che `#353` ha documentato. Serve prima un'azione core del fumo, come `CreateWater` lo e'
	//   dell'acqua.
	// - `Gadget.Insulator` (immunita' a una propagazione elettrica): e' un PASSIVO, e non concede un'azione.
	//   Il motore non ha un modello di immunita' per categoria — `RT-FEAT-STATUS-FRAMEWORK` (E36) lo
	//   costruira'. Oggi sarebbe un gadget che non fa niente.
	// - `Gadget.Sensor` (alza la Team Knowledge in un'area): la conoscenza parziale e' **E13, assente**. E il
	//   catalogo stesso dichiara raggio e durata «non specificati dalla fonte», quindi mancano anche i numeri.
	// - `Gadget.Anchor` (impedisce **una** spinta): il campo `PushResistance` esiste ma e' una SOGLIA
	//   permanente, non un contatore. Darlo a chi porta l'ancora lo renderebbe immune a OGNI spinta del gioco
	//   — tutte valgono 1 — cioe' esattamente l'immunita' non decisa che `D-075` ha appena tolto a Riktor.
	//   «Una» spinta richiede un consumo per turno che non esiste.

	return Gadgets;
}

TArray<URTEquipmentData*> URTCatalogLibrary::MakeReactionModules()
{
	TArray<URTEquipmentData*> Modules;

	auto Module = [](const TCHAR* Id, const TCHAR* Nome, const TCHAR* CoreAction, const TCHAR* Vantaggio,
		const TCHAR* Svantaggio, const TArray<FRTActionEffectSpec>& Effects)
	{
		URTEquipmentData* M = NewObject<URTEquipmentData>();
		M->EquipmentId = Id;
		M->DisplayName = FText::FromString(Nome);
		M->Slot = ERTEquipmentSlot::ReactionModule;
		M->Advantage = FText::FromString(Vantaggio);
		M->Drawback = FText::FromString(Svantaggio);
		M->GrantedActionId = CoreAction; // da qui vengono fase, priorita' e soprattutto il TRIGGER
		M->GrantedEffects = Effects;
		// Una reazione non ha ricarica propria nel catalogo §3: il limite e' «una attivazione per turno», che
		// il resolver garantisce sul percorso E5 — un cooldown qui sarebbe un secondo limite non dichiarato.
		M->CooldownTurns = 0;
		return M;
	};

	// `Reaction.CounterShot` — 14 danni su chi ti ha colpito. Costruito su `Action.Counter`, che e' gia' una
	// reazione con `HitByDirectAttack`: il modulo non introduce ne' un trigger ne' un pass suo. I 14 sono del
	// catalogo equipaggiamento §3 e stanno **sotto** i 16 dell'azione core — e' il prezzo di averla come
	// equipaggiamento invece che come abilita' d'eroe.
	Modules.Add(Module(TEXT("Reaction.CounterShot"), TEXT("Contrattacco"), TEXT("Action.Counter"),
		TEXT("14 danni a chi ti colpisce, senza spendere l'azione del turno"),
		TEXT("14 danni invece dei 16 di Action.Counter, e occupa l'unico slot reazione"),
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 14) }));

	// `Reaction.ReactiveShield` — scudo 15 quando subisci danno. Stesso trigger del contrattacco, effetto
	// opposto: e' la prova che il regime non dipende dall'azione ma dai DATI (ADR-0004 §2), e che due moduli
	// possono condividere il trigger senza condividere il mestiere.
	//
	// ⚠️ Non e' costruito su `Action.Shield`, che pure esiste: quella e' in **Preparation** e non e' una
	// reazione (nessun `ReactionTrigger`, scudo 25). Ci si scherma in preparazione, non in risposta — e
	// costruirci sopra un modulo darebbe un'abilita' che il pass delle reazioni non guarda mai.
	Modules.Add(Module(TEXT("Reaction.ReactiveShield"), TEXT("Scudo reattivo"), TEXT("Action.Counter"),
		TEXT("15 punti scudo quando subisci un colpo diretto"),
		TEXT("scudo 15 invece dei 25 di Action.Shield, che pero' va preparata in anticipo"),
		{ FRTActionEffectSpec(ERTActionEffect::Shield, 15) }));

	// `Reaction.AllyIntercept` — ti interponi al posto di un alleato bersagliato. E' l'unico dei tre che usa
	// `AllyHitByDirectAttack`, e riusa `Action.Intercept` **senza effetti propri**: il suo esito non e' un
	// effetto ma un cambio di bersaglio, che vive nella semantica dell'azione core (CP 5.3). Dichiarare qui
	// un `Damage` o uno `Shield` significherebbe fraintendere cosa fa.
	Modules.Add(Module(TEXT("Reaction.AllyIntercept"), TEXT("Interposizione"), TEXT("Action.Intercept"),
		TEXT("prendi al posto di un alleato entro 2 celle il colpo che era per lui"),
		TEXT("il danno lo subisci tu, e occupa l'unico slot reazione"),
		{}));

	// `Reaction.EmergencyDash` — ti sposti di 1 quando sei bersagliato. Costruito su `Action.Counter` come
	// gli altri due: da li' vengono fase, priorita' e `HitByDirectAttack`.
	//
	// Il suo effetto e' `SelfReposition`, nato con D-093 perche' nessun `ERTActionEffect` sapeva spostare la
	// SORGENTE — `Push` e `Pull` muovono il bersaglio. Chi fugge resta girato verso la minaccia (D-104), e lo
	// spostamento passa dagli stessi dieci passi della spinta (#541): traccia con causa, hazard attraversati,
	// facing, piano che segue.
	Modules.Add(Module(TEXT("Reaction.EmergencyDash"), TEXT("Dash d'emergenza"), TEXT("Action.Counter"),
		TEXT("ti sposti di una cella quando sei bersagliato, restando fronte a chi ti ha preso di mira"),
		TEXT("nessun danno e nessuna protezione: sposta soltanto, e se non c'e' dove andare si spreca"),
		{ FRTActionEffectSpec(ERTActionEffect::SelfReposition, 1) }));

	// `Reaction.Anchor` — annulla lo spostamento che stai per subire. E' l'unico dei cinque costruito su
	// `Action.Anchor`, perche' e' l'unica azione core con `AboutToBeDisplaced`: sopra `Action.Counter`
	// erediterebbe il trigger dei colpi e reagirebbe alla cosa sbagliata.
	//
	// Da non confondere con `Gadget.Anchor` del catalogo §2, che porta lo stesso nome e fa un'altra cosa (una
	// resistenza permanente, non una reazione che si consuma) e resta non costruito.
	Modules.Add(Module(TEXT("Reaction.Anchor"), TEXT("Ancoraggio"), TEXT("Action.Anchor"),
		TEXT("la prima spinta o trazione del turno non ti sposta, a qualunque distanza"),
		TEXT("si consuma anche quando una guardia sarebbe bastata, e occupa l'unico slot reazione"),
		{ FRTActionEffectSpec(ERTActionEffect::CancelDisplacement, 1) }));

	// `Reaction.Cleanse` — annulla il controllo che stai per ricevere. Costruito su `Action.Purge`, l'unica
	// azione core con `AboutToReceiveControl`: il nome del MODULO resta quello del catalogo §3, il nome
	// dell'AZIONE no, perche' `Action.Cleanse` e' gia' un'altra cosa (vedi il commento sul catalogo azioni).
	//
	// Con due controlli nello stesso Blast ne annulla **il piu' grave**, e non si spende affatto se il
	// controllo in arrivo non cambierebbe nulla — chi e' gia' radicato non brucia la reazione per un rinnovo.
	// `Reaction.HazardEscape` — l'ULTIMO dei sette, e il piu' lungo da arrivare. Non gli mancava un dato ne'
	// un momento: gli mancava l'EVENTO. Finche' una superficie che nasceva sotto un'unita' ferma non le faceva
	// niente (`#570`), nel Cleanup non c'era nessun danno imminente da cui fuggire e il modulo sarebbe stato
	// inerte — la trappola di `Phase.MistVeil` (`#353`).
	//
	// Si fugge verso la cella che si ha DAVANTI, e il facing lo dichiara il giocatore: la fuga e' prevedibile
	// guardando il campo invece che arbitraria. Se davanti non si puo', si ripiega sull'ordine canonico delle
	// direzioni; se non c'e' nessuna cella sicura, la reazione si spende senza salvare — come `EmergencyDash`.
	Modules.Add(Module(TEXT("Reaction.HazardEscape"), TEXT("Fuga hazard"), TEXT("Action.Evade"),
		TEXT("quando la cella sotto di te diventa pericolosa ti sposti di una, verso dove stai guardando"),
		TEXT("una sola volta per turno, e se sei circondato dal fuoco si spreca"),
		{ FRTActionEffectSpec(ERTActionEffect::SelfReposition, 1) }));

	Modules.Add(Module(TEXT("Reaction.Cleanse"), TEXT("Pulizia automatica"), TEXT("Action.Purge"),
		TEXT("il controllo che stai per ricevere non ti tocca: fra due, salta il piu' grave"),
		TEXT("uno solo per turno, e non ferma il prolungamento di un controllo che hai gia' addosso"),
		{ FRTActionEffectSpec(ERTActionEffect::CancelStatus, 1) }));

	return Modules;
}

TArray<FString> URTCatalogLibrary::WarnOnVariantForAttack(const FRTActionDef& BasicAttack,
	const URTEquipmentData* Variant)
{
	TArray<FString> Warnings;
	if (Variant == nullptr || Variant->Slot != ERTEquipmentSlot::WeaponVariant)
	{
		return Warnings;
	}
	const FString Chi = FString::Printf(TEXT("%s su %s"),
		*Variant->EquipmentId.ToString(), *BasicAttack.ActionId.ToString());

	// 1. STATUS DUPLICATO. E' la regola generale dietro un caso concreto: `Weapon.Suppressive` applica
	// `Slow`, e `Riktor.ImpactShot` lo applica gia' — quindi la variante fa pagare 5 danni su 8 per un
	// effetto che l'eroe possiede. Non e' subottimale, e' priva di senso, e D-086 la vieta. La regola non
	// nomina Riktor: vale per ogni eroe futuro il cui attacco base porti gia' uno status.
	for (const FRTActionEffectSpec& Aggiunto : Variant->AddedEffects)
	{
		if (Aggiunto.Effect != ERTActionEffect::Status || !Aggiunto.StatusTag.IsValid()) { continue; }
		for (const FRTActionEffectSpec& Esistente : BasicAttack.Effects)
		{
			if (Esistente.Effect == ERTActionEffect::Status && Esistente.StatusTag == Aggiunto.StatusTag)
			{
				Warnings.Add(FString::Printf(
					TEXT("%s: la variante applica '%s' che l'attacco base applica gia' — costo pagato per nulla"),
					*Chi, *Aggiunto.StatusTag.ToString()));
			}
		}
	}

	// 2. DANNO AZZERATO O NEGATIVO. `ApplyWeaponVariant` non clampa deliberatamente, perche' un attacco
	// spinto sotto zero e' un difetto di bilanciamento che deve restare **visibile**: questo e' il posto
	// dove si vede.
	const FRTActionDef Modificata = ApplyWeaponVariant(BasicAttack, Variant);
	int32 Prima = 0, Dopo = 0;
	for (const FRTActionEffectSpec& S : BasicAttack.Effects)
	{
		if (S.Effect == ERTActionEffect::Damage) { Prima = S.Amount; break; }
	}
	for (const FRTActionEffectSpec& S : Modificata.Effects)
	{
		if (S.Effect == ERTActionEffect::Damage) { Dopo = S.Amount; break; }
	}
	if (Prima > 0 && Dopo <= 0)
	{
		Warnings.Add(FString::Printf(TEXT("%s: il danno diretto scende da %d a %d — pulsante finto"),
			*Chi, Prima, Dopo));
	}
	return Warnings;
}

TArray<FString> URTCatalogLibrary::ValidateLoadout(const TArray<const URTEquipmentData*>& Loadout)
{
	// I difetti dei singoli pezzi valgono anche qui: tre oggetti rotti hanno la forma giusta e il contenuto
	// sbagliato, e un controllo che contasse soltanto li accetterebbe.
	TArray<FString> Errors = ValidateEquipment(Loadout);

	int32 Varianti = 0, Gadgets = 0, Moduli = 0;
	for (const URTEquipmentData* Item : Loadout)
	{
		if (Item == nullptr) { continue; } // gia' segnalato da ValidateEquipment
		switch (Item->Slot)
		{
		case ERTEquipmentSlot::WeaponVariant:  ++Varianti; break;
		case ERTEquipmentSlot::Gadget:         ++Gadgets;  break;
		case ERTEquipmentSlot::ReactionModule: ++Moduli;   break;
		}
	}

	// «Esattamente uno» in entrambe le direzioni, con messaggi distinti: zero e due portano a correzioni
	// diverse — un pezzo dimenticato contro un pezzo di troppo — e un solo messaggio per entrambi
	// costringerebbe a contare a mano per capire quale dei due sia.
	auto Conta = [&Errors](int32 Quanti, const TCHAR* Che)
	{
		if (Quanti == 0)
		{
			Errors.Add(FString::Printf(TEXT("loadout: manca %s"), Che));
		}
		else if (Quanti > 1)
		{
			Errors.Add(FString::Printf(TEXT("loadout: %d %s, ne serve esattamente 1"), Quanti, Che));
		}
	};
	Conta(Varianti, TEXT("la variante d'arma"));
	Conta(Gadgets,  TEXT("il gadget"));
	Conta(Moduli,   TEXT("il modulo di reazione"));
	return Errors;
}

void URTCatalogLibrary::EquipWeaponVariant(URTActionData* BasicAttack, const URTEquipmentData* Variant)
{
	if (BasicAttack == nullptr)
	{
		return;
	}
	BasicAttack->Def = ApplyWeaponVariant(BasicAttack->Def, Variant);

	// Gli specchi legacy, che sono il motivo per cui questa funzione esiste. `MakeEquipmentAction` fa gia' la
	// stessa cosa per i gadget, e con la stessa ragione scritta accanto: `URTActionData` li ha a 5 e 30 di
	// default, e chi non li riallinea manda in partita un'azione con portata e potenza inventate.
	BasicAttack->RangeCells = BasicAttack->Def.RangeCells;
	BasicAttack->CooldownTurns = BasicAttack->Def.CooldownTurns;

	// `Power` si ricalcola dagli effetti e non si somma al vecchio valore: `ResolveCombat` ci ricade quando
	// l'azione non dichiara un `Damage`, e un `+=` qui accumulerebbe a ogni riequipaggiamento.
	BasicAttack->Power = 0;
	for (const FRTActionEffectSpec& Spec : BasicAttack->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { BasicAttack->Power = Spec.Amount; break; }
	}
}

FName URTCatalogLibrary::DefaultWeaponVariantFor(const FName& HeroId)
{
	// D-089 — il default RINFORZA l'identita' dell'eroe; compensarne la debolezza resta la scelta
	// alternativa del giocatore. E' una tabella e non una catena di `if`: il giorno in cui il roster cresce
	// si aggiunge una riga, e un eroe senza riga ottiene `None` invece di un default silenzioso e sbagliato.
	//
	// ⚠️ Nessuno usa `Weapon.Overcharge`, ed e' deliberato: il suo costo e' `WV-1`, ancora aperto (#510). Un
	// default il cui prezzo si decide dopo cambierebbe insieme a quella risposta.
	static const TMap<FName, FName> Defaults = {
		// Gadget vede a 7 e sparava a 4: `Precision` e' l'unica che riduce quel divario (18 a portata 5).
		{ FName(TEXT("Hero.Gadget")),    FName(TEXT("Weapon.Precision")) },
		// Phase e' il setter del roster, e `Impact` porta la sua spinta da 1 a 2 (D-085).
		{ FName(TEXT("Hero.Phase")),    FName(TEXT("Weapon.Impact")) },
		// Wraith e' il piu' mobile (Move 6): `Suppressive` gli da' come impedirlo agli altri.
		{ FName(TEXT("Hero.Wraith")),  FName(TEXT("Weapon.Suppressive")) },
		// Riktor tiene `Impact` perche' e' l'unica che NON gli toglie danno — paga in portata — e l'attacco
		// base diventa displacement, coerente con Utility/Emergency (ADR-0007).
		{ FName(TEXT("Hero.Riktor")), FName(TEXT("Weapon.Impact")) },
	};
	const FName* Found = Defaults.Find(HeroId);
	return Found ? *Found : FName();
}

FName URTCatalogLibrary::DefaultGadgetFor(const FName& HeroId)
{
	// Fonte: **§4 di `docs/balance/RT_EquipmentCatalog_v0.1.md`**, colonna «Gadget». Le righe qui sotto sono
	// una COPIA di quella tabella, e la parola conta: il markdown decide, il C++ esegue.
	//
	// 🔴 **Nulla impedisce piu' che divergano in silenzio.** Le confrontava per ID
	// `scripts/check-equipment-defaults.py`, rimosso con **D-182** il 2026-08-21: la copia c'e' ancora,
	// il guardiano no. Chi cambia un valore qui apra §4 del catalogo, e viceversa.
	//
	// ⚠️ Il catalogo motiva per esteso la sola variante d'arma (D-089). Per gadget e moduli dichiara la
	// scelta senza argomentarla, e qui NON si inventa una motivazione che la fonte non da': si scrive cosa
	// fa il pezzo, che e' verificabile, e si lascia la ragione a chi ha compilato la tabella.
	static const TMap<FName, FName> Defaults = {
		// Isolante: immunita' a **una** propagazione elettrica. Gadget e' l'eroe elettrico del roster.
		{ FName(TEXT("Hero.Gadget")), FName(TEXT("Gadget.Insulator")) },
		// Sprinkler: acqua raggio 1. Dal 2026-08-16 e' anche l'unico produttore d'acqua che il roster puo'
		// portare in campo — `Hero.Phase.FluidTrail` l'ha persa con D-046 superata (#1006).
		{ FName(TEXT("Hero.Phase")), FName(TEXT("Gadget.Sprinkler")) },
		// Copertura portatile: crea una copertura bassa su un bordo. Riktor e' l'eroe delle strutture.
		{ FName(TEXT("Hero.Riktor")), FName(TEXT("Gadget.PortableCover")) },
		// Sensore: alza la Team Knowledge in un'area.
		{ FName(TEXT("Hero.Wraith")), FName(TEXT("Gadget.Sensor")) },
	};
	const FName* Found = Defaults.Find(HeroId);
	return Found ? *Found : FName();
}

FName URTCatalogLibrary::DefaultReactionModuleFor(const FName& HeroId)
{
	// Fonte: §4 del catalogo equipaggiamento, colonna «Reazione». Vale la stessa nota di `DefaultGadgetFor`.
	static const TMap<FName, FName> Defaults = {
		// Scudo reattivo: scudo 15 quando subisci danno.
		{ FName(TEXT("Hero.Gadget")), FName(TEXT("Reaction.ReactiveShield")) },
		// Fuga hazard: `Reposition 1` quando la cella diventa pericolosa.
		{ FName(TEXT("Hero.Phase")), FName(TEXT("Reaction.HazardEscape")) },
		// 🔴 **Purificazione, non interposizione** (`#1403`, [D-218]). §4 prescriveva
		// `Reaction.AllyIntercept`, costruito su `Action.Intercept` — e la reazione di KIT di Riktor,
		// `Hero.Riktor.Interposition`, e' costruita sullo **stesso** `Action.Intercept`. Lo slot di loadout
		// spendeva su cio' che l'eroe ha gia': due voci per una capacita' sola, e nessuna scelta reale al
		// giocatore. `Reaction.Cleanse` non gli toglie niente e gli da' l'unica risposta esistente allo
		// `Status.Slow` che `Hero.Riktor.ImpactShot` — il suo **attacco base** — applica a ogni colpo
		// (`#1479`). ⚠️ Questa riga si scosta da §4 del catalogo equipaggiamento, e lo dichiara: la fonte
		// prescriveva un duplicato, e per [D-210] il codice recepito prevale su un catalogo di `balance/`.
		{ FName(TEXT("Hero.Riktor")), FName(TEXT("Reaction.Cleanse")) },
		// Dash d'emergenza: `Reposition 1` quando sei bersagliato.
		{ FName(TEXT("Hero.Wraith")), FName(TEXT("Reaction.EmergencyDash")) },
	};
	const FName* Found = Defaults.Find(HeroId);
	return Found ? *Found : FName();
}

TArray<FName> URTCatalogLibrary::DefaultLoadoutFor(const FName& HeroId)
{
	const TArray<FName> Prescritti = {
		DefaultWeaponVariantFor(HeroId), DefaultGadgetFor(HeroId), DefaultReactionModuleFor(HeroId)
	};

	// TUTTO O NIENTE, e su DUE condizioni diverse.
	//
	// 1. Un pezzo non DICHIARATO (`None`): l'eroe non ha una riga in §4.
	// 2. Un pezzo dichiarato ma **non spedito**: §4 lo prescrive e il catalogo v0.1 non lo costruisce.
	//    Non e' un'ipotesi — succede a due eroi su quattro. §4 assegna `Gadget.Insulator` a Gadget e
	//    `Gadget.Sensor` a Wraith, e `MakeGadgets` li dichiara assenti con la loro ragione: il primo e' un
	//    PASSIVO e il motore non ha immunita' per categoria (`RT-FEAT-STATUS-FRAMEWORK`, E36); il secondo
	//    dipende dalla conoscenza parziale, che e' E13 e non esiste — e il catalogo stesso ne dichiara
	//    raggio e durata «non specificati dalla fonte».
	//
	// ⚠️ **La condizione 2 si misura, non si elenca.** La tentazione era scrivere «Gadget e Wraith non hanno
	// default»: sarebbe vero oggi e falso il giorno in cui E36 atterra, e nessuno tornerebbe a correggerlo.
	// Chiedendo invece al catalogo se il pezzo esiste, il default di Gadget comincia a funzionare **da se'**
	// quando `Gadget.Insulator` viene spedito, senza che questa funzione cambi di una riga.
	//
	// E vuoto invece che parziale: un loadout a due pezzi verrebbe rifiutato da `ValidateLoadout`, e il
	// chiamante si troverebbe a gestire tre livelli piu' in la' un errore che nasce qui.
	for (const FName& Id : Prescritti)
	{
		if (Id.IsNone() || FindEquipment(Id) == nullptr)
		{
			return {};
		}
	}
	return Prescritti;
}

FRTActionDef URTCatalogLibrary::ApplyWeaponVariant(const FRTActionDef& BasicAttack,
	const URTEquipmentData* Variant)
{
	FRTActionDef Modified = BasicAttack;
	if (Variant == nullptr || Variant->Slot != ERTEquipmentSlot::WeaponVariant)
	{
		return Modified; // un gadget non modifica l'attacco base, e non e' un errore chiederlo
	}

	// Portata e cooldown non scendono sotto zero: sarebbero un catalogo che `ValidateActions` rifiuta, e la
	// variante finirebbe per produrre un'azione illegale invece di un'arma piu' corta.
	Modified.RangeCells = FMath::Max(0, Modified.RangeCells + Variant->RangeDeltaCells);
	Modified.CooldownTurns = FMath::Max(0, Modified.CooldownTurns + Variant->CooldownDeltaTurns);

	// Il DANNO invece non si clampa. Un attacco base spinto sotto zero da una variante e' un difetto di
	// bilanciamento, e un `Max(0, ...)` qui lo trasformerebbe in «zero danni» — un pulsante finto, cioe' la
	// cosa che ADR-0007 esiste per evitare. Resta visibile, e il validator lo dice sul catalogo.
	// La fascia viene dal danno della DEFINIZIONE, letto PRIMA di toccare qualunque cosa: e' l'invariante
	// di [D-087], e leggerlo dopo renderebbe il costo circolare.
	const ERTAttackDamageBand Band = DamageBandOf(DeclaredDamage(BasicAttack));
	const int32* Found = Variant->DamageDeltaByBand.Find(Band);
	const int32 DamageDelta = Found ? *Found : 0;

	if (DamageDelta != 0)
	{
		bool bFound = false;
		for (FRTActionEffectSpec& Spec : Modified.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage)
			{
				Spec.Amount += DamageDelta;
				bFound = true;
				break; // il primo Damage e' il danno diretto: gli altri effetti restano quelli che sono
			}
		}
		// Un attacco base senza effetto Damage dichiarato non esiste nel roster v0.1, ma se esistesse una
		// variante non deve INVENTARGLIENE uno: aggiungere qui `Damage = -4` darebbe un'azione che cura.
		(void)bFound;
	}

	Modified.Effects.Append(Variant->AddedEffects);
	return Modified;
}

URTActionData* URTCatalogLibrary::MakeEquipmentAction(const URTEquipmentData* Item, UObject* Outer)
{
	if (Item == nullptr || Item->GrantedActionId.IsNone())
	{
		return nullptr;
	}

	const FRTActionDef Core = FindCoreAction(Item->GrantedActionId);
	if (Core.ActionId.IsNone())
	{
		return nullptr; // il gadget dichiara un'azione che il catalogo non ha: meglio nulla di un'azione muta
	}

	URTActionData* Action = NewObject<URTActionData>(Outer ? Outer : GetTransientPackage());
	Action->Def = Core;
	Action->Def.ActionId = Item->EquipmentId;   // nel TurnLog si legge il gadget, non l'azione generica
	// ...e proprio per questo la provenienza va conservata: sovrascrivere `ActionId` con l'ID del pezzo
	// cancellava l'unica traccia di QUALE azione core questo pezzo concede. Senza, il gate della
	// raggiungibilita' deve ricopiare a mano «Sprinkler concede CreateWater» in una stringa di prosa —
	// che e' esattamente il difetto che il campo esiste per chiudere.
	Action->Def.DerivedFromActionId = Item->GrantedActionId;
	Action->Def.CooldownTurns = Item->CooldownTurns;

	// Gli effetti PROPRI sostituiscono quelli del core (CP 7.3): un modulo di reazione eredita dal core cio'
	// che lo rende una reazione — fase, priorita', `ReactionTrigger` — ma i numeri sono suoi. `Slot` e
	// `ReactionTrigger` restano quelli copiati sopra, e sono il motivo per cui il modulo va costruito da
	// un'azione che e' GIA' una reazione: sopra un'azione principale sarebbe silenziosamente inerte.
	if (Item->GrantedEffects.Num() > 0)
	{
		Action->Def.Effects = Item->GrantedEffects;
	}
	Action->RangeCells = Action->Def.RangeCells;
	Action->CooldownTurns = Action->Def.CooldownTurns;
	// Stessi campi specchio di `MakeGenericActions`, e per la stessa ragione: `URTActionData` li ha a 5 e 30
	// (default legacy dell'MVP quadrato), quindi non copiarli fa entrare nel kit un gadget con portata e
	// potenza inventate. Oggi l'unico equipaggiamento concede `Action.CreateCover` e non si nota; il giorno
	// che ne concedesse una self-target — `Guard`, `Brace` — il gadget la porterebbe come azione d'attacco
	// da 30 danni, che e' esattamente il difetto appena chiuso per gli eroi.
	Action->bSelfTarget = Action->Def.bSelfTarget;
	Action->Power = 0;
	for (const FRTActionEffectSpec& Spec : Action->Def.Effects)
	{
		if (Spec.Effect == ERTActionEffect::Damage) { Action->Power = Spec.Amount; break; }
	}
	return Action;
}

TArray<FString> URTCatalogLibrary::ValidateEquipment(const TArray<const URTEquipmentData*>& Equipment)
{
	TArray<FString> Errors;
	TSet<FName> Seen;

	for (int32 i = 0; i < Equipment.Num(); ++i)
	{
		const URTEquipmentData* Item = Equipment[i];
		if (Item == nullptr)
		{
			Errors.Add(FString::Printf(TEXT("equipaggiamento #%d: riferimento nullo"), i));
			continue;
		}

		const FString Where = Item->EquipmentId.IsNone()
			? FString::Printf(TEXT("equipaggiamento #%d"), i)
			: Item->EquipmentId.ToString();

		if (Item->EquipmentId.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: EquipmentId mancante"), *Where));
		}
		else if (Seen.Contains(Item->EquipmentId))
		{
			Errors.Add(FString::Printf(TEXT("%s: EquipmentId duplicato"), *Where));
		}
		else
		{
			Seen.Add(Item->EquipmentId);
		}

		// Regola di prodotto: la scelta e' orizzontale. Un equipaggiamento senza svantaggio dichiarato e'
		// potere che si accumula, cioe' esattamente cio' che il canone esclude.
		if (Item->Drawback.IsEmpty())
		{
			Errors.Add(FString::Printf(TEXT("%s: nessuno svantaggio dichiarato"), *Where));
		}
		if (Item->CooldownTurns < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: cooldown negativo (%d)"), *Where, Item->CooldownTurns));
		}

		// Per una VARIANTE D'ARMA lo svantaggio dichiarato a parole non basta, e la ragione e' che `Drawback`
		// e' una `FText`: nessuna regola la legge, quindi una variante potrebbe raccontare un costo che i suoi
		// numeri non pagano — «-4 danni» scritto accanto a tre delta tutti migliorativi. Sarebbe potere
		// verticale con una didascalia rassicurante, cioe' precisamente cio' che la regola di prodotto vieta.
		//
		// Uno svantaggio MISURABILE e' uno solo di questi tre: meno danno, meno portata, piu' ricarica.
		if (Item->Slot == ERTEquipmentSlot::WeaponVariant)
		{
			// 1. NESSUN BUCO: ogni fascia raggiungibile dev'essere dichiarata. Una fascia mancante non vale
			//    zero — varrebbe «questa variante non fa niente su quegli attacchi», che e' una scelta morta
			//    ([D-086]) travestita da omissione. Con `Find` che ritorna `nullptr` sarebbe silenziosa.
			static const ERTAttackDamageBand AllBands[] = {
				ERTAttackDamageBand::Low, ERTAttackDamageBand::Medium, ERTAttackDamageBand::High };
			static const TCHAR* BandNames[] = { TEXT("Low"), TEXT("Medium"), TEXT("High") };

			for (int32 B = 0; B < 3; ++B)
			{
				if (!Item->DamageDeltaByBand.Contains(AllBands[B]))
				{
					Errors.Add(FString::Printf(
						TEXT("%s: variante d'arma senza delta per la fascia %s — una fascia non dichiarata non e' zero"),
						*Where, BandNames[B]));
				}
			}

			// 2. LO SVANTAGGIO E' PER FASCIA. `Drawback` e' una `FText` e nessuna regola la legge, quindi una
			//    variante potrebbe raccontare un costo che i suoi numeri non pagano. Con i delta per fascia il
			//    rischio si triplica: una variante puo' pagare su `Low` ed essere gratis su `High`, cioe'
			//    proprio dove il potere pesa di piu'.
			//    ⚠️ Il costo NON dev'essere per forza il danno: `Weapon.Overcharge` paga in tempo
			//    (`CooldownDeltaTurns`, [D-090]) e sul danno migliora su ogni fascia. La regola resta «paga
			//    qualcosa», valutata fascia per fascia.
			for (int32 B = 0; B < 3; ++B)
			{
				const int32* Delta = Item->DamageDeltaByBand.Find(AllBands[B]);
				if (Delta == nullptr) { continue; }  // gia' segnalato sopra: non si raddoppia il messaggio

				const bool bPaysSomething =
					*Delta < 0 || Item->RangeDeltaCells < 0 || Item->CooldownDeltaTurns > 0;
				if (!bPaysSomething)
				{
					Errors.Add(FString::Printf(
						TEXT("%s: variante d'arma senza svantaggio misurabile sulla fascia %s — danno %+d, portata %+d, ricarica %+d"),
						*Where, BandNames[B], *Delta, Item->RangeDeltaCells, Item->CooldownDeltaTurns));
				}
			}
		}
	}

	return Errors;
}

namespace
{
	FRTActionDef ShippedAction(const FName& Id, ERTResolutionPhase Phase, int32 Priority, int32 Range,
		int32 Cooldown, ERTActionFallback Fallback, const TArray<FRTActionEffectSpec>& Effects,
		bool bInterruptible = true, ERTActionSlot Slot = ERTActionSlot::Main,
		ERTMovementStyle Movement = ERTMovementStyle::None)
	{
		FRTActionDef Def;
		Def.Effects = Effects;
		Def.ActionId = Id;
		Def.ResolutionPhase = Phase;
		Def.Priority = Priority;
		Def.RangeCells = Range;
		Def.CooldownTurns = Cooldown;
		Def.Fallback = Fallback;
		Def.bCanBeInterrupted = bInterruptible;
		Def.Slot = Slot;
		Def.MovementStyle = Movement;
		// Chi corre a perdifiato non para: lo `Sprint` e' l'unica azione della v0.1 che nega la reazione.
		Def.bAllowsReaction = (Id != FName(TEXT("Action.Sprint")));
		return Def;
	}
}


TArray<FRTActionDef> URTCatalogLibrary::GetCoreActionCatalog()
{
	TArray<FRTActionDef> Catalog;

	// `Action.Sprint` (catalogo v0.1 §2) — 8 MP, occupa il SOLO slot movimento [D-028], applica `Status.Exposed`
	// fino al Cleanup. Per le azioni di mobilita' rapida `RangeCells` e' il BUDGET in punti movimento, non un
	// numero di celle: su terreno difficile si arriva meno lontano (e' lo stesso budget del movimento normale,
	// con un'altra quantita').
	//
	// Lo svantaggio dello scatto lungo e' `Exposed`, dichiarato come EFFETTO: chi corre allo scoperto incassa
	// +5 dal primo colpo. Niente di tutto cio' e' scritto nell'orchestratore.
	Catalog.Add(ShippedAction(TEXT("Action.Sprint"), ERTResolutionPhase::FastMovement, /*Priority*/ 60,
		/*Range (MP)*/ 8, /*Cooldown*/ 0, ERTActionFallback::Stop,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Exposed, /*Turni*/ 1) },
		/*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::Budget));

	// `Action.Wait` (catalogo v0.1 §1) — non fa nulla e risolve per ultima (priorita' 100). Serve gia' ora
	// perche' e' cio' in cui `Fallback.Wait` trasforma un'azione: senza, il fallback dovrebbe inventarsi in
	// codice un'azione vuota, cioe' una seconda definizione della stessa cosa.
	//
	// Il catalogo le da' fallback «—»: l'enum non ha un valore "nessuno", e per un'azione che non muove e non
	// colpisce `Stop` e `Cancel` sono lo stesso comportamento osservabile (niente). Si usa `Stop` perche' e'
	// quello che il validator richiede alle azioni di fase Move — un'eccezione in meno, non una regola nuova.
	Catalog.Add(ShippedAction(TEXT("Action.Wait"), ERTResolutionPhase::NormalMovement, /*Priority*/ 100,
		/*Range*/ 0, /*Cooldown*/ 0, ERTActionFallback::Stop, {},
		/*bInterruptible*/ false, ERTActionSlot::None));

	// `Action.Move` — il percorso normale, dopo il Blast (ADR-0003 §3). Nessun effetto dichiarato: a muovere
	// l'unita' e' il resolver dei percorsi, che avanza a micro-step sullo snapshot. Un effetto "MoveTo" qui
	// duplicherebbe quella decisione in un secondo posto.
	Catalog.Add(ShippedAction(TEXT("Action.Move"), ERTResolutionPhase::NormalMovement, /*Priority*/ 50,
		/*Range (MP)*/ 5, /*Cooldown*/ 0, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::Budget));

	// `Action.BasicAttack` — identita', fase, priorita' e fallback stanno qui; DANNO e PORTATA no, perche'
	// dipendono dall'eroe e dalla sua arma (catalogo §1, tabella delle fasce). Li applica MakeBasicAttack:
	// mettere qui un numero significherebbe sceglierne uno arbitrario per tutti.
	Catalog.Add(ShippedAction(TEXT("Action.BasicAttack"), ERTResolutionPhase::Attack, /*Priority*/ 50,
		/*Range*/ 0, /*Cooldown*/ 0, ERTActionFallback::Cancel, {},
		/*bInterruptible*/ true, ERTActionSlot::Main));
	Catalog.Last().bCountsAsAttack = true; // aggressione dichiarata [`INT-8`]

	// `Action.Guard` — si prepara nel Prep e vale per il turno: -15 al primo danno diretto, resiste a una
	// spinta di 1 cella, scade nel Cleanup. Non interrompibile (catalogo §1).
	Catalog.Add(ShippedAction(TEXT("Action.Guard"), ERTResolutionPhase::Preparation, /*Priority*/ 40,
		/*Range (self)*/ 0, /*Cooldown*/ 0, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Guarded, /*Turni*/ 1) },
		/*bInterruptible*/ false, ERTActionSlot::Main));
	Catalog.Last().bSelfTarget = true; // si va in guardia su se' stessi: nessun bersaglio da scegliere

	// `Action.Interact` — portata 1: solo oggetti ADIACENTI.
	//
	// ✅ **Dichiara `SetDoorState` verso `Open`** ([D-148] + [D-151], 2026-08-16/17). Questa riga diceva
	// «nessun effetto dichiarato finche' non esistono oggetti da attivare»: le porte esistono da CP 9.3, e
	// mancava soltanto **questo** anello — l'effetto era applicabile (`RTHexDoorLibrary::SetDoorState`),
	// trasportabile (`RTTurnManager` lo traduce in `bChangesDoor`/`DoorState`) e raccolto
	// (`RTHexCombatLibrary` su `FirstDoorEdge`), e nessuna azione lo DICHIARAVA. Un effetto che nessuno
	// dichiara non e' un'azione che esiste (#1014).
	//
	// [D-148] — l'effetto sta nel catalogo **core** e non in un profilo d'eroe perche' aprire una porta e'
	// UNIVERSALE: chiunque la apre allo stesso modo. E' il confine di [D-033], che tiene invece fuori
	// portata ed effetto di `BasicAttack` e `Overwatch`, dove dipendono dall'eroe.
	//
	// [D-151] — **`Open` e nient'altro.** Non commuta e non chiede `Closed`: `CanTransition` vieta
	// `Locked -> Open` ma AMMETTE `Locked -> Closed`, che a una porta bloccata toglie il lock. Finche'
	// nessuna azione dichiarava `SetDoorState` quel percorso era irraggiungibile; questa riga lo rende
	// raggiungibile, e limitarla a `Open` e' cio' che lo tiene fuori portata senza toccare `CanTransition`.
	// La commutazione resta la decisione aperta `INT-7`.
	//
	// ⚠️ Lo stato viaggia in `Amount` — interi soltanto, invariante #4 — e `Open` vale **zero**. A
	// distinguere «dichiarato» da «campo di default» e' `bChangesDoor`, che `RTTurnManager` alza trovando la
	// spec e non leggendone il valore: senza quel flag ogni azione del catalogo ordinerebbe di aprire ogni
	// porta sulla propria linea di tiro.
	//
	// ⚠️ Con portata **1** la «prima porta sulla traiettoria» che `FirstDoorEdge` cerca **e'** il bordo
	// bersagliato: il meccanismo di CP 9.3 vale per `Interact` senza modifiche. Non e' una coincidenza da
	// lasciare implicita — se la portata crescesse, l'azione aprirebbe una porta che il giocatore non ha
	// scelto, ed e' una delle ragioni per cui [D-149] la tiene a 1.
	//
	// `Action.Activate` NON E' PIU' NEL CATALOGO (#199). [D-014] la dichiarava «assorbita semanticamente da
	// `Interact`» e [D-025] lo ha confermato scegliendo le sette generiche senza di lei: il catalogo la
	// spediva comunque, cioe' due azioni per una cosa sola — la doppia verita' runtime che l'issue vieta.
	// Toglierla ORA costa una riga; dopo CP 10.1, quando gli oggetti interagibili esisteranno davvero, il
	// costo sarebbe ogni consumatore scritto nel frattempo.
	//
	// Lo Stable ID e' stato cancellato del tutto con [D-134], che supera la clausola di D-014 «gli Stable ID
	// legacy non si cancellano»: quella regola proteggeva le tracce gia' scritte, e nessuna traccia versionata
	// contiene `Action.Activate` — il corpus golden porta solo `Action.Move`.
	Catalog.Add(ShippedAction(TEXT("Action.Interact"), ERTResolutionPhase::Attack, /*Priority*/ 80,
		/*Range*/ 1, /*Cooldown*/ 0, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::SetDoorState,
			static_cast<int32>(ERTHexDoorState::Open)) },
		/*bInterruptible*/ true, ERTActionSlot::Main));
	// Il puntatore deriva la forma del bersaglio da QUESTO campo, non dagli effetti ne' dall'ActionId:
	// `StructureOp != None` -> il giocatore punta un BORDO (`RTPointerInteraction.cpp`). Senza,
	// `Action.Interact` chiederebbe una cella, e una porta non e' una cella. Lo stato che chiede vive
	// nell'effetto qui sopra — questo campo dice *su cosa* agisce, non *verso cosa*.
	Catalog.Last().StructureOp = ERTStructureOp::SetDoorState;

	// `Action.Overwatch` (CP 14.5) — si ARMA nel Prep e reagisce durante i micro-step del Move. E' l'azione
	// che rende reale l'infrastruttura di E14: fino a qui `FRTOverwatchWatcher` esisteva e lo costruivano solo
	// i test, quindi nessuna partita poteva aprire una finestra.
	//
	// PORTATA ed EFFETTO restano fuori di proposito, ed e' la stessa scelta di `Action.BasicAttack` poche
	// righe sopra: area, arco, raggio e «cosa scatta» sono il **profilo**, non l'azione
	// ([D-014], [D-033] — «un'azione generica e' universale come comando, framework e semantica di fase; il
	// suo effetto concreto dipende dal profilo dell'eroe»). I profili dei quattro eroi della v0.1 sono ancora
	// una domanda aperta (`brief-azioni-generiche-overwatch.md` §8: «quali profili per i quattro eroi»), e un
	// numero scritto qui ne sceglierebbe uno arbitrario per tutti — indistinguibile, dopo un mese, da una
	// decisione presa davvero.
	//
	// Non interrompibile e senza cooldown, come da catalogo §1. Il cono NON e' un parametro: **e' il facing**
	// dell'unita' (ADR-0005 §4c), quindi non c'e' una direzione da dichiarare qui.
	Catalog.Add(ShippedAction(TEXT("Action.Overwatch"), ERTResolutionPhase::Preparation, /*Priority*/ 45,
		/*Range*/ 0, /*Cooldown*/ 0, ERTActionFallback::Cancel, {},
		/*bInterruptible*/ false, ERTActionSlot::Main));
	// Come `Guard` e `Brace`: in pianificazione non si sceglie un bersaglio — l'Overwatch arma una zona, e chi
	// entrera' nel cono e' esattamente cio' che al momento di armare non si sa. Il bersaglio si sceglie al
	// `FIRE`, dentro la finestra, e non e' un dato di catalogo.
	// ⚠️ Il flag ha un secondo effetto voluto: tiene l'Overwatch fuori dalle candidate d'ATTACCO del bot
	// (`RTTurnManager.cpp` ~610), dove sarebbe entrato con `Power` 0. Non lo fa invece entrare nel ramo «se
	// ferito usa un supporto» (~242), che richiede anche un effetto che ripristini — e qui non ce ne sono.
	Catalog.Last().bSelfTarget = true;

	// --- Mobilita' LINEARI (catalogo §2) ---------------------------------------------------------------
	// Tutte in macro-fase Dash: riposizionarsi in fretta e' cio' che permette di sparare da un'altra parte
	// nello stesso turno (ADR-0003 §3). Tutte con `Fallback.Stop`: se la traiettoria si chiude ci si ferma
	// nell'ultima cella valida, non si annulla e non si aggira.

	// `Dodge` — 3 celle su una delle sei direzioni. Occupa lo slot MOVIMENTO (D-028): chi scatta si e' mosso
	// per questo turno e non prosegue col Move, ma l'azione principale gli resta — *schivo e sparo*.
	//
	// ⚠️ **Si chiamava `Action.Dash` fino a D-230**, e il nome faceva due lavori: la macro-fase e l'azione.
	// Un turno risolve `ERTMatchPhase::Dash`, e dentro quella fase ci sono `Dodge`, `Charge`, `Leap`,
	// `Reposition` e `Sprint`: `Dash` e' il MOMENTO, non una delle cose che ci accadono. Finche' era
	// entrambi, «lo scatto risolve nel Dash» era una frase che non si poteva disambiguare.
	//
	// ⛔ **Non e' `Action.Evade`, e non si consolida con lei.** Quella e' una REAZIONE — slot
	// `ERTActionSlot::Reaction`, trigger `CellBecameHazardous`, valutata nel Cleanup — e condivide con
	// questa solo lo spostamento di una cella. Slot diversi: un'unita' puo' pianificare `Dodge` E tenere
	// `Evade` pronta nello stesso turno, e fonderle glielo toglierebbe.
	Catalog.Add(ShippedAction(TEXT("Action.Dodge"), ERTResolutionPhase::FastMovement, /*Priority*/ 30,
		/*Range*/ 3, /*Cooldown*/ 1, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::LinearDash));

	// `Charge` — 3 celle, si ferma ADDOSSO al primo nemico e lo colpisce: 20 danni piu' una spinta di 1.
	// Gli effetti sono dichiarati qui, ma si applicano nel Blast (codice 20/30 del catalogo): il movimento e'
	// fase 20, l'impatto e' controllo, e il controllo risolve per priorita' dentro il Blast.
	Catalog.Add(ShippedAction(TEXT("Action.Charge"), ERTResolutionPhase::FastMovement, /*Priority*/ 35,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Stop,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 20), FRTActionEffectSpec(ERTActionEffect::Push, 1) },
		/*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::LinearCharge));
	Catalog.Last().bCountsAsAttack = true; // consegna danno a un'unita' [`INT-8`]
	// Occupa il MOVIMENTO come ogni altra mobilita' rapida [D-191]: che una carica faccia danno a chi raggiunge
	// non cambia CHE COSA ha speso. Fino al 2026-08-26 questo capoverso diceva l'opposto - «l'unica mobilita'
	// lineare che resta sulla principale, e chi carica conserva il movimento» - seguendo la clausola di D-028
	// che D-191 ha superato: con la carica sulla principale Riktor pianificava `Ram` E l'attacco base, e il
	// resolver eseguiva entrambe. Chi vuole che una mobilita' costi ANCHE la principale lo dichiara con
	// `MovementAndMain`, che oggi nessuna azione dei cataloghi usa.

	// `Leap` — 3 celle scavalcando cio' che sta in mezzo (unita', coperture basse). La cella d'atterraggio
	// invece la si subisce: dev'essere percorribile e libera.
	Catalog.Add(ShippedAction(TEXT("Action.Leap"), ERTResolutionPhase::FastMovement, /*Priority*/ 25,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::LinearLeap));

	// `Reposition` — due celle e nient'altro: nessuno stato, nessuna traversata. E' lo scatto "tattico" che si
	// paga poco, e la differenza con `Sprint` sta tutta nei dati (2 celle in linea contro 8 MP piu' Exposed).
	Catalog.Add(ShippedAction(TEXT("Action.Reposition"), ERTResolutionPhase::FastMovement, /*Priority*/ 40,
		/*Range*/ 2, /*Cooldown*/ 1, ERTActionFallback::Stop, {},
		/*bInterruptible*/ true, ERTActionSlot::Movement, ERTMovementStyle::LinearDash));

	// --- Azioni OFFENSIVE (catalogo §3) ----------------------------------------------------------------
	// Tutte nel Blast tranne la soppressione, che si PREPARA. La priorita' e' cio' che le distingue davvero:
	// dentro la stessa macro-fase risolvono nell'ordine 40 (marchio) → 55 (linea) → 60 (precisione) →
	// 65 (area) → 80 (pesante). Il marchio arriva per primo perche' il suo +6 deve poter valere sui colpi
	// dello stesso turno; il pesante per ultimo perche' e' cio' che il catalogo compra con i suoi 35 danni.

	// `PrecisionAttack` — 24 danni fissi, portata dell'arma +1 (la mette MakePrecisionAttack: qui, come per
	// `BasicAttack`, un numero sarebbe arbitrario). E' usabile DOPO uno Sprint da [D-028], e non per un `if`
	// sull'ActionId: lo scatto lungo occupa il solo slot movimento, quindi la principale resta libera - *corro
	// e sparo*, la stessa forma di *schivo e sparo*. Prima di D-028 il catalogo la dichiarava illegale e
	// ValidateActionSlots lo otteneva come caso della regola generale; oggi la stessa regola generale la
	// AMMETTE, ed e' cio' che pinna `RefactorTactics.Actions.PrecisionAttack.WeaponRangePlusOne`.
	Catalog.Add(ShippedAction(TEXT("Action.PrecisionAttack"), ERTResolutionPhase::Attack, /*Priority*/ 60,
		/*Range*/ 0, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 24) }));
	Catalog.Last().bCountsAsAttack = true; // aggressione dichiarata [`INT-8`]

	// `HeavyAttack` — 35 danni e priorita' 80: risolve tardi, ed e' il prezzo che paga per essere il colpo
	// piu' duro. Interrompibile: se un `Action.Interrupt` la coglie prima del Blast non produce NULLA — non
	// mezzo danno, non un effetto parziale (lo garantisce URTActionEffectLibrary::ProduceEvents).
	// Contro le STRUTTURE vale 20, non 35 (DoD di CP 9.2): un colpo pesante scalfisce un muro meno di una
	// carica da sfondamento dedicata (`Gadget.BreachCharge`, 35 a struttura, epic E7 #61). Sono due scale
	// diverse, ed e' per questo che sono due effetti dichiarati e non un numero solo.
	Catalog.Add(ShippedAction(TEXT("Action.HeavyAttack"), ERTResolutionPhase::Attack, /*Priority*/ 80,
		/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 35),
		  FRTActionEffectSpec(ERTActionEffect::DamageStructure, 20) }));
	Catalog.Last().bCountsAsAttack = true; // aggressione dichiarata [`INT-8`]

	// `LineAttack` — 22 danni al PRIMO bersaglio valido su una delle sei direzioni, portata 5. Non e' la
	// `Shape::Line` delle abilita' d'archetipo (che colpisce tutti quelli attraversati): la risolve
	// URTOffensiveActionLibrary::ResolveLineAttack, che si ferma sul primo che incontra.
	// `Fallback.AttackCell`: se il bersaglio si sposta, la linea parte comunque dov'era puntata.
	Catalog.Add(ShippedAction(TEXT("Action.LineAttack"), ERTResolutionPhase::Attack, /*Priority*/ 55,
		/*Range*/ 5, /*Cooldown*/ 1, ERTActionFallback::AttackCell,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 22) }));
	Catalog.Last().bCountsAsAttack = true; // aggressione dichiarata [`INT-8`]

	// `CircularAoE` — 18 danni in un esagono di raggio 1, centro entro 4 celle. `RangeCells` e' la portata
	// del CENTRO, il raggio dell'area sta nell'intento (`FRTHexAttackIntent::AreaRadius`): sono due numeri
	// diversi e confonderli farebbe esplodere l'area a quattro celle di distanza.
	//
	// **Friendly fire**: non serve piu' dichiararlo qui. Dal 2026-08-08 `bFriendlyFire` e' vero di DEFAULT
	// (vedi `FRTActionDef`), perche' la riga esplicita qui sotto non raggiungeva il roster: gli eroi si
	// costruiscono con `MakeHeroAction`, che non aveva il parametro, e «la copia da qui e basta» non e'
	// avvenuto — `Gadget.Overload` aveva preso danno e raggio ma non il fuoco amico.
	Catalog.Add(ShippedAction(TEXT("Action.CircularAoE"), ERTResolutionPhase::Attack, /*Priority*/ 65,
		/*Range (centro)*/ 4, /*Cooldown*/ 2, ERTActionFallback::AttackCell,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 18) }));
	Catalog.Last().bCountsAsAttack = true; // aggressione dichiarata [`INT-8`]

	// `SuppressiveLine` — si PREPARA (fase 10, quindi macro-fase Prep) e si attiva su un trigger: il primo
	// nemico che entra in una cella controllata durante il Move prende 16 danni e si ferma li'. Una sola
	// attivazione per turno. Non interrompibile: una volta preparata la linea, c'e'.
	//
	// E' l'unica offensiva che non risolve nel Blast, e la ragione e' che il suo effetto non ha un bersaglio
	// al momento della pianificazione: ce l'ha chi ci cammina dentro.
	Catalog.Add(ShippedAction(TEXT("Action.SuppressiveLine"), ERTResolutionPhase::Preparation, /*Priority*/ 30,
		/*Range*/ 5, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 16) },
		/*bInterruptible*/ false));
	Catalog.Last().bCountsAsAttack = true; // consegna danno a un'unita' [`INT-8`]

	// `MarkTarget` — nessun danno proprio: applica `Status.Marked` per un turno, e il prossimo attacco
	// alleato contro quel bersaglio infligge +6 e consuma il marchio. Priorita' 40, la piu' bassa delle
	// offensive, perche' un marchio che arrivasse dopo i colpi non servirebbe a nulla.
	// `Range 0` = **portata del portatore**, come `Action.PrecisionAttack` («range dell'arma +1», catalogo §3):
	// non e' un'azione a portata nulla. La traduzione la fa `ARTTurnManager` sull'istanza (CP 8.2), che prima
	// copriva le sole azioni non catalogate e lasciava quindi queste due invalidabili per fuori portata.
	Catalog.Add(ShippedAction(TEXT("Action.MarkTarget"), ERTResolutionPhase::Attack, /*Priority*/ 40,
		/*Range*/ 0, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Marked, /*Turni*/ 1) }));
	Catalog.Last().bCountsAsAttack = true; // aggressione dichiarata [`INT-8`]

	// --- Difensive e reazioni (catalogo §4) ---------------------------------------------------------------
	// ATTENZIONE alla riga «Slot» della tabella: solo `Counter`, `Intercept` e `Deflect` occupano lo slot
	// REAZIONE (0-1 per unita', indipendente da Movimento e Principale, trigger valutato sullo snapshot del
	// Blast — CP 5.1). `Brace`, `Shield` e `Cleanse` sono azioni PRINCIPALI: si dichiarano e basta, senza
	// trigger, e risolvono nella loro fase come qualunque altra azione. Trattarle tutte e cinque come
	// "reazioni" perche' stanno nella stessa sezione del catalogo sarebbe una lettura sbagliata della tabella.
	//
	// Le reazioni non dichiarano un `Fallback` vero (il catalogo lo dice esplicitamente): `Cancel` resta un
	// segnaposto inerte, non usato da nessun percorso.
	//
	// Range 0 per tutte: la tabella non dichiara una portata per questa sezione. Per le tre difensive su se
	// stessi 0 e' il valore giusto (`self`); per `Counter` significa che il contrattacco raggiunge chi ha
	// colpito, chiunque sia — la portata di un colpo di ritorno non e' un dato che il catalogo fornisce, e
	// inventarne uno cambierebbe quali attacchi si possono punire.

	// `Counter` — contrattacco da 16 danni contro chi ha colpito, DOPO il colpo ricevuto. Il danno e'
	// dichiarato qui come effetto: il resolver lo legge dal `Def`, non da una costante propria.
	{
		FRTActionDef Counter = ShippedAction(TEXT("Action.Counter"), ERTResolutionPhase::Control, /*Priority*/ 20,
			/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::Damage, 16) }, /*bInterruptible*/ true,
			ERTActionSlot::Reaction);
		Counter.ReactionTrigger = ERTReactionTrigger::HitByDirectAttack;
		Catalog.Add(Counter);
	}

	// `Deflect` — riduce di 20 il danno diretto che l'ha innescata, DICHIARANDOLO come effetto
	// (`ERTActionEffect::DamageReduction`, CP 5.5). Fino a CP 5.2 il numero viveva solo come
	// `URTCombatLibrary::DeflectDamageReduction` letta da un `if (ActionId == "Action.Deflect")` nel
	// `TurnManager`: la costante resta la fonte del valore, ma chi lo applica ora lo legge dai dati — cosi' una
	// reazione d'eroe puo' riusare la stessa semantica con un numero proprio senza un secondo ramo.
	// Resta diverso dal -15 di `Action.Guard`, che non e' una reazione: quello e' uno stato di Prep.
	{
		FRTActionDef Deflect = ShippedAction(TEXT("Action.Deflect"), ERTResolutionPhase::Control, /*Priority*/ 15,
			/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::DamageReduction, URTCombatLibrary::DeflectDamageReduction) },
			/*bInterruptible*/ true, ERTActionSlot::Reaction);
		Deflect.ReactionTrigger = ERTReactionTrigger::HitByDirectAttack;
		Catalog.Add(Deflect);
	}

	// `Intercept` — l'intercettore DIVENTA il bersaglio di un colpo diretto a un alleato entro 2 celle.
	// Nessun effetto dichiarato: non aggiunge danno, ne' sposta, ne' applica stati — cambia CHI subisce un
	// colpo altrui, che non e' esprimibile come `FRTActionEffectSpec` (quelli agiscono su un bersaglio dato).
	//
	// Range **2**: qui il numero e' dichiarato dal catalogo ("un alleato entro 2 celle"), non deciso da noi.
	// Priorita' **10**, la piu' bassa fra le reazioni (Deflect 15, Counter 20), ed e' una regola, non un
	// dettaglio: cambiando il bersaglio dei colpi, Intercept deve risolvere PRIMA che le altre reazioni
	// valutino chi e' stato colpito — altrimenti il bersaglio originale contrattaccherebbe per un colpo che
	// non ha piu' ricevuto.
	{
		FRTActionDef Intercept = ShippedAction(TEXT("Action.Intercept"), ERTResolutionPhase::Control, /*Priority*/ 10,
			/*Range*/ 2, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}, /*bInterruptible*/ true,
			ERTActionSlot::Reaction);
		Intercept.ReactionTrigger = ERTReactionTrigger::AllyHitByDirectAttack;
		Catalog.Add(Intercept);
	}

	// `Anchor` (CP 7.5, `#505`) — chi la dichiara NON viene spostato dalla spinta o dalla trazione di questo
	// Blast. E' la reazione core dello spostamento, e la quarta del catalogo.
	//
	// Esiste come azione core, e non solo come modulo di equipaggiamento, perche' e' dall'azione core che un
	// modulo eredita cio' che lo rende una reazione: fase, priorita' e soprattutto il TRIGGER. Costruito su
	// `Action.Counter`, `Reaction.Anchor` erediterebbe `HitByDirectAttack` e si attiverebbe sui colpi — un
	// altro mestiere, e per giunta silenziosamente sbagliato.
	//
	// Priorita' **5**, sotto Intercept (10), Deflect (15) e Counter (20). Non contende niente a nessuno: non
	// produce colpi, non riduce danno e risolve in un punto del turno tutto suo, dove le altre non arrivano.
	// Un numero pero' va scelto, e il piu' basso e' quello che dichiara «questa non precede nessuno» invece di
	// suggerire una precedenza che nessun caso puo' osservare.
	//
	// `Amount` 1 e non 0: l'annullamento non ha un «quanto», ma `ProduceEvents` scarta gli effetti a entita'
	// non positiva, e dichiararlo 0 lo renderebbe un effetto che non esiste.
	{
		FRTActionDef Anchor = ShippedAction(TEXT("Action.Anchor"), ERTResolutionPhase::Control, /*Priority*/ 5,
			/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::CancelDisplacement, 1) }, /*bInterruptible*/ true,
			ERTActionSlot::Reaction);
		Anchor.ReactionTrigger = ERTReactionTrigger::AboutToBeDisplaced;
		Catalog.Add(Anchor);
	}

	// `Purge` (CP 7.5, `#505`) — la reazione core del CONTROLLO: annulla lo stato di controllo che stai per
	// ricevere. Quinta e ultima reazione core della v0.1.
	//
	// ⚠️ **Non si chiama `Cleanse`, e la differenza non e' cosmetica**: `Action.Cleanse` esiste gia' ed e'
	// un'azione PRINCIPALE che sceglie fra gli stati **gia' posseduti** seguendo la lista che il giocatore
	// dichiara in pianificazione (`ARTUnit::PlannedCleansePriority`). Li' l'ambiguita' e' reale e la scelta
	// va dichiarata; qui lo stato lo determina l'evento, e con piu' controlli si annulla il piu' grave. Due
	// mestieri diversi sotto lo stesso nome sarebbero diventati un ramo `if` nel resolver.
	//
	// Priorita' **5** come `Action.Anchor`: risolve in un punto del turno tutto suo, dove nessun'altra
	// reazione arriva, quindi non contende niente a nessuno.
	{
		FRTActionDef Purge = ShippedAction(TEXT("Action.Purge"), ERTResolutionPhase::Control, /*Priority*/ 5,
			/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::CancelStatus, 1) }, /*bInterruptible*/ true,
			ERTActionSlot::Reaction);
		Purge.ReactionTrigger = ERTReactionTrigger::AboutToReceiveControl;
		Catalog.Add(Purge);
	}

	// `Evade` (CP 7.5, `#505`) — la reazione core dell'AMBIENTE: ti sposti di una cella quando quella sotto di
	// te diventa pericolosa. Sesta e ultima reazione core della v0.1.
	//
	// L'unica che risolve fuori dal Blast: il suo trigger nasce nel Cleanup, dove le superfici vengono create.
	// La fase dichiarata resta `Control` come le altre reazioni — a portarla nel Cleanup e' il PUNTO di
	// valutazione (`URTReactionLibrary::PassPointFor`), non la fase dell'azione: sono due cose diverse, e
	// confonderle qui avrebbe fatto risolvere l'azione nel Blast insieme al resto.
	//
	// Non si chiama `HazardEscape` per la stessa ragione per cui `Purge` non si chiama `Cleanse`: quello e' il
	// nome del MODULO nel catalogo §3, e l'azione core e' un'altra entita'.
	{
		FRTActionDef Evade = ShippedAction(TEXT("Action.Evade"), ERTResolutionPhase::Control, /*Priority*/ 5,
			/*Range*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::SelfReposition, 1) }, /*bInterruptible*/ true,
			ERTActionSlot::Reaction);
		Evade.ReactionTrigger = ERTReactionTrigger::CellBecameHazardous;
		Catalog.Add(Evade);
	}

	// `Brace` — azione PRINCIPALE di Prep. Dichiara DUE stati: `Braced` (-10 a ogni danno diretto e blocca la
	// prima spinta) e `Root` (blocca il movimento volontario). Root e' riuso 1:1 di un meccanismo gia'
	// collaudato — azzera movimento e scatto, non tocca attacchi ne' spostamento subito, che e' esattamente
	// cio' che il catalogo chiede a chi si irrigidisce: si pianta per incassare, non smette di combattere.
	Catalog.Add(ShippedAction(TEXT("Action.Brace"), ERTResolutionPhase::Preparation, /*Priority*/ 30,
		/*Range (self)*/ 0, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Braced, /*Turni*/ 1),
		  FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Root, /*Turni*/ 1) },
		/*bInterruptible*/ false, ERTActionSlot::Main));
	Catalog.Last().bSelfTarget = true; // come Guard: lo stato lo prende chi la pianifica

	// `Shield` — azione PRINCIPALE di Prep: 25 punti di scudo TEMPORANEO, consumati prima della salute e
	// scaduti nel Cleanup. Stesso identico meccanismo di `Guardian.Barrier` (che ne da' 40): `ResolvePrep`
	// traduce l'effetto in `AddTemporaryShield` senza sapere quale azione l'abbia prodotto. Non protegge dal
	// controllo senza danno per costruzione — uno scudo assorbe danno, e Root/Slow non ne sono.
	Catalog.Add(ShippedAction(TEXT("Action.Shield"), ERTResolutionPhase::Preparation, /*Priority*/ 35,
		/*Range (self)*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Shield, 25) },
		/*bInterruptible*/ false, ERTActionSlot::Main));

	// `Cleanse` — azione PRINCIPALE, codice 30 (controllo) quindi risolve nel Blast PRIMA del danno: purificarsi
	// dopo aver incassato il colpo che lo stato ha aggravato non servirebbe a niente. Nessun effetto dichiarato:
	// "rimuovi uno stato a scelta" non e' esprimibile come `FRTActionEffectSpec` (che applica, non toglie), e
	// soprattutto QUALE stato lo decide il piano del giocatore (`ARTUnit::PlannedCleansePriority`), non il dato
	// dell'azione.
	Catalog.Add(ShippedAction(TEXT("Action.Cleanse"), ERTResolutionPhase::Control, /*Priority*/ 25,
		/*Range (self)*/ 0, /*Cooldown*/ 2, ERTActionFallback::Cancel, {},
		/*bInterruptible*/ true, ERTActionSlot::Main));

	// --- Azioni di CONTROLLO (catalogo §5) -------------------------------------------------------------
	// Tutte risolvono nel Blast (fase dichiarata `Control`, codice 30) PRIMA del danno: la priorita' le mette
	// nell'ordine 20 (Interrupt) → 25 (Root) → 40 (Push/Pull) → 50 (Slow) — sotto la piu' bassa offensiva
	// (MarkTarget, anch'essa 40): un'interruzione o un radicamento devono valere prima che qualunque colpo
	// parta, non dopo. Range 1 per Push/Root/Interrupt/Slow, **2 per Pull**: la tabella del catalogo non
	// dichiarava una portata (unica sezione senza colonna Range), quindi il numero e' deciso qui — e per Pull
	// e' diverso dagli altri quattro per un motivo geometrico dichiarato sotto, non per svista.
	//
	// Push/Root/Slow riusano la STESSA pipeline di `ResolveCombat` che gia' applica gli effetti di
	// Guardian.Sweep/Ranger.Burst (un'azione senza danno e' comunque un "colpo" con Power 0: l'effetto
	// collaterale passa lo stesso). Interrupt e' l'eccezione: cancella l'INTERA azione di un'altra unita', non
	// un effetto su un bersaglio, e per questo non dichiara nessun `FRTActionEffectSpec` — la sua conseguenza
	// si applica filtrando `Plan.Hits` prima che diventino danno o eventi (`ARTTurnManager::ResolveCombat`).

	// `Push` — spinta di 1 cella, che allontana. Riusa lo stesso meccanismo di knockback di Guardian.Sweep.
	Catalog.Add(ShippedAction(TEXT("Action.Push"), ERTResolutionPhase::Control, /*Priority*/ 40,
		/*Range*/ 1, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Push, 1) }));
	Catalog.Last().bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo, come `MarkTarget` [`INT-8`]

	// `Pull` — trazione di 1 cella, che avvicina: prima azione del catalogo a usare `ERTActionEffect::Pull`.
	// Range **2**, non 1 come le altre quattro: con targeting a 1 (adiacenza) e trazione di 1, il bersaglio
	// finirebbe SEMPRE sulla cella di chi tira — sempre occupata, quindi la trazione si annullerebbe per
	// costruzione, in ogni caso, senza eccezioni. E' l'unica delle cinque a deviare, e la ragione e'
	// geometrica: bisogna poter agganciare un bersaglio a 2 celle per tirarlo a 1 senza finirgli addosso.
	Catalog.Add(ShippedAction(TEXT("Action.Pull"), ERTResolutionPhase::Control, /*Priority*/ 40,
		/*Range*/ 2, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Pull, 1) }));
	Catalog.Last().bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo, come `MarkTarget` [`INT-8`]

	// `Root` — blocca il movimento per 1 turno. Cancella i micro-step di movimento NON ANCORA risolti (fase
	// Move, dopo il Blast) tramite `GetEffectiveMoveRange`, che azzera il budget per chi e' radicato — non
	// impedisce attacchi, Guard o Activate, che non passano da quel budget.
	Catalog.Add(ShippedAction(TEXT("Action.Root"), ERTResolutionPhase::Control, /*Priority*/ 25,
		/*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Root, /*Turni*/ 1) }));
	Catalog.Last().bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo, come `MarkTarget` [`INT-8`]

	// `Slow` — +1 al costo di OGNI cella per 1 turno (non dimezza il raggio: e' un meccanismo diverso da
	// quello che `Ranger.Burst` applicava allo stesso tag prima di questo checkpoint — vedi
	// `URTCombatLibrary::EffectiveMoveRange` e il modificatore di costo in `FRTHexSimUnit`). Non riduce la
	// portata delle mobilita' lineari (Dash/Charge/Leap/Reposition): quelle non hanno un costo per cella da
	// aumentare, e la v0.1 le dichiara fuori dall'effetto di Slow.
	Catalog.Add(ShippedAction(TEXT("Action.Slow"), ERTResolutionPhase::Control, /*Priority*/ 50,
		/*Range*/ 1, /*Cooldown*/ 1, ERTActionFallback::Cancel,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Slow, /*Turni*/ 1) }));
	Catalog.Last().bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo, come `MarkTarget` [`INT-8`]

	// `Interrupt` — nessun effetto dichiarabile: la sua conseguenza e' cancellare l'azione di un'altra unita',
	// non modificarne le statistiche. Agisce solo su chi dichiara `bCanBeInterrupted = true` — il controllo
	// e' fatto da `ARTTurnManager::ResolveCombat`, non da un flag che questa azione porterebbe con se'.
	Catalog.Add(ShippedAction(TEXT("Action.Interrupt"), ERTResolutionPhase::Control, /*Priority*/ 20,
		/*Range*/ 1, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));
	Catalog.Last().bCountsAsAttack = true; // controllo OSTILE: raggiunge il bersaglio come colpo, come `MarkTarget` [`INT-8`]

	// --- Azioni AMBIENTALI (catalogo §6) -----------------------------------------------------------------
	// `Electrify` — la combo firma del gioco (CP 8.3). Fase `Environment` (codice 50), quindi risolve nel
	// **Cleanup**, dopo il Move: cosi' colpisce anche chi e' appena entrato nell'acqua, che e' il punto
	// tattico dell'azione. Portata 4, cooldown 2, danno iniziale 20 (catalogo azioni §6).
	//
	// `PropagationLimit = 3` e' il primo valore non nullo di quel campo: fino a qui esisteva solo come regola
	// del validator («una propagazione illimitata rende il turno impredicibile», errore dichiarato §17). Il
	// danno PROPAGATO (12) non e' un secondo `Effects`, perche' non e' un effetto dell'azione sul bersaglio
	// ma il valore che l'ambiente porta oltre: vive come `URTCombatLibrary::PropagatedElectricDamage`,
	// accanto alle altre costanti di calcolo (Guard, Deflect, Brace, Burning).
	{
		FRTActionDef Electrify = ShippedAction(TEXT("Action.Electrify"), ERTResolutionPhase::Environment,
			/*Priority*/ 30, /*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::Damage, 20) });
		Electrify.PropagationLimit = 3;
		Catalog.Add(Electrify);
	}

	// `Ignite` e `CreateWater` — le due azioni che CAMBIANO la mappa (CP 8.4). Il catalogo azioni le elenca
	// entrambe con priorita' 60 e portata 4; qui risolvono nella sola fase `Environment` (Cleanup), non nel
	// Blast: una cella che prende fuoco a meta' turno cambierebbe il costo di un percorso gia' calcolato.
	//
	// Nessuna delle due dichiara `Effects`: il loro esito non e' un effetto su un'UNITA' (danno, cura, stato)
	// ma una modifica della CELLA, che `FRTActionEffectSpec` non sa esprimere — gli effetti si applicano a
	// bersagli, non a terreno. La superficie che creano e la durata vivono nel resolver ambientale, che e'
	// l'unico posto in cui il terreno dinamico esiste.
	//
	// **Durata 2 turni** per entrambe, dal catalogo terreni §2 (fuoco) e dal catalogo azioni §6 (acqua).
	// La superficie creata e' un DATO dell'azione, non un ramo nel resolver: cosi' un'abilita' d'eroe che
	// copia questa definizione (D-046: `Phase.FluidTrail`) eredita il comportamento senza che nessuno debba
	// aggiungere il suo nome a un `if`.
	{
		FRTActionDef Ignite = ShippedAction(TEXT("Action.Ignite"), ERTResolutionPhase::Environment, /*Priority*/ 60,
			/*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::Cancel, {});
		Ignite.bCreatesSurface = true;
		Ignite.SurfaceCreated = ERTHexSurface::Fire;
		Ignite.SurfaceRadius = 0; // la sola cella bersaglio: dichiarato, non lasciato al default
		Catalog.Add(Ignite);
	}
	{
		FRTActionDef CreateWater = ShippedAction(TEXT("Action.CreateWater"), ERTResolutionPhase::Environment,
			/*Priority*/ 60, /*Range*/ 4, /*Cooldown*/ 2, ERTActionFallback::Cancel, {});
		CreateWater.bCreatesSurface = true;
		CreateWater.SurfaceCreated = ERTHexSurface::ShallowWater;
		CreateWater.SurfaceRadius = 1; // «acqua raggio 1» (catalogo azioni §6): era cablato nel resolver
		Catalog.Add(CreateWater);
	}

	// `ModifyArc` — apre o chiude un COLLEGAMENTO fra celle. Non tocca le superfici: cambia la topologia, ed e'
	// per questo che la DoD chiede che **incrementi la revisione** della mappa — il numero che invalida le
	// cache di percorso.
	//
	// **Fase cambiata in CP 9.4** (2026-08-08), da `Environment` (Cleanup) ad `Attack` (Blast). La ragione
	// scritta qui prima — «cambiare un arco a meta' Blast renderebbe invalido un percorso gia' calcolato in
	// questo stesso turno» — era vera quando e' stata scritta e non lo e' piu': `TruncatePathToTopology`
	// (CP 9.3) tronca il percorso al primo passo che il grafo non offre piu', quindi un percorso invalidato
	// non produce un fantasma ma una fermata con reason code (`BlockedByTopology`).
	//
	// Il guadagno e' l'uniformita': porte, muri e ponti cambiano tutti a fase conclusa nel Blast e il Move che
	// segue li vede. Due tempi diversi per due oggetti topologici sarebbero una regola che nessun giocatore
	// puo' dedurre guardando il campo.
	//
	// `Attack` (40) copre «attacchi, abilita', cure, **interazioni**»: e' un'interazione con la mappa.
	Catalog.Add(ShippedAction(TEXT("Action.ModifyArc"), ERTResolutionPhase::Attack, /*Priority*/ 75,
		/*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Cancel, {}));

	// `CreateCover` — ERIGE una copertura bassa su un bordo (CP 9.5). Come `Ignite`, `CreateWater` e `ModifyArc`
	// non dichiara `Effects`: il suo esito e' una modifica della MAPPA, che `FRTActionEffectSpec` non sa
	// esprimere. Integrita' 30 e durata 2 turni vengono dal catalogo terreni (`Structure.KineticPanel`), e
	// stanno nel resolver insieme alla nozione di turno.
	//
	// **Fase `Preparation`, non `Attack`** — e qui il catalogo azioni v0.1 (che diceva Blast) e' stato
	// allineato al catalogo eroi, non viceversa (D-a, 2026-08-09). La ragione che porto' `ModifyArc` nel Blast
	// a CP 9.4 — «porte, muri e ponti cambiano tutti nello stesso momento, e il Move che segue li vede» —
	// riguarda la TOPOLOGIA: un arco o una porta cambiano il grafo, e un percorso gia' calcolato va troncato.
	// Una copertura BASSA non tocca ne' il grafo ne' la vista (E9.1): riduce il danno. Non c'e' nessun percorso
	// da invalidare, quindi l'uniformita' topologica non la riguarda — mentre la ragione opposta si': eretta
	// nel Blast arriverebbe DOPO aver incassato i colpi di quel Blast, cioe' nel turno in cui la si paga non
	// servirebbe a niente.
	{
		FRTActionDef CreateCover = ShippedAction(TEXT("Action.CreateCover"), ERTResolutionPhase::Preparation,
			/*Priority*/ 75, /*Range*/ 3, /*Cooldown*/ 2, ERTActionFallback::Cancel, {});
		CreateCover.StructureOp = ERTStructureOp::CreateCover;
		Catalog.Add(CreateCover);
	}

	// `Heal` — cura 20, portata 3, e **puo' bersagliare se stessi** (catalogo azioni §6). Priorita' 70: risolve
	// DOPO gli attacchi (50-65), quindi cura le ferite di questo turno e non quelle del turno prima.
	// A differenza delle ambientali risolve nel **Blast**: e' un'azione di supporto, non una modifica del campo.
	{
		FRTActionDef Heal = ShippedAction(TEXT("Action.Heal"), ERTResolutionPhase::Attack, /*Priority*/ 70,
			/*Range*/ 3, /*Cooldown*/ 1, ERTActionFallback::Cancel,
			{ FRTActionEffectSpec(ERTActionEffect::Heal, 20) });
		Catalog.Add(Heal);
	}

	return Catalog;
}

int32 URTCatalogLibrary::BasicAttackDamageForRange(int32 WeaponRangeCells)
{
	// Tabella delle fasce (catalogo v0.1 §1): corpo a corpo 28/r1 · corto 25/r3 · medio 22/r4 · lungo 20/r6.
	// Piu' lontano si colpisce, meno si fa male: e' la scelta orizzontale del catalogo, non una scala di potenza.
	// Le portate intermedie ricadono nella fascia il cui limite le contiene (r2 -> corto, r5 -> lungo).
	if (WeaponRangeCells <= 1) { return 28; }
	if (WeaponRangeCells <= 3) { return 25; }
	if (WeaponRangeCells <= 4) { return 22; }
	return 20;
}

FRTActionDef URTCatalogLibrary::MakeBasicAttack(int32 WeaponRangeCells)
{
	// L'attacco base di UN eroe: l'identita' viene dal catalogo, i due numeri che dipendono dall'arma li mette
	// la fascia. Cosi' `Action.BasicAttack` resta una sola azione con un solo ID, invece di quattro varianti.
	FRTActionDef Def = FindCoreAction(TEXT("Action.BasicAttack"));
	if (Def.ActionId.IsNone())
	{
		return Def; // catalogo incompleto: meglio una definizione vuota che una inventata qui
	}

	const int32 Range = FMath::Max(1, WeaponRangeCells);
	Def.RangeCells = Range;
	Def.Effects.Reset();
	Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, BasicAttackDamageForRange(Range)));
	return Def;
}

FRTActionDef URTCatalogLibrary::MakeWeaponAttack(const FName& ActionId, int32 WeaponRangeCells)
{
	// Il catalogo dichiara per queste azioni un targeting "bersaglio" senza numero: la portata e' quella
	// dell'arma dell'eroe. Metterne una qui significherebbe sceglierne una arbitraria per tutti.
	FRTActionDef Def = FindCoreAction(ActionId);
	if (Def.ActionId.IsNone())
	{
		return Def; // catalogo incompleto: meglio una definizione vuota che una inventata qui
	}

	Def.RangeCells = FMath::Max(1, WeaponRangeCells);
	return Def;
}

FRTActionDef URTCatalogLibrary::MakePrecisionAttack(int32 WeaponRangeCells)
{
	// Il **+1** e' l'identita' della precisione (catalogo §3): si colpisce una cella piu' lontano di quanto
	// arrivi l'arma. Sta in una funzione che porta il nome dell'azione, non in un `if` sull'ActionId dentro
	// MakeWeaponAttack — cosi' aggiungere un'altra azione con un bonus diverso non tocca nulla di questo.
	return MakeWeaponAttack(TEXT("Action.PrecisionAttack"), FMath::Max(1, WeaponRangeCells) + 1);
}

bool URTCatalogLibrary::TakesMovementSlot(const FRTActionDef& Action)
{
	return Action.Slot == ERTActionSlot::Movement || Action.Slot == ERTActionSlot::MovementAndMain;
}

bool URTCatalogLibrary::TakesMainSlot(const FRTActionDef& Action)
{
	return Action.Slot == ERTActionSlot::Main || Action.Slot == ERTActionSlot::MovementAndMain;
}

TArray<FString> URTCatalogLibrary::ValidateActionSlots(const TArray<FRTActionDef>& PlannedActions)
{
	TArray<FString> Errors;

	// Chi ha gia' preso quale slot: serve a NOMINARE il colpevole («la principale e' occupata da Sprint»),
	// perche' un errore che dice solo "slot pieno" costringe a ricostruire il piano a mano.
	FName MovementTakenBy;
	FName MainTakenBy;

	for (const FRTActionDef& Action : PlannedActions)
	{
		const bool bTakesMovement = TakesMovementSlot(Action);
		const bool bTakesMain = TakesMainSlot(Action);

		if (bTakesMovement)
		{
			if (!MovementTakenBy.IsNone())
			{
				Errors.Add(FString::Printf(TEXT("%s: slot movimento gia' occupato da %s"),
					*Action.ActionId.ToString(), *MovementTakenBy.ToString()));
			}
			else
			{
				MovementTakenBy = Action.ActionId;
			}
		}

		if (bTakesMain)
		{
			if (!MainTakenBy.IsNone())
			{
				Errors.Add(FString::Printf(TEXT("%s: azione principale gia' occupata da %s"),
					*Action.ActionId.ToString(), *MainTakenBy.ToString()));
			}
			else
			{
				MainTakenBy = Action.ActionId;
			}
		}
	}

	return Errors;
}

FRTActionDef URTCatalogLibrary::FindCoreAction(const FName& ActionId)
{
	// Nessuna traduzione: l'ID che arriva e' l'ID che si cerca. Fino a [D-134] qui passava
	// `ResolveLegacyActionId`, che traduceva gli Stable ID ritirati nel loro erede; la macchina e' stata
	// rimossa perche' non aveva nulla da proteggere — il corpus di tracce versionate non contiene un solo
	// ID ritirato, e il gioco non e' ancora uscito.
	for (const FRTActionDef& Def : GetCoreActionCatalog())
	{
		if (Def.ActionId == ActionId) { return Def; }
	}
	return FRTActionDef();
}

TArray<FName> URTCatalogLibrary::GetGenericActionIds()
{
	// L'ordine e' quello di D-025 per le cinque che entrano, e conta: sono accodate al kit, quindi diventano
	// indici stabili. Cambiarlo sposta gli indici di ogni unita' — e `PlannedAbilityIndex` e' un indice.
	//
	// ⚠️ `Action.Overwatch` entra IN CODA (CP 14.5), e la posizione non e' estetica: in D-025 e' comunque
	// l'ultima delle sette, ma soprattutto accodarla lascia `Wait`/`Guard`/`Brace` agli indici 0/1/2 che
	// avevano. Inserirla in mezzo avrebbe spostato `Guard` e `Brace` sotto i piedi di ogni piano gia' scritto
	// — `PlannedAbilityIndex` e `PlannedReactionAbility` sono indici — senza che nulla smettesse di compilare.
	// Pinnato da `Overwatch.ActionIsInCoreCatalog`, che confronta la lista per intero e non solo l'ultima.
	//
	// ⚠️ `Action.Interact` entra IN CODA per la stessa ragione, e con la stessa storia: fino al 2026-08-26 era
	// fuori perche' *«nessun codice risolve un'interazione»*, e quel motivo e' scaduto. Oggi l'azione dichiara
	// `SetDoorState -> Open` [D-148/D-151], `ARTTurnManager` traduce l'effetto in `bChangesDoor`/`DoorState`
	// per QUALUNQUE principale pianificata, `URTHexCombatLibrary` raccoglie l'op sulla prima porta della
	// traiettoria e `URTHexDoorLibrary::SetDoorState` la applica. L'altra meta' esiste: e' il criterio con cui
	// erano entrate `Guard`, `Brace` e poi `Overwatch`.
	//
	// ⚠️ Entra con UN bersaglio funzionante — le porte. Consolle, ascensori, generatori, sprinkler, ponti e
	// obiettivi del catalogo §1 non esistono, e questa riga non li promette.
	return { TEXT("Action.Wait"), TEXT("Action.Guard"), TEXT("Action.Brace"), TEXT("Action.Overwatch"),
	         TEXT("Action.Interact") };
}

namespace
{
	/**
	 * Il nome player-facing di un'azione generica. I cinque valori sono la colonna «Azione» di
	 * `docs/balance/RT_ActionCatalog_v0.1.md` §1: **presi**, non scelti qui.
	 *
	 * ⚠️ Un `ActionId` non presente restituisce testo vuoto, e
	 * `RefactorTactics.Actions.EveryGenericHasADisplayName` diventa rosso. E' voluto, ed e' lo stesso
	 * meccanismo di `HeroActionDisplayName`: una generica aggiunta domani senza nome si fa notare subito
	 * invece di comparire in partita come `[RT] RTUnit_0: abilita' attiva -> `.
	 *
	 * 🔴 **Fino al 2026-08-26 questa tabella non esisteva e le generiche entravano nel kit SENZA nome.**
	 * Il difetto era invisibile perche' nessun tasto le raggiungeva: [#1439](https://github.com/DegrassiAaron/refactor-tactics-main/pull/1439)
	 * ha dato loro un tasto, e la prima pressione lo avrebbe mostrato a schermo. E' la stessa famiglia di
	 * [#892], che aveva coperto le venti abilita' d'eroe e non queste — perche' allora non si vedevano.
	 */
	FText GenericActionDisplayName(const FName& Id)
	{
		static const TMap<FName, FString> Names = {
			{ TEXT("Action.Wait"),      TEXT("Attesa") },
			{ TEXT("Action.Guard"),     TEXT("Guardia") },
			{ TEXT("Action.Brace"),     TEXT("Irrigidimento") },
			{ TEXT("Action.Overwatch"), TEXT("Guardia reattiva") },
			{ TEXT("Action.Interact"),  TEXT("Interagisci") },
		};
		const FString* Found = Names.Find(Id);
		return Found ? FText::FromString(*Found) : FText::GetEmpty();
	}
}

TArray<URTActionData*> URTCatalogLibrary::MakeGenericActions(UObject* Outer)
{
	TArray<URTActionData*> Actions;
	for (const FName& Id : GetGenericActionIds())
	{
		const FRTActionDef Def = FindCoreAction(Id);
		// Un ID che il catalogo non conosce non produce un'azione vuota: quella entrerebbe nel kit con
		// `ActionId` nullo e si presenterebbe come un comando reale che non fa niente.
		if (Def.ActionId.IsNone())
		{
			continue;
		}
		URTActionData* Action = NewObject<URTActionData>(Outer);
		Action->Def = Def;
		// Campi SPECCHIO allineati al catalogo. `ARTPlayerController` e `ARTTurnManager` leggono ancora
		// questi e non il `Def`, e i loro default sono quelli legacy dell'MVP quadrato: `RangeCells` 5 e
		// `Power` 30. Senza questa propagazione ogni azione generica entrava nel kit con portata 5 e potenza
		// 30 QUALUNQUE cosa il catalogo dichiarasse — e il bot valutava `Action.Wait`, che di portata ne ha
		// 0 e di danno nessuno, fra le candidate d'ATTACCO come un colpo da 30 a distanza 5.
		Action->RangeCells = Def.RangeCells;
		Action->bSelfTarget = Def.bSelfTarget;
		// 🔴 La RICARICA mancava, e non era un dettaglio di test (#1552). `ConsumeAbility` legge questo
		// specchio e non il `Def`: con lo zero di default non scriveva niente in `AbilityCooldowns`,
		// `CanUseAbility` rispondeva sempre `true`, e `Action.Brace` — che il catalogo dichiara con
		// `Cooldown 1` — era riarmabile ogni turno da OGNI eroe del roster, perche' le generiche si
		// accodano al kit di tutti. Gli altri tre costruttori la copiavano gia' (`MakeHeroAction`,
		// `MakeEquipmentAction`, l'attacco base): questo era l'unico che non lo faceva.
		Action->CooldownTurns = Def.CooldownTurns;
		Action->Power = 0;
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { Action->Power = Spec.Amount; break; }
		}
		// Il NOME arriva dal catalogo di bilanciamento, e senza di esso l'azione entra nel kit muta: il
		// giocatore che la arma legge `abilita' attiva -> ` e non sa cosa ha armato.
		Action->DisplayName = GenericActionDisplayName(Id);
		Actions.Add(Action);
	}
	return Actions;
}

const URTEquipmentData* URTCatalogLibrary::FindEquipment(FName EquipmentId)
{
	if (EquipmentId.IsNone())
	{
		return nullptr;
	}

	// I tre cataloghi nell'ordine del documento (§1 armi, §2 gadget, §3 reazioni). Si scorre e si esce al
	// primo: un `EquipmentId` ripetuto fra due sezioni sarebbe un difetto di catalogo che `ValidateEquipment`
	// deve prendere, non un'ambiguita' da risolvere qui scegliendo.
	for (const TArray<URTEquipmentData*>& Catalog :
		{ MakeWeaponVariants(), MakeGadgets(), MakeReactionModules() })
	{
		for (URTEquipmentData* Item : Catalog)
		{
			if (Item && Item->EquipmentId == EquipmentId)
			{
				return Item;
			}
		}
	}
	return nullptr;
}
