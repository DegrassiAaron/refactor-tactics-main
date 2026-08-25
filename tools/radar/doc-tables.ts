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
 *  ➕ **Dal 2026-08-25 i controlli sono due**, e chiedono cose diverse: `findBrokenRows` chiede *«questa
 *  riga ha le celle delle sorelle?»*, `findOrphanRows` chiede *«questa riga ha delle sorelle?»*. Il
 *  secondo nasce da un difetto passato per verificato: una voce del Decision Log inserita **dopo** la
 *  riga vuota che chiudeva la tabella rendeva come testo con le pipe a vista, e `--check` usciva `0` —
 *  un blocco di una riga non ha sorelle con cui confrontare la larghezza, quindi il primo controllo
 *  taceva per costruzione.
 *
 *  ⚠️ **`docs/research/` è sempre escluso**, `docs/archive/` salvo `--with-archive`. Il primo è input
 *  north-star non ancora consumato (CLAUDE.md), e le **dodici** righe orfane misurate stanno tutte lì,
 *  in un solo PRD importato: farle contare significherebbe nascere rossi su materiale che il progetto
 *  non possiede e nessuno correggerà.
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perché non venga scoperto dopo: che il separatore `|---|` abbia
 *  la stessa larghezza dell'intestazione (è un caso che il rendering perdona), l'allineamento delle
 *  colonne, e le tabelle HTML. Guarda solo le righe che cominciano con `|`, e ignora quelle dentro un
 *  blocco di codice — un documento che **cita** markdown non sta dichiarando una tabella. */
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

