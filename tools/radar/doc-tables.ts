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

/** Apertura o chiusura di un blocco di codice: almeno tre backtick o tre tilde, anche indentati.
 *  Il marcatore e la sua LUNGHEZZA contano: un fence si chiude solo con lo stesso carattere e almeno
 *  altrettanti caratteri (CommonMark §4.5). */
const FENCE = /^\s*(`{3,}|~{3,})/;

/** La riga separatrice di una tabella GFM.
 *
 *  ⚠️ **GFM non chiede tre trattini per cella**: la spec dice «celle il cui unico contenuto sono
 *  trattini e, opzionalmente, due punti». `| - | - |` e' quindi valido, e segnalarlo sarebbe un falso
 *  positivo. Cio' che NON e' valido e' una cella **vuota**: `|   |   |` non ha trattini ed e' una riga
 *  di dati, non un delimitatore — che e' il caso in cui il vecchio regex, permissivo su tutta la
 *  classe `[\s:|-]`, certificava come tabella due righe orfane. */
function isDelimiterRow(line: string): boolean {
  const parts = line.split(CELL);
  // I frammenti ai bordi sono vuoti quando la riga comincia o finisce con `|`: le pipe di bordo sono
  // opzionali in GFM, quindi si scartano solo se effettivamente vuoti.
  if (parts.length >= 2 && parts[0]!.trim() === '') parts.shift();
  if (parts.length >= 1 && parts[parts.length - 1]!.trim() === '') parts.pop();
  return parts.length > 0 && parts.every((c) => /^:?-+:?$/.test(c.trim()));
}

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
  let marker: string | null = null; // il fence APERTO: carattere e lunghezza

  for (let k = 0; k < lines.length; k++) {
    const m = FENCE.exec(lines[k]!);
    if (m) {
      const found = m[1]!;
      if (marker === null) {
        marker = found;
        mask[k] = true;
        continue;
      }
      // Chiude solo lo stesso carattere, lungo almeno quanto l'apertura. Senza questo controllo un
      // `~~~~ water ~~~~` dentro un blocco ```text inverte la maschera per tutto il resto del file —
      // e nel repository esiste gia' un documento fatto cosi'.
      if (found[0] === marker[0] && found.length >= marker.length) {
        marker = null;
        mask[k] = true;
        continue;
      }
    }
    mask[k] = marker !== null;
  }

  // ⚠️ **Un fence rimasto aperto rende la maschera inaffidabile da li' a fine file**, e il modo in cui
  // sbaglia e' il peggiore: SPEGNE i controlli invece di accenderli, quindi un backtick perso
  // basterebbe a far uscire `0` su un documento rotto. Non ci si fida: si torna a controllare tutto,
  // che e' il comportamento che questo file aveva prima di conoscere i fence. Lo sbilanciamento e'
  // segnalato a parte da `findUnbalancedFence`, cosi' non resta muto.
  if (marker !== null) return new Array<boolean>(lines.length).fill(false);
  return mask;
}

/** La riga che apre un fence mai chiuso, se c'e'. Un documento in questo stato non e' controllabile
 *  con sicurezza, ed e' un difetto in se': va detto, non aggirato. */
