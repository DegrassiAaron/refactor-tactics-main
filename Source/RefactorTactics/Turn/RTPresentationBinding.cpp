#include "Turn/RTPresentationBinding.h"

TArray<FRTPresentationBinding> URTPresentationBindingLibrary::DeclaredBindings()
{
	TArray<FRTPresentationBinding> Out;

	// Move — il playback muove il cilindro e accende la locomozione. `bIsMovingVisually` e' cio' che
	// `URTUnitAnimInstance` legge per passare Idle<->Run (`RTTurnManager.cpp:6208`), e `SetVisualLocation`
	// posiziona all'inizio della prima anim perche' non ci sia un flash sulla cella finale.
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::Move,
		{ FName(TEXT("bIsMovingVisually")), FName(TEXT("SetVisualLocation")) }));

	// Attack — due cue e due soggetti diversi: l'attaccante gioca il colpo, il bersaglio lo incassa
	// (`RTTurnManager.cpp:6257-6258`). Sono `BlueprintImplementableEvent` su `ARTUnit`: se un BP non le
	// implementa non succede nulla, e la logica resta invariata (invariante #1).
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::Attack,
		{ FName(TEXT("PlayAttackMontage")), FName(TEXT("PlayHitMontage")) }));

	// HazardDamage — NoPresentation, deciso il 2026-08-31.
	//
	// 🔴 **La clausola fa parte della dichiarazione, e non e' pedanteria.** Il danno da terreno ACCADE ed e'
	// tutt'altro che marginale — `Fire` fa 10 danni all'ingresso contro gli 8 del Cleanup, passa da
	// `ApplyDamage(..., Environmental, ...)`, puo' uccidere, e ha la voce canonica nel TurnLog dal 2026-08-16
	// (`#1067`). Non si mostra perche' ha gia' due canali visibili: la barra vita, e il combat log.
	//
	// ✅ **Il PRODUTTORE ora esiste** (`#2460`, 2026-09-05). Fino a quel giorno questa voce portava la
	// clausola *«oggi il valore non ha un produttore … va rivista appena ne acquista uno»*: e' quel giorno,
	// e questa e' la revisione. L'evento nasce in `ARTTurnManager::AppendLogEntry`, dallo stesso punto e
	// nello stesso istante della voce di TurnLog, per entrambe le cause che
	// `URTTurnLogLibrary::IsEnvironmentalDamage` riconosce — `Terrain.<Surface>` all'ingresso e
	// `Status.Burning` nel Cleanup.
	//
	// 🔴 **La voce resta `NoPresentation`, e la scelta e' deliberata.** Il produttore consegna il **dato**;
	// la cue e' lavoro di `#2455`. Promuoverla adesso a una presentazione dichiarerebbe disegnato un evento
	// che nessuno disegna — cioe' il difetto opposto e peggiore, ed e' precisamente quello che questa
	// tabella esiste per impedire.
	//
	// ⚠️ **Corretta insieme una frase che era falsa**: diceva *«quando uccide, l'evento emesso e' `Defeated`,
	// che una presentazione ce l'ha»*. Misurato: `Defeated` lo emette **solo** `ResolveCombatPasses`, da
	// `NewlyDefeated` calcolato sul Blast. Chi muore bruciato nel Cleanup, o entrando nel fuoco, **non lo
	// produce**: resta coperto dal catch-all di `ConcludeTurn` (`DestroyDefeatedUnits`). Chi costruira' la
	// cue non deve credere che la morte da hazard abbia gia' un beat proprio — non ce l'ha.
	Out.Add(FRTPresentationBinding::MakeNoPresentation(ERTResolvedEventType::HazardDamage,
		TEXT("Il danno da terreno si legge dalla barra vita e dal combat log: una cue dedicata non aggiunge ")
		TEXT("leggibilita' in v0.1, e D-124 tiene il sistema VFX degli status fuori dal perimetro. Il ")
		TEXT("produttore ESISTE da #2460 (AppendLogEntry, per ogni causa che IsEnvironmentalDamage riconosce): ")
		TEXT("l'assenza e' una scelta sul disegno, non un evento muto. La cue e' lavoro di #2455. ATTENZIONE: ")
		TEXT("la morte da hazard non emette Defeated - la nasconde il catch-all di ConcludeTurn.")));

	// AttackFootprint — NoPresentation, e per una ragione OPPOSTA a quella di HazardDamage.
	//
	// 🔴 **Qui l'assenza e' temporanea e attesa, non una scelta di design.** `HazardDamage` non si mostra
	// perche' si e' deciso che non deve; questo evento esiste **precisamente perche' un giorno si mostri** —
	// #1945 lo introduce per portare a valle le celle risolte, e la cue che le disegna (tracer, impatto,
	// resa dell'area) e' lavoro di E21 che non e' ancora stato fatto.
	//
	// ⚠️ **Dichiarare cue inventate sarebbe peggio che dichiarare l'assenza.** Le altre voci di questa
	// tabella nominano funzioni che il C++ chiama davvero; scrivere qui il nome di un effetto che nessuno
	// ha ancora scritto renderebbe la tabella una lista di intenzioni, e il gate smetterebbe di misurare
	// qualcosa. L'unica frase vera oggi e' che questo evento non si mostra.
	//
	// ⚠️ **Questa voce va RIVISTA, non ereditata**, appena la cue nasce: e' il segnaposto che il gate
	// sorveglia, ed e' il motivo per cui il dato viene emesso prima del disegno e non insieme a lui.
	Out.Add(FRTPresentationBinding::MakeNoPresentation(ERTResolvedEventType::AttackFootprint,
		TEXT("Il dato esiste perche' la cue POSSA essere costruita: #1945 porta a valle le celle risolte, e ")
		TEXT("la resa dell'area e' fuori dal suo scope (E21). Nessuna cue oggi lo consuma. ATTENZIONE: a ")
		TEXT("differenza di HazardDamage questa assenza e' TEMPORANEA e attesa - la voce va rivista appena ")
		TEXT("la cue nasce, non ereditata.")));

	// Defeated — la morte visiva e' DIFFERITA: l'unita' sparisce dopo che il colpo o l'attraversamento e'
	// stato mostrato. La presentazione non decide quando si muore: lo decide il resolver, e questa cue lo
	// mostra dopo.
	//
	// ⚠️ **Le due funzioni non sono piu' chiamate insieme, e l'ordine e' il punto** (#2452, 2026-09-05).
	// `PlayDefeatMontage` parte a fine della fase in cui l'unita' e' caduta; `HideForDefeat` avviene una
	// volta sola, in `FinishPlayback`. Erano adiacenti, e l'hide precedeva il montaggio: `HideForDefeat`
	// chiama `SetActorHiddenInGame(true)`, che propaga alla skeletal, quindi `Death` partiva su un attore
	// gia' nascosto e non veniva mai disegnato.
	//
	// 🔑 La coppia resta dichiarata QUI perche' questa tabella nomina **cio' che l'evento mostra**, non il
	// punto del codice che lo chiama: entrambe le funzioni fanno ancora parte della presentazione di
	// `Defeated`. Separarle avrebbe reso l'evento parzialmente muto per il gate.
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::Defeated,
		{ FName(TEXT("HideForDefeat")), FName(TEXT("PlayDefeatMontage")) }));

	// ReactionResolved — NoPresentation, e l'assenza e' TEMPORANEA come quella di `AttackFootprint`.
	//
	// 🔴 **Questo evento nasce proprio perche' un giorno si mostri**, ed e' il caso piu' netto della
	// tabella: due voci PIE — `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE` — esistono per giudicare a schermo
	// cio' che senza una cue non si vede. `PIE-VIS-DEFLECT` lo dice per intero: senza il momento della
	// reazione *«resta la sola barra che scende poco»*, cioe' un attacco debole invece di una difesa
	// riuscita.
	//
	// ⚠️ **Il dato viene prima del disegno, deliberatamente** (#2191): la grammatica visiva della
	// reazione e' fuori dallo scope di quella issue, che lo dichiara. Dichiarare qui una cue inventata
	// renderebbe questa tabella una lista di intenzioni — la stessa ragione scritta per `AttackFootprint`.
	//
	// ✅ **Questo valore un PRODUTTORE ce l'ha**: `RunReactionPass` lo emette dove la reazione scatta, e
	// `Reactions.Counter.DealsDamageToAttacker` lo presidia — validato per mutazione. Quindi il gate non e'
	// verde su un evento muto: e' verde su un evento che accade e che nessuno disegna ancora.
	//
	// ⚠️ **Fino a `#2460` questa riga diceva «a differenza di `HazardDamage`», e quel confronto e' scaduto**:
	// da allora ogni valore dell'enum ha un produttore, e la tabella non ha piu' voci mute. Cio' che resta
	// vero e' la distinzione fra le tre assenze, ed e' un altro asse: `HazardDamage` non si disegna **per
	// scelta**, questo e `AttackFootprint` non si disegnano **ancora**.
	//
	// ⚠️ Voce da RIVEDERE, non da ereditare: appena la cue nasce, le due voci PIE diventano giudicabili.
	Out.Add(FRTPresentationBinding::MakeNoPresentation(ERTResolvedEventType::ReactionResolved,
		TEXT("Il momento della reazione esiste perche' la cue POSSA essere costruita: #2191 lo emette dove la ")
		TEXT("reazione scatta, e la grammatica visiva e' fuori dal suo scope. Nessuna cue oggi lo consuma. ")
		TEXT("ATTENZIONE: assenza TEMPORANEA e attesa - due voci PIE (VIS-DEFLECT, VIS-INTERPOSE) restano non ")
		TEXT("giudicabili finche' non nasce, e la voce va rivista allora, non ereditata.")));

	// StatusChanged — NoPresentation, e l'assenza e' TEMPORANEA come quella di `AttackFootprint` e
	// `ReactionResolved`.
	//
	// 🔴 **Il dato esiste perche' la cue POSSA essere costruita, ed e' il caso piu' documentato dei tre**:
	// [D-320] ha gia' deciso COME si mostrera' — un `UWidgetComponent` per unita', con le undici icone
	// `RT_UI_Icon_Status_*` gia' versionate — e questa voce va rivista quando quel widget nasce. Oggi
	// l'unico canale a schermo e' il ripiego testuale di `RTHUD.cpp`, che mostra **due** stati su undici.
	//
	// ⚠️ **Chi scrivera' la cue deve sapere due cose che il tipo dell'evento non dice da solo**:
	//  - il verso (nascita o morte) si chiede a `URTTurnLogLibrary::IsStatusBirth`, mai deducendolo a
	//    occhio dai dieci valori di `ERTStatusOutcome`;
	//  - `Status.Electrified` produce una nascita (`AppliedInstantly`) e **mai** una morte — e' inerte
	//    (`#1324`): un'icona persistente aperta su di lui resterebbe accesa per sempre.
	//
	// ⚠️ Voce da RIVEDERE, non da ereditare.
	Out.Add(FRTPresentationBinding::MakeNoPresentation(ERTResolvedEventType::StatusChanged,
		TEXT("Gli stati arrivano al playback perche' la cue POSSA essere costruita: #2245 li emette dove la ")
		TEXT("voce di TurnLog viene scritta, e il disegno e' D-320 (WidgetComponent + le undici icone gia' ")
		TEXT("versionate), che non esiste ancora. Nessuna cue oggi lo consuma. ATTENZIONE: assenza ")
		TEXT("TEMPORANEA e attesa - la voce va rivista quando il widget nasce, non ereditata. Chi la scrive ")
		TEXT("chieda il verso a IsStatusBirth, e sappia che Status.Electrified nasce e non muore mai.")));

	return Out;
}

