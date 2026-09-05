// Il READOUT numerico di un'azione, e la domanda che lo giustifica: «quanto fa questa abilita'?»
//
// La risposta non e' un numero. Un parametro vive in tre case — il `Def` del catalogo, i campi specchio di
// `URTActionData`, e `Def.Effects[]` per il danno — e la regola che decide quale vince non e' applicata da
// tutti i consumatori. Questi test verificano che il readout **dichiari** quella realta' invece di
// nasconderla scegliendo un valore.

#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTActionReadout.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTReadout
{
	/** La voce con quella chiave, o `nullptr`. Per il danno torna la PRIMA: le altre si cercano per indice. */
	const FRTActionParameterView* Trova(const TArray<FRTActionParameterView>& Viste, const TCHAR* Chiave)
	{
		for (const FRTActionParameterView& V : Viste)
		{
			if (V.ParameterKey == FName(Chiave)) { return &V; }
		}
		return nullptr;
	}

	int32 Conta(const TArray<FRTActionParameterView>& Viste, const TCHAR* Chiave)
	{
		int32 N = 0;
		for (const FRTActionParameterView& V : Viste)
		{
			if (V.ParameterKey == FName(Chiave)) { ++N; }
		}
		return N;
	}
}

/**
 * **Il readout dice cio' che il consumatore legge**, e questo e' il test che giustifica l'intera slice.
 *
 * Non «il readout e' coerente con se stesso» — quello sarebbe una tautologia — ma: per ogni parametro, il
 * valore che il readout marca come `ConsumedValue` e' **lo stesso campo** che il consumatore reale
 * interroga in partita. I consumatori sono nominati uno per uno, con il loro sito, perche' sono diversi fra
 * loro:
 *
 *   - PORTATA  -> il bot, `ARTTurnManager` righe ~1271 e ~1329: `AddCandidates(..., Ability->RangeCells,
 *                 Ability->Power, ...)`, senza il ternario che altri applicano;
 *   - RICARICA -> `ARTUnit::ConsumeAbility`, che legge `URTActionData::CooldownTurns` e non `Def.CooldownTurns`;
 *   - DANNO    -> il resolver, che legge gli effetti del catalogo e ricade sullo specchio solo quando il
 *                 catalogo tace.
 *
 * ⚠️ Oggi le due case CONCORDANO su tutto il roster, quindi il confronto potrebbe sembrare vacuo. Non lo e':
 * quella concordanza e' un fatto **misurato** da `Actions.HeroKitsMatchTheirCatalogDef` (`#1963`), non una
 * proprieta' del linguaggio. Se un giorno una copia sparisse, quel guardiano diventerebbe rosso e questo
 * test direbbe **quale** consumatore ha cominciato a leggere un altro numero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadoutAgreesWithTheConsumerTest,
	"RefactorTactics.Actions.ReadoutAgreesWithTheConsumer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadoutAgreesWithTheConsumerTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0))
	{
		return false;
	}

	const URTActionData* Nuda = GetDefault<URTActionData>();
	if (!TestNotNull(TEXT("i default di URTActionData sono leggibili"), Nuda))
	{
		return false;
	}

	// Anti-vacuita': senza almeno un'azione che si discosta dai default legacy (`RangeCells = 5`,
	// `Power = 30`), i confronti qui sotto potrebbero essere veri per coincidenza.
	int32 PortataDiversaDalDefault = 0;
	int32 DannoDiversoDalDefault = 0;
	int32 AbilitaEsaminate = 0;

	for (const URTHeroData* Hero : Roster)
	{
		if (!TestNotNull(TEXT("l'eroe del roster esiste"), Hero)) { continue; }

		for (const TObjectPtr<URTActionData>& Voce : Hero->Actions)
		{
			const URTActionData* Azione = Voce;
			if (Azione == nullptr) { continue; }

			TArray<FRTActionParameterView> Viste;
			const ERTActionReadoutResult Esito =
				URTActionReadoutLibrary::DescribeActionParameters(Azione, Viste);

			const FString Id = Azione->Def.ActionId.ToString();
			if (!TestEqual(*FString::Printf(TEXT("`%s`: il readout riesce"), *Id),
				Esito, ERTActionReadoutResult::Ok))
			{
				continue;
			}

			++AbilitaEsaminate;

			const FRTActionParameterView* Portata = RTReadout::Trova(Viste, TEXT("Action.RangeCells"));
			if (TestNotNull(*FString::Printf(TEXT("`%s`: la portata compare nel readout"), *Id), Portata))
			{
				// Il consumatore e' il BOT, e legge lo specchio senza ternario.
				TestEqual(*FString::Printf(
					TEXT("`%s`: ConsumedValue della portata e' cio' che il bot legge (Ability->RangeCells)"), *Id),
					Portata->ConsumedValue, Azione->RangeCells);
				TestEqual(*FString::Printf(
					TEXT("`%s`: DeclaredValue della portata e' cio' che il catalogo dichiara"), *Id),
					Portata->DeclaredValue, Azione->Def.RangeCells);
				if (Azione->Def.RangeCells != Nuda->RangeCells) { ++PortataDiversaDalDefault; }
			}

			const FRTActionParameterView* Ricarica = RTReadout::Trova(Viste, TEXT("Action.CooldownTurns"));
			if (TestNotNull(*FString::Printf(TEXT("`%s`: la ricarica compare nel readout"), *Id), Ricarica))
			{
				// Il consumatore e' `ConsumeAbility`, e legge lo specchio.
				TestEqual(*FString::Printf(
					TEXT("`%s`: ConsumedValue della ricarica e' cio' che ConsumeAbility legge"), *Id),
					Ricarica->ConsumedValue, Azione->CooldownTurns);
				TestEqual(*FString::Printf(TEXT("`%s`: la ricarica dichiara MirrorWins"), *Id),
					Ricarica->AuthorityRule, ERTParameterAuthority::MirrorWins);
			}

			const FRTActionParameterView* Danno = RTReadout::Trova(Viste, TEXT("Action.Damage"));
			if (Danno != nullptr)
			{
				TestEqual(*FString::Printf(
					TEXT("`%s`: ConsumedValue del primo danno e' la proiezione dello specchio"), *Id),
					Danno->ConsumedValue, Azione->Power);
				TestEqual(*FString::Printf(TEXT("`%s`: il danno viene da un effetto del catalogo"), *Id),
					Danno->StorageHome, ERTParameterStorageHome::EffectSpec);
				TestTrue(*FString::Printf(TEXT("`%s`: il danno porta l'indice del proprio effetto"), *Id),
					Danno->EffectIndex != INDEX_NONE);
				if (Danno->DeclaredValue != Nuda->Power) { ++DannoDiversoDalDefault; }
			}
		}
	}

	TestTrue(*FString::Printf(TEXT("esaminate almeno cinque abilita' per eroe (%d su %d eroi)"),
		AbilitaEsaminate, Roster.Num()), AbilitaEsaminate >= Roster.Num() * 5);
	TestTrue(*FString::Printf(
		TEXT("almeno un'abilita' ha portata diversa dal default %d, o il confronto vale per coincidenza"),
		Nuda->RangeCells), PortataDiversaDalDefault > 0);
	TestTrue(*FString::Printf(
		TEXT("almeno un'abilita' ha danno diverso dal default %d, idem"), Nuda->Power),
		DannoDiversoDalDefault > 0);

	return true;
}

/**
 * **Due colpi dichiarati sono due voci**, e lo specchio ne porta uno solo.
 *
 * ⚠️ **La fixture e' sintetica, e la ragione va detta**: misurato sul catalogo al `996804dc`, **nessuna
 * azione base** dichiara due effetti `Damage` — quelle multi-effetto ne hanno uno piu' `Status`/`Push`. Il
 * caso a due colpi esiste, ma vive in una VARIANTE di loadout: `Hero.Gadget.LinearDischarge.Branched`
 * dichiara due `Damage 18` in `FRTAbilityVariant::Effects`. Le varianti sono fuori dallo scope di questa
 * slice — i loro `Effects` sostituiscono per intero quelli del `Def` e sono un readout diverso — quindi
 * `Branched` e' la MOTIVAZIONE del caso, non la sua fixture.
 *
 * Costruire l'azione a mano e' percio' corretto e non un ripiego: verifica che il readout regga una forma
 * che il dato di produzione produrra' il giorno in cui la variante entrera' nel Workbench.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadoutSurfacesEveryDamageEffectTest,
	"RefactorTactics.Actions.ReadoutSurfacesEveryDamageEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadoutSurfacesEveryDamageEffectTest::RunTest(const FString&)
{
	URTActionData* Ramificata = NewObject<URTActionData>();
	if (!TestNotNull(TEXT("l'azione di prova esiste"), Ramificata))
	{
		return false;
	}

	Ramificata->Def.ActionId = TEXT("Test.DueColpi");
	Ramificata->Def.RangeCells = 4;
	Ramificata->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 18));
	Ramificata->Def.Effects.Add(FRTActionEffectSpec(ERTActionEffect::Damage, 18));
	// Lo specchio come lo costruisce `MakeHeroAction`: il PRIMO effetto `Damage`, poi `break`.
	Ramificata->Power = 18;
	Ramificata->RangeCells = 4;

	TArray<FRTActionParameterView> Viste;
	if (!TestEqual(TEXT("il readout riesce"),
		URTActionReadoutLibrary::DescribeActionParameters(Ramificata, Viste), ERTActionReadoutResult::Ok))
	{
		return false;
	}

	TestEqual(TEXT("due effetti Damage dichiarati danno DUE voci, non una"),
		RTReadout::Conta(Viste, TEXT("Action.Damage")), 2);

	TArray<const FRTActionParameterView*> Danni;
	for (const FRTActionParameterView& V : Viste)
	{
		if (V.ParameterKey == FName(TEXT("Action.Damage"))) { Danni.Add(&V); }
	}
	if (!TestEqual(TEXT("le due voci di danno sono raccolte"), Danni.Num(), 2))
	{
		return false;
	}

	TestNotEqual(TEXT("le due voci si distinguono per EffectIndex"),
		Danni[0]->EffectIndex, Danni[1]->EffectIndex);

	TestTrue(TEXT("il PRIMO danno concorda con lo specchio, che lo proietta"),
		Danni[0]->bHomesAgree);
	// ⚠️ `false` qui non e' un difetto del dato: lo specchio non ha una rappresentazione del secondo colpo,
	// perche' `Power` proietta il primo e si ferma. E' esattamente il fatto che il designer deve vedere.
	TestFalse(TEXT("il SECONDO danno non concorda: lo specchio non lo porta affatto"),
		Danni[1]->bHomesAgree);

	return true;
}

/**
 * **L'elenco e' ordinato, stabile, e non si accoda a quello di prima.**
 *
 * Il secondo verso non e' pedanteria: un chiamante che riusa lo stesso `TArray` per due azioni e ignora
 * l'esito leggerebbe i parametri della precedente come se fossero di questa. Il `Reset` all'ingresso lo
 * impedisce, e senza questo test si potrebbe togliere senza che nulla diventi rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadoutIsOrderedAndStableTest,
	"RefactorTactics.Actions.ReadoutIsOrderedAndStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadoutIsOrderedAndStableTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0)) { return false; }
	if (!TestTrue(TEXT("il primo eroe ha un kit"), Roster[0] && Roster[0]->Actions.Num() > 0)) { return false; }

	const URTActionData* Azione = Roster[0]->Actions[0];
	if (!TestNotNull(TEXT("l'abilita' esiste"), Azione)) { return false; }

	TArray<FRTActionParameterView> Prima;
	TArray<FRTActionParameterView> Seconda;
	URTActionReadoutLibrary::DescribeActionParameters(Azione, Prima);
	URTActionReadoutLibrary::DescribeActionParameters(Azione, Seconda);

	if (!TestEqual(TEXT("due letture danno lo stesso numero di voci"), Prima.Num(), Seconda.Num()))
	{
		return false;
	}
	TestTrue(TEXT("e il readout non e' vuoto"), Prima.Num() > 0);

	for (int32 i = 0; i < Prima.Num(); ++i)
	{
		TestEqual(*FString::Printf(TEXT("voce %d: stessa chiave in entrambe le letture"), i),
			Prima[i].ParameterKey, Seconda[i].ParameterKey);
		TestEqual(*FString::Printf(TEXT("voce %d: stesso valore dichiarato"), i),
			Prima[i].DeclaredValue, Seconda[i].DeclaredValue);
	}

	// L'array in ingresso viene SVUOTATO: si parte da uno sporco e si verifica che l'avanzo non sopravviva.
	TArray<FRTActionParameterView> Sporca;
	Sporca.Add(FRTActionParameterView());
	Sporca.Add(FRTActionParameterView());
	URTActionReadoutLibrary::DescribeActionParameters(Azione, Sporca);
	TestEqual(TEXT("l'array in ingresso e' svuotato: nessun avanzo della lettura precedente"),
		Sporca.Num(), Prima.Num());

	return true;
}

/**
 * **Un riferimento nullo fallisce dicendo che e' nullo**, e non con un elenco vuoto.
 *
 * Un `TArray` vuoto e' indistinguibile da «azione senza parametri», ed e' la stessa ragione per cui il
 * formato scenario tiene `ERTTestOutcome::Error` separato da `Fail`: un difetto dello STRUMENTO non deve
 * poter passare per un dato legittimo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadoutUnknownActionFailsExplicitlyTest,
	"RefactorTactics.Actions.ReadoutUnknownActionFailsExplicitly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadoutUnknownActionFailsExplicitlyTest::RunTest(const FString&)
{
	TArray<FRTActionParameterView> Viste;
	Viste.Add(FRTActionParameterView()); // sporca: l'errore deve comunque svuotare

	TestEqual(TEXT("un'azione nulla e' UnknownAction, non un readout vuoto"),
		URTActionReadoutLibrary::DescribeActionParameters(nullptr, Viste),
		ERTActionReadoutResult::UnknownAction);
	TestEqual(TEXT("e l'array e' svuotato anche sul ramo d'errore"), Viste.Num(), 0);

	return true;
}

/**
 * **Un'azione NON CATALOGATA dichiara `MirrorWins`, e non e' un caso limite inventato.**
 *
 * `URTActionData::Def` documenta che un `ActionId` assente significa «azione non ancora catalogata, non un
 * errore», e il ternario dei consumatori (`Def.ActionId.IsNone() ? specchio : Def`) ricade sullo specchio
 * per **tutti** in quel caso. L'autorita' non «dipende dal consumatore»: e' dello specchio per costruzione,
 * e il readout deve dirlo invece di riportare la stessa etichetta di un'azione catalogata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReadoutUncataloguedActionSaysMirrorWinsTest,
	"RefactorTactics.Actions.ReadoutUncataloguedActionSaysMirrorWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReadoutUncataloguedActionSaysMirrorWinsTest::RunTest(const FString&)
{
	URTActionData* NonCatalogata = NewObject<URTActionData>();
	if (!TestNotNull(TEXT("l'azione di prova esiste"), NonCatalogata)) { return false; }
	// `ActionId` volutamente assente.
	NonCatalogata->RangeCells = 7;

	TArray<FRTActionParameterView> Viste;
	URTActionReadoutLibrary::DescribeActionParameters(NonCatalogata, Viste);

	const FRTActionParameterView* Portata = RTReadout::Trova(Viste, TEXT("Action.RangeCells"));
	if (!TestNotNull(TEXT("la portata compare"), Portata)) { return false; }

	TestEqual(TEXT("senza ActionId l'autorita' della portata e' dello specchio"),
		Portata->AuthorityRule, ERTParameterAuthority::MirrorWins);

	// Anti-vacuita' del confronto: un'azione CATALOGATA deve dire un'altra cosa, o l'assertion sopra
	// sarebbe vera anche se il readout rispondesse `MirrorWins` sempre.
	URTActionData* Catalogata = NewObject<URTActionData>();
	if (!TestNotNull(TEXT("l'azione catalogata di prova esiste"), Catalogata)) { return false; }
	Catalogata->Def.ActionId = TEXT("Test.Catalogata");

	TArray<FRTActionParameterView> VisteCat;
	URTActionReadoutLibrary::DescribeActionParameters(Catalogata, VisteCat);
	const FRTActionParameterView* PortataCat = RTReadout::Trova(VisteCat, TEXT("Action.RangeCells"));
	if (!TestNotNull(TEXT("la portata dell'azione catalogata compare"), PortataCat)) { return false; }

	TestEqual(TEXT("con ActionId l'autorita' della portata dipende dal consumatore"),
		PortataCat->AuthorityRule, ERTParameterAuthority::DependsOnConsumer);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
