/** Verifica che le stat base degli eroi non divergano fra il catalogo markdown e il C++.
 *
 *  Uso:  node tools/radar/catalog-code.ts
 *
 *  Chiude [#576](https://github.com/DegrassiAaron/refactor-tactics-main/issues/576). Il difetto che
 *  l'ha generata: il 2026-08-10 **D-075** porto' `Bastion.PushResistance` a `0` nel codice, e il
 *  catalogo continuo' a dichiarare `1` per **due giorni**, in due punti, piu' tre nella scheda
 *  personaggio e uno nel piano delle issue. Nessun gate divento' rosso — quelli di allora verificavano
 *  i **link** e i **simboli citati**, nessuno i **valori** — e il difetto emerse per caso.
 *
 *  **Tre fonti, non due.** Le schede §1–§4, la tabella §5 che ne **ripete** i valori, e i literal C++.
 *  Nella PR #572 le due meta' del catalogo erano sbagliate entrambe, ma niente lo garantisce la
 *  prossima volta: un confronto a due lati direbbe che il catalogo e' allineato al codice mentre la
 *  sua tabella riassuntiva dice altro.
 *
 *  ⚠️ **Non prescrive quale lato correggere, ed e' la scelta piu' importante di questo file.**
 *  [D-023] rende i cataloghi l'autorita' dei numeri, e la conclusione ovvia sarebbe «allinea il
 *  codice al catalogo». E' sbagliata: se questo gate fosse esistito il 2026-08-10 avrebbe segnalato
 *  `Bastion.PushResistance: codice 0 ≠ catalogo 1`, ma il codice aveva ragione e il documento era
 *  indietro. Un gate che indicasse il codice come lato da riparare avrebbe **annullato la decisione**.
 *  Stampa i due valori e si ferma.
 *
 *  🔴 **Il rischio e' il falso verde, non il falso positivo.** Oggi i valori sono literal e si parsano.
 *  Il giorno in cui uno diventa `= kBaseHealth`, o un campo viene rinominato, il parser non trova
 *  nulla e il gate resta verde **proprio quando servirebbe**. Percio' dichiara la **copertura** — 4
 *  eroi × 6 campi = 24, piu' 20 per la tabella §5 che non porta la `Debolezza` — e fallisce se e'
 *  sotto l'atteso. Il conteggio si stampa anche in verde: un gate che non dice quanto ha guardato non
 *  e' distinguibile da uno che non guarda.
 *
 *  ⚠️ **Fuori scope, dichiarato**: le **abilita'** (il catalogo le descrive in prosa, e un gate che le
 *  parsasse sarebbe rumoroso), gli **altri tre cataloghi** (azioni, equipaggiamento, terreni), e
 *  qualunque valore che non sia una delle sei stat base. */
import { readFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';

const CATALOG_REL = 'docs/balance/RT_HeroCatalog_v0.1.md';
const CPP_REL = 'Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp';
const HERO_CATALOG = new URL('../../' + CATALOG_REL, import.meta.url);
const CPP_SOURCE = new URL('../../' + CPP_REL, import.meta.url);

/** Le sei statistiche base di un eroe, come le dichiara una delle tre fonti. */
export interface HeroStats {
  health: number;
  movePoints: number;
  visionRange: number;
  pushResistance: number;
  affinity: string;
  weakness: string;
}

/** Il campo C++ che porta ciascuna statistica. */
const CPP_FIELDS: Record<string, keyof HeroStats> = {
  MaxHealth: 'health',
  MovePoints: 'movePoints',
  VisionRange: 'visionRange',
  PushResistance: 'pushResistance',
  Affinity: 'affinity',
  Weakness: 'weakness',
};

/** `Gadget->MaxHealth = 90;` · `Gadget->Affinity = TEXT("Affinity.Electricity");`
 *
 *  Solo **literal**: un `= kBaseHealth` non viene letto, e la copertura cade invece di passare in
 *  silenzio. E' il modo di guasto che questo gate teme di piu' — un parser che non trova niente resta
 *  verde proprio quando servirebbe. */
const ASSIGN = /^\s*(\w+)->(\w+)\s*=\s*(?:TEXT\("([^"]*)"\)|(-?\d+))\s*;/gm;

export function parseCpp(text: string): Map<string, Partial<HeroStats>> {
  const out = new Map<string, Partial<HeroStats>>();
  for (const m of text.matchAll(ASSIGN)) {
    const field = CPP_FIELDS[m[2]!];
    if (!field) continue;
    const hero = m[1]!;
    const stats = out.get(hero) ?? {};
    // Una stringa dove ci si aspetta un numero (e viceversa) non viene convertita in silenzio: il
    // campo resta assente e la copertura lo dichiara.
    if (m[3] !== undefined && (field === 'affinity' || field === 'weakness')) {
      stats[field] = m[3];
    } else if (m[4] !== undefined && field !== 'affinity' && field !== 'weakness') {
      stats[field] = Number(m[4]);
    }
    out.set(hero, stats);
  }
  return out;
}

/** Il campo di catalogo che porta ciascuna statistica. */
const CATALOG_FIELDS: Record<string, keyof HeroStats> = {
  Salute: 'health',
  Movimento: 'movePoints',
  'Range visivo': 'visionRange',
  'Resistenza Push': 'pushResistance',
  Affinità: 'affinity',
  Debolezza: 'weakness',
};

/** Le quattro affinita' del roster, dalla parola italiana del catalogo all'identificatore del codice.
 *
 *  ⚠️ E' l'asimmetria del catalogo, ed e' voluta lì: l'`Affinità` e' scritta in italiano perche' la
 *  scheda si legge, la `Debolezza` porta l'identificatore fra backtick perche' e' stata decisa dopo
 *  (CP 6.2–6.5) e il token era gia' noto. Un parser che le trattasse allo stesso modo produrrebbe
 *  quattro estrazioni vuote — e la copertura cadrebbe senza dire perche'.
 *
 *  Vocabolario **chiuso**: una parola nuova non vale stringa vuota, fa fallire il parser. */
const AFFINITY_WORDS: Record<string, string> = {
  elettricità: 'Affinity.Electricity',
  acqua: 'Affinity.Water',
  strutture: 'Affinity.Structures',
  movimento: 'Affinity.Movement',
};

/** L'identificatore dentro una cella: `` acqua (`Affinity.Water`) `` -> `Affinity.Water`, oppure la
 *  parola italiana tradotta -> `elettricità` -> `Affinity.Electricity`. */
function cellAffinity(cell: string, field: string, hero: string): string {
  const token = cell.match(/`(Affinity\.[A-Za-z]+)`/)?.[1];
  if (token) return token;

  const word = cell.trim().split(/[\s(—]/)[0]!.toLowerCase();
  const mapped = AFFINITY_WORDS[word];
  if (!mapped) {
    throw new Error(
      `${hero}: campo "${field}" non porta ne' un \`Affinity.X\` ne' una parola nota: "${cell.trim()}". ` +
        `Le parole note sono: ${Object.keys(AFFINITY_WORDS).join(', ')}.`,
    );
  }
  return mapped;
}

/** Le sezioni eroe `## <n>. <Nome> — <sottotitolo>` con la loro tabella delle statistiche.
 *
 *  Il discriminante e' **strutturale** — la tabella `| Statistica | Valore |` — non il nome della
 *  sezione: un titolo puo' cambiare, la tabella e' cio' che si consuma. Stessa scelta di
 *  `parse-catalog.ts`, e per la stessa ragione. */
export function parseCatalogSections(text: string): Map<string, Partial<HeroStats>> {
  const out = new Map<string, Partial<HeroStats>>();
  const sections = text
    .split(/^## \d+\. /m)
    .slice(1)
    .filter((s) => /\|\s*Statistica\s*\|\s*Valore\s*\|/.test(s));

  for (const section of sections) {
    const hero = section.slice(0, section.indexOf('\n')).split('—')[0]!.trim();
    const stats: Partial<HeroStats> = {};

    for (const line of section.split('\n')) {
      const cells = line.split('|');
      if (cells.length < 3) continue;
      const label = cells[1]!.trim();
      const field = CATALOG_FIELDS[label];
      if (!field) continue;

      if (field === 'affinity' || field === 'weakness') {
        stats[field] = cellAffinity(cells[2]!, label, hero);
      } else {
        const n = cells[2]!.match(/-?\d+/);
        if (!n) throw new Error(`${hero}: campo "${label}" non contiene un intero: "${cells[2]!.trim()}"`);
        stats[field] = Number(n[0]);
      }
    }
    out.set(hero, stats);
  }
  return out;
}

/** Le colonne della tabella §5, nell'ordine in cui compaiono. `Debolezza` non c'e': la tabella ne
 *  porta **cinque** dei sei campi, ed e' un fatto della sua forma, non una dimenticanza del parser. */
const SUMMARY_COLUMNS: (keyof HeroStats)[] = [
  'health',
  'movePoints',
  'visionRange',
  'pushResistance',
  'affinity',
];

/** La tabella `## 5. Confronto rapido`, che **ripete** i valori delle schede.
 *
 *  E' la terza fonte, e serve perche' le due meta' del catalogo possono divergere **fra loro**: nella
 *  PR #572 erano sbagliate entrambe, ma niente lo garantisce la prossima volta. Un gate a due lati
 *  direbbe che il catalogo e' allineato al codice mentre la sua tabella riassuntiva dice altro. */
export function parseSummaryTable(text: string): Map<string, Partial<HeroStats>> {
  const out = new Map<string, Partial<HeroStats>>();
  const section = text.split(/^## \d+\. /m).find((s) => /^\s*Confronto rapido/.test(s));
  if (!section) return out;

  // ⚠️ La sezione contiene PIU' di una tabella: §5.1 elenca le risorse firma con `| Eroe | Vista |
  // Ruolo | … | Cap |`, stessa larghezza e stessi `Eroe`, e letta per errore sovrascriveva i valori
  // buoni con i propri. Ci si ancora all'INTESTAZIONE e si smette alla riga vuota che chiude la
  // tabella: la posizione delle colonne ha senso solo sotto la sua intestazione.
  const lines = section.split('\n');
  const start = lines.findIndex((l) => /^\|\s*Eroe\s*\|\s*HP\s*\|/.test(l));
  if (start === -1) return out;
  const end = lines.findIndex((l, i) => i > start && l.trim() === '');

  for (const line of lines.slice(start, end === -1 ? undefined : end)) {
    const cells = line.split('|').map((c) => c.trim());
    // `| Eroe | 90 | 5 | 7 | 0 | elettricità | identita' |` -> 8 celle con i due bordi vuoti.
    if (cells.length < 8) continue;
    const hero = cells[1]!;
    // Discriminante STRUTTURALE, non lessicale: una riga di dati ha un intero in colonna `HP`.
    // `| Eroe | HP | …` supererebbe un controllo sulla forma del nome — `Eroe` e' un `[A-Z][a-z]+`
    // come `Gadget` — e il parser leggerebbe l'intestazione come un quinto eroe.
    if (!/^-?\d+$/.test(cells[2] ?? '')) continue;

    const stats: Partial<HeroStats> = {};
    SUMMARY_COLUMNS.forEach((field, i) => {
      const cell = cells[2 + i]!;
      if (field === 'affinity') {
        stats[field] = cellAffinity(cell, 'Affinità', hero);
      } else {
        const n = cell.match(/-?\d+/);
        if (n) stats[field] = Number(n[0]);
      }
    });
    out.set(hero, stats);
  }
  return out;
}

/** Un campo su cui le fonti non concordano. */
export interface Divergence {
  hero: string;
  field: keyof HeroStats;
  /** Il valore che ciascuna fonte dichiara. Una fonte che tace non compare. */
  values: Record<string, string | number>;
}

export interface Comparison {
  divergences: Divergence[];
  /** Quante estrazioni ha prodotto ciascun lato. */
  coverage: { sections: number; summary: number; cpp: number };
}

const ALL_FIELDS: (keyof HeroStats)[] = [
  'health',
  'movePoints',
  'visionRange',
  'pushResistance',
  'affinity',
  'weakness',
];

function count(m: Map<string, Partial<HeroStats>>): number {
  let n = 0;
  for (const stats of m.values()) n += ALL_FIELDS.filter((f) => stats[f] !== undefined).length;
  return n;
}

/** Confronta le tre fonti e riporta dove non concordano.
 *
 *  ⚠️ **Non prescrive quale lato correggere, ed e' deliberato.** [D-023] rende i cataloghi markdown
 *  l'autorita' dei numeri, e la conclusione ovvia sarebbe «allinea il codice al catalogo». E'
 *  sbagliata, e il caso che ha generato #576 lo dimostra: il 2026-08-10 **D-075** aveva gia' portato
 *  `Bastion.PushResistance` a `0` nel codice, e il catalogo dichiarava ancora `1`. Un gate che avesse
 *  indicato il codice come lato da riparare avrebbe rimesso `1`, **annullando la decisione**.
 *
 *  Stampa i valori e si ferma. Non sa quale sia giusto, e fingere di saperlo e' piu' pericoloso del
 *  silenzio di oggi. */
export function compare(
  sections: Map<string, Partial<HeroStats>>,
  summary: Map<string, Partial<HeroStats>>,
  cpp: Map<string, Partial<HeroStats>>,
): Comparison {
  const divergences: Divergence[] = [];
  const heroes = [...new Set([...sections.keys(), ...summary.keys(), ...cpp.keys()])].sort();

  for (const hero of heroes) {
    for (const field of ALL_FIELDS) {
      const values: Record<string, string | number> = {};
      const s = sections.get(hero)?.[field];
      const t = summary.get(hero)?.[field];
      const c = cpp.get(hero)?.[field];
      if (s !== undefined) values['schede'] = s;
      if (t !== undefined) values['tabella §5'] = t;
      if (c !== undefined) values['C++'] = c;

      const distinct = new Set(Object.values(values));
      if (distinct.size > 1) divergences.push({ hero, field, values });
    }
  }

  return {
    divergences,
    coverage: { sections: count(sections), summary: count(summary), cpp: count(cpp) },
  };
}

// ---------------------------------------------------------------------------------------------
// Comando
// ---------------------------------------------------------------------------------------------

/** Il roster della v0.1. Cresce a otto con E35: e' un numero atteso, non una scoperta. */
const EXPECT_HEROES = 4;

function main() {
  const catalog = readFileSync(HERO_CATALOG, 'utf8');
  const cppText = readFileSync(CPP_SOURCE, 'utf8');

  const sections = parseCatalogSections(catalog);
  const summary = parseSummaryTable(catalog);
  // Il C++ dichiara anche altre struct: si tengono solo gli eroi che il catalogo conosce, altrimenti
  // una variabile locale qualsiasi con un campo omonimo entrerebbe nel confronto.
  const cpp = new Map([...parseCpp(cppText)].filter(([hero]) => sections.has(hero)));

  const { divergences, coverage } = compare(sections, summary, cpp);

  const want = { sections: EXPECT_HEROES * 6, summary: EXPECT_HEROES * 5, cpp: EXPECT_HEROES * 6 };
  const short: string[] = [];
  for (const side of ['sections', 'summary', 'cpp'] as const) {
    if (coverage[side] < want[side]) short.push(`${side}: ${coverage[side]}/${want[side]}`);
  }

  // La copertura si stampa SEMPRE, anche in verde: un gate che non dice quanto ha guardato non e'
  // distinguibile da uno che non guarda.
  console.error(
    `campi confrontati — schede ${coverage.sections}/${want.sections} · ` +
      `tabella §5 ${coverage.summary}/${want.summary} · C++ ${coverage.cpp}/${want.cpp}`,
  );

  if (short.length > 0) {
    console.error(
      `\nerrore: copertura sotto l'atteso (${short.join(' · ')}).\n` +
        `Un lato che non si parsa piu' rende questo gate VERDE proprio quando servirebbe: succede se un\n` +
        `literal diventa una costante (\`= kBaseHealth\`), se un campo C++ viene rinominato, o se il\n` +
        `catalogo cambia le etichette di riga. Non e' un falso allarme, e' il gate che si dichiara cieco.`,
    );
    process.exit(1);
  }

  if (divergences.length === 0) {
    console.error(`le tre fonti concordano su tutti i campi dei ${sections.size} eroi`);
    return;
  }

  console.error(`\n${divergences.length} campi divergono:`);
  for (const d of divergences) {
    const detail = Object.entries(d.values)
      .map(([src, v]) => `${src}=${v}`)
      .join(' · ');
    console.error(`  ${d.hero}.${d.field}: ${detail}`);
  }
  console.error(
    `\nFonti: ${CATALOG_REL} (schede §1-§4 e tabella §5) · ${CPP_REL}\n` +
      `⚠️ Quale lato sia giusto NON lo dice questo gate, deliberatamente: il 2026-08-10 il codice aveva\n` +
      `ragione e il catalogo era indietro (D-075). Leggi il Decision Log prima di scegliere.`,
  );
  process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
