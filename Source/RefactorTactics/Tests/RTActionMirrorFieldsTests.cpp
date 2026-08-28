#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * I CAMPI SPECCHIO di `URTActionData`, e chi si accorge se smettono di essere copiati.
 *
 * ## Il difetto che questo file chiude (#1552)
 *
 * `URTActionData` duplica tre valori che il `FRTActionDef` del catalogo gia' dichiara — `RangeCells`,
 * `Power`, `bSelfTarget` — perche' `ARTTurnManager` e `ARTPlayerController` leggono ancora **lo specchio**
 * e non il `Def`. Chi costruisce un'azione deve copiarli a mano, e la produzione lo fa in piu' posti.
 *
 * 🔴 **Nessuno di quei campi era sorvegliato**, misurato per mutazione durante #1548:
 *
 *     bSelfTarget = Def.bSelfTarget  ->  `false` fisso   ->  1346/1346, 0 fallimenti
 *     Power derivato dagli effetti   ->  rimosso          ->  1346/1346, 0 fallimenti
 *
 * Cioe' si poteva smettere del tutto di copiare e la suite restava verde. E i default che restano al
 * posto della copia non sono innocui: `URTActionData` nasce con `RangeCells = 5` e `Power = 30`, eredita'
 * dell'MVP quadrato. Un'azione non allineata non entra nel kit «vuota» — entra come un attacco da 30 a
 * distanza 5 che il catalogo non dichiara. Il difetto e' gia' stato chiuso due volte in produzione (per
 * gli eroi, poi per l'equipaggiamento) e in entrambi i casi lo ha trovato una persona, non un test.
 *
 * ## Perche' `MakeGenericActions` e non gli eroi
 *
 * Le azioni d'eroe hanno gia' un guardiano: `RefactorTactics.Catalog.HeroKitsMatchTheirCatalogDef` in
 * `RTCatalogTests.cpp` confronta `Def` e specchio per le venti abilita' del roster. Ma **esclude
 * esplicitamente le generiche**, e la ragione scritta li' e' che «arrivano da `MakeGenericActions` e
 * lasciano i campi legacy a zero».
 *
 * ⚠️ **Quell'affermazione oggi e' falsa.** `MakeGenericActions` i campi legacy li copia eccome — sono le
 * righe `Action->RangeCells = Def.RangeCells` e `Action->bSelfTarget = Def.bSelfTarget`. L'esclusione era
 * corretta quando e' stata scritta e non lo e' piu': ha lasciato scoperto il percorso che ogni unita' del
 * gioco attraversa, perche' `ARTUnit::ConfigureFromHeroData` accoda le generiche al kit di **ogni** eroe.
 */
namespace RTMirrorFields
{
	/** Il danno che il catalogo DICHIARA per un'azione: il primo effetto `Damage`, zero se non ce n'e'. */
	int32 DeclaredDamageOf(const FRTActionDef& Def)
	{
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage)
			{
				return Spec.Amount;
			}
		}
		return 0;
	}

	/**
	 * I default di `URTActionData` appena costruito, letti dall'oggetto e non riscritti qui.
	 *
	 * Servono all'anti-vacuita': un test che confronta specchio e `Def` non distingue «copiato» da «non
	 * copiato» se il valore del catalogo coincide per caso col default. Sapere quali sono i default
	 * permette di ASSERIRE che almeno un'azione se ne discosta.
	 */
	const URTActionData* Defaults()
	{
		return GetDefault<URTActionData>();
	}
}

