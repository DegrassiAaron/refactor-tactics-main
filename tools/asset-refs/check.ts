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
 *  (`AGENTS.md`). Si esegue prima di committare un binario. */
import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { extractPackageRefs, findUntrackedRefs, type AssetRefs } from './refs.ts';

/**
 * Le assenze **dichiarate**, con la ragione. Un prefisso entra qui per una decisione, mai per far
 * tacere il gate.
 *
 * - `/Game/FabAsset/` — i pack Paragon: ~48 GB fuori dal repository per scelta, e la ragione per cui
 *   `Content/**` e' ignorato con negazioni mirate. Il cook e' limitato a `/Game/RT`.
 */
const ALLOWED_PREFIXES = ['/Game/FabAsset/'];

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

const findings = findUntrackedRefs(assets, tracked, ALLOWED_PREFIXES);

for (const { asset, ref } of findings) {
  console.error(`${asset}\n  -> ${ref}  (nessun file versionato lo soddisfa)`);
}

console.log(
  `${assets.length} asset versionati, ${findings.length} riferimenti non versionati` +
  ` (eccezioni dichiarate: ${ALLOWED_PREFIXES.join(', ')})`,
);

process.exit(findings.length === 0 ? 0 : 1);
