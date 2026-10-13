#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Map/RTHexCellData.h" // ERTHexSurface, ERTHexCoverType

/**
 * IL GLOSSARIO: il nome e l'effetto in una riga di uno stato, di una superficie, di un riparo (`#3744`).
 *
 * 🔑 **Una tabella sola, chiavata per id stabile**, nella forma di `RTActionDescriptions`. Le chiavi sono id che
 * esistono gia': il nome del tag per uno stato (`Status.Burning`); per una superficie la causa che il TurnLog
 * scrive quando fa danno (`Terrain.Fire`), che per le altre e' l'id del catalogo dei terreni; l'id dello stesso
 * catalogo per un riparo (`Structure.LowCover`).
 *
 * ⛔ **Un testo non porta numeri ne' nomi di tag.** Durata, danno e costo li compone il gioco dai dati: un numero
 * scritto qui invecchierebbe al primo ribilanciamento, e nessun gate se ne accorgerebbe.
 *
 * ⚠️ **Oggi nessuno la legge in partita, ed e' dichiarato**: i lettori sono lo sguardo e la scheda dell'ispettore
 * del mondo (`docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`, fette F2 e F3). Chi la trova
 * inerte non ha trovato un difetto.
 *
 * 📄 Le stesse voci stanno in `docs/balance/RT_Glossary_v0.1.md`, che e' cio' che l'autore rilegge.
 * `tools/radar/glossary.ts --check` confronta le due copie, e non dice quale correggere. Che ogni stato, ogni
 * superficie e ogni riparo abbia una voce lo verificano i test `RefactorTactics.UI.Glossary*`.
 */
namespace RTGlossary
{
	/** Il nome da leggere di `Key`, oppure vuoto: una chiave senza voce non ha un nome da mostrare. */
	FText NameFor(const FName& Key);

	/** L'effetto in una riga di `Key`, oppure vuoto. */
	FText EffectFor(const FName& Key);

	/** La chiave di uno stato: il nome del suo tag. `None` per un tag non valido. */
	FName KeyForStatus(const FGameplayTag& Tag);

	/** La chiave di una superficie: il prefisso delle cause di terreno del TurnLog piu' il nome del valore. */
	FName KeyForSurface(ERTHexSurface Surface);

	/** La chiave di un tipo di riparo. `None` per `ERTHexCoverType::None`, che non e' un riparo. */
	FName KeyForCover(ERTHexCoverType Type);

	/** I nomi: li leggono il gate e il radar. */
	const TArray<TPair<FName, FString>>& Names();

	/** Gli effetti, con le stesse chiavi nello stesso ordine dei nomi: lo verifica il gate. */
	const TArray<TPair<FName, FString>>& Effects();
}
