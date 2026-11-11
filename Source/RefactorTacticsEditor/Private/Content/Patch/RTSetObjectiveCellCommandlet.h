#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTSetObjectiveCellCommandlet.generated.h"

/**
 * Dichiara (o ritira) la cella OBIETTIVO di una mappa d'autore, e lo fa in modo ripetibile.
 *
 * Perche' un commandlet e non quattro clic nell'Editor: un `.uasset` non e' diffabile, quindi il diff di
 * una PR non puo' mostrare *cosa* e' cambiato dentro la mappa. Qui il cambiamento e' un comando scritto —
 * si rilegge, si ripete su un'altra mappa, e se qualcuno lo disfa per sbaglio si riapplica identico. E'
 * la stessa ragione per cui `RTBuildIconCatalog` esiste: sessantuno chiavi a mano sono sessantuno
 * occasioni di sbagliare, e nessuno se ne accorge fino a schermo.
 *
 * ⚠️ **Non decide DOVE va l'obiettivo**: quella e' una scelta di contenuto (`D-241` — dove sta l'obiettivo
 * decide come si gioca la mappa), e va passata sulla riga di comando. Il commandlet la esegue e verifica
 * che sia sensata; non la inventa.
 *
 * Cosa rifiuta, invece di salvare un asset che sembra a posto:
 *   - una cella che la mappa non contiene;
 *   - una cella che **blocca il movimento**: un obiettivo su cui nessuno puo' salire non e' contendibile,
 *     e la partita lo dichiarerebbe `Unclaimed` per sempre senza che nulla suoni;
 *   - una mappa che dichiara gia' un obiettivo diverso, a meno di `-Force`: piu' obiettivi simultanei sono
 *     `CP 31.1`, post-v0.1, e il TurnLog oggi ne nomina uno solo (`FirstObjectiveCell`).
 *
 * Stampa l'hash della mappa **prima e dopo**: e' il numero che cambia per chi legge la PR, e senza vederlo
 * qui bisognerebbe fidarsi del binario.
 *
 * Uso:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTSetObjectiveCell
 *         -Map=/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena -Cell=0,-3,0 [-DryRun] [-Clear] [-Force]
 *
 * `-DryRun` non scrive nulla e dice cosa farebbe: e' il primo comando da lanciare, sempre.
 */
UCLASS()
class URTSetObjectiveCellCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTSetObjectiveCellCommandlet();

	virtual int32 Main(const FString& Params) override;
};
