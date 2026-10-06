#include "Ability/RTActionDescriptions.h"

// Le frasi del roster v0.1 (`#3499`), scritte da chi implementa a partire dai dati e da rivedere dall'autore.
// ⚠️ L'ordine e' quello di `docs/balance/RT_ActionDescriptions_v0.1.md`: generiche, poi un eroe alla volta
// nell'ordine del catalogo eroi. Il radar confronta voce per voce, non l'ordine, ma due copie nello stesso
// ordine si rileggono affiancate.
//
// ⛔ Niente numeri, niente nomi di tag: la frase dice COSA fa l'azione, i numeri li compone il gioco.
const TArray<TPair<FName, FString>>& RTActionDescriptions::All()
{
	static const TArray<TPair<FName, FString>> Voci = {
		// Generiche (D-025)
		{ TEXT("Action.Wait"),      TEXT("Non agisci in questo turno.") },
		{ TEXT("Action.Guard"),     TEXT("Ti metti in guardia: ogni colpo che arriva di fronte perde una parte del danno.") },
		{ TEXT("Action.Brace"),     TEXT("Ti irrigidisci: attutisci un po' i colpi da ogni direzione e resisti alle spinte.") },
		{ TEXT("Action.Overwatch"), TEXT("Sorvegli la direzione in cui guardi e spari al nemico che vi passa; puoi muoverti solo per ritirarti.") },
		{ TEXT("Action.Interact"),  TEXT("Usi un oggetto su una cella vicina, per esempio una porta.") },

		// Aevik
		{ TEXT("Hero.Aevik.ArcPulse"),          TEXT("Una scarica elettrica a distanza su un bersaglio.") },
		{ TEXT("Hero.Aevik.LinearDischarge"),   TEXT("Una scarica in linea retta, piu' dannosa su chi e' bagnato.") },
		{ TEXT("Hero.Aevik.ConductiveNode"),    TEXT("Elettrizza una cella: la scarica corre lungo l'acqua e le superfici conduttive vicine.") },
		{ TEXT("Hero.Aevik.Overload"),          TEXT("Un'esplosione elettrica ad area, che interrompe anche i dispositivi.") },
		{ TEXT("Hero.Aevik.ReactiveCapacitor"), TEXT("Reazione: quando vieni colpito ti scherma e restituisce una scarica a chi ti attacca.") },

		// Muiren
		{ TEXT("Hero.Muiren.PressureJet"),  TEXT("Un getto d'acqua in linea: bagna il bersaglio e lo spinge indietro.") },
		{ TEXT("Hero.Muiren.CircularTide"), TEXT("Un'onda ad area attorno a un punto: cura gli alleati o colpisce, secondo la variante scelta.") },
		{ TEXT("Hero.Muiren.FluidTrail"),   TEXT("Uno scatto rapido verso una cella vicina.") },
		{ TEXT("Hero.Muiren.MistVeil"),     TEXT("Crea una nube di nebbia che ostacola la vista.") },
		{ TEXT("Hero.Muiren.FlowReaction"), TEXT("Reazione pensata per riposizionarti dopo un attacco: in questa versione non ha ancora effetto.") },
		{ TEXT("Hero.Muiren.TideGuard"),    TEXT("Uno scudo temporaneo su di te, da scegliere prima di sapere se sarai colpito.") },

		// Branth
		{ TEXT("Hero.Branth.ImpactShot"),    TEXT("Un colpo cinetico che rallenta il bersaglio.") },
		{ TEXT("Hero.Branth.KineticPanel"),  TEXT("Erige un pannello di copertura sul lato di una cella.") },
		{ TEXT("Hero.Branth.Reconfigure"),   TEXT("Sposta o ruota una copertura gia' in campo.") },
		{ TEXT("Hero.Branth.Ram"),           TEXT("Carichi contro un nemico: lo colpisci e lo spingi.") },
		{ TEXT("Hero.Branth.Interposition"), TEXT("Reazione: ti metti in mezzo e incassi il colpo diretto a un alleato vicino.") },
		{ TEXT("Hero.Branth.MortarShot"),    TEXT("Un tiro ad arco su un'area, che arriva anche dove non vedi.") },

		// Ivrin
		{ TEXT("Hero.Ivrin.PulseShot"),     TEXT("Un colpo a impulsi a distanza su un bersaglio.") },
		{ TEXT("Hero.Ivrin.InterceptShot"), TEXT("Miri una cella vicina e colpisci chi ci passa durante il movimento.") },
		{ TEXT("Hero.Ivrin.PassingBlade"),  TEXT("Scatti in avanti e colpisci chi attraversi.") },
		{ TEXT("Hero.Ivrin.Deflection"),    TEXT("Reazione: devii parte del danno che ricevi.") },
		{ TEXT("Hero.Ivrin.Feint"),         TEXT("Pensata per marcare una cella e riposizionarti: in questa versione non ha ancora effetto.") },
		{ TEXT("Hero.Ivrin.PhaseGuard"),    TEXT("Uno scudo temporaneo su di te, da scegliere prima di sapere se sarai colpito.") },
	};
	return Voci;
}

FText RTActionDescriptions::For(const FName& ActionId)
{
	for (const TPair<FName, FString>& Voce : All())
	{
		if (Voce.Key == ActionId)
		{
			return FText::FromString(Voce.Value);
		}
	}
	return FText::GetEmpty();
}
