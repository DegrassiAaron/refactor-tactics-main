/** Verifica che nessuna **ancora chiusa** resti muta in `capability-roadmaps.md`.
 *
 *  Uso:  node tools/radar/anchor-state.ts [--check] [--doc PERCORSO] [--repo OWNER/NOME]
 *
 *  ## Il problema che chiude
 *
 *  Quel documento dichiara una convenzione: **le issue chiuse si annotano, le aperte no**. Ne segue
 *  che un'ancora **muta si legge come aperta** — e nessuno la rimisura, perche' la fotografia di stato
 *  in testa al file non e' un gate, e' una data.
 *
 *  🔑 **Misurato tre volte in quattro giorni, e la terza dalla sessione che aveva appena certificato.**
 *  Il 2026-09-20 la certificazione e' durata ventidue minuti (`#543`); il 2026-09-21 e' scaduta con
 *  `#1805`, trovata il 2026-09-23; e lo stesso 2026-09-23, **due ore dopo** quella rimisura, si sono
 *  chiuse `#2579` e `#2745` — entrambe citate, entrambe mute. La prima l'ha chiusa la sessione stessa.
 *
 *  ⚠️ **E il controllo scritto quel giorno non le avrebbe viste**: confrontava le citazioni che
 *  *portano* una parola di stato con GitHub, e trovava zero divergenze — correttamente, perche' il
 *  buco non e' nelle annotazioni sbagliate, e' nelle annotazioni **assenti**.
 *
 *  ## Cosa conta come ancora: una RIGA DI TABELLA
 *
 *  Il documento dichiara la regola nella propria testa: *«la formula giusta e' "nessuna **ancora**",
 *  non "nessuna riga"»* — `#1754` e' chiusa, compare senza annotazione, ed e' citata **in prosa** come
 *  riferimento di codice: non dichiara nulla sul proprio stato.
 *
 *  Tradotto in un criterio meccanico: si guardano **solo le righe di tabella** (`|` in apertura). E'
 *  conservativo di proposito — un gate che segnala prosa produce rumore, e un gate rumoroso viene
 *  disattivato al terzo falso positivo, come `issue-refs.ts` dichiara nel proprio docstring.
 *
 *  ## Le due letture, e perche' sono due
 *
 *  Per ogni citazione si cerca una parola di stato in due finestre:
 *
 *  - **stretta** — dalla citazione alla successiva. Dice se quella citazione ha un'annotazione propria;
 *  - **larga** — dalla citazione alla fine della cella. Dice se e' coperta da un'annotazione di gruppo.
 *
 *  Si segnala solo quando **entrambe** tacciono. Senza la larga, la riga
 *  `#1626 · #1627 · #1629 · #1630 (chiuse)` — che e' **corretta** — darebbe tre falsi positivi su
 *  quattro: misurato il 2026-09-23, un controllo con la sola finestra stretta ne produceva sette.
 *
 *  ## Cosa NON verifica, dichiarato perche' non venga scoperto dopo
 *
 *  - le citazioni **fuori da una riga di tabella**, per la ragione sopra;
 *  - se un'annotazione e' **sbagliata** invece che assente: quello lo dice il confronto con GitHub, e
 *    questo gate non lo sostituisce — sono due difetti diversi e si trovano in due modi diversi;
 *  - se la **descrizione** accanto a un'ancora e' scaduta. `#2579 (solo per eroe)` era falsa nel merito
 *    — la issue e' chiusa proprio perche' il contributo non e' piu' solo per eroe — e nessun comando
 *    puo' accorgersene;
 *  - le **issue di un altro repository** citate come `OWNER/REPO#123`.
 *
 *  ## Rete assente
 *
 *  Il gate legge GitHub. Senza `gh`, senza autenticazione o senza rete stampa **NOT RUN** ed esce
 *  **0**: `CLAUDE.md` §6 pretende che una verifica non eseguita si dichiari, e un gate che finge un
 *  verde offline e' peggio di uno che non gira. E' la stessa scelta di `issue-refs.ts`.
 *
 *  La copertura si stampa **sempre**, anche in verde: un gate che non dice quanto ha guardato non e'
 *  distinguibile da uno che non guarda (#576). */
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

/** Le parole con cui il documento annota uno stato. `NOT_PLANNED` e `COMPLETED` compaiono quando la
 *  distinzione conta — una chiusura «come capitolo, non come lavoro» non e' lavoro annullato. */
const STATO = /\bchius[ae]\b|\bCLOSED\b|\bNOT_PLANNED\b|\bCOMPLETED\b|\batterrat[ae]\b/i;

/** Una citazione `#NNNN` dentro una riga di tabella. */
export interface Anchor {
  /** 1-based, come la stampa un editor. */
  line: number;
  issue: number;
  /** La riga intera, per il rapporto: chi legge deve capire dove intervenire senza aprire il file. */
  text: string;
}

