/** Il criterio eseguibile di `G14` — *«canone, roadmap, cataloghi, README, PIE senza contraddizioni»*.
 *
 *  Uso:  node tools/radar/doc-coherence.ts [--check] [--repo OWNER/NOME]
 *
 *  ## Il problema che chiude
 *
 *  `G14` aveva un esecutore — il preambolo di §3 di [`v0.1-definition-of-done.md`] gli assegna `PIA-0` e
 *  `PIA-2` — e **non un criterio**: la colonna *«Come si verifica»* diceva per intero *«canone, roadmap,
 *  cataloghi, README, PIE senza contraddizioni»*. Nessun documento nominato, nessuna definizione di
 *  «contraddizione», nessun comando. Un gate cosi' non ha un momento in cui diventa verde: chi lo
 *  dichiara tale registra un'impressione, mentre il DoD pretende *«una voce e' ✅ solo con evidenza
 *  allegata»* (#3371).
 *
 *  E non era teorico. Due contraddizioni gli erano passate sotto, misurate il 2026-09-27:
 *   - il corpo di **#38** dichiara bloccanti `PIE-HEXPLAY-6` (❌) e `-8` (🟡), che nel registro sono ✅;
 *   - **`PIE-HEX-MODE-H`** e' ❌ mentre **#931**, la issue che quel verdetto ha aperto, e' CLOSED.
 *
 *  Le asserzioni `A3` e `A4` esistono per trovare **questi due**, e i loro controlli positivi sono
 *  esattamente questi due casi: un criterio che non li trova non funziona, e lo si sa prima di crederci.
 *
 *  ## Le cinque asserzioni, e che cosa NON coprono
 *
 *  | # | Asserzione | Sorgenti confrontate |
 *  |---|---|---|
 *  | `A1` | I totali dichiarati del registro PIE coincidono col ricalcolo, e nessuna riga e' senza marcatore | `test-manuali-pie.md` con se stesso |
 *  | `A2` | Ogni seduta `critical: true` e' **convocata** (una issue la nomina) oppure **eseguita** (i suoi `verifies` hanno un verdetto, o i suoi `artifacts` sono tracciati) | `editor-sessions.yaml` × issue × registro PIE |
 *  | `A3` | Nessun corpo di issue **aperta** attribuisce a una voce `PIE-*` uno stato diverso da quello che il registro le da' oggi | issue × `test-manuali-pie.md` |
 *  | `A4` | Nessuna voce ❌ o 🟡 ha CLOSED la **prima** issue citata nella propria cella di stato — quella che il verdetto ha aperto | `test-manuali-pie.md` × issue |
 *  | `A5` | Le tre cardinalita' di `roadmap-v0.1.md` coincidono: somma della colonna `CP` di §3 = id di checkpoint **unici** in §5 = totale scritto | `roadmap-v0.1.md` con se stesso |
 *
 *  La sesta asserzione del criterio — *«i percorsi citati dai documenti risolvono»* — **non e' qui**:
 *  e' `doc-links.ts`, che esiste dal 2026-08-25 e non si riscrive. `G14` si esegue con **due** comandi.
 *
 *  ⚠️ **Cosa resta scoperto**, dichiarato perche' non venga scoperto dopo: di `canone, roadmap,
 *  cataloghi, README` solo la *roadmap* ha qui un'asserzione propria (`A5`); canone, cataloghi e README
 *  sono coperti **solo** da `doc-links.ts`, cioe' sui percorsi e non sui contenuti. Un catalogo che
 *  contraddice il C++ e' `catalog-code.ts`, non questo.
 *
 *  ## Le scelte che un falso positivo avrebbe reso inservibili, ognuna misurata
 *
 *  🔴 **Il blockquote NON e' un marcatore storico, qui.** `issue-refs.ts` lo tratta come tale, e per il
 *  proprio dominio ha ragione: la nota additiva datata di questo repository e' un blockquote. Ma la
 *  *«DoD viva»* di una issue lo e' altrettanto — e il difetto misurato di **#38 vive dentro un
 *  blockquote**. Ereditare quel filtro avrebbe perso il caso per cui `A3` esiste.
 *
 *  🔴 **La casella `- [ ]` descrive un traguardo, non lo stato corrente.** Misurato: senza questo
 *  filtro `A3` segnala `#3104` (*«la voce torna ✅ solo cosi'»*) e `#1775` (*«diventa eseguibile»*) —
 *  frasi al futuro, lette come dichiarazioni di stato.
 *
 *  🔴 **La finestra dopo il nome della voce si chiude al nome successivo.** Misurato su `#2476`:
 *  *«`PIE-HEXPLAY-6` — piu' `PIE-HEXPLAY-8` 🟡»* attribuirebbe il 🟡 **alla voce sbagliata** con una
 *  finestra a lunghezza fissa. Il glifo appartiene all'ultimo nome che lo precede.
 *
 *  🔴 **`A4` guarda la PRIMA issue citata, non tutte.** Le celle di stato sono narrazione additiva in
 *  ordine cronologico: la prima issue e' quella dove il verdetto e' stato *aperto* (*«Aperta come
 *  #931»*). Misurato il 2026-09-27 sulle 36 righe ❌/🟡: *«tutte le issue citate CLOSED»* ne trova **6**
 *  e **manca `PIE-HEX-MODE-H`**, perche' la sua cella cita anche #996 che e' aperta; *«la prima citata
 *  e' CLOSED»* ne trova **9** e lo prende. La stessa tabella, due predicati, due risposte opposte.
 *
 *  🔴 **`A2` non si puo' misurare in locale.** `editor-sessions.yaml` ha `issues: []` su `U5`, ma `U5`
 *  un convocatore ce l'ha — **#2477**, il cui titolo e' *«Seduta U5 — …»*. Un'asserzione negativa letta
 *  sul solo file avrebbe segnalato `U5` come non convocata: e' la forma di difetto che il referto che
 *  apre #3371 ha commesso, ed e' per questo che il convocatore si cerca **fra le issue**.
 *
 *  🔴 **`A5` conta id UNICI, non righe di tabella.** Misurato: `E47` porta una **seconda** tabella —
 *  l'allocazione delle track — le cui righe riaprono con `| **E47.n** |`. Contando le righe se ne
 *  leggono **13** dove i checkpoint sono **7**.
 *
 *  ⚠️ **E `A5` accetta due forme di id**, perche' §4.1 di `roadmap-v0.1.md` rende il prefisso
 *  obbligatorio e le epic nate dopo lo portano: `| **13.1** |` ed `| **E21.1** |` sono lo stesso
 *  oggetto. Leggendo solo la prima, `E21`, `E23` e `E47` risultano **a zero checkpoint**.
 *
 *  ## Rete assente
 *  `A2`, `A3` e `A4` leggono GitHub. Senza `gh`, senza autenticazione o senza rete si dichiarano
 *  **NOT RUN** — mai PASS — e il comando esce **0**: `CLAUDE.md` §6 pretende che una verifica non
 *  eseguita si dichiari, e un gate che finge un verde offline e' peggio di uno che non gira. `A1` e `A5`
 *  sono locali e girano comunque.
 *
 *  La copertura si stampa **sempre**, anche in verde: un gate che non dice quanto ha guardato non e'
 *  distinguibile da uno che non guarda (#576). */
