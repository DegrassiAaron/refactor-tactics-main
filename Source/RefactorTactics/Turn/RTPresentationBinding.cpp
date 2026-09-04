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
	// (`#1067`). Non si mostra perche' ha gia' due canali visibili: la barra vita, e il combat log. E quando
	// uccide, l'evento emesso e' `Defeated`, che una presentazione ce l'ha: si vede morire, non si vede
	// bruciare.
	//
	// ⚠️ **Ma oggi questo valore non ha un PRODUTTORE**: il TurnLog registra il danno, la `ResolvedTimeline`
	// non riceve mai l'evento. Questa voce vale «quando accadra'», non «perche' non accade», e va **rivista**
	// il giorno in cui qualcuno lo emette — altrimenti il gate resta verde su un evento muto, cioe'
	// esattamente il difetto che questa tabella esiste per impedire. A questo stesso fatto e' gia' successo
	// una volta sul canale della traccia (`#1067`).
	Out.Add(FRTPresentationBinding::MakeNoPresentation(ERTResolvedEventType::HazardDamage,
		TEXT("Il danno da terreno si legge dalla barra vita e dal combat log, e quando uccide emette Defeated ")
		TEXT("(che ha presentazione): una cue dedicata non aggiunge leggibilita' in v0.1, e D-124 tiene il ")
		TEXT("sistema VFX degli status fuori dal perimetro. ATTENZIONE: oggi il valore non ha un produttore ")
		TEXT("(nessuno emette l'evento) - questa voce vale «quando accadra'» e va rivista appena ne acquista uno.")));

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
	// stato mostrato (`RTTurnManager.cpp:6293-6300`). La presentazione non decide quando si muore: lo decide
	// il resolver, e questa cue lo mostra dopo.
	Out.Add(FRTPresentationBinding(ERTResolvedEventType::Defeated,
		{ FName(TEXT("HideForDefeat")), FName(TEXT("PlayDefeatMontage")) }));

	// ReactionResolved — NoPresentation, e per la stessa ragione di `AttackFootprint`: l'assenza e'
	// TEMPORANEA e attesa, non una scelta di design.
	//
	// 🔴 **L'evento esiste precisamente perche' un giorno si mostri.** `#2191` lo introduce per dare un
	// momento alla reazione risolta, e le due voci che ne dipendono — `PIE-VIS-DEFLECT` e
	// `PIE-VIS-INTERPOSE` — chiedono che quel momento si VEDA. La cue che lo disegna e' presentazione, e la
	// issue la mette fuori scope apposta: si decide quando l'evento esiste, non prima.
	//
	// ⚠️ **Dichiarare una cue inventata sarebbe peggio che dichiarare l'assenza**, e vale qui come per
	// `AttackFootprint`: le altre voci nominano funzioni che il C++ chiama davvero.
	//
	// ⚠️ **Questa voce va RIVISTA appena la cue nasce**, non ereditata. Finche' resta, le due voci PIE
	// restano ⛔: l'evento c'e', il momento a schermo no.
	Out.Add(FRTPresentationBinding::MakeNoPresentation(ERTResolvedEventType::ReactionResolved,
		TEXT("Il momento esiste perche' la cue POSSA essere costruita: #2191 porta a valle il fatto che una ")
		TEXT("reazione ha agito, e la sua resa e' fuori dal suo scope. Nessuna cue oggi lo consuma. ")
		TEXT("ATTENZIONE: come AttackFootprint questa assenza e' TEMPORANEA e attesa - la voce va rivista ")
		TEXT("appena la cue nasce, e con lei le due voci PIE che aspettano di vedere quel momento.")));

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
