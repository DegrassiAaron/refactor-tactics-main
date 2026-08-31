/** Verifica che le tabelle Markdown dei documenti abbiano righe della stessa larghezza.
 *
 *  Uso:  node tools/radar/doc-tables.ts [--check] [--with-archive]
 *
 *  Il problema che chiude, ed è **metà** di quello che l'annuncio prometteva: dal **2026-08-21** (D-182)
 *  `check-docs-tables.py` non esiste più, e con lui l'unico controllo che vedeva *«una riga vuota che
 *  spezza una tabella e una cella con una pipe non escapata»*. Qui rientra **solo la seconda metà** — la
 *  riga staccata dalla sua tabella resta scoperta, e sta scritto sotto in «Cosa NON verifica».
 *  Il costo si è misurato il **2026-08-25**: **otto** righe rotte in cinque documenti di `docs/`, di due
 *  tipi. (Otto, non sette: contate su `5873aff0` con `git show -- docs/ AGENTS.md | grep -cE '^-\|'`.)
 *
 *  Una riga con **meno** celle perde una colonna in rendering — la successiva si fonde con la
 *  precedente. Una con **più** celle le sposta tutte, e succede quando un frammento di codice inline
 *  contiene una pipe non escapata: `if (!IsValid(X) || !X->IsAlive())` porta un `||` che il Markdown
 *  legge come **due separatori**, e la riga esplode.
 *
 *  ⚠️ **Una pipe preceduta da backslash NON è un separatore**, ed è la prima cosa che un contatore
 *  ingenuo sbaglia: `Attack \| Ability \| Overwatch` è testo, non tre celle. Il falso positivo che ne
 *  segue costa credibilità al gate, che è il modo noto in cui un gate viene disattivato.
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perché non venga scoperto dopo — e **misurato**, non dedotto:
 *
 *  - **La riga staccata** dalla sua tabella da una riga vuota (D-192, D-193): un blocco di meno di tre
 *    righe non ha una maggioranza, quindi si tace. Non è un caso di scuola — sul corpus di oggi sono
 *    **18** blocchi non confrontati, e ci sono difetti veri dentro: quattro tabelle sole in
 *    `docs/research/prd/prd-personaggi-azioni-e-bilanciamento.md`. Per questo la riga di copertura
 *    stampa i blocchi **confrontati** e dichiara a parte quelli scartati.
 *  - **Una riga indentata di 1–3 spazi**, che GFM accetta come riga di tabella: qui non comincia con
 *    `|`, quindi spezza il blocco in frammenti che la regola delle tre righe scarta — e con essi
 *    **l'intera tabella** smette di essere confrontata, in silenzio.
 *  - **Il difetto maggioritario**: la larghezza attesa è quella della maggioranza, quindi tre righe
 *    rotte su cinque fanno segnalare le due sane e tacere sulle tre.
 *  - **L'allineamento** delle colonne e le **tabelle HTML**. Guarda solo le righe che cominciano con `|`.
 *  - **Le tabelle dentro un blockquote**, cioè le righe che cominciano con `> |`. Segue dal bullet qui
 *    sopra — si guardano solo le righe che aprono con `|` — ma va detto per esteso perché sono **68**
 *    separatori `> |---|` in `docs/` (contati il 2026-08-31 con
 *    `grep -rh --include='*.md' '^> |[- :|]*|$' docs/ | wc -l`), non un caso di scuola. ⚠️ **Verificato
 *    per mutazione, non dedotto**: tolta una cella a una riga di una tabella in blockquote di
 *    `test-manuali-pie.md`, `--check` è rimasto **verde** e il totale confrontato non si è mosso.
 *
 *  ⚠️ **Falso positivo noto**: la pipe finale, che GFM rende **facoltativa**. `| q | r | s` conta due
 *  celle invece di tre. Oggi non capita nel corpus — le righe che cominciano con `|` senza finirci
 *  chiudono tutte con un commento HTML dopo l'ultima pipe — ma la prima scritta senza nasce rossa.
 *
 *  ➕ **Il separatore `|---|` invece È verificato**, al contrario di quanto dichiarava la prima stesura
 *  di questo docstring: sta nel conteggio e viene riportato. Ed è giusto che lo sia — GFM richiede che
 *  il delimitatore abbia le colonne dell'intestazione, altrimenti il blocco non rende come tabella. */
import { readFileSync, readdirSync, statSync, existsSync } from 'node:fs';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { join, relative, sep } from 'node:path';

const REPO_ROOT = fileURLToPath(new URL('../../', import.meta.url));
const DOCS_DIR = fileURLToPath(new URL('../../docs/', import.meta.url));

/** Una riga di tabella che non ha lo stesso numero di celle delle sue sorelle. */
export interface BrokenRow {
  /** Riga nel documento, 1-based. */
  line: number;
  cells: number;
  /** Quante ne hanno le altre righe della stessa tabella. */
  expected: number;
  text: string;
}

/** Separatore di cella: una pipe **non** preceduta da backslash. */
const CELL = /(?<!\\)\|/;

/** Le celle VERE: `split` produce anche i due frammenti vuoti ai bordi (`| a | b |` -> `['', a, b, '']`),
 *  e riportarli come celle darebbe a chi legge un numero che non corrisponde a cio' che vede. */
function countCells(line: string): number {
  const parts = line.split(CELL);
  return Math.max(0, parts.length - 2);
}