import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
import { join } from 'node:path';

import { REPO_ROOT } from './docs-corpus.ts';
import { git } from './git.ts';

/** I tre documenti che questo gate confronta. Sono percorsi dalla radice del repository. */
export const REGISTRO_PIE = 'docs/technical/test-manuali-pie.md';
export const SEDUTE = 'docs/roadmap/editor-sessions.yaml';
export const ROADMAP = 'docs/roadmap/roadmap-v0.1.md';

/** Il repository GitHub da interrogare, quando non arriva da `--repo`. */
const DEFAULT_REPO = 'DegrassiAaron/refactor-tactics-main';

/** I quattro marcatori di stato del registro PIE. L'ordine non conta: si cerca il primo che compare. */
const GLIFO = /✅|🟡|❌|⏳/;

// ---------------------------------------------------------------------------------------------
// Il registro PIE
// ---------------------------------------------------------------------------------------------

export interface VocePie {
  /** `PIE-HEXPLAY-6`, senza backtick. */
  id: string;
  /** Il marcatore letto nella cella di stato, o `null` se la cella non ne porta nessuno. */
  glifo: string | null;
  /** La cella di stato per intero, cioe' `$(NF-1)` del comando canonico. */
  stato: string;
  /** Riga del file, 1-based. */
  riga: number;
  /** Le issue citate nella cella di stato, **nell'ordine in cui compaiono**, senza duplicati. */
  issues: number[];
}