export function findUnbalancedFence(text: string): number | null {
  const lines = text.split('\n');
  let marker: string | null = null;
  let openedAt = -1;
  for (let k = 0; k < lines.length; k++) {
    const m = FENCE.exec(lines[k]!);
    if (!m) continue;
    const found = m[1]!;
    if (marker === null) {
      marker = found;
      openedAt = k;
    } else if (found[0] === marker[0] && found.length >= marker.length) {
      marker = null;
    }
  }
  return marker === null ? null : openedAt + 1;
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

  // ⚠️ **Si parte dal DELIMITATORE, non dalle righe con una pipe.** Cercare «tutte le righe che
  // contengono `|`» sembra la generalizzazione giusta — in GFM le pipe ai bordi sono opzionali — ed e'
  // stata misurata: **393 falsi positivi**, su prosa che cita `Attack | Ability` fra backtick e su ogni
  // tabella dentro un blockquote, dove le righe cominciano con `>`. Una tabella invece esiste **se e
  // solo se** ha una riga delimitatore: partendo di li' l'appartenenza si calcola senza indovinare, e
  // le tabelle senza pipe ai bordi rientrano da sole.
  const inTable = new Array<boolean>(lines.length).fill(false);
  for (let k = 0; k < lines.length; k++) {
    if (fenced[k] || !isDelimiterRow(lines[k]!)) continue;
    // L'intestazione e' la riga subito sopra, se c'e' ed e' piena: senza di lei il delimitatore e'
    // orfano quanto una riga qualsiasi.
    const header = k - 1;
    if (header < 0 || fenced[header] || lines[header]!.trim() === '') continue;
    inTable[header] = true;
    inTable[k] = true;
    // I dati scendono finche' le righe restano piene e contengono una pipe.
    for (let j = k + 1; j < lines.length && !fenced[j] && lines[j]!.trim() !== '' && CELL.test(lines[j]!); j++) {
      inTable[j] = true;
    }
  }

  // Orfana = comincia con `|` — quindi il markdown la renderebbe come riga di tabella — ma non
  // appartiene a nessuna. Il criterio resta stretto di proposito: allargarlo alle righe che contengono
  // una pipe **in mezzo** e' esattamente cio' che ha prodotto i 393.
  for (let k = 0; k < lines.length; k++) {
    if (!fenced[k] && lines[k]!.startsWith('|') && !inTable[k]) {
      out.push({ line: k + 1, text: lines[k]!.slice(0, 160) });
    }
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
      (f) => withArchive || (relative(DOCS_DIR, f).split(sep)[0] ?? '') !== 'archive',
    ),
  ];

  /** `research/` e' input north-star non ancora consumato (CLAUDE.md): non e' autorita' implicita, e le
   *  sue tabelle arrivano da documenti IMPORTATI. Le **dodici** righe orfane misurate stanno tutte li',
   *  in un solo PRD, e correggerle sarebbe cosmetica su materiale che il progetto non possiede.
   *
   *  ⚠️ **Esce dalle sole ORFANE, non dal controllo di larghezza**: li' i difetti misurati sono **zero**,
   *  quindi toglierlo dalla scansione perderebbe 38 documenti senza guadagnare niente — e la
   *  giustificazione qui sopra copre le orfane, non la larghezza. */
  const skipsOrphans = (f: string) => (relative(DOCS_DIR, f).split(sep)[0] ?? '') === 'research';

  const widthProblems: string[] = [];
  const orphanProblems: string[] = [];
  const fenceProblems: string[] = [];
  let tables = 0;

  for (const file of files) {
    const rel = relative(REPO_ROOT, file).split(sep).join('/');
    const text = readFileSync(file, 'utf8');
    // Conta le tabelle per poter dichiarare la copertura, non solo i difetti. Si contano i
    // **delimitatori**, perche' ogni tabella ne ha esattamente uno: contare gli inizi di blocco
    // includerebbe fra le «tabelle esaminate» proprio i blocchi che `findOrphanRows` sta dichiarando
    // NON essere tabelle, e il numero non si potrebbe leggere come «tabelle verificate».
    const lines = text.split('\n');
    const fenced = fencedMask(lines);
    tables += lines.filter(
      (l, k) => !fenced[k] && isDelimiterRow(l) && k > 0 && !fenced[k - 1] && lines[k - 1]!.trim() !== '',
    ).length;
    const openedAt = findUnbalancedFence(text);
    if (openedAt !== null) {
      fenceProblems.push(`${rel}:${openedAt}: blocco di codice aperto e mai chiuso`);
    }
    for (const b of findBrokenRows(text)) {
      widthProblems.push(`${rel}:${b.line}: ${b.cells} celle invece di ${b.expected}\n      ${b.text}`);
    }
    if (!skipsOrphans(file)) {
      for (const o of findOrphanRows(text)) {
        orphanProblems.push(`${rel}:${o.line}: non appartiene a nessuna tabella\n      ${o.text}`);
      }
    }
  }

  console.error(
    `tabelle esaminate: ${tables} in ${files.length} documenti` +
      (withArchive ? '' : ' (docs/archive/ escluso: passa --with-archive)') +
      ' · in docs/research/ si controlla la larghezza, non le righe orfane',
  );

  if (widthProblems.length + orphanProblems.length + fenceProblems.length === 0) {
    console.error('ogni riga sta in una tabella, e ha la larghezza delle sorelle');
    return;
  }

  // I tre difetti si correggono in modi diversi, quindi si stampano separati: un rimedio dato per il
  // difetto sbagliato manda chi legge a escapare una pipe quando gli manca un delimitatore.
  if (widthProblems.length > 0) {
    console.error(
      `\n${widthProblems.length} righe non hanno la larghezza delle sorelle:\n  ${widthProblems.join('\n  ')}`,
    );
    console.error(
      `\n⚠️ Due cause: una pipe **mancante** fonde due colonne, una pipe **in più** viene quasi sempre da` +
        ` codice inline con \`||\` o \`|\` non escapati — lì si scrive \`\\|\`.`,
    );
  }
  if (orphanProblems.length > 0) {
    console.error(
      `\n${orphanProblems.length} righe non appartengono a nessuna tabella:\n  ${orphanProblems.join('\n  ')}`,
    );
    console.error(
      `\n⚠️ Il markdown le rende come TESTO, pipe a vista. Di solito manca la riga \`|---|---|\`, oppure` +
        ` una riga vuota ha staccato la riga dalla sua tabella: si toglie la riga vuota.`,
    );
  }
  if (fenceProblems.length > 0) {
    console.error(`\n${fenceProblems.length} blocchi di codice non chiusi:\n  ${fenceProblems.join('\n  ')}`);
    console.error(
      `\n⚠️ Finché restano aperti il resto del documento non è controllabile con sicurezza, e questo` +
        ` controllo torna a esaminare tutto invece di fidarsi — quindi qui possono comparire esempi citati.`,
    );
  }
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
