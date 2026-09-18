/** Il confronto **catalogo azioni ↔ C++**, che fino al 2026-09-18 non esisteva.
 *
 *  `D-023` rende i cataloghi Markdown l'autorita' dei numeri competitivi, e `catalog-code.ts` verifica che
 *  quell'autorita' non diverga dal codice — ma copriva una fetta sola, gli eroi, e lo dichiarava di se'.
 *  Le **azioni** non erano coperte da niente: il catalogo porta righe `Action.*` in sei tabelle e nessun
 *  comando sapeva dire se coincidessero col codice (#2578).
 *
 *  Il precedente ha un prezzo noto: il 2026-08-10 `D-075` porto' `Bastion.PushResistance` a `0` nel codice
 *  e il catalogo continuo' a dichiarare `1` per due giorni, in cinque punti, senza che nessun gate
 *  diventasse rosso. Per le azioni si era ancora in quello stato.
 *
 *  ⚠️ **Non prescrive quale lato correggere**, ed e' la scelta piu' importante che eredita da
 *  `catalog-code.ts`: se quel gate fosse esistito il 2026-08-10 avrebbe segnalato `codice 0 ≠ catalogo 1`,
 *  ma il codice aveva ragione e il documento era indietro. Stampa i due valori e si ferma.
 *
 *  ⛔ **Cosa NON confronta, dichiarato perche' non venga scoperto dopo:**
 *
 *   1. la **prosa** degli effetti e del targeting. Il catalogo li descrive a parole («cono da facing»,
 *      «anti-spinta»), e un gate che le parsasse sarebbe rumoroso — e un gate che si ignora e' peggio di
 *      un gate assente;
 *   2. il **range quando non e' un intero**: il catalogo vi scrive anche budget in punti movimento
 *      (`5 MP`), `arma`, `self`. Dove i due lati non sono entrambi numeri il campo non e' confrontabile,
 *      e viene **contato a parte** invece che taciuto;
 *   3. gli altri due cataloghi — equipaggiamento e terreni — che sono una fetta separata, stesso schema;
 *   4. il **rumore** (`D-041`): `FRTActionDef` non lo porta, quindi non c'e' un secondo valore da
 *      confrontare.
 *
 *  🔴 **Il modo di guasto temuto e' il falso verde, non il falso positivo.** Se un literal diventasse una
 *  costante nominata, o l'intestazione di una tabella cambiasse, il parser non troverebbe piu' nulla e il
 *  gate resterebbe verde **proprio quando servirebbe**. Percio' ogni lato dichiara una copertura
 *  **derivata**: le celle promesse dalle intestazioni del catalogo e le chiamate `ShippedAction(` del C++
 *  si contano strutturalmente, e i campi letti si confrontano con quel conto. Nessuno dei due attesi e'
 *  scritto a mano. */

/** I fatti di un'azione, nel vocabolario del **C++**: il catalogo Markdown viene tradotto in questo, non
 *  viceversa, perche' l'enum e' l'elenco chiuso e le etichette italiane sono la vista. */
export interface ActionFacts {
  /** Valore di `ERTResolutionPhase`. */
  phase?: string;
  priority?: number;
  /** `RangeCells`, solo quando **entrambi** i lati sono numeri. */
  range?: number;
  cooldown?: number;
  /** Valore di `ERTActionFallback`. */
  fallback?: string;
  /** Valore di `ERTActionSlot`. */
  slot?: string;
  /** `true` se l'azione e' interrompibile, cioe' se `InterruptPolicy` non e' `None`.
   *
   *  ⚠️ Booleano e non il nome dell'enum: il catalogo scrive `sì`/`no`, e pretendere che dichiari *quale*
   *  politica gli darebbe una colonna che non ha. Il confronto e' su cio' che entrambi i lati sanno dire. */
  interruptible?: boolean;
}

export const ACTION_FIELDS: (keyof ActionFacts)[] = [
  'phase',
  'priority',
  'range',
  'cooldown',
  'fallback',
  'slot',
  'interruptible',
];

/** Le colonne del catalogo che portano un fatto confrontabile. Le tabelle delle sei sezioni **non hanno le
 *  stesse colonne** — §3 non ha `Slot`, §4 mette `Effetto` dove §1 ha `Interr.` — quindi l'intestazione e'
 *  l'unica cosa che dice quale cella e' quale, e l'atteso di copertura si deriva da li'. */
