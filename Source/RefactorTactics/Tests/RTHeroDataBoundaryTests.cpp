// Il confine `URTHeroData` -> `ARTUnit`: ogni campo del dato d'eroe arriva sull'unita', oppure e'
// esplicitamente esentato con un motivo.
//
// Nasce da #715. `ARTUnit` e' una COPIA PER VALORE di `URTHeroData`: `ConfigureFromHeroData` legge i campi
// che gli servono e scarta il puntatore. Un confine cosi' non si accorge di quello che perde, e infatti ne
// aveva persi due su dieci — `DisplayName` (trovato consolidando D-120: a schermo si leggeva il nome legacy
// mentre ogni documento diceva quello canonico) e `HearingThreshold` (trovato cercando il primo: dichiarato,
// popolato per eroe e validato dal catalogo, ma inarrivabile da una partita, quindi
// `URTAcousticPropagationLibrary::IsAudible` non era alimentabile e Lane B era bloccata senza saperlo).
//
// Il test verifica sul TIPO, non su un'istanza: un campo nuovo aggiunto domani a `URTHeroData` diventa rosso
// il giorno stesso, che e' l'unico momento in cui la discussione e' ancora economica. Stessa forma di
// `RefactorTactics.Equipment.NoInMatchProgression`.

#include "Misc/AutomationTest.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Unit/RTUnit.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Come il valore dell'eroe deve arrivare a destinazione. */
	enum class ERTTransport : uint8
	{
		/** Identico: il campo dell'unita' vale esattamente quello dell'eroe. */
		Exact,
		/** Prefisso: l'unita' contiene il valore dell'eroe all'inizio, e vi aggiunge del suo. */
		Prefix,
	};

	/** Dove un campo di `URTHeroData` deve arrivare sull'unita'. `UnitField == nullptr` = esentato. */
	struct FRTHeroFieldRoute
	{
		const TCHAR* HeroField;
		const TCHAR* UnitField;
		ERTTransport Transport;
		const TCHAR* ExemptionReason;
	};

	// ⚠️ Questa tabella e' la SPECIFICA del confine, non una fotografia del codice: dichiara dove ogni campo
	// DEVE arrivare. Il test la confronta con la realta', quindi e' rosso in due casi distinti — un campo
	// dell'eroe che non compare qui (nessuno ha deciso che farne) e un campo dichiarato qui che il codice non
	// trasporta davvero.
	//
	// Per esentare un campo servono due cose: `UnitField = nullptr` e un motivo scritto. ⚠️ Il test verifica
	// solo che il motivo NON SIA VUOTO, non che sia buono: `TEXT("x")` lo soddisfa. L'esenzione e' quindi
	// un'ammissione visibile nel diff, non una barriera — chi rivede la PR e' l'unico che puo' respingerla.
	// Detto altrimenti: questa riga rende il silenziamento *esplicito*, non *difficile*.
	const FRTHeroFieldRoute Routes[] = {
		{ TEXT("HeroId"),           TEXT("HeroId"),           ERTTransport::Exact,  nullptr },
		{ TEXT("DisplayName"),      TEXT("HeroDisplayName"),  ERTTransport::Exact,  nullptr },
		{ TEXT("MaxHealth"),        TEXT("MaxHealth"),        ERTTransport::Exact,  nullptr },
		{ TEXT("MovePoints"),       TEXT("MoveRange"),        ERTTransport::Exact,  nullptr },
		{ TEXT("VisionRange"),      TEXT("VisionRange"),      ERTTransport::Exact,  nullptr },
		{ TEXT("HearingThreshold"), TEXT("HearingThreshold"), ERTTransport::Exact,  nullptr },
		{ TEXT("PushResistance"),   TEXT("PushResistance"),   ERTTransport::Exact,  nullptr },
		{ TEXT("Affinity"),         TEXT("Affinity"),         ERTTransport::Exact,  nullptr },
		{ TEXT("Weakness"),         TEXT("Weakness"),         ERTTransport::Exact,  nullptr },
		// E14.7 [D-047]. Trasportato e non dedotto dall'`HeroId`, che pure l'unita' porta: il resolver legge
		// l'unita', e farlo risalire all'eroe darebbe a ogni sito che valuta una finestra una dipendenza su
		// `URTHeroCatalogLibrary`. `NAME_None` e' legittimo — e' Riktor, cioe' profilo base.
		{ TEXT("ReactionProfileId"), TEXT("ReactionProfileId"), ERTTransport::Exact, nullptr },
		// `Prefix` e non `Exact`, ed e' una regola di dominio: `ConfigureFromHeroData` copia le azioni
		// dell'eroe e **accoda** le sette azioni generiche di D-025, che sono dell'unita' quanto le sue.
		// L'ordine non e' cosmetico — l'attacco base resta l'indice 0, e `PlannedAbilityIndex` e' un indice,
		// non un ID: accodare in testa sposterebbe in silenzio ogni piano gia' scritto, bot compreso.
		{ TEXT("Actions"),          TEXT("Abilities"),        ERTTransport::Prefix, nullptr },
	};

	const FRTHeroFieldRoute* FindRoute(const FString& HeroField)
	{
		for (const FRTHeroFieldRoute& R : Routes)
		{
			if (HeroField == R.HeroField)
			{
				return &R;
			}
		}
		return nullptr;
	}

	/**
	 * Valore di una proprieta' come testo, SOLO per il messaggio d'errore.
	 *
	 * 🔴 **Non usarlo per decidere se due valori sono uguali.** Con `Delta = nullptr`,
	 * `ExportText_InContainer` non scrive niente quando il valore coincide con il default del TIPO
	 * (`ExportText_Direct` salta l'export se `Identical(Data, nullptr)`): un `int32` a `0` e un `FName` a
	 * `NAME_None` esportano stringa vuota. Confrontare quelle stringhe fa passare qualunque campo il cui
	 * valore di catalogo sia il default — che e' esattamente la classe di difetto che questo test esiste per
	 * scoprire. Preso in code review dopo che il gate era rimasto VERDE con il trasporto di
	 * `PushResistance` (0 su Phase, come il default di `ARTUnit`) rimosso.
	 */
	FString ValueAsTextForMessage(const FProperty* Prop, const void* Container)
	{
		FString Out;
		Prop->ExportText_InContainer(0, Out, Container, nullptr, nullptr, PPF_None);
		return Out.IsEmpty() ? FString(TEXT("<default del tipo>")) : Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroDataBoundaryTest,
	"RefactorTactics.Unit.HeroDataCrossesTheBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroDataBoundaryTest::RunTest(const FString&)
{
	// --- 1. Ogni campo dell'eroe e' classificato -----------------------------------------------------
	// Il caso che questo blocco protegge non e' quello di oggi: e' l'undicesimo campo, aggiunto fra un mese
	// da chi non sa che questo confine esiste.
	// `ExcludeSuper`: il confine riguarda i campi che questo progetto dichiara, non la meccanica di
	// `UDataAsset` (`AssetBundleData` e simili), che non ha niente da fare su un'unita' e che comparirebbe
	// come falso allarme a ogni aggiornamento dell'engine.
	int32 HeroFieldCount = 0;
	for (TFieldIterator<FProperty> It(URTHeroData::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		++HeroFieldCount;
		const FString Nome = It->GetName();
		const FRTHeroFieldRoute* Route = FindRoute(Nome);

		if (Route == nullptr)
		{
			AddError(FString::Printf(
				TEXT("URTHeroData::%s non e' classificato: dichiara dove arriva sull'unita' oppure esentalo ")
				TEXT("con un motivo, in Routes[] di questo test."), *Nome));
			continue;
		}

		if (Route->UnitField == nullptr)
		{
			TestTrue(*FString::Printf(TEXT("l'esenzione di %s dichiara un motivo"), *Nome),
				Route->ExemptionReason != nullptr && FCString::Strlen(Route->ExemptionReason) > 0);
		}
	}

	// La tabella non deve descrivere campi che non esistono piu': un instradamento orfano e' una regola che
	// nessuno applica, e nasconde il fatto che il campo e' stato rimosso o rinominato.
	for (const FRTHeroFieldRoute& R : Routes)
	{
		TestNotNull(*FString::Printf(TEXT("Routes[] cita URTHeroData::%s, che deve esistere"), R.HeroField),
			URTHeroData::StaticClass()->FindPropertyByName(FName(R.HeroField)));
	}

	TestEqual(TEXT("ogni campo di URTHeroData ha un instradamento"),
		HeroFieldCount, static_cast<int32>(UE_ARRAY_COUNT(Routes)));

	// --- 2. Il campo di destinazione esiste sull'unita' -----------------------------------------------
	for (const FRTHeroFieldRoute& R : Routes)
	{
		if (R.UnitField == nullptr)
		{
			continue;
		}
		TestNotNull(*FString::Printf(
			TEXT("ARTUnit::%s deve esistere per ricevere URTHeroData::%s"), R.UnitField, R.HeroField),
			ARTUnit::StaticClass()->FindPropertyByName(FName(R.UnitField)));
	}

	// --- 3. Il valore arriva davvero ------------------------------------------------------------------
	// Il punto 2 da solo direbbe che il campo esiste, non che qualcuno lo riempie: e' la differenza fra una
	// dichiarazione e un produttore, ed e' il difetto che questo repository ha gia' pagato piu' volte.
	//
	// 🔴 **L'eroe di prova e' SINTETICO, e non e' un vezzo.** La prima versione usava `MakePhase()`, e il
	// gate restava VERDE anche togliendo il trasporto di `PushResistance`: il valore di Phase e' `0`, che e'
	// **identico al default di `ARTUnit`**, quindi «copiato 0» e «mai copiato, default 0» sono lo stesso
	// stato e nessun confronto puo' distinguerli. Preso in code review, e il rimedio proposto — cambiare il
	// modo di confrontare i valori — **non bastava**: il difetto non era il confronto, era la fixture.
	//
	// Qui ogni campo vale qualcosa che NESSUN default dell'unita' produce (`MaxHealth` 100, `MoveRange` 4,
	// `VisionRange` 5, `HearingThreshold` 5, `PushResistance` 0, i due `FName` a `NAME_None`), quindi un
	// campo non trasportato resta al default e la differenza si vede. Il test verifica il MECCANISMO del
	// confine; che i numeri di catalogo siano quelli giusti e' un'altra domanda, e ha altri test.
	URTHeroData* Hero = NewObject<URTHeroData>();
	if (!TestNotNull(TEXT("l'eroe di prova esiste"), Hero))
	{
		return false;
	}
	Hero->HeroId = TEXT("Hero.Fixture");
	Hero->DisplayName = FText::FromString(TEXT("Fixture"));
	Hero->MaxHealth = 137;
	Hero->MovePoints = 9;
	Hero->VisionRange = 11;
	Hero->HearingThreshold = 7;
	Hero->PushResistance = 3;
	Hero->Affinity = TEXT("Fixture.Affinita");
	Hero->Weakness = TEXT("Fixture.Debolezza");
	// Almeno un'azione: serve al ramo `Prefix`, che pretende le azioni dell'eroe PIU' le generiche.
	Hero->Actions = URTHeroCatalogLibrary::MakeGadget()->Actions;

	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("l'unita' di prova esiste"), Unit))
	{
		return false;
	}
	Unit->ConfigureFromHeroData(Hero);

	for (const FRTHeroFieldRoute& R : Routes)
	{
		if (R.UnitField == nullptr)
		{
			continue;
		}

		const FProperty* HeroProp = URTHeroData::StaticClass()->FindPropertyByName(FName(R.HeroField));
		const FProperty* UnitProp = ARTUnit::StaticClass()->FindPropertyByName(FName(R.UnitField));
		if (HeroProp == nullptr || UnitProp == nullptr)
		{
			// Gia' segnalato dai punti 1/2: non si conta due volte lo stesso difetto, altrimenti il numero
			// di errori smette di dire quanti campi sono rotti.
			continue;
		}

		if (R.Transport == ERTTransport::Prefix)
		{
			// Confronto strutturale, non testuale: il testo esportato di una TArray non si presta a un
			// «inizia per», e il punto qui e' che i primi N elementi siano gli STESSI oggetti.
			const int32 Attesi = Hero->Actions.Num();
			// `>` e non `>=`: `Prefix` significa «le azioni dell'eroe PIU' qualcosa dell'unita'», e quel
			// qualcosa sono le azioni generiche di D-025. Con `>=` un `MakeGenericActions` che smettesse di
			// accodare lascerebbe il test verde, e il nome del trasporto direbbe il falso.
			if (!TestTrue(*FString::Printf(
					TEXT("ARTUnit::%s contiene le %d azioni dell'eroe E le generiche accodate"),
					R.UnitField, Attesi),
					Unit->Abilities.Num() > Attesi))
			{
				continue;
			}
			for (int32 i = 0; i < Attesi; ++i)
			{
				TestEqual(*FString::Printf(
					TEXT("ARTUnit::%s[%d] e' l'azione %d dell'eroe, nello stesso ordine"), R.UnitField, i, i),
					Unit->Abilities[i].Get(), Hero->Actions[i].Get());
			}
			continue;
		}

		// Il tipo prima del valore: un instradamento che mappa `int32` su `FName` non e' un valore sbagliato,
		// e' una tabella sbagliata — e confrontare due layout diversi non avrebbe senso.
		if (!TestTrue(*FString::Printf(
				TEXT("URTHeroData::%s e ARTUnit::%s hanno lo stesso tipo"), R.HeroField, R.UnitField),
				HeroProp->SameType(UnitProp)))
		{
			continue;
		}

		// `Identical` con entrambi i puntatori: confronta i due VALORI. Vedi il commento di
		// `ValueAsTextForMessage` sul perche' il confronto per testo esportato era vacuo.
		const void* HeroValue = HeroProp->ContainerPtrToValuePtr<void>(Hero);
		const void* UnitValue = UnitProp->ContainerPtrToValuePtr<void>(Unit);
		TestTrue(*FString::Printf(
			TEXT("URTHeroData::%s arriva su ARTUnit::%s (atteso %s, trovato %s)"),
			R.HeroField, R.UnitField,
			*ValueAsTextForMessage(HeroProp, Hero), *ValueAsTextForMessage(UnitProp, Unit)),
			HeroProp->Identical(HeroValue, UnitValue, PPF_None));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