/**
 * **Ogni azione generica rispecchia il proprio `Def`**, e se una riga di copia sparisce questo test cade.
 *
 * Il test e' una PROPRIETA' sul catalogo reale, non un elenco di valori attesi scritto a mano: chiede a
 * `MakeGenericActions` cio' che costruisce e lo confronta con cio' che il `Def` dichiara. Un'azione
 * generica aggiunta domani entra nel test da sola.
 *
 * ⚠️ **Le guardie di anti-vacuita' non sono decorative.** Se ogni generica dichiarasse per caso
 * `RangeCells = 5` — il default — togliere la riga di copia non cambierebbe niente e il test resterebbe
 * verde mentre il campo non e' piu' sorvegliato. Le tre `TestTrue` qui sotto asseriscono che almeno
 * un'azione si discosta dal default per ciascun campo: sono cio' che rende il resto capace di mordere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGenericActionsMirrorTheirDefTest,
	"RefactorTactics.Actions.GenericActionsMirrorTheirCatalogDef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGenericActionsMirrorTheirDefTest::RunTest(const FString&)
{
	const TArray<URTActionData*> Generiche = URTCatalogLibrary::MakeGenericActions(GetTransientPackage());

	if (!TestEqual(TEXT("le generiche costruite sono quante il catalogo ne dichiara"),
		Generiche.Num(), URTCatalogLibrary::GetGenericActionIds().Num()))
	{
		return false;
	}
	if (!TestTrue(TEXT("e ce n'e' almeno una"), Generiche.Num() > 0))
	{
		return false;
	}

	const URTActionData* Nuda = RTMirrorFields::Defaults();
	if (!TestNotNull(TEXT("i default di URTActionData sono leggibili"), Nuda))
	{
		return false;
	}

	// Anti-vacuita': quante azioni si discostano dal default per ciascun campo. Se un contatore resta a
	// zero, l'assertion corrispondente e' vera per coincidenza e non sorveglia niente.
	int32 PortataDiversaDalDefault = 0;
	int32 DannoDiversoDalDefault = 0;
	int32 BersaglioDiversoDalDefault = 0;

	for (const URTActionData* Azione : Generiche)
	{
		if (!TestNotNull(TEXT("l'istanza esiste"), Azione)) { continue; }

		const FString Id = Azione->Def.ActionId.ToString();
		const int32 Dichiarato = RTMirrorFields::DeclaredDamageOf(Azione->Def);

		TestEqual(*FString::Printf(TEXT("`%s`: la portata rispecchia il catalogo"), *Id),
			Azione->RangeCells, Azione->Def.RangeCells);
		TestEqual(*FString::Printf(TEXT("`%s`: il danno rispecchia gli effetti del catalogo"), *Id),
			Azione->Power, Dichiarato);
		TestEqual(*FString::Printf(TEXT("`%s`: l'auto-bersaglio rispecchia il catalogo"), *Id),
			Azione->bSelfTarget, Azione->Def.bSelfTarget);

		if (Azione->Def.RangeCells != Nuda->RangeCells) { ++PortataDiversaDalDefault; }
		if (Dichiarato != Nuda->Power) { ++DannoDiversoDalDefault; }
		if (Azione->Def.bSelfTarget != Nuda->bSelfTarget) { ++BersaglioDiversoDalDefault; }
	}

	TestTrue(*FString::Printf(
		TEXT("almeno una generica dichiara una portata diversa dal default %d, o il confronto sopra non ")
		TEXT("distingue una copia da una riga mancante"), Nuda->RangeCells), PortataDiversaDalDefault > 0);
	TestTrue(*FString::Printf(
		TEXT("almeno una generica dichiara un danno diverso dal default %d, idem"), Nuda->Power),
		DannoDiversoDalDefault > 0);
	TestTrue(TEXT("almeno una generica dichiara un auto-bersaglio diverso dal default, idem"),
		BersaglioDiversoDalDefault > 0);

	return true;
}

/**
 * **Il kit che l'unita' riceve non porta danno che il catalogo non dichiara.**
 *
 * E' la conseguenza osservabile del test qui sopra, sul percorso vero: `ARTUnit::ConfigureFromHeroData`
 * accoda le generiche al kit di ogni eroe, e da li' le leggono giocatore, bot e harness.
 *
 * ⚠️ Il ripiego che rende il difetto visibile sta in `RTTurnManager_Blast.cpp`:
 *
 *     Intent.Power = EffectiveAttackPower(DeclaredDamage > 0 ? DeclaredDamage : Ability->Power, 0);
 *
 * Quando il catalogo non dichiara `Damage`, il resolver ricade sullo specchio. Con la copia, lo specchio
 * vale `0` e l'azione non fa danno; senza, vale il default legacy `30` — e `Action.Guard`, che il catalogo
 * descrive come una preparazione difensiva, entrerebbe in partita capace di ferire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitKitCarriesNoUndeclaredDamageTest,
	"RefactorTactics.Actions.UnitKitCarriesNoUndeclaredDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitKitCarriesNoUndeclaredDamageTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0))
	{
		return false;
	}

	// Anti-vacuita': quante azioni del kit NON dichiarano danno. Sono le sole per cui il ripiego su
	// `Ability->Power` e' raggiungibile — se non ce ne fossero, il ciclo non misurerebbe niente.
	int32 SenzaDannoDichiarato = 0;

	for (const URTHeroData* Hero : Roster)
	{
		ARTUnit* Unit = NewObject<ARTUnit>();
		if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
		Unit->ConfigureFromHeroData(Hero);

		if (!TestTrue(TEXT("l'unita' ha un kit"), Unit->NumAbilities() > 0))
		{
			continue;
		}

		for (int32 i = 0; i < Unit->NumAbilities(); ++i)
		{
			const URTActionData* Azione = Unit->GetAbility(i);
			if (!Azione) { continue; }

			const int32 Dichiarato = RTMirrorFields::DeclaredDamageOf(Azione->Def);
			if (Dichiarato > 0)
			{
				continue;
			}
			++SenzaDannoDichiarato;

			// Il catalogo tace sul danno: lo specchio e' cio' che il resolver leggera', e deve tacere
			// anche lui. Un valore qui e' danno che nessuna riga di catalogo autorizza.
			TestEqual(*FString::Printf(
				TEXT("`%s` non dichiara danno, quindi il suo Power non ne inventa"),
				*Azione->Def.ActionId.ToString()), Azione->Power, 0);
		}
	}

	TestTrue(TEXT("almeno un'azione del kit non dichiara danno, o il ciclo non ha misurato niente"),
		SenzaDannoDichiarato > 0);

	return true;
}

/**
 * **Un'azione generica paga la ricarica che il catalogo le dichiara.**
 *
 * 🔴 **Questo test nasce ROSSO, ed e' il quarto campo specchio: `CooldownTurns`.** `MakeGenericActions`
 * copia portata, danno e auto-bersaglio, ma NON la ricarica — e il default di `URTActionData` e' `0`.
 * `ARTUnit::ConsumeAbility` legge lo specchio:
 *
 *     if (Ability->CooldownTurns > 0 && AbilityCooldowns.IsValidIndex(Index)) { ... }
 *
 * Con lo specchio a zero non scrive niente, `CanUseAbility` risponde sempre `true`, e `Action.Brace` —
 * che il catalogo dichiara con `Cooldown 1` — e' riarmabile ogni turno da ogni eroe del roster.
 *
 * ⚠️ **Perche' non se n'era accorto nessuno.** I test che verificano la ricarica di `Brace` costruiscono
 * l'azione con un helper locale (`AddDefAbility` in `RTDefensiveReactionTests.cpp`) che il cooldown lo
 * copia — quindi misurano un oggetto corretto su un percorso che in partita nessuno attraversa. Il kit
 * che l'unita' riceve davvero passa da `MakeGenericActions`, e quello non lo copia.
 *
 * ⚠️ **Non e' un test di ricarica**: quanto vale la ricarica di ciascuna azione lo pinnano altri test,
 * sul `Def`. Qui si verifica che il valore del catalogo ARRIVI fino al campo che il gioco legge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGenericActionsMirrorTheirCooldownTest,
	"RefactorTactics.Actions.GenericActionsMirrorTheirCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGenericActionsMirrorTheirCooldownTest::RunTest(const FString&)
{
	const TArray<URTActionData*> Generiche = URTCatalogLibrary::MakeGenericActions(GetTransientPackage());
	if (!TestTrue(TEXT("ci sono azioni generiche"), Generiche.Num() > 0))
	{
		return false;
	}

	// Anti-vacuita': se NESSUNA generica dichiarasse una ricarica, il confronto sotto sarebbe `0 == 0`
	// per tutte e non distinguerebbe una copia da una riga mancante.
	int32 ConRicaricaDichiarata = 0;

	for (const URTActionData* Azione : Generiche)
	{
		if (!Azione) { continue; }
		if (Azione->Def.CooldownTurns > 0) { ++ConRicaricaDichiarata; }

		TestEqual(*FString::Printf(TEXT("`%s`: la ricarica rispecchia il catalogo"),
			*Azione->Def.ActionId.ToString()), Azione->CooldownTurns, Azione->Def.CooldownTurns);
	}

	TestTrue(TEXT("almeno una generica dichiara una ricarica, o il confronto sopra non misura niente"),
		ConRicaricaDichiarata > 0);

	return true;
}

/**
 * **E la ricarica si paga davvero**, sul kit che l'unita' riceve.
 *
 * E' la conseguenza osservabile del test qui sopra: non «il campo e' copiato» ma «dopo l'uso l'azione
 * non e' subito riusabile». Passa da `ConsumeAbility` e `CanUseAbility`, cioe' dai due metodi che il
 * `ARTTurnManager` chiama in partita — tredici siti di chiamata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitPaysTheDeclaredCooldownTest,
	"RefactorTactics.Actions.UnitPaysTheDeclaredCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitPaysTheDeclaredCooldownTest::RunTest(const FString&)
{
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestTrue(TEXT("il roster non e' vuoto"), Roster.Num() > 0))
	{
		return false;
	}

	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit))
	{
		return false;
	}
	Unit->ConfigureFromHeroData(Roster[0]);

	// ⚠️ Si cerca fra le GENERICHE, non «la prima azione con ricarica»: le azioni dell'eroe vengono prima
	// nel kit e il loro cooldown lo copia `MakeHeroAction`, che lo fa correttamente. Un test che prende la
	// prima trova quella dell'eroe, passa, e non guarda mai il percorso rotto. MISURATO: scritto cosi'
	// restava verde mentre `GenericActionsMirrorTheirCooldown` cadeva su `Action.Brace`.
	const TArray<FName> IdGenerici = URTCatalogLibrary::GetGenericActionIds();
	int32 Indice = INDEX_NONE;
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* A = Unit->GetAbility(i);
		if (A && A->Def.CooldownTurns > 0 && IdGenerici.Contains(A->Def.ActionId))
		{
			Indice = i;
			break;
		}
	}

	if (!TestTrue(TEXT("il kit contiene un'azione GENERICA con ricarica dichiarata"), Indice != INDEX_NONE))
	{
		return false;
	}

	TestTrue(TEXT("prima dell'uso l'azione e' disponibile"), Unit->CanUseAbility(Indice));
	Unit->ConsumeAbility(Indice);
	TestTrue(*FString::Printf(TEXT("`%s` dopo l'uso e' in ricarica"),
		*Unit->GetAbility(Indice)->Def.ActionId.ToString()), Unit->GetAbilityCooldown(Indice) > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
