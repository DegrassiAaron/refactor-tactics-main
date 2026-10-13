#include "UI/RTGlossary.h"
#include "Turn/RTTurnLogLibrary.h" // il prefisso delle cause di terreno: lo stesso che il TurnLog scrive
#include "UObject/Class.h"         // UEnum: il file deve compilare anche fuori dal blob unity

// Le voci della v0.1 (`#3744`), scritte da chi implementa a partire dalle regole nel codice e da accettare
// dall'autore. L'ordine e' quello di `docs/balance/RT_Glossary_v0.1.md`: stati, superfici, ripari. Il radar
// confronta voce per voce, non l'ordine; due copie nello stesso ordine si rileggono affiancate.
//
// ⛔ Niente cifre, niente nomi di tag, solo ASCII: lo verifica `RefactorTactics.UI.GlossaryEntriesNameThingsThatExist`.
// ⚠️ Una frase segue la REGOLA, non il catalogo dei terreni: dove i due divergono — il ghiaccio fa scivolare chi
// vi TERMINA il movimento, non chi vi entra — vale il codice che la applica.
// ⚠️ Il radar trova le due tabelle dalla riga della loro definizione: nessun commento sopra di esse deve
// ripeterne la firma.
const TArray<TPair<FName, FString>>& RTGlossary::Names()
{
	static const TArray<TPair<FName, FString>> Voci = {
		// Stati
		{ TEXT("Status.Root"),        TEXT("Immobilizzato") },
		{ TEXT("Status.Slow"),        TEXT("Rallentato") },
		{ TEXT("Status.Reveal"),      TEXT("Rivelato") },
		{ TEXT("Status.Exposed"),     TEXT("Esposto") },
		{ TEXT("Status.Guarded"),     TEXT("In guardia") },
		{ TEXT("Status.Marked"),      TEXT("Marchiato") },
		{ TEXT("Status.Wet"),         TEXT("Bagnato") },
		{ TEXT("Status.Braced"),      TEXT("Irrigidito") },
		{ TEXT("Status.Burning"),     TEXT("In fiamme") },
		{ TEXT("Status.Obscured"),    TEXT("Nel fumo") },
		{ TEXT("Status.Electrified"), TEXT("Folgorato") },
		{ TEXT("Status.Unbalanced"),  TEXT("Sbilanciato") },
		{ TEXT("Status.Prone"),       TEXT("A terra") },
		{ TEXT("Status.Stunned"),     TEXT("Stordito") },

		// Superfici
		{ TEXT("Terrain.Floor"),        TEXT("Pavimento") },
		{ TEXT("Terrain.ShallowWater"), TEXT("Acqua bassa") },
		{ TEXT("Terrain.Rough"),        TEXT("Accidentato") },
		{ TEXT("Terrain.Fire"),         TEXT("Fuoco") },
		{ TEXT("Terrain.Conductive"),   TEXT("Superficie metallica") },
		{ TEXT("Terrain.Ice"),          TEXT("Ghiaccio") },
		{ TEXT("Terrain.Void"),         TEXT("Vuoto") },
		{ TEXT("Terrain.Smoke"),        TEXT("Fumo") },
		{ TEXT("Terrain.HighGround"),   TEXT("Quota elevata") },

		// Ripari
		{ TEXT("Structure.LowCover"),  TEXT("Copertura bassa") },
		{ TEXT("Structure.HighCover"), TEXT("Copertura alta") },
	};
	return Voci;
}

