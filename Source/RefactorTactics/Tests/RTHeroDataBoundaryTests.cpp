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
	// Per esentare un campo servono due cose: `UnitField = nullptr` e un motivo scritto. Il motivo non e'
	// decorazione — e' cio' che impedisce di esentare un campo per farlo smettere di essere rosso.
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

	/** Valore di una proprieta' come testo: unico confronto che regge int32, FName, FText e TArray insieme. */
	FString ValueAsText(const FProperty* Prop, const void* Container)
	{
		FString Out;
		Prop->ExportText_InContainer(0, Out, Container, nullptr, nullptr, PPF_None);
		return Out;
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
	// L'eroe scelto e' quello con i valori piu' distinguibili dai default di `ARTUnit`: la soglia d'udito di
	// Phase e' 3 contro il default 5, quindi un campo NON copiato resta 5 e si vede.
	const URTHeroData* Hero = URTHeroCatalogLibrary::MakeRiva();
	if (!TestNotNull(TEXT("il catalogo produce l'eroe di prova"), Hero))
	{
		return false;
	}

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
			if (!TestTrue(*FString::Printf(
					TEXT("ARTUnit::%s contiene almeno le %d azioni dell'eroe"), R.UnitField, Attesi),
					Unit->Abilities.Num() >= Attesi))
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

		const FString Atteso = ValueAsText(HeroProp, Hero);
		const FString Trovato = ValueAsText(UnitProp, Unit);
		TestEqual(*FString::Printf(
			TEXT("URTHeroData::%s arriva su ARTUnit::%s"), R.HeroField, R.UnitField), Trovato, Atteso);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
