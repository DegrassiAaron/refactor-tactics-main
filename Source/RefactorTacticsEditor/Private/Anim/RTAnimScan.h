#pragma once

#include "CoreMinimal.h"

/**
 * L'esito di una scansione di animazioni.
 *
 * ⚠️ **Struct semplice e non `USTRUCT`**: nessuno di questi campi attraversa la reflection, il Blueprint
 * o la serializzazione. E' tooling interno del modulo Editor, e una `USTRUCT` qui aggiungerebbe un
 * passaggio UHT senza comprare niente.
 */
struct FRTAnimScanResult
{
	/** Package path delle sole `UAnimSequence` trovate, ordinati. */
	TArray<FString> SequencePaths;

	/**
	 * 🔑 **Quanti asset sono stati scartati perche' NON erano `UAnimSequence`.**
	 *
	 * E' il controllo positivo del filtro, e non una curiosita': `SequencePaths.Num() > 0` passerebbe
	 * anche con un filtro rotto che accetta tutto. Sotto la cartella `Animations/` di un pack Paragon
	 * vivono `AimOffsets/` e `Blendspaces/`, quindi un filtro che funziona **deve** scartare qualcosa.
	 * Se scarta zero, non sta filtrando.
	 */
	int32 ScartatiPerClasse = 0;

	/**
	 * 🔴 **`false` = la misura NON e' stata eseguita**, e non «ha trovato zero».
	 *
	 * I pack Paragon vivono in `Content/FabAsset/`, che e' **gitignorato**: su ogni clone appena creato
	 * la cartella non esiste. Un risultato vuoto letto come «zero clip» sarebbe un falso negativo che
	 * assomiglia a una misura — la stessa distinzione che `URTAnimCatalogLibrary::ValidateReferents`
	 * fa con il suo `bOutRan`, e per la stessa ragione.
	 */
	bool bRan = false;

	/** Perche' non e' stata eseguita. Vuoto quando `bRan` e' vero. */
	FString MotivoNotRun;
};

/**
 * Enumera le sole `UAnimSequence` sotto una cartella di package, ricorsivamente.
 *
 * ⛔ **Restituisce, non scrive.** Il catalogo versionato (`Data/Anim/AnimCatalog.json`) lo tocca solo
 * un'invocazione esplicita da strumento: se lo scrivesse questa funzione, ogni esecuzione della suite
 * lascerebbe un file modificato nel working tree — e una suite che modifica l'albero mentre misura si
 * invalida da sola.
 *
 * ⚠️ **La classe si legge dal registry, non dal nome del file.** Sotto `Animations/` di Gadget non c'e'
 * nessun prefisso che distingua una sequenza da un aim offset: `AM_` e `BS_` danno **zero** risultati
 * in quella cartella. E' la stessa lezione di §AS.3b della guida animazioni — leggere la cartella, non
 * dedurre il nome.
 *
 * @param PackageFolder cartella in forma di package path, es. `/Game/FabAsset/.../Gadget/Animations`
 */
FRTAnimScanResult RTScanAnimSequencesUnder(const FString& PackageFolder);
