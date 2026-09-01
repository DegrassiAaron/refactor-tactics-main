/** Verifica che i percorsi e i comandi citati dalle **issue aperte** esistano ancora nel repository.
 *
 *  Uso:  node tools/radar/issue-refs.ts [--check] [--json] [--repo OWNER/NOME]
 *
 *  Il problema che chiude: `doc-links.ts` guarda i **documenti**, e per una ragione dichiarata nel suo
 *  docstring non guarda la prosa. Nessuno guardava le **issue**, e una issue non e' prosa qualsiasi:
 *  contiene criteri di chiusura. Il 2026-08-31 e' stato misurato il costo — **63** correzioni in un
 *  giorno, trovate a mano da due sessioni in parallelo dieci giorni dopo la rimozione che le aveva
 *  prodotte. Fra queste, quattro issue portavano dentro una casella `- [ ]` un comando che `D-182`
 *  aveva portato fuori dal repository: criteri di accettazione che nessuno poteva soddisfare, e nessun
 *  comando lo diceva.
 *
 *  ## La regola che rende il gate utilizzabile: CANCELLATO, non ASSENTE
 *
 *  Un percorso che non esiste ha **due** cause opposte, e confonderle rende il gate inservibile:
 *   - **mai esistito** → e' un *deliverable*. La issue chiede di crearlo. ✅ Legittimo, non si segnala.
 *   - **esistito e cancellato** → e' un *riferimento morto*. 🔴 Questo si segnala.
 *
 *  `git ls-tree` non li distingue: per lui sono identici. `git log --diff-filter=D` si'. Sulla misura
 *  del 2026-09-01 la distinzione ha tolto **114** falsi positivi su 170 — senza di essa il gate
 *  segnalerebbe ogni file che una issue si propone di scrivere, e verrebbe disattivato al terzo giro.
 *
 *  ## Cosa NON verifica, dichiarato perche' non venga scoperto dopo
 *   - le **issue chiuse**: non danno istruzioni a nessuno;
 *   - i **commenti**: solo il corpo, che e' la parte che si legge come contratto;
 *   - i percorsi **fuori da inline code**: senza i backtick non c'e' modo di distinguere un percorso
 *     da una frase, ed e' la stessa scelta che `doc-links.ts` motiva per i documenti;
 *   - le **righe storiche** — barrate, in blockquote, o che citano la decisione che ha rimosso il file:
 *     una nota che dice *«`feature-registry.yaml` e' uscito con D-181»* e' la **cura**, non il difetto;
 *   - i percorsi con **brace o glob** (`RTHUD.{h,cpp}`, `feature-registry.*`): non sono percorsi
 *     letterali e risolverli richiederebbe indovinare;
 *   - se il file **esiste ma il contenuto e' cambiato**: il gate misura l'esistenza, non la verita'.
 *
 *  ## Rete assente
 *  Il gate legge GitHub. Senza `gh`, senza autenticazione o senza rete stampa **NOT RUN** ed esce **0**:
 *  `CLAUDE.md` §6 pretende che una verifica non eseguita si dichiari, e un gate che finge un verde
 *  offline e' peggio di uno che non gira.
 *
 *  La copertura si stampa **sempre**, anche in verde: un gate che non dice quanto ha guardato non e'
 *  distinguibile da uno che non guarda (#576). */
import { execFileSync } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';

const REPO_ROOT = fileURLToPath(new URL('../../', import.meta.url));

/** Il repository GitHub da interrogare, quando non arriva da `--repo`. */
const DEFAULT_REPO = 'DegrassiAaron/refactor-tactics-main';

/** Un riferimento a un percorso del repository, citato dal corpo di una issue. */
export interface IssueRef {
  /** Numero della issue. */
  issue: number;
  /** Riga del corpo, 1-based. */
  line: number;
  /** Il percorso come e' scritto, ripulito. */
  path: string;
  /** La riga che lo contiene, per far vedere il contesto senza aprire GitHub. */
  text: string;
}

/** Le radici sotto cui un token e' un percorso di questo repository e non una parola qualsiasi. */
const RADICI = [
  'docs/',
  'scripts/',
  'tools/',
  'Source/',
  'Content/',
  'Scenarios/',
  'Plugins/',
  'Config/',
];