const COLUMNS: Record<string, keyof ActionFacts> = {
  'Cod.': 'phase',
  Prio: 'priority',
  Range: 'range',
  Distanza: 'range',
  CD: 'cooldown',
  Fallback: 'fallback',
  Slot: 'slot',
  'Interr.': 'interruptible',
};

/** Il **codice di fase** del catalogo (`Cod.`) e' cio' che corrisponde a `ERTResolutionPhase`.
 *
 *  🔴 **La colonna `Macro-fase` NON e' questo, e confonderle costa dodici falsi positivi — misurati.** La
 *  prima stesura di questo gate confrontava la macro-fase, e dichiarava divergenti `Counter`, `Intercept`,
 *  `Deflect`, `Cleanse`, `Push`, `Pull`, `Root`, `Interrupt`, `Slow`, `CreateWater`, `Ignite` ed
 *  `Electrify`: tutte righe in cui il catalogo scrive `Blast` e il C++ dice `Control` o `Environment`. Non
 *  divergevano. `Macro-fase` e' la fase di **Atlas** — `Prep · Dash · Blast · Move · Cleanup`, §Come si
 *  legge — e `Blast` vi raccoglie controllo, attacco e ambiente. Un gate rumoroso e' un gate che si
 *  disattiva, ed era esattamente il modo.
 *
 *  ⚠️ **Il codice `20` si sdoppia** (§Come si legge, [ADR-0003] §3): mobilita' rapida in `Dash`, percorso
 *  normale in `Move`. E' l'unico punto in cui la macro-fase serve, e serve come **disambiguatore**. */
const PHASE_BY_CODE: Record<string, string> = {
  '0': 'Snapshot',
  '10': 'Preparation',
  '30': 'Control',
  '40': 'Attack',
  '50': 'Environment',
  '60': 'Cleanup',
};

/** Le due fasi in cui il codice `20` si sdoppia, per macro-fase dichiarata. */
const MOVEMENT_BY_MACRO: Record<string, string> = {
  Dash: 'FastMovement',
  Move: 'NormalMovement',
};

const SLOTS: Record<string, string> = {
  Principale: 'Main',
  Movimento: 'Movement',
  Reazione: 'Reaction',
  'Movimento e principale': 'MovementAndMain',
};

/** Una cella che dichiara **assenza**: `—` e i suoi parenti. Non e' un valore da confrontare e non e' un
 *  difetto del parser: viene contata a parte. */
function isAbsent(cell: string): boolean {
  return cell === '' || cell === '—' || cell === '-' || cell === '–';
}

/** Toglie alla cella il markup che non porta significato — grassetto, corsivo, backtick, barrato, emoji e
 *  le note fra parentesi — e lascia il valore. `**Prep** *(arma)*` diventa `Prep`. */
