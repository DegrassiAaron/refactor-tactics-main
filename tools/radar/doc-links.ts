/** Verifica che i percorsi citati dai documenti risolvano davvero — `docs/` piu' `AGENTS.md`,
 *  `CLAUDE.md` e `README.md`.
 *
 *  Uso:  node tools/radar/doc-links.ts [--check] [--with-archive]
 *
 *  Il problema che chiude: dal **2026-08-21** (D-182) il repository non ha piu' gate Python, e con
 *  `check-docs-links.py` e' uscito l'unico controllo che vedeva un link rotto. Il costo si e' misurato
 *  il **2026-08-25**: delle 226 issue aperte, **144** citavano percorsi o strumenti che non esistono
 *  piu'. I documenti erano stati riparati a mano (#1232), le issue no, e nessun comando lo diceva.
 *
 *  Verifica **due** difetti, non uno:
 *   1. un link che non trova il file — `[x](percorso/sparito.md)`;
 *   2. un'etichetta che **mostra** un percorso vecchio mentre il link funziona. E' il gemello del
 *      primo e nessun controllo sui soli link lo vede: chi legge copia il percorso dall'etichetta,
 *      non dal bersaglio. Dopo un solo spostamento di cartelle ne sono stati contati **36** (D-182).
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perche' non venga scoperto dopo:
 *   - le **ancore** (`spec.md#sezione`): il file deve esistere, la sezione no;
 *   - gli **URL esterni** e la Wiki — quella e' `wiki-alt.ts`, che questo non copre e viceversa;
 *   - i percorsi citati **in prosa o dentro inline code**: senza le parentesi tonde non c'e' modo di
 *     distinguere un riferimento da un esempio, e provarci produrrebbe il rumore che disattiva un
 *     gate al terzo falso positivo. Un link dentro `` ` `` o dentro ``` e' un esempio, e viene tolto;
 *   - `docs/archive/` salvo `--with-archive`: e' storico, e si ripara solo se qualcuno decide di farlo.
 *
 *  La copertura si stampa **sempre**, anche in verde: un gate che non dice quanto ha guardato non e'
 *  distinguibile da uno che non guarda (#576). */
import { readFileSync, existsSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
import { posix, join, relative, sep } from 'node:path';

import { REPO_ROOT, docsCorpus } from './docs-corpus.ts';

/** ⚠️ **Il perimetro non si legge qui.** `REPO_ROOT`, `docs/`, `.claude/` e i tre documenti di
 *  governance della radice sono definiti in `docs-corpus.ts`, con la ragione di ciascuno: questo file
 *  li consuma e non li ridichiara, perche' due definizioni possono divergere senza che nessuna riga di
 *  copertura lo dica (#1405). */


/** Un riferimento a un file del repository, scritto come link Markdown. */
export interface DocLink {
  /** Testo fra parentesi quadre. */
  label: string;
  /** Percorso fra parentesi tonde, come e' scritto. */
  target: string;
}

/** Link Markdown. L'etichetta non puo' contenere `]`, il bersaglio non puo' contenere `)`. */
const LINK = /\[([^\]]*)\]\(([^)\s]+)\)/g;