/** Apertura o chiusura di un blocco di codice: ``` oppure ~~~, anche indentati. */
const FENCE = /^\s*(```|~~~)/;

/** La riga separatrice di una tabella GFM: `|---|---|`, con allineamenti opzionali (`|:--|--:|`). */
const DELIMITER = /^\|[\s:|-]+\|?\s*$/;

/** Quali righe stanno DENTRO un blocco di codice, e vanno ignorate da ogni controllo.
 *
 *  ⚠️ Un documento che **cita** markdown non sta dichiarando una tabella: senza questa maschera il
 *  controllo delle orfane nasce con cinque falsi positivi in `docs/`, tutti su esempi citati. Il
 *  commento in testa a questo file dice perche' e' grave — un gate che segnala il falso viene
 *  disattivato, e poi non segnala piu' nemmeno il vero.
 *
 *  Le righe di apertura e chiusura sono marcate anch'esse: non cominciano con `|`, quindi non
 *  cambierebbero nulla, ma marcarle rende la maschera leggibile da sola. */
function fencedMask(lines: string[]): boolean[] {
  const mask = new Array<boolean>(lines.length).fill(false);
  let open = false;
  for (let k = 0; k < lines.length; k++) {
    if (FENCE.test(lines[k]!)) {
      open = !open;
      mask[k] = true;
      continue;
    }
    mask[k] = open;
  }
  return mask;
}

export function findBrokenRows(text: string): BrokenRow[] {
  const lines = text.split('\n');
  const fenced = fencedMask(lines);
  const out: BrokenRow[] = [];
  let i = 0;

  while (i < lines.length) {
    if (!lines[i]!.startsWith('|') || fenced[i]) {
      i++;
      continue;
    }
    // Una tabella è un blocco di righe consecutive che cominciano con `|`.
    const start = i;
    while (i + 1 < lines.length && lines[i + 1]!.startsWith('|') && !fenced[i + 1]) i++;
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

/** Una riga che comincia con `|` ma non appartiene a nessuna tabella: il markdown la rende come
 *  testo letterale, pipe a vista. */
export interface OrphanRow {
  /** Riga nel documento, 1-based. */
  line: number;
  text: string;
}

/** Le righe `|` che non formano una tabella GFM.
 *
 *  Il problema che chiude: `findBrokenRows` confronta le righe di un blocco **fra loro**, quindi non
 *  ha nulla da dire su un blocco di una riga sola — e una riga di tabella isolata e' proprio il caso
 *  in cui il difetto e' totale, non parziale. E' successo il **2026-08-25**: una voce del Decision Log
 *  inserita dopo la riga vuota che chiudeva la tabella e' passata per verificata, con `--check` a `0`.
 *
 *  La regola e' una sola, e copre tre difetti diversi: **un blocco e' una tabella se ha almeno due
 *  righe e la SECONDA e' un delimitatore**. Cadono la riga isolata, l'intestazione staccata dal
 *  proprio `|---|` da una riga vuota, e il delimitatore rimasto solo.
 *
 *  ⚠️ **Non e' il controllo di larghezza con una soglia diversa**: quello chiede «questa riga ha le
 *  celle delle sorelle?», questo chiede «questa riga ha delle sorelle?». Tenerli separati e' anche il
 *  motivo per cui il primo puo' continuare a tacere sui blocchi corti, dove una maggioranza non
 *  esiste e sceglierla sarebbe indovinare. */
export function findOrphanRows(text: string): OrphanRow[] {
  const lines = text.split('\n');
  const fenced = fencedMask(lines);
  const out: OrphanRow[] = [];
  let i = 0;

  while (i < lines.length) {
    if (!lines[i]!.startsWith('|') || fenced[i]) {
      i++;
      continue;
    }
    const start = i;
    while (i + 1 < lines.length && lines[i + 1]!.startsWith('|') && !fenced[i + 1]) i++;
    const end = i;

    const isTable = end - start >= 1 && DELIMITER.test(lines[start + 1]!);
    if (!isTable) {
      for (let j = start; j <= end; j++) {
        out.push({ line: j + 1, text: lines[j]!.slice(0, 160) });
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
  // `archive/` e' storico, `research/` e' input north-star non ancora consumato: CLAUDE.md dichiara
  // che non e' autorita' implicita, e le sue tabelle arrivano da documenti IMPORTATI. Le dodici righe
  // orfane misurate il 2026-08-25 stanno tutte li', in un solo PRD — correggerle sarebbe cosmetica su
  // materiale che il progetto non possiede, e il gate nascerebbe rosso su qualcosa che nessuno
  // sistemera'.
  const SKIP = ['archive', 'research'];
  const files = [
    ...ROOT_DOCS.map((e) => join(REPO_ROOT, e)).filter((p) => existsSync(p)),
    ...markdownFiles(DOCS_DIR).filter((f) => {
      const top = relative(DOCS_DIR, f).split(sep)[0] ?? '';
      return withArchive ? top !== 'research' : !SKIP.includes(top);
    }),
  ];

  const problems: string[] = [];
  let tables = 0;

  for (const file of files) {
    const rel = relative(REPO_ROOT, file).split(sep).join('/');
    const text = readFileSync(file, 'utf8');
    // Conta le tabelle per poter dichiarare la copertura, non solo i difetti. Salta i blocchi dentro
    // un code fence: un esempio citato non e' una tabella del documento, e contarlo gonfierebbe una
    // copertura che nessun controllo esercita.
    const lines = text.split('\n');
    const fenced = fencedMask(lines);
    tables += lines.filter(
      (l, k) => l.startsWith('|') && !fenced[k] && !((lines[k - 1] ?? '').startsWith('|') && !fenced[k - 1]),
    ).length;
    for (const b of findBrokenRows(text)) {
      problems.push(`${rel}:${b.line}: ${b.cells} celle invece di ${b.expected}\n      ${b.text}`);
    }
    for (const o of findOrphanRows(text)) {
      problems.push(`${rel}:${o.line}: riga fuori da ogni tabella (manca il delimitatore)\n      ${o.text}`);
    }
  }

  console.error(
    `tabelle esaminate: ${tables} in ${files.length} documenti` +
      ` (docs/research/ sempre escluso${withArchive ? '' : ', docs/archive/ escluso: passa --with-archive'})`,
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