/** Le voci del registro PIE.
 *
 *  ⚠️ **La cella di stato e' `$(NF-1)`, non la settima.** E' il comando canonico del documento a dirlo, e
 *  la ragione e' misurata li': alcune righe portano piu' di sette campi per via di un `|` dentro una
 *  cella *precedente*, che e' innocuo. Cio' che romperebbe la lettura e' un `|` **nella cella di
 *  stato**, e la convenzione lo vieta. */
export function registroPie(text: string): VocePie[] {
  const out: VocePie[] = [];
  const righe = text.split(/\r?\n/);
  for (let i = 0; i < righe.length; i++) {
    const l = righe[i]!;
    if (!l.startsWith('| **PIE-')) continue;
    const nome = l.match(/^\| \*\*(PIE-[^*]+)\*\*/);
    if (!nome) continue;
    const campi = l.split('|');
    const stato = campi[campi.length - 2] ?? '';
    const g = stato.match(GLIFO);
    const issues: number[] = [];
    for (const m of stato.matchAll(/\/issues\/(\d+)/g)) {
      const n = Number(m[1]);
      if (!issues.includes(n)) issues.push(n);
    }
    out.push({ id: nome[1]!.trim(), glifo: g ? g[0] : null, stato, riga: i + 1, issues });
  }
  return out;
}

export interface TotaliPie {
  /** I numeri scritti nella riga «*N* voci: …», o `null` se la riga non c'e'. */
  dichiarati: { voci: number; verde: number; parziale: number; fallita: number; aperta: number } | null;
  /** Gli stessi numeri ricontati dalle righe. */
  contati: { voci: number; verde: number; parziale: number; fallita: number; aperta: number };
  /** Le voci la cui cella di stato non porta nessun marcatore: il `senza-marcatore` del comando canonico. */
  senzaMarcatore: string[];
}

/** La riga dei totali: `**254 voci**: ✅ **89 verdi** · 🟡 **28 parziali** · ❌ **8 fallite** · ⏳ **129 aperte**.` */
const RIGA_TOTALI =
  /\*\*(\d+) voci\*\*:\s*✅\s*\*\*(\d+)[^·]*·\s*🟡\s*\*\*(\d+)[^·]*·\s*❌\s*\*\*(\d+)[^·]*·\s*⏳\s*\*\*(\d+)/;

/** `A1` — i totali dichiarati contro il ricalcolo.
 *
 *  🔑 **Il difetto che chiude e' misurato e ricorrente**: il 2026-09-23 la riga dei totali diceva `222`
 *  dove il ricalcolo diceva `245` — **23 voci di scarto accumulate in silenzio** perche' ogni passata
 *  aggiungeva il proprio blocco senza toccare la riga. Il documento porta il comando che lo scopre; non
 *  aveva nessuno che lo lanciasse. */
export function totaliPie(text: string): TotaliPie {
  const voci = registroPie(text);
  const conta = (g: string) => voci.filter((v) => v.glifo === g).length;
  const m = text.match(RIGA_TOTALI);
  return {
    dichiarati: m
      ? { voci: Number(m[1]), verde: Number(m[2]), parziale: Number(m[3]), fallita: Number(m[4]), aperta: Number(m[5]) }
      : null,
    contati: {
      voci: voci.length,
      verde: conta('✅'),
      parziale: conta('🟡'),
      fallita: conta('❌'),
      aperta: conta('⏳'),
    },
    senzaMarcatore: voci.filter((v) => v.glifo === null).map((v) => v.id),
  };
}

// ---------------------------------------------------------------------------------------------
// A3 — una issue attribuisce a una voce uno stato che il registro non le da'
// ---------------------------------------------------------------------------------------------

export interface Contraddizione {
  issue: number;
  /** Riga del corpo, 1-based, come la vede chi apre la issue. */
  riga: number;
  voce: string;
  /** Il glifo scritto nella issue. */
  detto: string;
  /** Il glifo che il registro da' oggi a quella voce. */
  registro: string;
  /** La riga, tagliata, per far vedere il contesto senza aprire GitHub. */
  testo: string;
}