/** Una tabella è un blocco di righe consecutive che cominciano con `|`, estremi **inclusi**.
 *
 *  Esportata perché chi conta la copertura e chi confronta le larghezze devono vedere **le stesse**
 *  tabelle: dichiararne 1539 esaminate e confrontarne 1521 è il modo silenzioso in cui una copertura si
 *  gonfia, e la differenza — i blocchi troppo corti — è esattamente ciò che il controllo non sa fare. */
export function tableBlocks(lines: string[]): Array<{ start: number; end: number }> {
  const out: Array<{ start: number; end: number }> = [];
  let i = 0;
  while (i < lines.length) {
    if (!lines[i]!.startsWith('|')) {
      i++;
      continue;
    }
    const start = i;
    while (i + 1 < lines.length && lines[i + 1]!.startsWith('|')) i++;
    out.push({ start, end: i });
    i++;
  }
  return out;
}

/** Una tabella di due righe sole non ha una maggioranza: due larghezze diverse sarebbero 1 a 1, e dire
 *  quale sia quella giusta è indovinare. Si tace — ed è il limite dichiarato nel docstring. */
export function isComparable(block: { start: number; end: number }): boolean {
  return block.end - block.start >= 2;
}

function brokenInLines(lines: string[]): BrokenRow[] {
  const out: BrokenRow[] = [];

  for (const block of tableBlocks(lines)) {
    if (!isComparable(block)) continue;
    const { start, end } = block;

    // La larghezza attesa è quella della MAGGIORANZA, non quella dell'intestazione: se a essere
    // sbagliata fosse l'intestazione, ancorarsi a lei segnalerebbe tutte le righe buone. Il prezzo è
    // il caso opposto — un difetto maggioritario fa segnalare le righe sane — e sta nel docstring.
    const tally = new Map<number, number>();
    for (let j = start; j <= end; j++) {
      const n = countCells(lines[j]!);
      tally.set(n, (tally.get(n) ?? 0) + 1);
    }
    let expected = 0;
    let best = -1;
    for (const [n, k] of tally) {
      if (k > best) {
        best = k;
        expected = n;
      }
    }

    for (let j = start; j <= end; j++) {
      const n = countCells(lines[j]!);
      if (n !== expected) {
        out.push({ line: j + 1, cells: n, expected, text: lines[j]!.slice(0, 160) });
      }
    }
  }
  return out;
}

export function findBrokenRows(text: string): BrokenRow[] {
  return brokenInLines(text.split('\n'));
}

// ---------------------------------------------------------------------------------------------
// Comando
// ---------------------------------------------------------------------------------------------

function markdownFiles(root: string): string[] {
  const out: string[] = [];
  const walk = (dir: string) => {
    for (const entry of readdirSync(dir).sort()) {
      const full = join(dir, entry);
      if (statSync(full).isDirectory()) walk(full);
      else if (entry.endsWith('.md')) out.push(full);
    }
  };
  walk(root);
  return out;
}

function main() {
  const argv = process.argv.slice(2);
  const check = argv.includes('--check');
  const withArchive = argv.includes('--with-archive');

  // Stessi tre documenti di governance della radice che copre `doc-links.ts`, e per la stessa ragione.
  const ROOT_DOCS = ['AGENTS.md', 'CLAUDE.md', 'README.md'];
  const files = [
    ...ROOT_DOCS.map((e) => join(REPO_ROOT, e)).filter((p) => existsSync(p)),
    ...markdownFiles(DOCS_DIR).filter(
      (f) => withArchive || !relative(DOCS_DIR, f).startsWith('archive'),
    ),
  ];

  const problems: string[] = [];
  let compared = 0;
  let skipped = 0;

  for (const file of files) {
    const rel = relative(REPO_ROOT, file).split(sep).join('/');
    const lines = readFileSync(file, 'utf8').split('\n');
    // La copertura si dichiara sui blocchi CONFRONTATI: contare anche quelli troppo corti direbbe di
    // aver guardato tabelle che il controllo scarta, ed è un numero che nessuno può riprodurre.
    for (const block of tableBlocks(lines)) {
      if (isComparable(block)) compared++;
      else skipped++;
    }
    for (const b of brokenInLines(lines)) {
      problems.push(`${rel}:${b.line}: ${b.cells} celle invece di ${b.expected}\n      ${b.text}`);
    }
  }

  console.error(
    `tabelle confrontate: ${compared} in ${files.length} documenti` +
      (withArchive ? '' : ' (docs/archive/ escluso: passa --with-archive)'),
  );
  if (skipped > 0) {
    console.error(
      `⚠️ ${skipped} blocchi di meno di tre righe NON sono confrontati: senza maggioranza il controllo` +
        ` tace, ed è il caso della riga staccata dalla sua tabella (D-193). Sono nel docstring.`,
    );
  }

  if (problems.length === 0) {
    console.error('tutte le righe di tabella hanno la larghezza delle sorelle');
    return;
  }
  console.error(`\n${problems.length} righe rompono la loro tabella:\n  ${problems.join('\n  ')}`);
  console.error(
    `\n⚠️ Due cause, e si correggono diversamente: una pipe **mancante** fonde due colonne, una pipe **in` +
      ` più** viene quasi sempre da codice inline con \`||\` o \`|\` non escapati — lì si scrive \`\\|\`.`,
  );
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