const TArray<TPair<FName, FString>>& RTGlossary::Effects()
{
	static const TArray<TPair<FName, FString>> Voci = {
		// Stati
		{ TEXT("Status.Root"),        TEXT("Non puoi muoverti ne' scattare.") },
		{ TEXT("Status.Slow"),        TEXT("Ogni cella ti costa di piu': a piedi arrivi meno lontano.") },
		{ TEXT("Status.Reveal"),      TEXT("Gli avversari vedono che cosa stai preparando.") },
		{ TEXT("Status.Exposed"),     TEXT("Finche' dura, il primo colpo che ricevi in un turno fa piu' danno.") },
		{ TEXT("Status.Guarded"),     TEXT("Ogni colpo che arriva di fronte perde una parte del danno, e una spinta debole non ti sposta, finche' non sei sbilanciato.") },
		{ TEXT("Status.Marked"),      TEXT("Il prossimo colpo della squadra che ti ha marchiato fa piu' danno, poi il marchio si consuma.") },
		{ TEXT("Status.Wet"),         TEXT("La scarica lineare di Aevik ti fa piu' danno. Bagnarti spegne le fiamme.") },
		{ TEXT("Status.Braced"),      TEXT("Ogni colpo, da qualunque direzione, perde un po' di danno, e resisti alle spinte finche' non sei sbilanciato.") },
		{ TEXT("Status.Burning"),     TEXT("Subisci danno alla fine di ogni turno, finche' le fiamme durano o l'acqua le spegne.") },
		{ TEXT("Status.Obscured"),    TEXT("Sei nel fumo: attraverso il fumo si mira solo da vicino, in entrambi i versi.") },
		{ TEXT("Status.Electrified"), TEXT("Una scarica elettrica ti ha colpito: mirata su di te, oppure arrivata lungo acqua e metallo.") },
		{ TEXT("Status.Unbalanced"),  TEXT("Non puoi correre, e guardia e irrigidimento non ti tengono fermo; spinte, trazioni e scivolate ti portano piu' lontano, e una spinta o una trazione che ti sposta ti butta a terra.") },
		{ TEXT("Status.Prone"),       TEXT("Rialzarti ti costa una parte del movimento, e le tue reazioni non scattano.") },
		{ TEXT("Status.Stunned"),     TEXT("Non puoi usare l'azione principale, e le tue reazioni non scattano.") },

		// Superfici
		{ TEXT("Terrain.Floor"),        TEXT("Nessun effetto proprio. Puo' prendere fuoco.") },
		{ TEXT("Terrain.ShallowWater"), TEXT("Costa piu' movimento. Bagna chi ci entra e conduce l'elettricita'.") },
		{ TEXT("Terrain.Rough"),        TEXT("Costa piu' movimento, e non lo si attraversa ne' di scatto ne' in carica. Puo' prendere fuoco.") },
		{ TEXT("Terrain.Fire"),         TEXT("Costa piu' movimento. Chi ci entra subisce danno e prende fuoco.") },
		{ TEXT("Terrain.Conductive"),   TEXT("Conduce l'elettricita'.") },
		{ TEXT("Terrain.Ice"),          TEXT("Chi finisce qui il movimento con passi ancora da spendere scivola oltre, nella direzione in cui andava.") },
		{ TEXT("Terrain.Void"),         TEXT("Non e' un punto d'atterraggio: chi cade non si ferma qui.") },
		{ TEXT("Terrain.Smoke"),        TEXT("Chi ci sta dentro e' nel fumo: attraverso il fumo si mira solo da vicino.") },
		{ TEXT("Terrain.HighGround"),   TEXT("Nessun effetto proprio: indica una posizione rialzata, e il vantaggio viene dal piano della cella.") },

		// Ripari
		{ TEXT("Structure.LowCover"),  TEXT("Riduce il danno dei colpi mirati che ti arrivano di fronte attraverso quel lato; non ripara dalle esplosioni. Non ferma la vista ne' il passo.") },
		{ TEXT("Structure.HighCover"), TEXT("Chiude quel lato: non passano la vista, il passo e i colpi.") },
	};
	return Voci;
}

namespace
{
	/** Il testo di `Key` in `Voci`, oppure vuoto. Lineare come `RTActionDescriptions::For`: le voci sono poche. */
	FText GlossaryLookup(const TArray<TPair<FName, FString>>& Voci, const FName& Key)
	{
		for (const TPair<FName, FString>& Voce : Voci)
		{
			if (Voce.Key == Key)
			{
				return FText::FromString(Voce.Value);
			}
		}
		return FText::GetEmpty();
	}
}

FText RTGlossary::NameFor(const FName& Key)
{
	return GlossaryLookup(Names(), Key);
}

FText RTGlossary::EffectFor(const FName& Key)
{
	return GlossaryLookup(Effects(), Key);
}

FName RTGlossary::KeyForStatus(const FGameplayTag& Tag)
{
	return Tag.IsValid() ? Tag.GetTagName() : NAME_None;
}

FName RTGlossary::KeyForSurface(ERTHexSurface Surface)
{
	// 🔑 La stessa composizione con cui `ARTTurnManager` scrive la causa di un danno da terreno: il prefisso
	// arriva dalla libreria del TurnLog e non da un letterale, cosi' le due non divergono. Il TurnLog la scrive
	// solo per una superficie che fa danno; per le altre e' lo stesso schema, e l'id del catalogo dei terreni.
	return FName(*FString::Printf(TEXT("%s%s"), URTTurnLogLibrary::TerrainCausePrefix(),
		*StaticEnum<ERTHexSurface>()->GetNameStringByValue(static_cast<int64>(Surface))));
}

FName RTGlossary::KeyForCover(ERTHexCoverType Type)
{
	switch (Type)
	{
	case ERTHexCoverType::Low:  return FName(TEXT("Structure.LowCover"));
	case ERTHexCoverType::High: return FName(TEXT("Structure.HighCover"));
	default:                    return NAME_None;
	}
}