function plain(cell: string): string {
  return cell
    .replace(/~~([^~]*)~~/g, '$1')
    .replace(/\*\*/g, '')
    .replace(/\*\(([^)]*)\)\*/g, '')
    .replace(/\([^)]*\)/g, '')
    .replace(/[`*]/g, '')
    .replace(/[\u{1F300}-\u{1FAFF}\u{2600}-\u{27BF}\u{FE0F}]/gu, '')
    .trim();
}

/** Quello che una riga del catalogo dichiara, piu' cio' che non si e' potuto leggere. */
export interface CatalogRow {
  actionId: string;
  /** Riga nel documento, 1-based: il referto la cita, cosi' non si cerca a mano. */
  line: number;
  facts: ActionFacts;
  /** Le fasi che il codice dichiara — piu' d'una quando la cella ne porta due (`30/40`, `40/50`), cioe'
   *  quando l'azione attraversa due fasi. Il C++ ne sceglie una, e il confronto verifica che sia **fra
   *  queste**: pretendere l'uguaglianza segnalerebbe come divergenza una scelta che il catalogo ammette. */
  phases: string[];
  /** La riga barra il nome dell'azione (`~~Attiva~~`): il catalogo la dichiara **ritirata**, e il C++ che
   *  non la costruisce e' d'accordo, non in ritardo. */
  retired: boolean;
  /** Celle promesse dall'intestazione ma **non confrontabili**: prosa, unita', o `—`. */
  unreadable: { field: keyof ActionFacts; cell: string }[];
}

/** Le righe delle tabelle del catalogo, lette **per intestazione**.
 *
 *  ⚠️ Un `ActionId` puo' comparire in **due** tabelle — `Action.Brace` sta fra le generiche §1 e fra le
 *  difensive §4 — e le due righe non hanno le stesse colonne. Si tengono entrambe: se dichiarassero valori
 *  diversi per lo stesso campo, il confronto le vedrebbe divergere **ciascuna** dal C++, che e' cio' che un
 *  gate deve dire invece di scegliere in silenzio quale riga vale. */
export function parseActionCatalog(text: string): CatalogRow[] {
  const rows: CatalogRow[] = [];
  let header: string[] | null = null;

  text.split('\n').forEach((line, i) => {
    if (/^\|\s*ActionId\s*\|/.test(line)) {
      header = line.split('|').map((c) => c.trim());
      return;
    }
    if (!header || !/^\|\s*`Action\./.test(line)) return;

    const cells = line.split('|').map((c) => c.trim());
    const facts: ActionFacts = {};
    const phases: string[] = [];
    const unreadable: CatalogRow['unreadable'] = [];

    /** La macro-fase non e' un campo confrontabile: serve solo a sdoppiare il codice `20`. */
    const macro = plain(cells[header.indexOf('Macro-fase')] ?? '').split(/\s+/)[0] ?? '';

    header.forEach((name, col) => {
      const field = COLUMNS[name];
      if (!field || col >= cells.length) return;
      const raw = cells[col]!;
      const cell = plain(raw);
      if (isAbsent(cell)) {
        unreadable.push({ field, cell: raw });
        return;
      }
      switch (field) {
        case 'priority':
        case 'cooldown':
        case 'range': {
          if (/^-?\d+$/.test(cell)) facts[field] = Number(cell);
          else unreadable.push({ field, cell: raw });
          break;
        }
        case 'phase': {
          // `30` · `30/40` · `40/50`: ogni codice e' una fase, e una cella puo' portarne due.
          //
          // ⚠️ Un codice `20` accanto a una macro-fase che non lo riguarda — `Action.SuppressiveLine`
          // dichiara `10/20` e macro-fase `Prep` — non si sdoppia: quel codice resta **non risolto** e gli
          // altri si leggono lo stesso. Buttare via tutta la cella perderebbe il confronto sul `10`, che
          // il catalogo dichiara senza ambiguita'.
          const codes = cell.split('/').map((c) => c.trim());
          const read = codes
            .map((code) => (code === '20' ? MOVEMENT_BY_MACRO[macro] : PHASE_BY_CODE[code]))
            .filter((p): p is string => p !== undefined);
          if (read.length > 0) phases.push(...read);
          else unreadable.push({ field, cell: raw });
          break;
        }
        case 'slot': {
          const slot = SLOTS[cell];
          if (slot) facts.slot = slot;
          else unreadable.push({ field, cell: raw });
          break;
        }
        case 'fallback': {
          const m = cell.match(/^Fallback\.(\w+)$/);
          if (m) facts.fallback = m[1];
          else unreadable.push({ field, cell: raw });
          break;
        }
        case 'interruptible': {
          const v = cell.toLowerCase();
          if (v === 'sì' || v === 'si') facts.interruptible = true;
          else if (v === 'no') facts.interruptible = false;
          else unreadable.push({ field, cell: raw });
          break;
        }
      }
    });

    // L'ID puo' portare una nota — `` `Action.Sprint` *(vedi §2.1)* `` — che non fa parte del nome.
    rows.push({
      actionId: plain(cells[1]!),
      line: i + 1,
      facts,
      phases,
      retired: /~~/.test(cells[2] ?? ''),
      unreadable,
    });
  });

  return rows;
}

/** Le colonne che devono **sempre** portare un valore leggibile: un codice di fase, una priorita' e un
 *  cooldown sono numeri per definizione, e il catalogo non vi scrive prosa.
 *
 *  🔑 **Sono queste a fare da soglia**, e non tutte le colonne confrontabili: `Range` porta legittimamente
 *  `5 MP` o `arma`, e `Fallback`/`Slot`/`Interr.` portano `—` dove l'azione non li ha. Pretendere anche
 *  quelle renderebbe il gate rosso su una scrittura corretta — cioe' rumoroso — mentre lasciarle tutte
 *  fuori dalla soglia renderebbe il gate cieco al caso che teme: un numero che diventa una costante o
 *  un'etichetta, e smette di essere confrontato **senza che nessuno lo dica**. */
const STRICT_COLUMNS = ['Cod.', 'Prio', 'CD'];

