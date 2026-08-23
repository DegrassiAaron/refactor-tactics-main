/** Riferimenti fra package Unreal, letti dai byte invece che dall'editor.
 *
 *  Un `.uasset` non si diffa e non si legge a occhio, ma i package path vivono nella sua name table
 *  come stringhe ASCII: tanto basta per rispondere alla domanda che `#1069` ha lasciato aperta —
 *  **un asset versionato referenzia un file che git non ha?**
 *
 *  Il difetto che motiva il gate: `BP_GameMode.uasset` porta `DA_Format_Rotto`, un package **mai
 *  esistito in git**. Chi ha il file sul disco vede la partita fallire; chi clona non vede niente,
 *  perche' il riferimento penzolante risolve a null e il codice ripiega. Un difetto che dipende da un
 *  file non versionato non e' riproducibile da chi lo eredita — e nessun test lo vedeva. */

/** Un asset e i package che nomina. */
export type AssetRefs = { asset: string; refs: string[] };

/** Un riferimento che nessun file versionato soddisfa. */
export type Finding = { asset: string; ref: string };

/** Il package path che un file del repository occupa: `Content/X/Y.uasset` -> `/Game/X/Y`. */
export function toPackagePath(repoPath: string): string {
  return '/Game/' + repoPath.replace(/^Content\//, '').replace(/\.(uasset|umap)$/, '');
}

/** I due file che possono contenere un package: un livello e' `.umap`, tutto il resto `.uasset`. */
export function toContentPaths(ref: string): string[] {
  const base = 'Content/' + ref.replace(/^\/Game\//, '');
  return [base + '.uasset', base + '.umap'];
}

/** I package citati nei byte, nell'ordine di prima comparsa e senza ripetizioni. */
export function extractPackageRefs(bytes: Buffer): string[] {
  const found = bytes.toString('latin1').match(/\/Game\/[A-Za-z0-9_/]+/g) ?? [];
  const seen = new Set<string>();
  const out: string[] = [];
  for (const raw of found) {
    const ref = raw.replace(/\/+$/, '');
    // `/Game` da solo e' il mount point, non un asset.
    if (ref.split('/').length < 3 || seen.has(ref)) { continue; }
    seen.add(ref);
    out.push(ref);
  }
  return out;
}

/**
 * I riferimenti che non risolvono a un file versionato.
 *
 * `allowedPrefixes` sono le assenze **dichiarate**: un prefisso entra li' con la sua ragione scritta
 * accanto, non perche' il gate faceva rumore.
 */
export function findUntrackedRefs(
  assets: AssetRefs[],
  tracked: Set<string>,
  allowedPrefixes: string[],
): Finding[] {
  const findings: Finding[] = [];
  for (const { asset, refs } of assets) {
    const self = toPackagePath(asset);
    for (const ref of refs) {
      if (ref === self) { continue; }
      if (allowedPrefixes.some((prefix) => ref.startsWith(prefix))) { continue; }
      if (toContentPaths(ref).some((path) => tracked.has(path))) { continue; }
      findings.push({ asset, ref });
    }
  }
  return findings;
}
