#include "Turn/RTPresentationBinding.h"

TArray<FRTPresentationBinding> URTPresentationBindingLibrary::DeclaredBindings()
{
	TArray<FRTPresentationBinding> Out;

	// Move — il playback muove il cilindro e accende la locomozione. `bIsMovingVisually` e' cio' che
	// `URTUnitAnimInstance` legge per passare Idle<->Run (`RTTurnManager.cpp:6208`), e `SetVisualLocation`
	// posiziona all'inizio della prima anim perche' non ci sia un flash sulla cella finale.
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::Move,
		{ FName(TEXT("bIsMovingVisually")), FName(TEXT("SetVisualLocation")) }));

	// Attack — quattro cue e due soggetti: l'attaccante gioca il colpo, il bersaglio lo incassa
	// (`RTTurnManager.cpp`), e dal 2026-09-05 (`#2455`) il bersaglio mostra anche **quanto**.
	//
	// ⚠️ **Dal 2026-09-05 (`#2448`) la clip la sceglie il C++**: il `TurnManager` chiama
	// `PlayPresentationRole(Attack|Hit)`, che risolve dal CDO, suona sullo slot e POI notifica il
	// Blueprint passandogli cio' che ha suonato. `PlayAttackMontage` e `PlayHitMontage` restano i nomi
	// degli EVENTI Blueprint, che e' cio' che questa tabella dichiara: la cue vista da chi guarda non e'
	// cambiata.
	//
	// 🔴 **`ShowDamageToken` e `PulseHealthBar` invece sono C++ dalla A alla Z, e la differenza e' il punto
	// di `#2455`.** Cio' che decidono — la cifra, il segno, il caso `Amount <= 0`, quando la barra pulsa —
	// vive in funzioni pure che un test chiama senza costruire un widget
	// (`URTHudViewModel::BuildDamageToken`, `URTUnitOverlayWidget::FadeAlpha`). Un grafo Blueprint avrebbe
	// avuto copertura headless **zero**, ed e' la ragione che [D-320] punto (5) scrive per esteso.
	//
	// ⚠️ **Il numero mostrato e' `Amount`, che e' il danno NOMINALE e non gli HP persi**: per un `Attack`
	// vale `Hit.Power`, cioe' la potenza dell'intento meno la sola **copertura** — Deflect, Guard, Brace e
	// lo scudo agiscono a valle, su un altro array. Un colpo da 30 su un bersaglio in Brace con scudo mostra
	// `-30` mentre la barra scende di meno: e' la convenzione di `#2460`, scelta perche' i due canali
	// raccontino lo stesso colpo con lo stesso numero.
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::Attack,
		{ FName(TEXT("PlayAttackMontage")), FName(TEXT("PlayHitMontage")),
		  FName(TEXT("ShowDamageToken")), FName(TEXT("PulseHealthBar")) }));

	// HazardDamage — PendingPresentation dal 2026-09-05, owner `#2505`. Era `NoPresentation` dal 2026-08-31.
	//
	// 🔴 **Questa voce ha dichiarato tre cose diverse in cinque giorni, e ogni passaggio e' stato una
	// misura.** Il 2026-08-31 era `NoPresentation` con la clausola *«oggi il valore non ha un produttore …
	// va rivista appena ne acquista uno»*. Il 2026-09-05 `#2460` gliene ha dato uno — l'evento nasce in
	// `ARTTurnManager::AppendLogEntry`, per entrambe le cause che `URTTurnLogLibrary::IsEnvironmentalDamage`
	// riconosce — e la voce e' rimasta `NoPresentation` con un motivo riscritto: *«la cue e' lavoro di
	// #2455»*. `#2483` ha poi reso il verso dell'assenza un **dato**, e ha lasciato scritta la
	// contraddizione che ne risultava: la stessa stringa diceva sia *«e' una scelta sul disegno»* (decisa)
	// sia *«la cue e' lavoro di #2455»* (in attesa).
	//
	// ✅ **`#2455` e' l'owner di quella revisione, e la scioglie cosi': l'assenza e' IN ATTESA.** Non e' una
	// scelta di design contro il disegno — e' che **non c'e' un istante in cui giocare la cue**. Misurato:
	// `BeginPlayback` consuma `Move`, `Attack` e `Defeated` e nient'altro; `PlaybackPhases` si compone di
	// `Prep · Dash · Blast · Move` e **non contiene mai `Cleanup`**, che e' dove nasce meta' del danno da
	// fuoco (`Status.Burning`, 8, contro i 10 di `Terrain.Fire` all'ingresso); e un turno di solo danno
	// ambientale ha `PlaybackPhases.Num() == 0` e conclude subito.
	//
	// 🔁 **La voce e' `PendingPresentation`, e la decisione e' d'autore (2026-09-05).** `#2460` l'aveva
	// lasciata `NoPresentation` scrivendo *«la scelta e' deliberata»*, ma nella stessa frase diceva anche
	// *«la cue e' lavoro di #2455»* — e sotto la tassonomia che `#2483` ha introdotto poche ore dopo, le due
	// clausole si contraddicono: la prima e' un'assenza decisa, la seconda e' un'attesa con un owner. La
	// domanda e' stata posta all'autore invece di essere risolta a intuito, e la risposta e' **in attesa**.
	//
	// ⛔ **Non e' una promozione a "disegnato".** `PendingPresentation` dichiara che l'evento **non si
	// mostra** — esattamente come `NoPresentation` — e aggiunge chi lo mostrera'. Il timore scritto qui da
	// `#2460`, *«dichiarerebbe disegnato un evento che nessuno disegna»*, riguardava il passaggio a `Cues`,
	// che non e' questo: `FindMissingBindings` continua a non chiedere nessuna cue per questa voce.
	//
	// 🔑 **L'owner e' passato da `#2455` a `#2505`, e il passaggio e' esattamente cio' che questa riga
	// prevedeva.** La decisione d'autore aveva nominato `#2455` perche' era la issue che stava per
	// disegnare il feedback di combattimento; misurando, `#2455` ha scoperto che questo evento **non ha un
	// istante** in cui una cue possa essere giocata — `BeginPlayback` consuma solo `Move`, `Attack` e
	// `Defeated`, e `PlaybackPhases` non contiene **mai** `Cleanup`, dove nasce meta' del danno da fuoco
	// (`Status.Burning`, 8, contro i 10 di `Terrain.Fire` all'ingresso). `#2455` ha quindi consegnato il
	// token per `Attack`, che un beat ce l'ha, e ha aperto `#2505` per costruire quello che manca.
	//
	// ⚠️ **Il meccanismo ha funzionato come scritto**: la nota precedente diceva *«il giorno in cui #2455
	// chiude senza rivedere questa riga, `Presentation.AbsenceCensusIsPinned` diventa rosso»*. E' andata
	// cosi' — il test ha convocato la revisione, e la revisione ha aggiornato l'owner invece di lasciarlo
	// puntare a una issue chiusa. E' l'intero scopo di `PendingOwner` (`#2483`).
	//
	// ⚠️ **Cade insieme la vecchia motivazione** *«una cue dedicata non aggiunge leggibilita' in v0.1»*:
	// `#2455` costruisce esattamente quel token per `Attack`, quindi non e' piu' una posizione del progetto.
	// Resta vera [D-124], che tiene il **sistema VFX degli status** fuori dal perimetro — ma un testo
	// fluttuante non e' un VFX di status, e l'epic `#2453` lo elenca fra le primitive ammesse in v0.1.
	//
	// ⚠️ **Corretta insieme una frase che era falsa**: diceva *«quando uccide, l'evento emesso e' `Defeated`,
	// che una presentazione ce l'ha»*. Misurato: `Defeated` lo emette **solo** `ResolveCombatPasses`, da
	// `NewlyDefeated` calcolato sul Blast. Chi muore bruciato nel Cleanup, o entrando nel fuoco, **non lo
	// produce**: resta coperto dal catch-all di `ConcludeTurn` (`DestroyDefeatedUnits`). Chi costruira' la
	// cue non deve credere che la morte da hazard abbia gia' un beat proprio — non ce l'ha.
	Out.Add(FRTPresentationBinding::MakePendingPresentation(ERTResolvedEventType::HazardDamage,
		TEXT("Il produttore ESISTE da #2460 (AppendLogEntry, per ogni causa che IsEnvironmentalDamage ")
		TEXT("riconosce): l'evento accade e non e' muto. Cio' che manca e' l'ISTANTE in cui giocare una cue: ")
		TEXT("BeginPlayback non consuma questo tipo, e PlaybackPhases non contiene mai Cleanup, dove nasce ")
		TEXT("meta' del danno da fuoco. Costruire quel beat e' #2505; il token per Attack, che un beat ce ")
		TEXT("l'ha, e' gia' stato consegnato da #2455. ATTENZIONE: la morte da hazard non emette Defeated - ")
		TEXT("la nasconde il catch-all di ConcludeTurn, quindi chi scrive la cue non dia per scontato un ")
		TEXT("beat di morte che non esiste."),
		TEXT("#2505")));

	// AttackFootprint — PendingPresentation.
	//
	// ✅ **Dal 2026-09-05 (#2483) la differenza sta nel DATO, non in questo commento.** Fino ad allora
	// era `NoPresentation` con la nota dentro il motivo, e nessuna macchina leggeva quella frase.
	//
	// 🔴 **Qui l'assenza e' temporanea e attesa.** ⚠️ Fino al 2026-09-05 questa riga contrapponeva il caso a
	// `HazardDamage` — *«quello non si mostra perche' si e' deciso che non deve»* — e il contrasto **non
	// regge piu'**: la decisione d'autore ha portato anche `HazardDamage` in attesa, con owner `#2455`.
	// Questo evento esiste **precisamente perche' un giorno si mostri** —
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
	// 🔑 L'owner che la sciogliera' e' ora un CAMPO (`PendingOwner`), non una frase: `E21`.
	Out.Add(FRTPresentationBinding::MakePendingPresentation(ERTResolvedEventType::AttackFootprint,
		TEXT("Il dato esiste perche' la cue POSSA essere costruita: #1945 porta a valle le celle risolte, e ")
		TEXT("la resa dell'area e' fuori dal suo scope (E21). Nessuna cue oggi lo consuma."),
		TEXT("E21")));

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

	// ReactionResolved — PendingPresentation, come `AttackFootprint`: l'assenza e' TEMPORANEA.
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
	Out.Add(FRTPresentationBinding::MakePendingPresentation(ERTResolvedEventType::ReactionResolved,
		TEXT("Il momento della reazione esiste perche' la cue POSSA essere costruita: #2191 lo emette dove la ")
		TEXT("reazione scatta, e la grammatica visiva e' fuori dal suo scope. Nessuna cue oggi lo consuma. ")
		TEXT("Due voci PIE (VIS-DEFLECT, VIS-INTERPOSE) restano non giudicabili finche' non nasce."),
		TEXT("#2454")));

	// StatusChanged — PendingPresentation, e l'assenza e' TEMPORANEA come quella di `AttackFootprint` e
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
	Out.Add(FRTPresentationBinding::MakePendingPresentation(ERTResolvedEventType::StatusChanged,
		TEXT("Gli stati arrivano al playback perche' la cue POSSA essere costruita: #2245 li emette dove la ")
		TEXT("voce di TurnLog viene scritta, e il disegno e' D-320 (WidgetComponent + le undici icone gia' ")
		TEXT("versionate), che non esiste ancora. Nessuna cue oggi lo consuma. Chi la scrive chieda il verso ")
		TEXT("a IsStatusBirth, e sappia che Status.Electrified nasce e non muore mai."),
		TEXT("#2456")));

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

		if (Found->Kind == ERTPresentationKind::PendingPresentation)
		{
			// ✅ Una voce «in attesa» BEN FORMATA **copre** (`#2483`), e non e' una concessione: se il gate
			// andasse rosso su uno stato legittimo, la pressione sarebbe a cancellare la distinzione per
			// tornare verdi — e il gate tornerebbe a misurare niente. Rosso solo se e' MAL formata.
			if (Found->Rationale.TrimStartAndEnd().IsEmpty())
			{
				Missing.Add(FString::Printf(
					TEXT("%s: PendingPresentation dichiarato senza motivo scritto"), *TypeName));
				continue;
			}
			// 🔑 Senza owner, «in attesa» e' una promessa che nessuno puo' riscuotere: non si puo' chiedere
			// *«quell'owner e' ancora aperto?»*, che e' l'unica domanda per cui questo stato esiste.
			if (Found->PendingOwner.TrimStartAndEnd().IsEmpty())
			{
				Missing.Add(FString::Printf(
					TEXT("%s: PendingPresentation dichiarato senza owner che la sciolga"), *TypeName));
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
