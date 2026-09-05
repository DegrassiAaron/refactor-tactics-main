/** Percorso ricorsivo di una cartella, ordinato e stabile.
 *
 *  Esisteva gia' **tre volte** in `tools/radar/` — `doc-links.ts`, `doc-tables.ts`, `wiki-alt.ts` —
 *  identico a meno dell'estensione cercata. `scenario-notes.ts` sarebbe stata la quarta copia, e
 *  CLAUDE.md §7 dice `SEARCH -> REUSE / UPDATE -> CREATE solo per gap reale`.
 *
 *  ⚠️ **Le tre copie preesistenti NON sono state migrate qui**, ed e' una scelta dichiarata: hanno i
 *  propri test e migrarle e' un refactor, non la correzione di note che questo commit sta facendo.
 *  Chi le tocca dopo trova il posto pronto.
 *
 *  ⛔ **Non segue i symlink** e non si difende da un ciclo: `Scenarios/` e `docs/` non ne hanno, e
 *  fingere una difesa non provata sarebbe peggio che dichiararne l'assenza.
 */
import { readdirSync, statSync } from 'node:fs';
import { join } from 'node:path';

export function filesUnder(root: string, ext: string): string[] {
  const out: string[] = [];
  for (const nome of readdirSync(root).sort()) {
    const p = join(root, nome);
    if (statSync(p).isDirectory()) out.push(...filesUnder(p, ext));
    else if (nome.endsWith(ext)) out.push(p);
  }
  return out;
}
