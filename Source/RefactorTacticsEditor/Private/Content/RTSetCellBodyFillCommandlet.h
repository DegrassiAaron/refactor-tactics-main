#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTSetCellBodyFillCommandlet.generated.h"

/**
 * Dichiara il CORPO STRUTTURALE di una cella di una mappa d'autore, e lo fa in modo ripetibile (`#3361`).
 *
 * 🔴 **Esiste perche' era l'unico anello mancante di `#1865`.** Derivatore puro, posa nell'actor, filtro dei
 * piani e ricostruzione incrementale ci sono, ma **niente scriveva `FRTHexCellData::BodyFill`**: il pennello ne
 * esclude il campo di proposito (la sua maschera lo dichiara, e un test la pinna), e il Details dell'asset e'
 * vietato da `AGENTS.md` §7. La verifica PIE di `#1865` — dipingere due layer e guardare i corpi — non era
 * eseguibile.
 *
 * Perche' un commandlet e non quattro clic nell'Editor, sono le parole di `RTSetCellDoor` (`#2330`) e valgono
 * identiche: *«un `.uasset` non e' diffabile, quindi il diff di una PR non puo' mostrare cosa e' cambiato
 * dentro la mappa. Qui il cambiamento e' un comando scritto — si rilegge, si ripete su un'altra mappa, e se
 * qualcuno lo disfa per sbaglio si riapplica identico.»*
 *
 * ⛔ **Non contiene NESSUNA regola.** L'unica sta in `URTMapEditLibrary::SetCellBodyFill`, nel modulo runtime:
 * la cella deve esistere, perche' `AddOrUpdateCell` crea quella che non trova. Qui restano l'analisi degli
 * argomenti, l'hash prima/dopo e il salvataggio.
 *
 * ⚠️ **Non decide QUANTO riempire.** Il valore e' una scelta di contenuto (un ponte vuole `Third`, una collina
 * `Full`) e va passato sulla riga di comando. La frazione e' un'**intenzione**, non una misura: il derivatore la
 * concilia con cio' che c'e' sotto e dichiara il troncamento in `FRTStructuralBody::bTruncated`.
 *
 * Stampa l'hash della mappa **prima e dopo**, ed e' qui per un motivo diverso da quello di `RTSetCellDoor`: la
 * porta lo sposta, il corpo **no** (e' presentazione, primo non-goal di `#1865`). Un hash mosso da questo gesto
 * sarebbe un difetto, e va visto sulla riga invece di dedurlo dal fatto che il comando e' andato a buon fine.
 *
 * Uso:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTSetCellBodyFill
 *         -Map=/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena
 *         -Cell=0,0,1 -Fill=Full [-DryRun]
 *
 * `-Fill` vale uno dei valori di `ERTHexBodyFill` (`None` toglie il corpo). `-DryRun` non scrive nulla e dice
 * cosa farebbe: e' il primo comando da lanciare, sempre.
 */
UCLASS()
class URTSetCellBodyFillCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTSetCellBodyFillCommandlet();

	virtual int32 Main(const FString& Params) override;
};
