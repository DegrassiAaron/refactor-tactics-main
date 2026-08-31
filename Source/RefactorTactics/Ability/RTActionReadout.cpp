#include "Ability/RTActionReadout.h"

#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"

namespace
{
	FRTActionParameterView MakeView(const FName& Key, const FString& Label, int32 Declared, int32 Consumed,
		ERTParameterStorageHome Home, ERTParameterAuthority Authority, int32 EffectIndex = INDEX_NONE)
	{
		FRTActionParameterView View;
		View.ParameterKey = Key;
		View.DisplayName = FText::FromString(Label);
		View.DeclaredValue = Declared;
		View.ConsumedValue = Consumed;
		View.bHomesAgree = (Declared == Consumed);
		View.StorageHome = Home;
		View.AuthorityRule = Authority;
		View.EffectIndex = EffectIndex;
		return View;
	}
}

ERTActionReadoutResult URTActionReadoutLibrary::DescribeActionParameters(const URTActionData* Action,
	TArray<FRTActionParameterView>& OutParameters)
{
	// Svuotato PRIMA di ogni ritorno, compreso quello di errore: un chiamante che riusa l'array e ignora
	// l'esito leggerebbe altrimenti i parametri dell'azione precedente come se fossero di questa.
	OutParameters.Reset();

	if (Action == nullptr)
	{
		return ERTActionReadoutResult::UnknownAction;
	}

	// Un'azione senza `ActionId` non e' sconosciuta: e' NON CATALOGATA, ed e' uno stato legittimo che
	// `URTActionData::Def` documenta («`ActionId` assente = azione non ancora catalogata, non un errore»).
	// Cambia pero' chi decide: senza catalogo il ternario dei consumatori ricade sullo specchio per tutti,
	// quindi l'autorita' non «dipende dal consumatore» — e' dello specchio, per costruzione.
	const bool bCatalogata = !Action->Def.ActionId.IsNone();
	const ERTParameterAuthority AutoritaPortata = bCatalogata
		? ERTParameterAuthority::DependsOnConsumer
		: ERTParameterAuthority::MirrorWins;

	// PORTATA. Due case, e l'autorita' dipende da chi legge: chi applica il ternario prende il `Def`, il bot
	// prende lo specchio (`RTTurnManager.cpp:1271`, `:1329`). Non si sceglie qui: si dichiara.
	OutParameters.Add(MakeView(
		TEXT("Action.RangeCells"), TEXT("Portata (celle)"),
		Action->Def.RangeCells, Action->RangeCells,
		ERTParameterStorageHome::CatalogDef, AutoritaPortata));

	// RICARICA. `MirrorWins` e non `DependsOnConsumer`, ed e' una differenza misurata: `ARTUnit::ConsumeAbility`
	// legge `URTActionData::CooldownTurns` e nessun consumatore di ricarica applica il ternario. Dichiararla
	// `DependsOnConsumer` per simmetria con la portata sarebbe una comodita' che dice il falso.
	OutParameters.Add(MakeView(
		TEXT("Action.CooldownTurns"), TEXT("Ricarica (turni)"),
		Action->Def.CooldownTurns, Action->CooldownTurns,
		ERTParameterStorageHome::CatalogDef, ERTParameterAuthority::MirrorWins));

	// DANNO. Una voce per OGNI effetto `Damage`, non una sola: `URTActionData::Power` proietta il primo e si
	// ferma, quindi un'azione con due colpi dichiarati ne mostrerebbe uno. Il resolver invece legge gli
	// effetti (`RTTurnManager_Blast.cpp`: `DeclaredDamage > 0 ? DeclaredDamage : Ability->Power`), quindi qui
	// l'autorita' e' del catalogo e lo specchio e' solo un ripiego per chi non dichiara danno.
	int32 IndiceDannoVisto = 0;
	for (int32 i = 0; i < Action->Def.Effects.Num(); ++i)
	{
		const FRTActionEffectSpec& Spec = Action->Def.Effects[i];
		if (Spec.Effect != ERTActionEffect::Damage)
		{
			continue;
		}

		// Lo specchio rappresenta SOLO il primo effetto `Damage`. Per i successivi non esiste un valore
		// specchio da confrontare, e riportare `Action->Power` li' direbbe che lo specchio porta un numero
		// che invece appartiene a un altro colpo. Si riporta 0 e le case NON concordano — che e' il fatto.
		const bool bPrimo = (IndiceDannoVisto == 0);
		const int32 Specchio = bPrimo ? Action->Power : 0;

		OutParameters.Add(MakeView(
			TEXT("Action.Damage"),
			bPrimo ? TEXT("Danno") : FString::Printf(TEXT("Danno (colpo %d)"), IndiceDannoVisto + 1),
			Spec.Amount, Specchio,
			ERTParameterStorageHome::EffectSpec, ERTParameterAuthority::CatalogWins, i));

		++IndiceDannoVisto;
	}

	return ERTActionReadoutResult::Ok;
}
