/** Il confronto **frasi dei tooltip: documento ↔ C++** (`#3499`).
 *
 *  Le frasi d'autore delle azioni stanno in due posti, ed e' una scelta dichiarata: il gioco le legge dalla
 *  tabella C++ (`RTActionDescriptions.cpp`), l'autore le rilegge in `docs/balance/RT_ActionDescriptions_v0.1.md`.
 *  Due copie senza un gate divergono in silenzio, e chi rilegge il documento approverebbe un testo che il
 *  giocatore non vede. Questo e' il gate.
 *
 *  Confronta, per `ActionId`:
 *   - la **frase**: documento ↔ `RTActionDescriptions::All`;
 *   - il **nome**: documento ↔ `HeroActionDisplayName` e `GenericActionDisplayName`, perche' anche la colonna
 *     *Nome* e' una copia, e una copia senza gate e' il difetto che questo file esiste per chiudere;
 *   - la **presenza**: una frase senza riga, una riga senza frase, una riga doppia.
 *
 *  ⚠️ **Non prescrive quale lato correggere**, come `catalog-code.ts`: stampa i due valori e si ferma.
 *
 *  🔴 **Il guasto temuto e' il falso verde.** Se un parser smettesse di trovare voci — una funzione rinominata,
 *  un'intestazione cambiata — due insiemi vuoti concorderebbero. Percio' zero voci da un lato e' un errore, e la
 *  copertura si stampa sempre.
 *
 *  Uso:  node tools/radar/action-descriptions.ts [--check]
 */
import { fileURLToPath, pathToFileURL } from 'node:url';

export interface Voce {
  id: string;
  testo: string;
}

const COPPIA = /\{\s*TEXT\("([^"]+)"\),\s*TEXT\("((?:[^"\\]|\\.)*)"\)\s*\}/g;

/** Le coppie `{ TEXT("id"), TEXT("testo") }` del corpo che comincia con `inizio`, fino al primo `};`. */
export function cppPairs(text: string, inizio: string): Voce[] {
  const i = text.indexOf(inizio);
  if (i === -1) {
    return [];
  }
  const j = text.indexOf('};', i);
  const corpo = text.slice(i, j === -1 ? undefined : j);
  return [...corpo.matchAll(COPPIA)].map((m) => ({ id: m[1], testo: m[2].replace(/\\(.)/g, '$1') }));
}

export interface RigaDoc {
  id: string;
  nome: string;
  frase: string;
  line: number;
}

/** Le righe `| `Id` | Nome | Frase | … |` del documento. */
export function docRows(doc: string): RigaDoc[] {
  const out: RigaDoc[] = [];
  doc.split('\n').forEach((raw, i) => {
    const m = /^\|\s*`([^`]+)`\s*\|([^|]*)\|([^|]*)\|/.exec(raw.replace(/\r$/, ''));
    if (m) {
      out.push({ id: m[1], nome: m[2].trim(), frase: m[3].trim(), line: i + 1 });
    }
  });
  return out;
}

export interface Divergenza {
  id: string;
  campo: 'frase' | 'nome' | 'presenza';
  doc: string;
  cpp: string;
}

export function compare(righe: RigaDoc[], frasi: Voce[], nomi: Voce[]): Divergenza[] {
  const out: Divergenza[] = [];
  const perFrase = new Map(frasi.map((v) => [v.id, v.testo]));
  const perNome = new Map(nomi.map((v) => [v.id, v.testo]));
  const viste = new Set<string>();
  for (const r of righe) {
    if (viste.has(r.id)) {
      out.push({ id: r.id, campo: 'presenza', doc: `riga doppia (L${r.line})`, cpp: '-' });
      continue;
    }
    viste.add(r.id);
    const frase = perFrase.get(r.id);
    if (frase === undefined) {
      out.push({ id: r.id, campo: 'presenza', doc: `riga L${r.line}`, cpp: 'nessuna frase' });
      continue;
    }
    if (frase !== r.frase) {
      out.push({ id: r.id, campo: 'frase', doc: r.frase, cpp: frase });
    }
    const nome = perNome.get(r.id);
    if (nome !== r.nome) {
      out.push({ id: r.id, campo: 'nome', doc: r.nome, cpp: nome ?? 'nessun nome' });
    }
  }
  for (const v of frasi) {
    if (!viste.has(v.id)) {
      out.push({ id: v.id, campo: 'presenza', doc: 'nessuna riga', cpp: v.testo });
    }
  }
  return out;
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const { readFileSync } = await import('node:fs');
  const dallaRadice = (p: string) => readFileSync(fileURLToPath(new URL(`../../${p}`, import.meta.url)), 'utf8');

  const righe = docRows(dallaRadice('docs/balance/RT_ActionDescriptions_v0.1.md'));
  const frasi = cppPairs(dallaRadice('Source/RefactorTactics/Ability/RTActionDescriptions.cpp'), 'RTActionDescriptions::All()');
  const nomi = [
    ...cppPairs(dallaRadice('Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp'), 'FText HeroActionDisplayName('),
    ...cppPairs(dallaRadice('Source/RefactorTactics/Ability/RTCatalogLibrary.cpp'), 'FText GenericActionDisplayName('),
  ];
  console.error(`copertura: ${righe.length} righe nel documento · ${frasi.length} frasi e ${nomi.length} nomi in C++`);

  if (righe.length === 0 || frasi.length === 0 || nomi.length === 0) {
    console.error(
      "errore: un lato non ha prodotto voci. Il parser non trova piu' la sua sorgente, e un confronto vuoto " +
        'sarebbe un falso verde.',
    );
    process.exit(1);
  }

  const divergenze = compare(righe, frasi, nomi);
  if (divergenze.length > 0) {
    console.error(
      `errore: ${divergenze.length} divergenze fra documento e C++. Nessuno dei due lati e' presunto giusto:\n` +
        divergenze.map((d) => `  ${d.id}  [${d.campo}]\n    doc: ${d.doc}\n    cpp: ${d.cpp}`).join('\n'),
    );
    process.exit(1);
  }
  console.error('documento e C++ concordano su frasi e nomi');
}
