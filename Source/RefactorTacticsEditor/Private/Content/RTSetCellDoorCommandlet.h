#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTSetCellDoorCommandlet.generated.h"

/**
 * Posa una PORTA su un bordo di cella di una mappa d'autore, e lo fa in modo ripetibile (`#2330`).
 *
 * 🔴 **Esiste perche' era l'unico anello mancante.** Misurato prima di scriverlo: le mesh del kit sono
 * committate (`SM_Graybox_Door_Panel`, `SM_Graybox_Door_Locked`), `ARTHexMapActor` le disegna sul bordo,
 * `Action.Interact` le apre (CP 10.1, `#74`) e il formato mappa le porta dalla **v4** — ma **niente sapeva
 * crearne una** su un asset. La conseguenza, misurata da `#2312`: nell'intero contenuto versionato **non
 * esiste una porta**, quindi su nessuna mappa caricata da un asset il giocatore ha qualcosa su cui usare
 * `Interact`.
 *
 * Perche' un commandlet e non quattro clic nell'Editor — sono le parole che `RTSetObjectiveCell` usa gia',
 * e valgono identiche qui: *«un `.uasset` non e' diffabile, quindi il diff di una PR non puo' mostrare cosa
 * e' cambiato dentro la mappa. Qui il cambiamento e' un comando scritto — si rilegge, si ripete su un'altra
 * mappa, e se qualcuno lo disfa per sbaglio si riapplica identico.»*
 *
 * ⛔ **Non contiene NESSUNA regola.** Le regole stanno in `URTMapEditLibrary::AddDoor`, nel modulo runtime,
 * per la ragione che quella libreria dichiara: *«la regola e' del dominio, l'editor e' solo il gesto che la
 * invoca»*. Qui restano l'analisi degli argomenti, l'hash prima/dopo e il salvataggio. Il vantaggio pratico
 * e' misurabile: cosi' i rifiuti sono esercitabili dall'automation — `RefactorTactics.MapEdit.AddDoor*` —
 * mentre dentro un commandlet sarebbero raggiungibili solo aprendo un Editor.
 *
 * ⚠️ **Non decide DOVE va la porta.** Dov'e' una porta cambia la topologia della mappa, quindi come la si
 * gioca: e' una scelta di contenuto e va passata sulla riga di comando. Il commandlet la esegue e verifica
 * che sia sensata; non la inventa. Stessa disciplina di `RTSetObjectiveCell`.
 *
 * Cosa rifiuta, invece di salvare un asset che sembra a posto — i tre esiti vengono da `AddDoor`:
 *   - una cella che la mappa non contiene (`RefusedNoSuchCell`);
 *   - un bordo **oltre il quale non c'e' nessuna cella** (`RefusedNoNeighbour`): una porta e' sottrattiva,
 *     e sul perimetro non negherebbe nessuna adiacenza — si vedrebbe, cambierebbe l'hash, e non farebbe
 *     niente;
 *   - un bordo che ha **gia'** una porta (`RefusedDuplicate`).
 *
 * Stampa l'hash della mappa **prima e dopo**: e' il numero che cambia per chi legge la PR, e senza vederlo
 * qui bisognerebbe fidarsi del binario.
 *
 * Uso:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTSetCellDoor
 *         -Map=/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena
 *         -Cell=0,-1,0 -Edge=NE [-State=Closed] [-StableId=Door.NorthGate] [-DoorId=3] [-DryRun]
 *
 * `-DryRun` non scrive nulla e dice cosa farebbe: e' il primo comando da lanciare, sempre.
 * `-State` vale `Closed` se omesso — l'unico stato in cui una porta e' un oggetto **da attivare**.
 */
UCLASS()
class URTSetCellDoorCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTSetCellDoorCommandlet();

	virtual int32 Main(const FString& Params) override;
};
