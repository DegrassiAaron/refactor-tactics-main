#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTSetCellBodyFillCommandlet.generated.h"

/**
 * Dichiara il CORPO STRUTTURALE di una cella di una mappa d'autore, e lo fa in modo ripetibile (`#3361`).
 *
 * 🔴 **Esiste perche' era l'unico anello mancante di `#1865`.** Derivatore puro, posa nell'actor, filtro dei
 * piani e ricostruzione incrementale ci sono, ma nessun percorso di authoring su un asset scriveva
 * `FRTHexCellData::BodyFill`: il Details dell'asset e' vietato da `AGENTS.md` §7, e il pennello non e' il gesto
 * che manca (`ApplyBrush` copia la cella e ne sovrascrive solo i propri campi). La verifica PIE di `#1865` —
 * dipingere due layer e guardare i corpi — non era eseguibile.
 *
 * Perche' un commandlet e non quattro clic nell'Editor, sono le parole di `RTSetCellDoor` (`#2330`) e valgono
 * identiche: *«un `.uasset` non e' diffabile, quindi il diff di una PR non puo' mostrare cosa e' cambiato
 * dentro la mappa. Qui il cambiamento e' un comando scritto — si rilegge, si ripete su un'altra mappa, e se
 * qualcuno lo disfa per sbaglio si riapplica identico.»*
 *
 * ⛔ **Non contiene NESSUNA regola.** L'unica sta in `URTMapEditLibrary::SetCellBodyFill`, nel modulo runtime:
 * la cella deve esistere. Qui restano l'analisi degli argomenti, il confronto prima/dopo e il salvataggio.
 *
 * ⚠️ **Non decide QUANTO riempire.** Il valore e' una scelta di contenuto (un ponte vuole `Third`, una collina
 * `Full`) e va passato sulla riga di comando. La frazione e' un'**intenzione**, non una misura: il derivatore la
 * concilia con cio' che c'e' sotto e dichiara il troncamento in `FRTStructuralBody::bTruncated`.
 *
 * 🔴 **Cio' che NON deve cambiare, e il comando lo verifica prima di salvare.** `BodyFill` e' presentazione: non
 * entra in `ComputeHash`, e il comando non muove nemmeno `Revision` — che e' il primo termine di `HashMatchState`
 * e il `GraphRevision` di ogni voce di TurnLog. Un corpo che la muovesse cambierebbe l'identita' competitiva
 * della mappa senza che il gioco sia cambiato. Stampa hash e revisione prima e dopo; se uno dei due si e' mosso
 * **rifiuta di salvare** (exit 1): un asset cosi' e' il difetto, non la sua conseguenza.
 *
 * ⛔ **Editor CHIUSO, e le celle gia' salvate.** Il comando legge l'asset dal disco e lo riscrive: una cella
 * dipinta e non salvata risulta «assente», e un Editor che tiene la mappa in memoria sovrascriverebbe il campo
 * al suo prossimo salvataggio. E' la regola di `convenzioni-contenuti-ue.md`: due processi che scrivono sugli
 * stessi `.uasset` li corrompono.
 *
 * ⚠️ **`-Map` e' obbligatorio, senza default.** `RTSetCellDoor` punta per omissione all'arena di produzione; qui
 * una dimenticanza riscriverebbe un asset tracciato senza che nessuno l'abbia scelto. Riscrivere un corpo che
 * vale gia' il valore chiesto e' un no-op: l'asset non viene salvato.
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
