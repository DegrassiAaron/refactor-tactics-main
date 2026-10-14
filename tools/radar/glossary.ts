/** Il confronto **glossario: documento ↔ C++** (`#3744`).
 *
 *  Il nome e l'effetto di ogni stato, superficie e riparo stanno in due posti, ed e' una scelta dichiarata: il
 *  gioco li legge dalle due tabelle di `RTGlossary.cpp`, l'autore li rilegge in
 *  `docs/balance/RT_Glossary_v0.1.md`. Due copie senza un gate divergono in silenzio. Questo e' il gate.
 *
 *  🔑 **Il parser e il confronto sono quelli delle frasi d'azione** (`action-descriptions.ts`): stessa forma di
 *  tabella C++, stessa forma di riga nel documento. Qui cambiano solo le sorgenti e le due ancore.
 *
 *  Confronta, per chiave: l'**effetto** (documento ↔ `Effects`), il **nome** (documento ↔ `Names`), la
 *  **presenza** (una voce senza riga, una riga senza voce, una riga doppia).
 *
 *  ⚠️ **Non prescrive quale lato correggere**: stampa i due valori e si ferma.
 *
 *  ⚠️ **Non dice se una voce NOMINA qualcosa che esiste**, ne' se manca uno stato: quello lo verificano i test
 *  `RefactorTactics.UI.Glossary*`, che vedono i tag registrati e gli enum. Un verde qui significa «le due copie
 *  concordano», non «il glossario e' completo».
 *
 *  🔴 **Il guasto temuto e' il falso verde.** Se un'ancora smettesse di trovare la sua tabella, due insiemi
 *  vuoti concorderebbero. Percio' zero voci da un lato e' un errore, e la copertura si stampa sempre.
 *
 *  Uso:  node tools/radar/glossary.ts [--check]
 */
import { fileURLToPath, pathToFileURL } from 'node:url';
import { compare, cppPairs, docRows } from './action-descriptions.ts';
import type { Divergenza } from './action-descriptions.ts';

/** 🔑 L'ancora comprende la fine del tipo di ritorno: corrisponde alla DEFINIZIONE, non a un commento che la nomina. */
const ANCORA_NOMI = '>& RTGlossary::Names()';
const ANCORA_EFFETTI = '>& RTGlossary::Effects()';

export interface Esito {
  righe: number;
  nomi: number;
  effetti: number;
  divergenze: Divergenza[];
}

/** Legge le due copie e le confronta. `doc` e' il documento, `cpp` il sorgente di `RTGlossary.cpp`. */
export function confronta(doc: string, cpp: string): Esito {
  const righe = docRows(doc);
  const nomi = cppPairs(cpp, ANCORA_NOMI);
  const effetti = cppPairs(cpp, ANCORA_EFFETTI);
  return { righe: righe.length, nomi: nomi.length, effetti: effetti.length, divergenze: compare(righe, effetti, nomi) };
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const { readFileSync } = await import('node:fs');
  const dallaRadice = (p: string) => readFileSync(fileURLToPath(new URL(`../../${p}`, import.meta.url)), 'utf8');

  const esito = confronta(
    dallaRadice('docs/balance/RT_Glossary_v0.1.md'),
    dallaRadice('Source/RefactorTactics/UI/RTGlossary.cpp'),
  );
  console.error(`copertura: ${esito.righe} righe nel documento · ${esito.nomi} nomi e ${esito.effetti} effetti in C++`);

  if (esito.righe === 0 || esito.nomi === 0 || esito.effetti === 0) {
    console.error(
      "errore: un lato non ha prodotto voci. Il parser non trova piu' la sua sorgente, e un confronto vuoto " +
        'sarebbe un falso verde.',
    );
    process.exit(1);
  }

  if (esito.divergenze.length > 0) {
    console.error(
      `errore: ${esito.divergenze.length} divergenze fra documento e C++. Nessuno dei due lati e' presunto giusto:\n` +
        esito.divergenze.map((d) => `  ${d.id}  [${d.campo}]\n    doc: ${d.doc}\n    cpp: ${d.cpp}`).join('\n'),
    );
    process.exit(1);
  }
  console.error('documento e C++ concordano su nomi ed effetti');
}