/** Una voce PIE citata dentro inline code. Fuori dai backtick non e' un riferimento, ed e' la stessa
 *  scelta che `doc-links.ts` e `issue-refs.ts` motivano per i propri domini. */
const VOCE_IN_CODICE = /`(PIE-[A-Za-z0-9.\-]+)`/g;

/** Vero se la riga non dichiara uno **stato corrente** e quindi non si giudica.
 *
 *  Due sole forme, entrambe misurate, perche' ogni filtro in piu' e' un modo di non vedere:
 *   - `~~` — testo barrato: e' una ritrattazione, e segnalarla significa segnalare la cura;
 *   - `- [ ]` — una casella di DoD descrive un **traguardo**: *«la voce torna ✅ solo cosi'»* e' una
 *     condizione di chiusura, non un'affermazione sul presente.
 *
 *  ⛔ **Il blockquote NON e' fra queste.** Vedi il docstring in testa: il difetto di #38 vive li'. */
export function nonDichiaraStato(riga: string): boolean {
  if (riga.includes('~~')) return true;
  return /^\s*[-*]\s*\[\s\]/.test(riga);
}

/** Quanto testo dopo il nome di una voce puo' ancora portare il **suo** glifo.
 *
 *  Misurato sui casi reali: `` | **`PIE-HEXPLAY-6`** | **`RELEASE-V01`** | ❌ | `` ne chiede 28, e
 *  `` `PIE-V01-DEBUG` | `U15` | ✅ `` dodici. Quaranta li copre entrambi senza aprire la finestra a
 *  frasi intere. */
const FINESTRA = 40;

/** Le voci a cui il corpo di una issue attribuisce uno stato diverso da quello del registro.
 *
 *  `statoRegistro` risponde col glifo corrente di una voce, o `undefined` se la voce non esiste: una
 *  voce sconosciuta non e' una contraddizione, e' un refuso che appartiene a un altro controllo. */
export function contraddizioniPie(
  body: string,
  issue: number,
  statoRegistro: (voce: string) => string | undefined,
): Contraddizione[] {
  const out: Contraddizione[] = [];
  // I corpi arrivano con CRLF: senza normalizzare, il numero di riga stampato non e' quello che vede
  // chi apre la issue — la stessa misura che `issue-refs.ts` registra su #703.
  const righe = body.replace(/\r\n/g, '\n').split('\n');
  for (let i = 0; i < righe.length; i++) {
    const riga = righe[i]!;
    if (nonDichiaraStato(riga)) continue;
    for (const m of riga.matchAll(VOCE_IN_CODICE)) {
      const voce = m[1]!;
      const atteso = statoRegistro(voce);
      if (atteso === undefined) continue;
      const dopo = m.index! + m[0].length;
      // La finestra si chiude al nome successivo: il glifo appartiene all'ultima voce che lo precede.
      const prossima = riga.indexOf('`PIE-', dopo);
      const fine = prossima >= 0 ? Math.min(dopo + FINESTRA, prossima) : dopo + FINESTRA;
      const g = riga.slice(dopo, fine).match(GLIFO);
      if (!g || g[0] === atteso) continue;
      out.push({ issue, riga: i + 1, voce, detto: g[0], registro: atteso, testo: riga.trim().slice(0, 140) });
    }
  }
  return out;
}

// ---------------------------------------------------------------------------------------------
// A4 — un verdetto rosso o giallo la cui causa e' stata chiusa
// ---------------------------------------------------------------------------------------------

export interface DaRigiudicare {
  voce: string;
  glifo: string;
  riga: number;
  /** La prima issue citata nella cella di stato: quella dove il verdetto e' stato aperto. */
  causa: number;
}

/** Le voci ❌ o 🟡 la cui **prima** issue citata e' CLOSED: candidate a un rigiudizio, non difetti del
 *  gioco.
 *
 *  Il registro lo dice di se': *«Un ❌ che descrive un difetto gia' corretto e' peggio di un ⏳: chi
 *  legge conclude che il gioco sia rotto dove non lo e', e chi lavora nell'area lo prende come
 *  vincolo»*. Non aveva chi se ne accorgesse.
 *
 *  ⚠️ **Una voce che non cita nessuna issue non e' un errore.** Sono 19 su 36 alla misura del
 *  2026-09-27: pretendere la citazione farebbe uscire meta' della popolazione al primo giro, che e' il
 *  modo noto di far disattivare un gate. */