int32 URTPresentationBindingLibrary::DeclaredEventTypeCount()
{
	const UEnum* Enum = StaticEnum<ERTResolvedEventType>();
	// `NumEnums() - 1`: l'ultimo e' il `_MAX` sintetico che UHT aggiunge, e non e' un valore scrivibile.
	// Senza reflection non esiste una risposta onesta: `0` e' l'unica che non finge una copertura.
	return Enum ? FMath::Max(0, Enum->NumEnums() - 1) : 0;
}

FString URTPresentationBindingLibrary::EventTypeName(ERTResolvedEventType Type)
{
	const UEnum* Enum = StaticEnum<ERTResolvedEventType>();
	return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Type))
	            : FString::Printf(TEXT("%d"), static_cast<int32>(Type));
}

TArray<FString> URTPresentationBindingLibrary::FindMissingBindings(
	const TArray<FRTPresentationBinding>& Bindings)
{
	TArray<FString> Missing;

	const UEnum* Enum = StaticEnum<ERTResolvedEventType>();
	if (!Enum)
	{
		// Senza reflection il gate non puo' sapere cosa coprire, e tacere sarebbe la risposta peggiore:
		// direbbe «tutto coperto» proprio quando non ha guardato niente.
		Missing.Add(TEXT("ERTResolvedEventType: reflection non disponibile, copertura non verificabile"));
		return Missing;
	}

	// Si itera l'ENUM, non le voci: e' cio' che rende coperto per costruzione un valore aggiunto domani.
	// L'ordine dell'uscita segue i valori dell'enum, quindi e' deterministico senza bisogno di ordinare.
	const int32 Count = Enum->NumEnums() - 1;
	for (int32 I = 0; I < Count; ++I)
	{
		const ERTResolvedEventType Type = static_cast<ERTResolvedEventType>(Enum->GetValueByIndex(I));
		const FString TypeName = Enum->GetNameStringByIndex(I);

		// Quante voci parlano di questo tipo. Zero e' una dimenticanza; piu' di una e' un'ambiguita', e
		// un'ambiguita' non e' una dichiarazione: quale delle due valga non lo decide chi legge.
		const FRTPresentationBinding* Found = nullptr;
		int32 Declared = 0;
		for (const FRTPresentationBinding& B : Bindings)
		{
			if (B.Type == Type)
			{
				++Declared;
				if (!Found) { Found = &B; }
			}
		}

		if (Declared == 0)
		{
			Missing.Add(FString::Printf(TEXT("%s: nessuna voce lo dichiara"), *TypeName));
			continue;
		}
		if (Declared > 1)
		{
			Missing.Add(FString::Printf(
				TEXT("%s: dichiarato %d volte, quale valga e' ambiguo"), *TypeName, Declared));
			continue;
		}

		if (Found->Kind == ERTPresentationKind::NoPresentation)
		{
			// `NoPresentation` senza motivo e' indistinguibile da una dimenticanza che ha imparato a tacere:
			// e' il default implicito che `D-278` vieta, scritto a mano.
			if (Found->Rationale.TrimStartAndEnd().IsEmpty())
			{
				Missing.Add(FString::Printf(
					TEXT("%s: NoPresentation dichiarato senza motivo scritto"), *TypeName));
			}
			continue;
		}

		// `Cues` senza cue valide: la voce esiste, e non copre nulla. `NAME_None` non conta — dichiarare una
		// chiave vuota e' il widget dichiarato e mai disegnato.
		int32 ValidCues = 0;
		for (const FName& Cue : Found->Cues)
		{
			if (!Cue.IsNone()) { ++ValidCues; }
		}
		if (ValidCues == 0)
		{
			Missing.Add(FString::Printf(
				TEXT("%s: dichiarato con cue, ma nessuna cue valida"), *TypeName));
		}
	}

	return Missing;
}
