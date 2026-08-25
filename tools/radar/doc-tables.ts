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
 *  ⚠️ **Chi controlla cosa**: `docs/archive/` esce del tutto salvo `--with-archive`; `docs/research/`
 *  resta dentro per la **larghezza** ed esce dalle sole **orfane** — è input north-star non ancora
 *  consumato (CLAUDE.md), le sue orfane stanno tutte in un PRD importato, e i suoi difetti di larghezza
 *  sono zero, quindi toglierlo da entrambi perderebbe documenti in cambio di niente.
 *
 *  ⚠️ **Tre modi di uscire `1`**, non due: larghezza, riga fuori da ogni tabella, e **blocco di codice
 *  mai chiuso** — quest'ultimo perché un fence aperto rende la maschera inaffidabile, e tacere sarebbe
 *  peggio che dirlo.
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perché non venga scoperto dopo: che il separatore `|---|` abbia
 *  la stessa larghezza dell'intestazione (è un caso che il rendering perdona), l'allineamento delle
 *  colonne, e le tabelle HTML. **Segnala** solo righe che cominciano con `|` — una tabella dentro un
 *  blockquote (`> | … |`) non viene mai riportata — ma per decidere se una riga *appartiene* a una
 *  tabella legge anche le righe senza pipe di bordo, che in GFM sono valide. Ignora quanto sta dentro
 *  un blocco di codice: un documento che **cita** markdown non sta dichiarando una tabella. */
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

/** Il marcatore di un fence, se la riga ne apre o chiude uno: almeno tre backtick o tre tilde, fino a
 *  tre spazi di indentazione (CommonMark §4.5). Il carattere e la LUNGHEZZA contano — un fence si
 *  chiude solo con lo stesso carattere, lungo almeno quanto l'apertura.
 *
 *  ⚠️ **La info string di un fence a backtick non può contenere backtick** (CommonMark §4.5), ed è
 *  quello che distingue un'apertura da del codice inline: senza il controllo, una riga come
 *  ``` ```x``` | y ``` veniva letta come fence mai chiuso — il gate usciva `1` su un documento valido
 *  e la maschera si disattivava per tutto il file. */
function fenceMarker(line: string): string | null {
  // 🔴 **`[^\r\n]*` e non `.*`**: in JavaScript `\r` e' un *line terminator*, quindi `.` NON lo matcha e
  // `(.*)$` fallisce su ogni riga che finisce con CRLF. I documenti di questo repository sono
  // interamente CRLF, quindi la versione con `.*` non riconosceva **nessun** fence nei file veri — la
  // maschera era inerte ovunque, e i test non lo vedevano perche' le loro fixture usano `\n`.
  const m = /^ {0,3}(`{3,}|~{3,})([^\r\n]*)\r?$/.exec(line);
  if (!m) return null;
  const marker = m[1]!;
  if (marker[0] === '`' && m[2]!.includes('`')) return null;
  return marker;
}

/** L'inizio di un altro blocco, che in GFM CHIUDE una tabella tanto quanto una riga vuota: titolo ATX,
 *  linea orizzontale (`---`, `***`, `___`), citazione, elenco, o un fence.
 *
 *  ⚠️ Serve per fermare la discesa delle righe di dati. Una riga di prosa **non** chiude una tabella —
 *  diventa una riga di una cella — ma un `---` sì, ed e' proprio il caso in cui una voce di Decision
 *  Log staccata finisce sotto una linea orizzontale. */
function isBlockStart(line: string): boolean {
  const t = line.trim();
  return (
    /^#{1,6}(\s|$)/.test(t) || // titolo ATX
    /^(-{3,}|\*{3,}|_{3,})$/.test(t) || // linea orizzontale
    /^>/.test(t) || // citazione
    fenceMarker(line) !== null
  );
}

/** Le celle di una riga. Le pipe di bordo sono **opzionali** in GFM, quindi i frammenti ai bordi si
 *  scartano solo quando sono effettivamente vuoti. */
function cellsOf(line: string): string[] {
  const parts = line.split(CELL);
  if (parts.length >= 2 && parts[0]!.trim() === '') parts.shift();
  if (parts.length >= 1 && parts[parts.length - 1]!.trim() === '') parts.pop();
  return parts;
}

/** La riga separatrice di una tabella GFM.
 *
 *  ⚠️ **GFM non chiede tre trattini per cella**: la spec dice «celle il cui unico contenuto sono
 *  trattini e, opzionalmente, due punti». `| - | - |` e' quindi valido, e segnalarlo sarebbe un falso
 *  positivo. Cio' che NON e' valido e' una cella **vuota**: `|   |   |` non ha trattini ed e' una riga
 *  di dati, non un delimitatore — che e' il caso in cui il vecchio regex, permissivo su tutta la
 *  classe `[\s:|-]`, certificava come tabella due righe orfane. */
function isDelimiterRow(line: string): boolean {
  // 🔴 **Serve almeno una pipe**, ed è la sola cosa che separa un delimitatore da `---`, che in
  // Markdown è un titolo setext o una linea orizzontale. Senza questo controllo `---` sotto una riga
  // di prosa veniva letto come delimitatore, promuoveva quella riga a intestazione e assorbiva le
  // righe `|` sottostanti: il caso per cui questo controllo esiste — la voce di Decision Log staccata
  // dalla sua tabella — tornava invisibile.
  if (!CELL.test(line)) return false;
  const parts = cellsOf(line);
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
    const found = fenceMarker(lines[k]!);
    if (found !== null) {
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
    const found = fenceMarker(lines[k]!);
    if (found === null) continue;
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
    // L'intestazione e' la riga subito sopra: deve esserci, essere piena, e contenere una pipe —
    // senza, il delimitatore e' orfano quanto una riga qualsiasi.
    const header = k - 1;
    if (header < 0 || fenced[header] || lines[header]!.trim() === '' || !CELL.test(lines[header]!)) continue;
    // 🔴 **E deve avere lo STESSO numero di celle**: GFM lo dice esplicitamente — «the header row must
    // match the delimiter row in the number of cells. If not, a table will not be recognized» — e senza
    // il confronto un `| A | B | C |` sopra `|---|---|` passava per tabella mentre il markdown lo rende
    // come testo con le pipe a vista.
    if (cellsOf(lines[header]!).length !== cellsOf(lines[k]!).length) continue;
    inTable[header] = true;
    inTable[k] = true;
    // I dati scendono fino alla riga vuota **o all'inizio di un altro blocco**, che e' cio' che chiude
    // una tabella in GFM: «The table is broken at the first empty line, or beginning of another
    // block-level structure». Fermarsi alla prima riga senza pipe segnalava come orfana una riga che
    // sta in tabella — una riga di prosa dentro una tabella diventa una riga di UNA cella, non un
    // terminatore — ma non fermarsi affatto fa assorbire tutto cio' che segue un `---`.
    for (let j = k + 1; j < lines.length && !fenced[j] && lines[j]!.trim() !== '' && !isBlockStart(lines[j]!); j++) {
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
