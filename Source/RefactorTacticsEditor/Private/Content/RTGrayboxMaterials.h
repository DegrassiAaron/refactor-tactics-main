// I materiali del kit graybox (#1714): un master parametrico e sei istanze, generati e assegnati dallo
// stesso commandlet che scrive le mesh (`D-229`). Editor-only: qui si AUTORA il kit, non si gioca.

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

namespace RTGraybox
{
	/**
	 * I due parametri del master, e sono DUE apposta.
	 *
	 * `#1714` chiede «i parametri minimi (tiling, roughness, tint)». `Tiling` e `Metallic` non entrano, ed
	 * e' una scelta dichiarata invece che una dimenticanza: il kit non ha texture — l'UV del commandlet
	 * serve solo «a non lasciare il materiale senza coordinate» — quindi un parametro di tiling non
	 * moltiplicherebbe niente, e nessuna delle sei istanze userebbe `Metallic`. Un parametro scollegato e'
	 * peggio di un parametro assente: mente su cio' che il master sa fare, e chi lo trova ci costruisce
	 * sopra. Quando l'arte portera' una texture, il tiling nasce col suo consumatore.
	 */
	extern const FName ParamBaseColor;
	extern const FName ParamRoughness;

	/** Il master. Sotto `/Game/RT/World/Graybox/Materials/`, come tutto il kit condiviso (`D-173`). */
	extern const TCHAR* MasterAssetName;
	extern const TCHAR* MaterialsFolder;

	/**
	 * Una istanza del kit: quale mesh veste, e con quali due valori.
	 *
	 * 🔴 **I `BaseColor` sono NEUTRI — `R == G == B` — e non e' una rinuncia al colore, e' cio' che rende
	 * la verifica in scala di grigi vera per costruzione invece che da controllare.** `D-146` chiede che
	 * nessuna categoria dipenda dal solo canale cromatico; un kit senza canale cromatico non puo'
	 * violarla, e il secondo canale resta dove il contratto lo vuole (§7): la geometria.
	 */
	struct FRTGrayboxMaterialSpec
	{
		/** Il nome della mesh che questa istanza veste — la chiave con cui il commandlet la ritrova. */
		const TCHAR* MeshName;

		/** Nome dell'asset istanza, senza percorso. */
		const TCHAR* InstanceName;

		/** Valore neutro in spazio LINEARE: e' anche la luminanza, perche' il colore e' un grigio. */
		float Value;

		/** Ruvidita'. E' il secondo canale, e sopravvive alla scala di grigi: separa cio' che il valore no. */
		float Roughness;
	};

	/** Le sei istanze, una per mesh di §8.1. La tabella e' il contratto: i test la leggono da qui. */
	extern const FRTGrayboxMaterialSpec KitMaterials[6];

	/** Luminanza Rec.709. Per un grigio neutro coincide col valore, e la formula resta quella giusta. */
	float Luminance(const FLinearColor& Color);

	/**
	 * Crea (o riusa) il master e le sei istanze sotto `PackageRoot`, e le salva.
	 *
	 * Idempotente: se gli asset esistono, il graph del master viene RICOSTRUITO e i parametri delle
	 * istanze RIAPPLICATI — la sorgente e' questo codice, l'asset e' il suo output (`D-229`). Chi ritocca
	 * un'istanza a mano la perde alla rigenerazione, ed e' il comportamento che `#1714` chiede
	 * esplicitamente per il materiale delle mesh.
	 *
	 * @param PackageRoot   la radice del kit, di norma `/Game/RT/World/Graybox`
	 * @param bDryRun       nessuna scrittura: costruisce e logga, non salva
	 * @param OutInstances  mappa `MeshName` -> istanza, quello che il commandlet assegna alle mesh
	 * @return              numero di asset falliti; `0` significa che tutto e' a posto
	 */
	int32 BuildKitMaterials(
		const FString& PackageRoot,
		bool bDryRun,
		TMap<FString, UMaterialInterface*>& OutInstances);
}