/** Inline code a uno o piu' backtick. Un percorso fuori di qui e' prosa, e non si tocca. */
const INLINE_CODE = /`([^`\n]+?)`/g;

/** Punteggiatura che si appiccica a un percorso in una frase, e il suffisso `:riga`. */
const BORDI = /^[([{'"]+|[)\]},.;:'"]+$/g;
const SUFFISSO_RIGA = /:\d+$/;

/** I marcatori che rendono una riga **storica**: sta raccontando la rimozione, non prescrivendo. */
const STORICA = [
  /~~/, // barrato: il pattern che il repository usa per correggere nel corpo
  /\bD-18[12]\b/, // le due decisioni che hanno rimosso registro e scripts/
  /\b(26f6955a|d671df47)\b/, // i due commit di rimozione, citati per esteso dalle note
  /non vive (piu|più)/i,
  /non esiste (piu|più)?/i,
  /\b(uscit|ritirat|eliminat|rimoss)\w*/i,
  /non (e|è) citabile/i,
  /Rimisurato il \d{4}-\d{2}-\d{2}/i,
];

/** Il marcatore che esenta una issue, con il motivo sulla stessa riga.
 *
 *  Serve a **una** classe di casi: la issue il cui *oggetto* e' la rimozione. `#1165` documenta lo
 *  svuotamento di `docs/src/` e ne cita i percorsi 18 volte — barrarli snaturerebbe una issue corretta.
 *  Il motivo e' obbligatorio e viene stampato nella copertura: un'esenzione che non dice perche' e'
 *  indistinguibile da una dimenticanza. */
const ESENZIONE = /<!--\s*issue-refs:\s*ignora\s*[—-]\s*([^>]+?)\s*-->/;

/** Il motivo dell'esenzione, o `null` se la issue non e' esente. */
export function exemption(body: string): string | null {
  const m = body.match(ESENZIONE);
  return m ? m[1].trim() : null;
}

/** Vero se la riga sta **descrivendo** una rimozione invece di prescrivere un percorso vivo.
 *
 *  Il blockquote conta come storico perche' in questo repository la convenzione della nota additiva
 *  datata e' un blockquote: segnalarne il contenuto significherebbe segnalare la cura insieme alla
 *  malattia, ed e' esattamente il rumore che disattiva un gate. */
export function isHistorical(line: string): boolean {
  if (line.trimStart().startsWith('>')) return true;
  return STORICA.some((re) => re.test(line));
}

/** I percorsi del repository citati dentro inline code in una riga.
 *
 *  Un blocco di inline code puo' essere un **comando** (`python scripts/x.py validate`): si prende il
 *  primo token che somiglia a un percorso, che e' l'unica parte che il filesystem puo' falsificare. */
export function scanPaths(line: string): string[] {
  const out: string[] = [];
  for (const m of line.matchAll(INLINE_CODE)) {
    for (const raw of m[1].trim().split(/\s+/)) {
      const tok = raw.replace(BORDI, '').replace(SUFFISSO_RIGA, '');
      if (tok.includes('{') || tok.includes('*')) continue;
      if (tok.length <= 6) continue;
      if (!RADICI.some((r) => tok.startsWith(r))) continue;
      out.push(tok);
      break; // un solo percorso per blocco: il resto del comando sono argomenti
    }
  }
  return out;
}

/** I riferimenti morti nel corpo di una issue.
 *
 *  `isAlive` risponde se il percorso esiste **adesso**; `wasDeleted` se e' mai stato cancellato. Un
 *  percorso che non e' ne' vivo ne' cancellato e' un file **da creare**, e non e' un difetto. */
export function deadRefs(
  body: string,
  issue: number,
  isAlive: (p: string) => boolean,
  wasDeleted: (p: string) => boolean,
): IssueRef[] {
  const out: IssueRef[] = [];
  // I corpi delle issue arrivano con CRLF. Senza normalizzare, il numero di riga stampato non
  // corrisponde a quello che vede chi apre la issue — misurato il 2026-09-01 su #703: riga 71 via
  // `--json body`, riga 141 via `gh issue view -q .body`. Un gate che indica la riga sbagliata manda
  // a correggere il punto sbagliato.
  const righe = body.replace(/\r\n/g, '\n').split('\n');
  for (let i = 0; i < righe.length; i++) {
    const riga = righe[i];
    if (isHistorical(riga)) continue;
    for (const path of scanPaths(riga)) {
      if (isAlive(path)) continue;
      if (!wasDeleted(path)) continue; // mai esistito: e' un deliverable
      out.push({ issue, line: i + 1, path, text: riga.trim().slice(0, 120) });
    }
  }
  return out;
}

/** Un insieme di percorsi piu' le cartelle che li contengono: `docs/src/` conta come citabile se
 *  qualcosa sotto `docs/src/` esiste (o e' stato cancellato). */
export function withParents(paths: Iterable<string>): Set<string> {
  const s = new Set<string>();
  for (const p of paths) {
    s.add(p);
    const parti = p.split('/');
    for (let i = 1; i < parti.length; i++) s.add(parti.slice(0, i).join('/'));
  }
  return s;
}

