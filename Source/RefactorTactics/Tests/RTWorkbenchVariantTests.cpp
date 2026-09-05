// La VARIANTE SPERIMENTALE dello Skill Workbench: il dato che dice «questa abilita', ma con questo numero».
//
// Due proprieta' reggono l'intero contratto, e sono diverse fra loro:
//   1. l'override raggiunge ENTRAMBE le case in cui quel parametro vive, perche' consumatori diversi ne
//      leggono di diverse (`#1953`);
//   2. l'override non sopravvive alla run in cui e' stato applicato.
//
// La seconda e' quella che il kit originale chiedeva di verificare nel posto sbagliato — «non tocca il
// production asset» — e che qui si verifica dove il rischio e' davvero.

#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTActionReadout.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTWorkbenchVariant.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTVariante
{
	/** Il kit di Gadget come lista cruda: `Apply` lavora sulle istanze, non sull'eroe. */
	TArray<URTActionData*> KitDiGadget()
	{
		TArray<URTActionData*> Kit;
		const URTHeroData* Gadget = URTHeroCatalogLibrary::MakeGadget();
		if (Gadget == nullptr) { return Kit; }
		for (const TObjectPtr<URTActionData>& Voce : Gadget->Actions)
		{
			if (URTActionData* A = Voce) { Kit.Add(A); }
		}
		return Kit;
	}

	URTActionData* Azione(const TArray<URTActionData*>& Kit, const TCHAR* Id)
	{
		for (URTActionData* A : Kit)
		{
			if (A && A->Def.ActionId == FName(Id)) { return A; }
		}
		return nullptr;
	}

	/** Il danno dichiarato: il primo effetto `Damage`, `INDEX_NONE` se non ce n'e'. */
	int32 DannoDichiarato(const FRTActionDef& Def)
	{
		for (const FRTActionEffectSpec& S : Def.Effects)
		{
			if (S.Effect == ERTActionEffect::Damage) { return S.Amount; }
		}
		return INDEX_NONE;
	}

	FRTAbilityParameterOverride Ov(const TCHAR* Id, const FName& Chiave, int32 Valore)
	{
		FRTAbilityParameterOverride O;
		O.ActionId = FName(Id);
		O.ParameterKey = Chiave;
		O.Value = Valore;
		return O;
	}
}