export function daRigiudicare(
  voci: VocePie[],
  stato: (issue: number) => 'OPEN' | 'CLOSED' | undefined,
): DaRigiudicare[] {
  const out: DaRigiudicare[] = [];
  for (const v of voci) {
    if (v.glifo !== '❌' && v.glifo !== '🟡') continue;
    const causa = v.issues[0];
    if (causa === undefined) continue;
    if (stato(causa) !== 'CLOSED') continue;
    out.push({ voce: v.id, glifo: v.glifo, riga: v.riga, causa });
  }
  return out;
}

// ---------------------------------------------------------------------------------------------
// A2 — una seduta critica che nessuno convoca e che nessuno ha eseguito
// ---------------------------------------------------------------------------------------------

export interface Seduta {
  id: string;
  critical: boolean;
  /** I numeri di issue dichiarati dal campo `issues:`. */
  issues: number[];
  /** Gli id delle voci PIE che la seduta dichiara di giudicare. */
  verifies: string[];
  /** I percorsi che la seduta dichiara di produrre. */
  artifacts: string[];
}

/** Le sedute di `editor-sessions.yaml`, lette senza dipendenze.
 *
 *  ⚠️ **Un parser minimale, e dichiarato tale**: legge `id`, `critical`, `issues`, `verifies` e
 *  `artifacts`, in forma inline (`[a, b]`) o a lista, e ignora tutto il resto. `tools/radar/` non ha
 *  dipendenze e non le prende per un campo; il rischio e' che una forma YAML valida ma diversa venga
 *  letta come vuota, ed e' per questo che il conteggio delle sedute lette si **stampa**: se un giorno
 *  non tornasse, si vede.
 *
 *  ✅ Validato il 2026-09-27 contro `yaml.safe_load`: 59 sedute, 19 `critical`, stessi id. */