/** Quante celle le intestazioni promettono, per ogni riga: `all` sono tutte le colonne confrontabili,
 *  `strict` solo quelle che devono sempre essere leggibili.
 *
 *  Nessuno dei due e' scritto a mano: se una tabella perdesse la colonna `Prio`, l'atteso scenderebbe con
 *  essa — ed e' proprio il caso in cui il gate deve parlare, perche' il catalogo ha smesso di dichiarare un
 *  numero che il codice continua a portare. */
export function promisedCells(text: string): { all: number; strict: number } {
  let header: string[] | null = null;
  let all = 0;
  let strict = 0;
  for (const line of text.split('\n')) {
    if (/^\|\s*ActionId\s*\|/.test(line)) {
      header = line.split('|').map((c) => c.trim());
      continue;
    }
    if (!header || !/^\|\s*`Action\./.test(line)) continue;
    all += header.filter((name) => COLUMNS[name] !== undefined).length;
    strict += header.filter((name) => STRICT_COLUMNS.includes(name)).length;
  }
  return { all, strict };
}

/** Il corpo di `URTCatalogLibrary::GetCoreActionCatalog()`, isolato: `ShippedAction` e' una factory locale
 *  e potrebbe essere chiamata anche altrove nel file — un'altra funzione che costruisse azioni non-core
 *  entrerebbe nel confronto senza che nessuno l'abbia deciso. */
export function coreCatalogBody(text: string): string {
  const start = text.indexOf('URTCatalogLibrary::GetCoreActionCatalog()');
  if (start === -1) return '';
  const open = text.indexOf('{', start);
  if (open === -1) return '';
  let depth = 0;
  for (let i = open; i < text.length; i++) {
    if (text[i] === '{') depth++;
    else if (text[i] === '}') {
      depth--;
      if (depth === 0) return text.slice(open, i + 1);
    }
  }
  return text.slice(open);
}

/** Toglie i commenti: nel corpo del catalogo ce ne sono piu' che codice, e portano virgole e parentesi che
 *  romperebbero la divisione degli argomenti. */
function stripComments(src: string): string {
  return src.replace(/\/\*[\s\S]*?\*\//g, ' ').replace(/\/\/[^\n]*/g, ' ');
}

/** Divide gli argomenti di una chiamata al livello zero: le graffe della lista di effetti e le parentesi
 *  annidate non sono separatori. */
function splitArgs(src: string): string[] {
  const out: string[] = [];
  let depth = 0;
  let current = '';
  for (const ch of src) {
    if (ch === '(' || ch === '{' || ch === '[') depth++;
    else if (ch === ')' || ch === '}' || ch === ']') depth--;
    if (ch === ',' && depth === 0) {
      out.push(current.trim());
      current = '';
      continue;
    }
    current += ch;
  }
  if (current.trim() !== '') out.push(current.trim());
  return out;
}

const CALL = 'ShippedAction(';

/** I default della firma di `ShippedAction`, che **contano quanto i valori espliciti**: una chiamata che
 *  non passa lo slot dichiara `Main`, e il catalogo la confronta con quello. Leggerli come «assenti»
 *  renderebbe il gate cieco proprio sulle azioni scritte in forma breve. */
const DEFAULT_INTERRUPT = 'InterruptBeforeEffect';
const DEFAULT_SLOT = 'Main';

/** Le azioni dichiarate dal C++, una per chiamata a `ShippedAction`.
 *
 *  Solo **literal**: una priorita' passata come costante nominata non viene letta, il campo resta assente e
 *  la copertura cade invece di passare in silenzio. */
export function parseActionCpp(body: string): Map<string, ActionFacts> {
  const src = stripComments(body);
  const out = new Map<string, ActionFacts>();

  let at = src.indexOf(CALL);
  while (at !== -1) {
    const open = at + CALL.length;
    let depth = 1;
    let end = open;
    while (end < src.length && depth > 0) {
      if (src[end] === '(') depth++;
      else if (src[end] === ')') depth--;
      if (depth === 0) break;
      end++;
    }
    const args = splitArgs(src.slice(open, end));
    at = src.indexOf(CALL, end);

    const id = args[0]?.match(/TEXT\("([^"]+)"\)/)?.[1];
    if (!id) continue;

    const facts: ActionFacts = {};
    const enumValue = (arg: string | undefined, type: string): string | undefined =>
      arg?.match(new RegExp(type + '::(\\w+)'))?.[1];
    const intValue = (arg: string | undefined): number | undefined =>
      arg !== undefined && /^-?\d+$/.test(arg.trim()) ? Number(arg.trim()) : undefined;

    const phase = enumValue(args[1], 'ERTResolutionPhase');
    if (phase) facts.phase = phase;
    const priority = intValue(args[2]);
    if (priority !== undefined) facts.priority = priority;
    const range = intValue(args[3]);
    if (range !== undefined) facts.range = range;
    const cooldown = intValue(args[4]);
    if (cooldown !== undefined) facts.cooldown = cooldown;
    const fallback = enumValue(args[5], 'ERTActionFallback');
    if (fallback) facts.fallback = fallback;

    // Gli ultimi argomenti hanno un default e una chiamata puo' fermarsi prima: l'assenza dell'argomento
    // NON e' assenza del valore.
    const interrupt = args.length > 7 ? enumValue(args[7], 'ERTInterruptPolicy') : DEFAULT_INTERRUPT;
    if (interrupt) facts.interruptible = interrupt !== 'None';
    const slot = args.length > 8 ? enumValue(args[8], 'ERTActionSlot') : DEFAULT_SLOT;
    if (slot) facts.slot = slot;

    out.set(id, facts);
  }

  return out;
}

export interface ActionDivergence {
  actionId: string;
  field: keyof ActionFacts;
  /** Riga del catalogo da cui viene il valore, cosi' il referto non si cerca a mano. */
  line: number;
  catalog: string | number | boolean;
  cpp: string | number | boolean;
}

export interface ActionComparison {
  divergences: ActionDivergence[];
  /** ID che il catalogo dichiara e il C++ non costruisce. */
  onlyInCatalog: string[];
  /** ID che il C++ costruisce e il catalogo non dichiara. */
  onlyInCpp: string[];
  /** Righe che il catalogo dichiara **ritirate** barrandone il nome: non essere nel C++ e' il loro stato
   *  corretto, e segnalarle come mancanti sarebbe il rumore che disattiva un gate. Contate e nominate. */
  retired: string[];
  coverage: {
    catalogRows: number;
    catalogFields: number;
    /** Celle promesse dalle intestazioni: l'atteso, derivato. */
    catalogPromised: number;
    /** Celle che devono sempre essere leggibili (`Cod.` · `Prio` · `CD`), lette e attese. */
    strictRead: number;
    strictPromised: number;
    /** Celle promesse ma non confrontabili — prosa, unita', `—`. Contate, mai taciute. */
    catalogUnreadable: number;
    cppActions: number;
    cppFields: number;
  };
}

/** Una divergenza **gia' nota**, col motivo e con chi la possiede.
 *
 *  🔑 **Perche' esiste, invece di lasciare il gate rosso.** Il primo run di questo confronto — 2026-09-18 —
 *  ha trovato divergenze reali, e ripararle non e' questa issue (#2578 costruisce la misura, non cambia i
 *  numeri). Un gate che nasce rosso in permanenza pero' non si legge: [`D-361`] lo registra come il modo in
 *  cui un cancello muore senza che nessuno lo spenga. Le divergenze note sono quindi **dichiarate qui**, e
 *  ognuna nomina la issue che deve chiuderla.
 *
 *  ⛔ **Il motivo e' obbligatorio e viene stampato**: un'esenzione muta e' indistinguibile da una
 *  dimenticanza — e' la stessa regola di `issue-refs.ts`.
 *
 *  ⚠️ **Un'esenzione che non si verifica piu' fa fallire il gate.** Quando la divergenza viene riparata,
 *  questa riga va tolta: e' l'unico modo perche' l'elenco non diventi il posto dove le divergenze vanno a
 *  dormire. */
export interface KnownDivergence {
  actionId: string;
  /** Il campo che diverge, oppure il lato che dichiara l'azione da solo. */
  field?: keyof ActionFacts;
  side?: 'catalog' | 'cpp';
  reason: string;
}

export const KNOWN_DIVERGENCES: KnownDivergence[] = [
  // ✅ `Action.Sprint` non e' piu' qui: la divergenza di fase e' stata chiusa il 2026-09-18 (#3186) e la
  //    riga e' stata TOLTA nello stesso passaggio. E' il ciclo che questo elenco deve avere — una voce che
  //    sopravvive al proprio difetto fa fallire il gate come esenzione stantia, ed e' voluto. Verificato
  //    rimettendola: `errore: 1 divergenze dichiarate non si verificano piu' — stantia Action.Sprint.phase`.
  ...['Action.Anchor', 'Action.CreateSmoke', 'Action.Evade', 'Action.Mortar', 'Action.Purge', 'Action.Withdraw'].map(
    (actionId): KnownDivergence => ({
      actionId,
      side: 'cpp',
      reason:
        'azione core costruita da `GetCoreActionCatalog()` e non dichiarata da nessuna tabella del ' +
        "catalogo: D-023 rende il catalogo l'autorita' dei numeri, e questi non ce li ha — #3187 le possiede",
    }),
  ),
];

/** Separa le divergenze **nuove** da quelle dichiarate, e trova le esenzioni **stantie** — quelle che non
 *  corrispondono piu' a niente, cioe' che parlano di un difetto riparato. */
export function splitKnown(
  cmp: ActionComparison,
  known: KnownDivergence[] = KNOWN_DIVERGENCES,
): {
  unexpectedDivergences: ActionDivergence[];
  unexpectedOnlyInCatalog: string[];
  unexpectedOnlyInCpp: string[];
  expected: KnownDivergence[];
  stale: KnownDivergence[];
} {
  const matches = (k: KnownDivergence): boolean => {
    if (k.field) return cmp.divergences.some((d) => d.actionId === k.actionId && d.field === k.field);
    if (k.side === 'catalog') return cmp.onlyInCatalog.includes(k.actionId);
    if (k.side === 'cpp') return cmp.onlyInCpp.includes(k.actionId);
    return false;
  };
  const covered = (actionId: string, field?: keyof ActionFacts, side?: 'catalog' | 'cpp'): boolean =>
    known.some(
      (k) => k.actionId === actionId && k.field === field && k.side === side && k.reason.trim() !== '',
    );

  return {
    unexpectedDivergences: cmp.divergences.filter((d) => !covered(d.actionId, d.field, undefined)),
    unexpectedOnlyInCatalog: cmp.onlyInCatalog.filter((id) => !covered(id, undefined, 'catalog')),
    unexpectedOnlyInCpp: cmp.onlyInCpp.filter((id) => !covered(id, undefined, 'cpp')),
    expected: known.filter(matches),
    stale: known.filter((k) => !matches(k)),
  };
}

export function compareActions(
  rows: CatalogRow[],
  cpp: Map<string, ActionFacts>,
  promised: { all: number; strict: number },
): ActionComparison {
  const divergences: ActionDivergence[] = [];
  const seen = new Set<string>();
  const retired = new Set<string>();
  let catalogFields = 0;
  let catalogUnreadable = 0;
  let strictRead = 0;

  for (const row of rows) {
    seen.add(row.actionId);
    if (row.retired) retired.add(row.actionId);
    catalogFields += Object.keys(row.facts).length + (row.phases.length > 0 ? 1 : 0);
    catalogUnreadable += row.unreadable.length;
    strictRead +=
      (row.phases.length > 0 ? 1 : 0) +
      (row.facts.priority !== undefined ? 1 : 0) +
      (row.facts.cooldown !== undefined ? 1 : 0);

    const other = cpp.get(row.actionId);
    if (!other) continue;

    // La fase si confronta per **appartenenza**: una cella `30/40` ammette due esiti, e sceglierne uno al
    // posto del codice segnalerebbe come divergenza cio' che il catalogo ha gia' previsto.
    if (row.phases.length > 0 && other.phase !== undefined && !row.phases.includes(other.phase)) {
      divergences.push({
        actionId: row.actionId,
        field: 'phase',
        line: row.line,
        catalog: row.phases.join(' o '),
        cpp: other.phase,
      });
    }

    for (const field of ACTION_FIELDS) {
      if (field === 'phase') continue;
      const a = row.facts[field];
      const b = other[field];
      if (a === undefined || b === undefined) continue;
      if (a !== b) {
        divergences.push({ actionId: row.actionId, field, line: row.line, catalog: a, cpp: b });
      }
    }
  }

  let cppFields = 0;
  for (const facts of cpp.values()) cppFields += Object.keys(facts).length;

  return {
    divergences,
    onlyInCatalog: [...seen].filter((id) => !cpp.has(id) && !retired.has(id)).sort(),
    onlyInCpp: [...cpp.keys()].filter((id) => !seen.has(id)).sort(),
    retired: [...retired].sort(),
    coverage: {
      catalogRows: rows.length,
      catalogFields,
      catalogPromised: promised.all,
      strictRead,
      strictPromised: promised.strict,
      catalogUnreadable,
      cppActions: cpp.size,
      cppFields,
    },
  };
}
