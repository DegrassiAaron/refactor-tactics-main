/** Verifica che nessun asset versionato referenzi un file che git non ha.
 *
 *  Uso:  node tools/asset-refs/check.ts
 *
 *  Esce **1** se trova un riferimento penzolante, stampando asset e package. E' il criterio 5 di
 *  `#1069`: un `.uasset` versionato che punta a un file fuori dal repository si comporta in due modi
 *  opposti a seconda della macchina — errore fatale per chi ha il file, ripiego silenzioso per chi
 *  clona — e nessuno dei due e' osservabile senza aprire l'editor.
 *
 *  ⚠️ Gate **a mano**, come i due di `tools/radar/`: il repository non ha CI per scelta dichiarata
 *  (`AGENTS.md`). Si esegue prima di committare un binario.
 *
 *  🔴 **Che cosa misura, e che cosa no** (`#1280`). Risponde a *«il package CITA un path assente?»*,
 *  non a *«il package DIPENDE da un path assente?»*. Sul difetto per cui e' nato le due domande
 *  coincidevano — `DA_Format_Rotto` era un import vero, e il caricamento lo dichiarava a ogni Play.
 *  Non sempre: un `.umap` conserva in coda thumbnail e asset registry data, dove un path puo'
 *  sopravvivere a una migrazione di cartelle senza che nessuno lo segua piu'. Un rosso di questo
 *  gate e' quindi un **inizio di indagine**, e lo strumento che chiude la domanda e' il Reference
 *  Viewer dell'editor. */
import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { extractPackageRefs, findUntrackedRefs, findStaleExceptions, type AssetRefs } from './refs.ts';

/**
 * Le assenze **dichiarate**, con la ragione. Un prefisso entra qui per una decisione, mai per far
 * tacere il gate.
 *
 * - `/Game/FabAsset/` — i pack Paragon: ~48 GB fuori dal repository per scelta, e la ragione per cui
 *   `Content/**` e' ignorato con negazioni mirate. Il cook e' limitato a `/Game/RT`.
 */
const ALLOWED_PREFIXES = ['/Game/FabAsset/'];

/**
 * I residui **misurati uno per uno**, con la ragione. Non silenziano una famiglia: valgono per la
 * coppia asset + path, e `findStaleExceptions` li rende rossi il giorno in cui non servono piu'.
 */
const KNOWN_EXCEPTIONS = [
  {
    asset: 'Content/RT/Maps/Dev/L_DevSandbox/L_DevSandbox.umap',
    ref: '/Game/Maps/L_Prototype',
    // ⚠️ L'elenco fra parentesi e' una MISURA, non una memoria: si rifa' con l'estrattore di questo
    //    stesso gate, `extractPackageRefs(readFileSync(asset))`. Nel 2026-08-29 era scaduto — nominava
    //    `DA_Format_Scratch`, che #956 aveva sostituito con la fixture rigenerabile (#1657).
    why:
      `Metadato, non dipendenza — #1280. Il Reference Viewer non lo mostra fra le dipendenze del `
      + `livello (M_HexCell, DA_HexMap_Scratch_Basin, WBP_RT_ErrorModal e nient'altro); non ha mai `
      + `prodotto `
      + `un LoadError, a differenza del difetto gemello di #1069; e sta a 61394 byte su 67154, nella `
      + `coda del package dove vivono thumbnail e asset registry data. E' il path da cui la sandbox `
      + `fu duplicata, quando L_Prototype stava ancora in /Game/Maps/. Un Save non lo toglie: il `
      + `package non e' dirty, quindi Unreal non lo riscrive.`,
  },
];

const tracked = new Set(
  execFileSync('git', ['ls-files', 'Content/*.uasset', 'Content/*.umap'], { encoding: 'utf8' })
    .split('\n')
    .map((line) => line.trim())
    .filter(Boolean),
);

const assets: AssetRefs[] = [...tracked].map((asset) => ({
  asset,
  refs: extractPackageRefs(readFileSync(asset)),
}));

const findings = findUntrackedRefs(assets, tracked, ALLOWED_PREFIXES, KNOWN_EXCEPTIONS);
const stale = findStaleExceptions(assets, KNOWN_EXCEPTIONS);

for (const { asset, ref } of findings) {
  console.error(`${asset}\n  -> ${ref}  (nessun file versionato lo soddisfa)`);
}

for (const { asset, ref, why } of stale) {
  console.error(`ECCEZIONE MORTA: ${asset}
  -> ${ref} non c'e' piu'. Toglila da KNOWN_EXCEPTIONS.
  (era: ${why})`);
}

console.log(
  `${assets.length} asset versionati, ${findings.length} riferimenti non versionati` +
  ` (prefissi esclusi: ${ALLOWED_PREFIXES.join(', ')}; residui dichiarati: ${KNOWN_EXCEPTIONS.length}` +
  `${stale.length > 0 ? `, di cui ${stale.length} MORTI` : ''})`,
);

process.exit(findings.length === 0 && stale.length === 0 ? 0 : 1);