export function sedute(yamlText: string): Seduta[] {
  const out: Seduta[] = [];
  let cur: Seduta | null = null;
  let lista: 'issues' | 'verifies' | 'artifacts' | null = null;
  for (const raw of yamlText.split(/\r?\n/)) {
    const l = raw.replace(/\t/g, '  ');
    const id = l.match(/^ {2}- id:\s*(\S+)/);
    if (id) {
      cur = { id: id[1]!, critical: false, issues: [], verifies: [], artifacts: [] };
      out.push(cur);
      lista = null;
      continue;
    }
    if (!cur) continue;
    const chiave = l.match(/^ {4}([a-z_]+):\s*(.*)$/);
    if (chiave) {
      const k = chiave[1]!;
      const v = chiave[2]!.trim();
      lista = null;
      if (k === 'critical') cur.critical = v === 'true';
      else if (k === 'issues' || k === 'verifies' || k === 'artifacts') {
        if (v.startsWith('[')) {
          const voci = v
            .slice(1, v.lastIndexOf(']') >= 0 ? v.lastIndexOf(']') : undefined)
            .split(',')
            .map((s) => s.trim().replace(/^["']|["']$/g, ''))
            .filter(Boolean);
          if (k === 'issues') cur.issues = voci.map(Number).filter((n) => Number.isFinite(n));
          else cur[k] = voci;
        } else lista = k as typeof lista;
      }
      continue;
    }
    const item = l.match(/^ {6}- (.*)$/);
    if (item && lista) {
      const v = item[1]!.trim().replace(/^["']|["']$/g, '');
      if (lista === 'issues') {
        const n = Number(v);
        if (Number.isFinite(n)) cur.issues.push(n);
      } else cur[lista].push(v);
    }
  }
  return out;
}

export interface SedutaOrfana {
  id: string;
  /** Quante voci `verifies` hanno gia' un verdetto, su quante ne dichiara. */
  giudicate: number;
  dichiarate: number;
}

/** Le sedute `critical: true` che nessuno convoca e che nessuno ha eseguito.
 *
 *  Una seduta e' **convocata** se il suo `issues:` non e' vuoto, oppure se una issue — aperta o chiusa —
 *  la nomina come `` `U6` `` o «Seduta U6». E' **eseguita** se tutte le voci che dichiara di giudicare
 *  portano un verdetto diverso da ⏳ (e ne dichiara almeno una), oppure se un `artifacts` che dichiara
 *  e' tracciato da git: e' la derivazione che `editor-sessions.yaml` scrive di se' — *«lo stato della
 *  seduta → dalle voci `verifies` + `git ls-files` su `artifacts`»*.
 *
 *  ⚠️ Una seduta senza issue e senza `verifies` **non ha nessun modo di dirsi fatta**, e resta
 *  segnalata: e' il caso, non un buco del predicato. */
export function seduteOrfane(
  tutte: Seduta[],
  nominata: (id: string) => boolean,
  verdettoDi: (voce: string) => string | undefined,
  artefattoVivo: (path: string) => boolean,
): SedutaOrfana[] {
  const out: SedutaOrfana[] = [];
  for (const s of tutte) {
    if (!s.critical) continue;
    if (s.issues.length > 0) continue;
    if (nominata(s.id)) continue;
    const noti = s.verifies.filter((v) => verdettoDi(v) !== undefined);
    const giudicate = noti.filter((v) => verdettoDi(v) !== '⏳');
    const eseguita =
      (noti.length > 0 && giudicate.length === noti.length) || s.artifacts.some(artefattoVivo);
    if (eseguita) continue;
    out.push({ id: s.id, giudicate: giudicate.length, dichiarate: s.verifies.length });
  }
  return out;
}

/** Vero se il testo nomina la seduta: `` `U6` `` oppure «Seduta U6».
 *
 *  ⚠️ **Nominare non e' convocare**, e il predicato e' volutamente largo: e' un'asserzione **negativa**,
 *  e un predicato stretto la renderebbe vera per cecita'. Misurato il 2026-09-27 sulle 19 sedute
 *  critiche: `U5` ne trova 3 e `U6` sei, `U10` e `U14` **zero**. Il controllo positivo di questa
 *  asserzione e' proprio che `U5` — quella su cui il referto si era sbagliato — non esca. */
export function nominaSeduta(testo: string, id: string): boolean {
  return new RegExp('`' + id + '`|[Ss]eduta\\s+' + id + '\\b').test(testo);
}

// ---------------------------------------------------------------------------------------------
// A5 — le tre cardinalita' della roadmap
// ---------------------------------------------------------------------------------------------

export interface Cardinalita {
  /** Le epic di §3 col numero di checkpoint che ciascuna dichiara. */
  epic: { id: string; cp: number }[];
  /** Gli id di checkpoint **unici** trovati in §5, per prefisso di epic. */
  perEpic: Record<string, number>;
  /** La somma della colonna `CP` di §3. */
  somma: number;
  /** Gli id unici di §5, in tutto. */
  unici: number;
  /** I due numeri della riga «Totale: N epic, M checkpoint», o `null` se la riga manca. */
  totale: { epic: number; cp: number } | null;
}

/** Un checkpoint di §5, nelle **due** forme che la roadmap usa: `| **13.1** |` e `| **E21.1** |`.
 *  §4.1 rende il prefisso obbligatorio dal 2026-08-08, e le epic nate dopo lo portano. */
const CHECKPOINT = /^\| \*\*(?:~~)?(?:E)?(\d+\.\d+[a-z]?)(?:~~)?\*\*/;

/** Le tre cardinalita' di `roadmap-v0.1.md`. */
export function cardinalita(text: string): Cardinalita {
  const epic: { id: string; cp: number }[] = [];
  const idUnici = new Set<string>();
  let totale: { epic: number; cp: number } | null = null;
  let sezione = 0;
  for (const l of text.split(/\r?\n/)) {
    const h = l.match(/^## (\d+)\. /);
    if (h) sezione = Number(h[1]);
    if (sezione === 3) {
      const m = l.match(/^\| \*\*(E\d+)\*\* \| [^|]*\| [^|]*\| (\d+) \|/);
      if (m) epic.push({ id: m[1]!, cp: Number(m[2]) });
      const t = l.match(/\*\*Totale:\s*(\d+)\s*epic,\s*(\d+)\s*checkpoint\*\*/);
      if (t) totale = { epic: Number(t[1]), cp: Number(t[2]) };
    }
    if (sezione === 5) {
      const m = l.match(CHECKPOINT);
      if (m) idUnici.add(m[1]!);
    }
  }
  const perEpic: Record<string, number> = {};
  for (const id of idUnici) {
    const pre = id.split('.')[0]!;
    perEpic[pre] = (perEpic[pre] ?? 0) + 1;
  }
  return {
    epic,
    perEpic,
    somma: epic.reduce((a, e) => a + e.cp, 0),
    unici: idUnici.size,
    totale,
  };
}

/** Le epic per cui §3 e §5 non concordano. */
export function divergenze(c: Cardinalita): { epic: string; dichiarati: number; trovati: number }[] {
  return c.epic
    .map((e) => ({ epic: e.id, dichiarati: e.cp, trovati: c.perEpic[e.id.slice(1)] ?? 0 }))
    .filter((d) => d.dichiarati !== d.trovati);
}

// ---------------------------------------------------------------------------------------------
// Comando
// ---------------------------------------------------------------------------------------------

interface IssueGh {
  number: number;
  state: string;
  title: string;
  body: string;
}

/** Tutte le issue, aperte e chiuse, o `null` se GitHub non e' raggiungibile.
 *
 *  Chiuse comprese, e non e' un dettaglio: una issue chiusa non prescrive piu' niente — per questo
 *  `issue-refs.ts` le ignora — ma **ha convocato** una seduta e **ha aperto** un verdetto, che sono le
 *  due domande di `A2` e `A4`. Per `A3`, che legge cio' che una issue afferma oggi, si filtra su
 *  `OPEN`. */
function leggiIssue(repo: string): IssueGh[] | null {
  try {
    const raw = execFileSync(
      'gh',
      ['issue', 'list', '--repo', repo, '--state', 'all', '--limit', '4000', '--json', 'number,state,title,body'],
      { encoding: 'utf8', maxBuffer: 256 * 1024 * 1024, stdio: ['ignore', 'pipe', 'ignore'] },
    );
    const parsed = JSON.parse(raw);
    // Una lista vuota non e' un verde: una fetch che non fallisce e non porta dati fa passare tutto.
    if (!Array.isArray(parsed) || parsed.length === 0) return null;
    return parsed as IssueGh[];
  } catch {
    return null;
  }
}

type Esito = 'PASS' | 'FAIL' | 'NOT RUN';

function main(): void {
  const argv = process.argv.slice(2);
  const check = argv.includes('--check');
  const iRepo = argv.indexOf('--repo');
  const repo = iRepo >= 0 ? argv[iRepo + 1]! : DEFAULT_REPO;

  const leggi = (p: string) => readFileSync(join(REPO_ROOT, p), 'utf8');
  const registro = leggi(REGISTRO_PIE);
  const voci = registroPie(registro);
  const glifoDi = new Map(voci.map((v) => [v.id, v.glifo ?? undefined] as const));

  const esiti: { nome: string; esito: Esito; righe: string[] }[] = [];
  const dice = (nome: string, esito: Esito, righe: string[] = []) => esiti.push({ nome, esito, righe });

  // --- A1 -------------------------------------------------------------------------------------
  const t = totaliPie(registro);
  const a1: string[] = [];
  if (t.dichiarati === null) a1.push('la riga dei totali non e\' stata trovata nel registro');
  else {
    for (const k of ['voci', 'verde', 'parziale', 'fallita', 'aperta'] as const) {
      if (t.dichiarati[k] !== t.contati[k]) {
        a1.push(`${k}: dichiarato ${t.dichiarati[k]}, ricontato ${t.contati[k]}`);
      }
    }
  }
  for (const id of t.senzaMarcatore) a1.push(`${id}: la cella di stato non porta nessun marcatore`);
  dice(
    `A1 totali del registro PIE (ricontate ${t.contati.voci} voci: ` +
      `${t.contati.verde}/${t.contati.parziale}/${t.contati.fallita}/${t.contati.aperta})`,
    a1.length === 0 ? 'PASS' : 'FAIL',
    a1,
  );

  // --- A5 -------------------------------------------------------------------------------------
  const c = cardinalita(leggi(ROADMAP));
  const a5 = divergenze(c).map(
    (d) => `${d.epic}: §3 dichiara ${d.dichiarati} checkpoint, §5 ne porta ${d.trovati}`,
  );
  if (c.totale === null) a5.push('la riga «Totale: N epic, M checkpoint» non e\' stata trovata');
  else {
    if (c.totale.epic !== c.epic.length) a5.push(`epic: totale scritto ${c.totale.epic}, righe in §3 ${c.epic.length}`);
    if (c.totale.cp !== c.somma) a5.push(`checkpoint: totale scritto ${c.totale.cp}, somma della colonna CP ${c.somma}`);
    if (c.totale.cp !== c.unici) a5.push(`checkpoint: totale scritto ${c.totale.cp}, id unici in §5 ${c.unici}`);
  }
  dice(
    `A5 cardinalita' di roadmap-v0.1.md (${c.epic.length} epic in §3, somma CP ${c.somma}, ${c.unici} id unici in §5)`,
    a5.length === 0 ? 'PASS' : 'FAIL',
    a5,
  );

  // --- A2, A3, A4: serve GitHub ----------------------------------------------------------------
  const issues = leggiIssue(repo);
  if (issues === null) {
    const motivo = ['GitHub non raggiungibile (gh assente, non autenticato, o nessuna issue restituita)'];
    dice('A2 sedute critiche convocate o eseguite', 'NOT RUN', motivo);
    dice('A3 stato delle voci PIE citato dalle issue aperte', 'NOT RUN', motivo);
    dice('A4 verdetti ❌/🟡 la cui prima issue e\' chiusa', 'NOT RUN', motivo);
  } else {
    const stato = new Map(issues.map((i) => [i.number, i.state === 'OPEN' ? ('OPEN' as const) : ('CLOSED' as const)]));

    // A3
    const aperte = issues.filter((i) => i.state === 'OPEN');
    const a3: string[] = [];
    for (const i of aperte) {
      for (const d of contraddizioniPie(i.body ?? '', i.number, (v) => glifoDi.get(v))) {
        a3.push(`#${d.issue}:${d.riga} ${d.voce} — la issue dice ${d.detto}, il registro ${d.registro}  «${d.testo}»`);
      }
    }
    dice(`A3 stato delle voci PIE citato da ${aperte.length} issue aperte`, a3.length === 0 ? 'PASS' : 'FAIL', a3);

    // A4
    const rossi = voci.filter((v) => v.glifo === '❌' || v.glifo === '🟡');
    const a4 = daRigiudicare(voci, (n) => stato.get(n)).map(
      (d) => `${d.voce} (riga ${d.riga}) e' ${d.glifo}, ma #${d.causa} — la issue che il verdetto ha aperto — e' CLOSED`,
    );
    dice(`A4 verdetti da rigiudicare (su ${rossi.length} voci ❌ o 🟡)`, a4.length === 0 ? 'PASS' : 'FAIL', a4);

    // A2
    const tutte = sedute(leggi(SEDUTE));
    const critiche = tutte.filter((s) => s.critical);
    const corpi = issues.map((i) => (i.title ?? '') + '\n' + (i.body ?? ''));
    const tracciati = new Set(git(['ls-files']).split('\n').filter(Boolean));
    const a2 = seduteOrfane(
      tutte,
      (id) => corpi.some((testo) => nominaSeduta(testo, id)),
      (voce) => glifoDi.get(voce),
      (p) => tracciati.has(p),
    ).map(
      (s) =>
        `${s.id}: nessuna issue la nomina, e non risulta eseguita ` +
        `(${s.giudicate} verdetti su ${s.dichiarate} voci dichiarate)`,
    );
    dice(
      `A2 sedute critiche (${critiche.length} su ${tutte.length} lette da editor-sessions.yaml)`,
      a2.length === 0 ? 'PASS' : 'FAIL',
      a2,
    );
  }

  // --- referto ----------------------------------------------------------------------------------
  esiti.sort((a, b) => (a.nome < b.nome ? -1 : 1));
  for (const e of esiti) {
    console.error(`${e.esito.padEnd(7)} ${e.nome}`);
    for (const r of e.righe) console.error(`        ${r}`);
  }
  console.error(
    '\nA6 — «i percorsi citati dai documenti risolvono» NON e\' qui: e\' un comando a parte,\n' +
      '     `node tools/radar/doc-links.ts --check`. G14 si esegue con DUE comandi.',
  );

  const falliti = esiti.filter((e) => e.esito === 'FAIL');
  if (falliti.length > 0 && check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