/** Una citazione e' un'**ancora** se, dentro la propria cella, non ha prosa davanti.
 *
 *  Si guarda il tratto fra l'ultimo separatore d'elenco (`·`) — o l'inizio della cella — e la
 *  citazione: se, tolti markup e inline code, vi resta del testo, la citazione sta **dentro una frase**
 *  e non dichiara lo stato di niente.
 *
 *  🔴 **Senza questa regola il gate produce un falso positivo su tre**, misurato il 2026-09-23 sul
 *  documento vero: `| Shell × Replay | Main Menu e Result navigano verso lo stesso viewer di #472 |`
 *  e' una riga di tabella e una citazione legittima in prosa. `issue-refs.ts` dichiara nel proprio
 *  docstring il costo di sbagliare qui — *«verrebbe disattivato al terzo giro»* — e un gate disattivato
 *  vale meno di uno che non esiste, perche' qualcuno crede che stia guardando. */
function isAnchor(riga: string, at: number): boolean {
  const apreCella = riga.lastIndexOf('|', at);
  const inizio = Math.max(apreCella + 1, riga.lastIndexOf('·', at) + 1);
  const davanti = riga
    .slice(inizio, at)
    .replace(/`[^`]*`/g, '')  // inline code: `tools/radar/power.ts`, (`D-108`)
    .replace(/[*~_()\[\]\s,.—–-]/g, '');
  return davanti === '';
}

/** Le ancore CHIUSE che nessuna annotazione copre.
 *
 *  `closed` sono i numeri che GitHub dichiara chiusi. Passarlo dall'esterno e' cio' che rende questa
 *  funzione provabile senza rete — e il gate provabile senza GitHub. */
export function mutedClosedAnchors(doc: string, closed: ReadonlySet<number>): Anchor[] {
  const out: Anchor[] = [];

  doc.split('\n').forEach((riga, i) => {
    if (!riga.trimStart().startsWith('|')) return;

    const citazioni = [...riga.matchAll(/#(\d{2,5})/g)];
    citazioni.forEach((m, k) => {
      const issue = Number(m[1]);
      if (!closed.has(issue)) return;

      const da = m.index! + m[0].length;
      if (!isAnchor(riga, m.index!)) return;

      const prossima = citazioni[k + 1]?.index ?? riga.length;
      const stretta = riga.slice(da, prossima);
      // La cella finisce al `|` successivo; senza, la riga intera.
      const finCella = riga.indexOf('|', da);
      const larga = riga.slice(da, finCella === -1 ? riga.length : finCella);

      if (!STATO.test(stretta) && !STATO.test(larga)) {
        out.push({ line: i + 1, issue, text: riga.trim() });
      }
    });
  });

  return out;
}

/** I numeri citati dal documento, ovunque: la query a GitHub li chiede tutti in un colpo. */
export function citedIssues(doc: string): number[] {
  return [...new Set([...doc.matchAll(/#(\d{2,5})/g)].map((m) => Number(m[1])))].sort((a, b) => a - b);
}

/** Lo stato reale, da GitHub. `null` quando la rete o `gh` non ci sono — che NON e' un insieme vuoto:
 *  un insieme vuoto direbbe «nessuna chiusa» e produrrebbe il verde falso che questo gate rifiuta. */
export function closedOnGitHub(numbers: number[], repo: string): ReadonlySet<number> | null {
  if (numbers.length === 0) return new Set();
  const query = `{ repository(owner:"${repo.split('/')[0]}", name:"${repo.split('/')[1]}") { ${numbers
    .map((n) => `i${n}: issueOrPullRequest(number:${n}){ ... on Issue{number state} ... on PullRequest{number state} }`)
    .join(' ')} } }`;
  try {
    const out = execFileSync('gh', ['api', 'graphql', '-f', `query=${query}`], {
      encoding: 'utf8',
      stdio: ['ignore', 'pipe', 'pipe'],
    });
    const data = JSON.parse(out).data?.repository;
    if (!data) return null;
    return new Set(
      Object.values(data)
        .filter((v): v is { number: number; state: string } => !!v && typeof v === 'object')
        .filter((v) => v.state !== 'OPEN')
        .map((v) => v.number),
    );
  } catch {
    return null;
  }
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const { readFileSync } = await import('node:fs');
  const { fileURLToPath } = await import('node:url');

  const arg = (name: string, fallback: string) => {
    const i = process.argv.indexOf(name);
    return i !== -1 && process.argv[i + 1] ? process.argv[i + 1] : fallback;
  };
  const docPath = arg('--doc', fileURLToPath(new URL('../../docs/roadmap/capability-roadmaps.md', import.meta.url)));
  const repo = arg('--repo', 'DegrassiAaron/refactor-tactics-main');

  const doc = readFileSync(docPath, 'utf8');
  const cited = citedIssues(doc);
  const closed = closedOnGitHub(cited, repo);

  if (closed === null) {
    console.error(`NOT RUN: GitHub non raggiungibile (gh assente, non autenticato o rete giu'). ${cited.length} citazioni non verificate.`);
    process.exit(0);
  }

  const mute = mutedClosedAnchors(doc, closed);
  console.error(`copertura: ${cited.length} citazioni · ${closed.size} chiuse · ${mute.length} ancore mute`);

  if (mute.length > 0) {
    console.error(
      `errore: ${mute.length} ancore chiuse senza annotazione in ${docPath}.\n` +
        `Il documento dichiara che le chiuse si annotano: una muta si legge come aperta.\n` +
        mute.map((a) => `  L${a.line}  #${a.issue}  ${a.text.slice(0, 120)}`).join('\n'),
    );
    process.exit(1);
  }
  console.error('nessuna ancora chiusa e muta');
}