/**
 * **L'override raggiunge entrambe le case**, per tutti e tre i parametri.
 *
 * Non e' pedanteria di simmetria: `#1953` ha misurato che lo stesso parametro e' letto da consumatori
 * diversi in case diverse — il bot su `Ability->RangeCells`, `ConsumeAbility` su
 * `URTActionData::CooldownTurns`, il resolver sugli effetti del catalogo. Una variante che ne scrivesse una
 * sola produrrebbe il *pulsante finto* di [D-090]: un attacco che il bot pianifica e il resolver rifiuta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantOverrideReachesBothHomesTest,
	"RefactorTactics.Workbench.OverrideReachesBothHomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantOverrideReachesBothHomesTest::RunTest(const FString&)
{
	TArray<URTActionData*> Kit = RTVariante::KitDiGadget();
	if (!TestTrue(TEXT("il kit di Gadget non e' vuoto"), Kit.Num() > 0)) { return false; }

	URTActionData* ArcPulse = RTVariante::Azione(Kit, TEXT("Hero.Gadget.ArcPulse"));
	URTActionData* Overload = RTVariante::Azione(Kit, TEXT("Hero.Gadget.Overload"));
	if (!TestNotNull(TEXT("ArcPulse c'e'"), ArcPulse)) { return false; }
	if (!TestNotNull(TEXT("Overload c'e'"), Overload)) { return false; }

	// Anti-vacuita': i valori nuovi devono differire da quelli di partenza, o il confronto dopo l'`Apply`
	// sarebbe vero anche senza aver scritto niente.
	const int32 PortataPrima = ArcPulse->Def.RangeCells;
	const int32 DannoPrima = RTVariante::DannoDichiarato(ArcPulse->Def);
	const int32 RicaricaPrima = Overload->Def.CooldownTurns;
	if (!TestTrue(TEXT("i valori di partenza sono noti"), DannoPrima != INDEX_NONE)) { return false; }

	FRTWorkbenchVariant Variante;
	Variante.VariantId = TEXT("Test.TreParametri");
	Variante.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::RangeCells(), PortataPrima + 1));
	Variante.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::Damage(), DannoPrima + 5));
	Variante.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.Overload"),
		RTActionParameterKeys::CooldownTurns(), RicaricaPrima + 1));

	FRTWorkbenchVariant Ripristino;
	if (!TestEqual(TEXT("l'applicazione riesce"),
		URTWorkbenchVariantLibrary::Apply(Variante, Kit, Ripristino), ERTVariantApplyResult::Ok))
	{
		return false;
	}

	// PORTATA — la casa del catalogo e quella che il bot legge.
	TestEqual(TEXT("portata: il Def porta il valore nuovo"), ArcPulse->Def.RangeCells, PortataPrima + 1);
	TestEqual(TEXT("portata: lo specchio che il BOT legge porta lo stesso valore"),
		ArcPulse->RangeCells, PortataPrima + 1);

	// RICARICA — lo specchio qui non e' un di piu': `ConsumeAbility` legge solo quello.
	TestEqual(TEXT("ricarica: il Def porta il valore nuovo"), Overload->Def.CooldownTurns, RicaricaPrima + 1);
	TestEqual(TEXT("ricarica: lo specchio che ConsumeAbility legge porta lo stesso valore"),
		Overload->CooldownTurns, RicaricaPrima + 1);

	// DANNO — l'effetto del catalogo e la proiezione dello specchio.
	TestEqual(TEXT("danno: l'effetto del catalogo porta il valore nuovo"),
		RTVariante::DannoDichiarato(ArcPulse->Def), DannoPrima + 5);
	TestEqual(TEXT("danno: lo specchio Power porta lo stesso valore"), ArcPulse->Power, DannoPrima + 5);

	return true;
}

/**
 * **Il ripristino riporta ai valori canonici**, e l'inverso lo produce chi ha scritto.
 *
 * `Apply` restituisce la variante INVERSA invece di appoggiarsi a uno snapshot separato: i valori
 * precedenti li conosce solo chi li sta sovrascrivendo, e uno snapshot preso altrove sarebbe una seconda
 * verita' che puo' divergere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantResetRestoresCanonicalValuesTest,
	"RefactorTactics.Workbench.ResetRestoresCanonicalValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantResetRestoresCanonicalValuesTest::RunTest(const FString&)
{
	TArray<URTActionData*> Kit = RTVariante::KitDiGadget();
	URTActionData* ArcPulse = RTVariante::Azione(Kit, TEXT("Hero.Gadget.ArcPulse"));
	if (!TestNotNull(TEXT("ArcPulse c'e'"), ArcPulse)) { return false; }

	const int32 PortataCanonica = ArcPulse->Def.RangeCells;
	const int32 DannoCanonico = RTVariante::DannoDichiarato(ArcPulse->Def);

	FRTWorkbenchVariant Variante;
	Variante.VariantId = TEXT("Test.DaAnnullare");
	Variante.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::RangeCells(), PortataCanonica + 3));
	Variante.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::Damage(), DannoCanonico + 9));

	FRTWorkbenchVariant Ripristino;
	URTWorkbenchVariantLibrary::Apply(Variante, Kit, Ripristino);

	// Anti-vacuita': se l'applicazione non avesse scritto, il ripristino sarebbe verde a vuoto.
	if (!TestNotEqual(TEXT("premessa: la variante ha davvero cambiato la portata"),
		ArcPulse->Def.RangeCells, PortataCanonica))
	{
		return false;
	}

	TestFalse(TEXT("il ripristino ha un id proprio, distinguibile nel report"), Ripristino.VariantId.IsNone());
	TestNotEqual(TEXT("e non e' l'id della variante che annulla"), Ripristino.VariantId, Variante.VariantId);

	FRTWorkbenchVariant Inutile;
	TestEqual(TEXT("il ripristino si applica"),
		URTWorkbenchVariantLibrary::Apply(Ripristino, Kit, Inutile), ERTVariantApplyResult::Ok);

	TestEqual(TEXT("portata tornata canonica nel Def"), ArcPulse->Def.RangeCells, PortataCanonica);
	TestEqual(TEXT("portata tornata canonica nello specchio"), ArcPulse->RangeCells, PortataCanonica);
	TestEqual(TEXT("danno tornato canonico"), RTVariante::DannoDichiarato(ArcPulse->Def), DannoCanonico);
	TestEqual(TEXT("danno tornato canonico nello specchio"), ArcPulse->Power, DannoCanonico);

	return true;
}

/**
 * **La variante non raggiunge il catalogo**, e il test sta dove il rischio e' davvero.
 *
 * ⚠️ Il kit originale chiedeva `VariantDoesNotMutateCanonicalAbility` pensando a un `.uasset` di
 * produzione. **Quel rischio quasi non esiste**: le abilita' nascono da `NewObject<URTActionData>` in
 * `URTHeroCatalogLibrary`, il catalogo E' codice, e non c'e' nessun asset da sporcare — un test scritto
 * contro l'asset sarebbe verde anche col difetto.
 *
 * Il rischio reale e' che l'override sopravviva alla run: se qualcuno domani mettesse in cache le istanze
 * del catalogo, la seconda partita della stessa sessione partirebbe con i numeri dell'esperimento
 * precedente. Questo test chiede al catalogo un kit NUOVO dopo l'applicazione, e pretende la baseline.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantSecondKitIsBaselineTest,
	"RefactorTactics.Workbench.SecondKitWithoutVariantIsBaseline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantSecondKitIsBaselineTest::RunTest(const FString&)
{
	TArray<URTActionData*> Primo = RTVariante::KitDiGadget();
	URTActionData* ArcPulsePrimo = RTVariante::Azione(Primo, TEXT("Hero.Gadget.ArcPulse"));
	if (!TestNotNull(TEXT("ArcPulse del primo kit"), ArcPulsePrimo)) { return false; }

	const int32 PortataCanonica = ArcPulsePrimo->Def.RangeCells;
	const int32 DannoCanonico = RTVariante::DannoDichiarato(ArcPulsePrimo->Def);

	FRTWorkbenchVariant Variante;
	Variante.VariantId = TEXT("Test.NonDeveSopravvivere");
	Variante.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::RangeCells(), PortataCanonica + 4));
	Variante.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::Damage(), DannoCanonico + 11));

	FRTWorkbenchVariant Ripristino;
	URTWorkbenchVariantLibrary::Apply(Variante, Primo, Ripristino);

	if (!TestNotEqual(TEXT("premessa: la variante ha cambiato il PRIMO kit"),
		ArcPulsePrimo->Def.RangeCells, PortataCanonica))
	{
		return false;
	}

	// Il secondo kit non viene ripristinato: viene CHIESTO DI NUOVO al catalogo, che e' quel che fa una
	// seconda esecuzione. Se l'override fosse sopravvissuto, comparirebbe qui.
	TArray<URTActionData*> Secondo = RTVariante::KitDiGadget();
	URTActionData* ArcPulseSecondo = RTVariante::Azione(Secondo, TEXT("Hero.Gadget.ArcPulse"));
	if (!TestNotNull(TEXT("ArcPulse del secondo kit"), ArcPulseSecondo)) { return false; }

	TestEqual(TEXT("il secondo kit ha la portata canonica nel Def"),
		ArcPulseSecondo->Def.RangeCells, PortataCanonica);
	TestEqual(TEXT("e nello specchio"), ArcPulseSecondo->RangeCells, PortataCanonica);
	TestEqual(TEXT("il secondo kit ha il danno canonico"),
		RTVariante::DannoDichiarato(ArcPulseSecondo->Def), DannoCanonico);
	TestEqual(TEXT("e nello specchio"), ArcPulseSecondo->Power, DannoCanonico);

	return true;
}

/**
 * **Un ingresso invalido non scrive niente**, e la verifica e' sullo STATO, non sull'esito.
 *
 * Un test che si accontentasse del codice di ritorno sarebbe verde anche con un'applicazione parziale —
 * cioe' col difetto che la validazione in due tempi esiste per impedire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantInvalidWritesNothingTest,
	"RefactorTactics.Workbench.InvalidVariantWritesNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantInvalidWritesNothingTest::RunTest(const FString&)
{
	TArray<URTActionData*> Kit = RTVariante::KitDiGadget();
	URTActionData* ArcPulse = RTVariante::Azione(Kit, TEXT("Hero.Gadget.ArcPulse"));
	if (!TestNotNull(TEXT("ArcPulse c'e'"), ArcPulse)) { return false; }

	const int32 PortataPrima = ArcPulse->Def.RangeCells;
	const int32 SpecchioPrima = ArcPulse->RangeCells;

	// Il primo override e' VALIDO, il secondo no: se la scrittura non fosse preceduta dalla validazione
	// completa, il primo sarebbe gia' atterrato quando il secondo fallisce.
	FRTWorkbenchVariant Mista;
	Mista.VariantId = TEXT("Test.Mista");
	Mista.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::RangeCells(), PortataPrima + 2));
	Mista.Overrides.Add(RTVariante::Ov(TEXT("Hero.Inesistente.Azione"),
		RTActionParameterKeys::RangeCells(), 9));

	FRTWorkbenchVariant Ripristino;
	TestEqual(TEXT("un ActionId sconosciuto fallisce esplicitamente"),
		URTWorkbenchVariantLibrary::Apply(Mista, Kit, Ripristino), ERTVariantApplyResult::UnknownAction);

	TestEqual(TEXT("e il Def NON e' stato toccato dal primo override, che era valido"),
		ArcPulse->Def.RangeCells, PortataPrima);
	TestEqual(TEXT("ne' lo specchio"), ArcPulse->RangeCells, SpecchioPrima);
	TestEqual(TEXT("il ripristino resta vuoto: non c'e' niente da annullare"), Ripristino.Overrides.Num(), 0);

	// Chiave sconosciuta.
	FRTWorkbenchVariant Chiave;
	Chiave.VariantId = TEXT("Test.ChiaveIgnota");
	Chiave.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"), TEXT("Action.Inventato"), 1));
	TestEqual(TEXT("una ParameterKey sconosciuta fallisce esplicitamente"),
		URTWorkbenchVariantLibrary::Apply(Chiave, Kit, Ripristino), ERTVariantApplyResult::UnknownParameter);
	TestEqual(TEXT("e non ha scritto"), ArcPulse->Def.RangeCells, PortataPrima);

	// Indice d'effetto che non punta a un `Damage`.
	FRTWorkbenchVariant Indice;
	Indice.VariantId = TEXT("Test.IndiceFuoriPosto");
	FRTAbilityParameterOverride Fuori = RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::Damage(), 40);
	Fuori.EffectIndex = 99;
	Indice.Overrides.Add(Fuori);
	TestEqual(TEXT("un EffectIndex fuori range fallisce esplicitamente"),
		URTWorkbenchVariantLibrary::Apply(Indice, Kit, Ripristino), ERTVariantApplyResult::InvalidEffectIndex);

	return true;
}

/**
 * **Una variante senza id non e' applicabile.**
 *
 * Non e' formalismo: il confronto baseline/variante che questo dato serve produce un report, e una
 * variante che non si puo' nominare rende quel report illeggibile — «una delle due ha fatto 45» non dice
 * quale esperimento era.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantMissingIdFailsTest,
	"RefactorTactics.Workbench.MissingVariantIdFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantMissingIdFailsTest::RunTest(const FString&)
{
	TArray<URTActionData*> Kit = RTVariante::KitDiGadget();
	URTActionData* ArcPulse = RTVariante::Azione(Kit, TEXT("Hero.Gadget.ArcPulse"));
	if (!TestNotNull(TEXT("ArcPulse c'e'"), ArcPulse)) { return false; }
	const int32 PortataPrima = ArcPulse->Def.RangeCells;

	FRTWorkbenchVariant SenzaId;
	SenzaId.Overrides.Add(RTVariante::Ov(TEXT("Hero.Gadget.ArcPulse"),
		RTActionParameterKeys::RangeCells(), PortataPrima + 1));

	FRTWorkbenchVariant Ripristino;
	TestEqual(TEXT("senza VariantId l'applicazione fallisce"),
		URTWorkbenchVariantLibrary::Apply(SenzaId, Kit, Ripristino), ERTVariantApplyResult::MissingVariantId);
	TestEqual(TEXT("e non ha scritto"), ArcPulse->Def.RangeCells, PortataPrima);

	// Anti-vacuita': la STESSA variante con un id riesce, o il fallimento sopra potrebbe venire da altro.
	SenzaId.VariantId = TEXT("Test.OraHoUnNome");
	TestEqual(TEXT("con un id la stessa variante si applica"),
		URTWorkbenchVariantLibrary::Apply(SenzaId, Kit, Ripristino), ERTVariantApplyResult::Ok);
	TestEqual(TEXT("e ora ha scritto"), ArcPulse->Def.RangeCells, PortataPrima + 1);

	return true;
}

/**
 * **Una variante vuota e' baseline, non un errore.**
 *
 * E' il lato «senza variante» del confronto, e deve essere applicabile senza casi speciali: il chiamante
 * che esegue baseline e variante passa lo stesso codice due volte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVariantEmptyIsBaselineTest,
	"RefactorTactics.Workbench.EmptyVariantIsBaseline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVariantEmptyIsBaselineTest::RunTest(const FString&)
{
	TArray<URTActionData*> Kit = RTVariante::KitDiGadget();
	URTActionData* ArcPulse = RTVariante::Azione(Kit, TEXT("Hero.Gadget.ArcPulse"));
	if (!TestNotNull(TEXT("ArcPulse c'e'"), ArcPulse)) { return false; }
	const int32 PortataPrima = ArcPulse->Def.RangeCells;

	FRTWorkbenchVariant Vuota;
	Vuota.VariantId = TEXT("Test.Baseline");
	TestTrue(TEXT("una variante senza override si dichiara baseline"), Vuota.IsBaseline());

	FRTWorkbenchVariant Ripristino;
	TestEqual(TEXT("e si applica senza errori"),
		URTWorkbenchVariantLibrary::Apply(Vuota, Kit, Ripristino), ERTVariantApplyResult::Ok);
	TestEqual(TEXT("lasciando i valori canonici"), ArcPulse->Def.RangeCells, PortataPrima);
	TestEqual(TEXT("e un ripristino vuoto"), Ripristino.Overrides.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