/** Bersagli che non sono file del repository: web, posta, ancora interna. */
const NOT_A_FILE = /^(https?:|mailto:|#)/;

/** Blocco recintato ` ```…``` ` e inline code a uno o piu' backtick, nell'ordine in cui vanno tolti. */
const FENCE = /^```[\s\S]*?^```/gm;
// Il contenuto e' `[\s\S]` e non `[^`]`: l'inline code a due backtick esiste **apposta** per
// contenerne uno singolo — ``[`../vecchio/x.md`](../nuovo/x.md)`` — e una classe che esclude il
// backtick spezzerebbe la coppia esterna lasciando in piedi mezzo link.
const INLINE_CODE = /(`+)[\s\S]*?\1/g;

/** Il testo senza il codice: quel che resta e' prosa, e solo lì un link è un riferimento.
 *
 *  Un link dentro il codice e' un **esempio**. `docs/README.md` insegna il difetto dell'etichetta che
 *  mente mostrandone uno finto, e un referto cita la sintassi `[testo](url)` per spiegare come
 *  contava: segnalarli significherebbe dare torto alla documentazione che spiega questo controllo. */
function withoutCode(text: string): string {
  return text.replace(FENCE, '').replace(INLINE_CODE, '');
}

export function scanLinks(text: string): DocLink[] {
  const out: DocLink[] = [];
  for (const m of withoutCode(text).matchAll(LINK)) {
    const target = m[2]!;
    if (NOT_A_FILE.test(target)) continue;
    out.push({ label: m[1]!, target });
  }
  return out;
}

/** Il percorso che il link indica, relativo alla radice del repository.
 *
 *  L'ancora e la query non fanno parte del file: `spec.md#sezione` e `spec.md` sono lo stesso file, e
 *  un gate che li distinguesse segnalerebbe come morto ogni link a una sezione. Che l'**ancora**
 *  esista non e' verificato qui, ed e' dichiarato nel docstring del comando. */
export function resolveTarget(target: string, fromFile: string): string {
  const clean = target.split('#')[0]!.split('?')[0]!;
  return posix.normalize(posix.join(posix.dirname(fromFile), clean));
}

/** Un link che non risolve. */
export interface DeadLink extends DocLink {
  /** Il percorso risolto rispetto alla radice, quello che si e' cercato e non c'era. */
  resolved: string;
}

/** I link di un documento che non trovano un file.
 *
 *  `exists` e' iniettato invece di chiamare `existsSync` qui dentro: rende la funzione pura e
 *  testabile su un albero finto, senza dover scrivere file veri per provare un caso. */
export function deadLinks(
  text: string,
  fromFile: string,
  exists: (path: string) => boolean,
): DeadLink[] {
  const out: DeadLink[] = [];
  for (const link of scanLinks(text)) {
    const resolved = resolveTarget(link.target, fromFile);
    if (exists(resolved)) continue;
    out.push({ ...link, resolved });
  }
  return out;
}

/** Sembra un percorso di file, e non una frase: ha una barra e un'estensione nota. */
const LOOKS_LIKE_PATH = /^[A-Za-z0-9_.\-/]+\.(md|ts|py|json|yaml|yml|cpp|h|uasset|umap|svg|png)$/;

/** Le etichette che **mostrano** un percorso che non esiste, mentre il link funziona.
 *
 *  E' il difetto gemello di `deadLinks` e nessun controllo sui soli link lo vede: chi legge copia il
 *  percorso dall'etichetta, non dal bersaglio. Dopo un solo spostamento di cartelle ne sono stati
 *  contati **36** (D-182).
 *
 *  Un'etichetta di prosa non viene giudicata: `[la mappa degli scenari](…)` non promette un percorso,
 *  e pretendere che corrisponda produrrebbe rumore su ogni link scritto bene. */
export function staleLabels(
  text: string,
  fromFile: string,
  exists: (path: string) => boolean,
): DeadLink[] {
  const out: DeadLink[] = [];
  for (const link of scanLinks(text)) {
    if (!LOOKS_LIKE_PATH.test(link.label)) continue;
    const shown = link.label.startsWith('docs/') || link.label.startsWith('tools/')
      ? link.label
      : resolveTarget(link.label, fromFile);
    if (exists(shown)) continue;
    out.push({ ...link, resolved: shown });
  }
  return out;
}

// ---------------------------------------------------------------------------------------------
// Comando
// ---------------------------------------------------------------------------------------------

function main() {
  const argv = process.argv.slice(2);
  const check = argv.includes('--check');
  const withArchive = argv.includes('--with-archive');

  // Il corpus — i tre documenti di governance della radice, `docs/` meno l'archivio e `.claude/` — sta
  // in `docs-corpus.ts`, non qui: era copiato identico nell'altro gate, e due copie divergono in
  // silenzio continuando a stampare la stessa riga di copertura (#1405).
  const files = docsCorpus({ withArchive });

  const problems: string[] = [];
  let links = 0;

  for (const file of files) {
    const rel = relative(REPO_ROOT, file).split(sep).join('/');
    const text = readFileSync(file, 'utf8');
    const exists = (p: string) => existsSync(join(REPO_ROOT, p));

    links += scanLinks(text).length;
    for (const d of deadLinks(text, rel, exists)) {
      problems.push(`${rel}: link a \`${d.resolved}\` — non esiste  [${d.label}]`);
    }
    for (const s of staleLabels(text, rel, exists)) {
      problems.push(`${rel}: etichetta \`${s.label}\` mostra un percorso che non esiste (il link risolve)`);
    }
  }

  // La copertura si stampa **anche in verde**: un gate che non dice quanto ha guardato non e'
  // distinguibile da uno che non guarda (#576).
  console.error(
    `link esaminati: ${links} in ${files.length} documenti (radice, docs/ e .claude/)` +
      (withArchive ? '' : ' (docs/archive/ escluso: passa --with-archive)'),
  );

  if (problems.length === 0) {
    console.error('tutti i percorsi citati risolvono');
    return;
  }
  console.error(`\n${problems.length} percorsi non risolvono:\n  ${problems.join('\n  ')}`);
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
