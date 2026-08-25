/** Verifica che le tabelle Markdown dei documenti abbiano righe della stessa larghezza.
 *
 *  Uso:  node tools/radar/doc-tables.ts [--check]
 *
 *  Il problema che chiude: dal **2026-08-21** (D-182) `check-docs-tables.py` non esiste più, e con lui
 *  l'unico controllo che vedeva *«una riga vuota che spezza una tabella e una cella con una pipe non
 *  escapata»*. Il costo si è misurato il **2026-08-25**: sette righe rotte in `docs/`, di due tipi.
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
 *  ⚠️ **Cosa NON verifica**, dichiarato perché non venga scoperto dopo: che il separatore `|---|` abbia
 *  la stessa larghezza dell'intestazione (è un caso che il rendering perdona), l'allineamento delle
 *  colonne, e le tabelle HTML. Guarda solo le righe che cominciano con `|`. */
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

export function findBrokenRows(text: string): BrokenRow[] {
  const lines = text.split('\n');
  const out: BrokenRow[] = [];
  let i = 0;

  while (i < lines.length) {
    if (!lines[i]!.startsWith('|')) {
      i++;
      continue;
    }
    // Una tabella è un blocco di righe consecutive che cominciano con `|`.
    const start = i;
    while (i + 1 < lines.length && lines[i + 1]!.startsWith('|')) i++;
    const end = i;

    // La larghezza attesa è quella della MAGGIORANZA, non quella dell'intestazione: se a essere
    // sbagliata fosse l'intestazione, ancorarsi a lei segnalerebbe tutte le righe buone.
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

    // Una tabella di due righe sole non ha una maggioranza: due larghezze diverse sarebbero 1 a 1, e
    // dire quale sia quella giusta è indovinare. Si tace.
    if (end - start >= 2) {
      for (let j = start; j <= end; j++) {
        const n = countCells(lines[j]!);
        if (n !== expected) {
          out.push({ line: j + 1, cells: n, expected, text: lines[j]!.slice(0, 160) });
        }
      }
    }
    i = end + 1;
  }
  return out;
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
  let tables = 0;

  for (const file of files) {
    const rel = relative(REPO_ROOT, file).split(sep).join('/');
    const text = readFileSync(file, 'utf8');
    // Conta le tabelle per poter dichiarare la copertura, non solo i difetti.
    tables += text.split('\n').filter((l, k, a) => l.startsWith('|') && !(a[k - 1] ?? '').startsWith('|')).length;
    for (const b of findBrokenRows(text)) {
      problems.push(`${rel}:${b.line}: ${b.cells} celle invece di ${b.expected}\n      ${b.text}`);
    }
  }

  console.error(
    `tabelle esaminate: ${tables} in ${files.length} documenti` +
      (withArchive ? '' : ' (docs/archive/ escluso: passa --with-archive)'),
  );

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