function git(args: string[]): string {
  return execFileSync('git', args, { cwd: REPO_ROOT, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
}

/** Le issue aperte, o `null` se GitHub non e' raggiungibile: il chiamante deve dichiarare NOT RUN. */
function fetchIssues(repo: string): { number: number; title: string; body: string }[] | null {
  try {
    const raw = execFileSync(
      'gh',
      ['issue', 'list', '--repo', repo, '--state', 'open', '--limit', '1000', '--json', 'number,title,body'],
      { encoding: 'utf8', maxBuffer: 64 * 1024 * 1024, stdio: ['ignore', 'pipe', 'ignore'] },
    );
    const parsed = JSON.parse(raw);
    // Una lista vuota non e' un verde: e' il difetto che il §7.3 del referto del 2026-08-31 documenta
    // — una fetch che non fallisce e non porta dati fa passare qualunque cosa.
    if (!Array.isArray(parsed) || parsed.length === 0) return null;
    return parsed;
  } catch {
    return null;
  }
}

function main(): void {
  const argv = process.argv.slice(2);
  const check = argv.includes('--check');
  const json = argv.includes('--json');
  const repoFlag = argv.indexOf('--repo');
  const repo = repoFlag >= 0 ? argv[repoFlag + 1] : DEFAULT_REPO;

  const issues = fetchIssues(repo);
  if (issues === null) {
    console.error(
      'NOT RUN: GitHub non raggiungibile (gh assente, non autenticato, o nessuna issue restituita).\n' +
        '         Il gate NON dichiara un verde che non ha misurato — CLAUDE.md §6.',
    );
    return; // exit 0: non blocca chi lavora offline
  }

  const vivi = withParents(git(['ls-tree', '-r', '--name-only', 'HEAD']).split('\n').filter(Boolean));
  const cancellati = withParents(
    git(['log', '--diff-filter=D', '--name-only', '--pretty=format:', '--all'])
      .split('\n')
      .map((s) => s.trim())
      .filter(Boolean)
      .filter((p) => !vivi.has(p)),
  );

  // 🔴 Il falso verde piu' probabile di questo gate, e non fa rumore: `git log --diff-filter=D`
  // su un clone **shallow** non vede nessuna cancellazione, quindi `wasDeleted` e' sempre falso e il
  // gate passa qualunque cosa. `actions/checkout` fa depth=1 di default: senza `fetch-depth: 0` il
  // gate direbbe verde per sempre. Si dichiara NOT RUN invece di mentire.
  const shallow = git(['rev-parse', '--is-shallow-repository']).trim() === 'true';
  if (shallow || cancellati.size === 0) {
    console.error(
      'NOT RUN: la storia del repository non e\' completa' +
        (shallow ? ' (clone shallow)' : ' (nessuna cancellazione trovata)') +
        '.\n         Senza di essa nessun percorso risulta rimosso e il gate passerebbe tutto.\n' +
        '         In CI serve `fetch-depth: 0`; in locale `git fetch --unshallow`.',
    );
    return; // exit 0: NOT RUN non blocca, ma non e' un verde
  }

  const isAlive = (p: string) => vivi.has(p.replace(/\/$/, ''));
  const wasDeleted = (p: string) => cancellati.has(p.replace(/\/$/, ''));

  const morti: IssueRef[] = [];
  const esenti: { issue: number; motivo: string }[] = [];
  for (const i of issues) {
    const body = i.body ?? '';
    const motivo = exemption(body);
    if (motivo !== null) {
      esenti.push({ issue: i.number, motivo });
      continue;
    }
    morti.push(...deadRefs(body, i.number, isAlive, wasDeleted));
  }

  if (json) {
    console.log(JSON.stringify(morti, null, 2));
  }

  // La copertura si stampa **anche in verde** (#576), e le esenzioni si stampano **sempre**: una
  // esenzione silenziosa e' un buco che nessuno rivede.
  console.error(
    `issue aperte esaminate: ${issues.length - esenti.length} su ${issues.length}` +
      ` · percorsi vivi: ${vivi.size} · cancellati noti: ${cancellati.size}`,
  );
  for (const e of esenti) console.error(`  esente #${e.issue}: ${e.motivo}`);

  if (morti.length === 0) {
    console.error('nessuna issue cita un percorso rimosso dal repository');
    return;
  }

  const perIssue = new Map<number, IssueRef[]>();
  for (const m of morti) {
    if (!perIssue.has(m.issue)) perIssue.set(m.issue, []);
    perIssue.get(m.issue)!.push(m);
  }
  console.error(
    `\n${morti.length} riferimenti a percorsi RIMOSSI, in ${perIssue.size} issue aperte:`,
  );
  for (const [num, refs] of [...perIssue.entries()].sort((a, b) => a[0] - b[0])) {
    console.error(`  #${num}`);
    for (const r of refs) console.error(`    L${r.line}  ${r.path}\n           ${r.text}`);
  }
  console.error(
    '\nOgni riga qui sopra prescrive un percorso che il repository ha cancellato.\n' +
      'Una casella `- [ ]` che lo contiene e\' un criterio di chiusura non soddisfacibile.',
  );
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
