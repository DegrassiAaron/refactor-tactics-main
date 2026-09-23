/** Verifica che nessun numero `D-nnn` del Decision Log sia rivendicato due volte.
 *
 *  Uso:  node tools/radar/decision-ids.ts [--check]
 *
 *  **Il problema che chiude.** Il registro assegna i numeri a mano, e chi ne prende uno non vede cosa
 *  stanno prendendo gli altri. La sezione `## Note` del registro e' la cronaca del difetto: su **51**
 *  prese annotate, **18** raccontano una collisione, un numero ripreso o uno «perso in corsa»
 *  (`sed -n '/^## Note/,$p' docs/decisions/RT_PDR_00_Decision_Log.md | grep -cE '^- \*\*D-[0-9]'` e la
 *  stessa con `grep -ciE 'collision|ripres|perso in corsa|rivendicava'`). La risposta e' sempre stata
 *  *piu' disciplina* — misura a tre posti, poi a quattro, poi a cinque — e il difetto ha continuato a
 *  vincere. Questo gate la sostituisce con una macchina.
 *
 *  **Due controlli, e il secondo e' quello che serve oggi.**
 *
 *   1. **In albero** — due righe con lo stesso numero nello stesso file. E' il difetto del **merge**:
 *      due rami sani che atterrano in punti diversi della tabella si fondono senza conflitto e
 *      producono la doppia riga. Ha tre precedenti reali in git (`a5041c57` porta `D-039`,
 *      `f1b2038c` ne fa tre in un colpo, `c4d5e6e8` e' un commit da una riga sola su `D-091`).
 *      ⚠️ **Non si ripete dal 2026-08-11**: e' storia, e da solo questo controllo sarebbe *vero ma
 *      scaduto*.
 *   2. **Fra ref** — lo stesso numero aggiunto da due rami diversi rispetto a `origin/main`. E' il
 *      difetto che accade **adesso**: tutte e sette le collisioni dal 2026-09-08 in poi — `D-349`
 *      `D-350` `D-356` `D-364` `D-375` `D-403` `D-430` — vivono fra due ref, e su ciascun ramo il
 *      file e' internamente sano. Un gate a un albero e' **verde su entrambi** i rami che collidono.
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perche' non venga scoperto dopo:
 *   - **le decisioni, solo i numeri.** L'unico incidente davvero registrato, `D-044`, non era una
 *     collisione di numero (044 contro 060) ma di **contenuto**: due sessioni che decidevano la stessa
 *     cosa con numeri diversi. Questo gate non l'avrebbe preso, e un suo verde non significa «nessuno
 *     sta decidendo due volte la stessa cosa»;
 *   - **i buchi**. I numeri mancanti sono documentati e permanenti: segnalarli sarebbe rumore a ogni
 *     esecuzione, per sempre. E «il prossimo libero» non ha una risposta sola, perche' il registro
 *     dichiara aperti alcuni numeri saltati;
 *   - **i rami mai pushati**, su questa macchina o su un'altra, e le PR da fork non fetchate: il gate
 *     vede i ref che esistono in `refs/remotes/origin`, e nient'altro;
 *   - **la sezione `## Note`**: e' la cronaca delle prese, non il registro, e non viene letta;
 *   - **le rivendicazioni fuori dal diff del registro** — un numero annunciato solo nel corpo di una
 *     issue o di una PR e' invisibile qui;
 *   - **una riga che rivendica piu' numeri insieme** (`| **D-435 · D-436** |`). Oggi in tabella sono
 *     zero, ma il registro annota prese a gruppo: se un giorno una riga cosi' compare, questo gate la
 *     legge come una presa sola e perde le altre. E' il falso negativo da sorvegliare.
 *
 *  ⛔ **Freschezza.** Un ref remoto locale puo' essere stantio: il ramo e' stato cancellato su GitHub e
 *  il suo numero non e' piu' rivendicato da nessuno. Non e' un dettaglio — alla prima stesura di questo
 *  gate l'**unico** reperto prodotto offline era esattamente questo, cioe' un falso positivo su uno su
 *  uno. Percio' `git ls-remote` fa parte del gate, e senza rete il gate **lo dichiara** invece di
 *  fingere di aver guardato.
 *
 *  La copertura si stampa **sempre**, anche in verde: un gate che non dice quanto ha guardato non e'
 *  distinguibile da uno che non guarda (#576). */

import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
import { join } from 'node:path';

import { REPO_ROOT } from './docs-corpus.ts';

/** Il registro, e nient'altro. ⛔ Non si passa da `docsCorpus()`: fuori da questo file ci sono righe
 *  di forma rivendicazione — changelog, archivio, piani — e **tutte** duplicano numeri legittimamente.
 *  Allargare il perimetro produrrebbe decine di falsi positivi al primo giro. */
export const REGISTRO = 'docs/decisions/RT_PDR_00_Decision_Log.md';

/** Riconosce una **rivendicazione**, cioe' la prima cella di una riga-decisione — non un riferimento.
 *
 *  🔑 Il `\|` finale e' il pezzo che fa il lavoro: chiude la cella, e nessuna delle citazioni nel corpo
 *  puo' soddisfarlo. Senza, il gate raccoglie i `D-nnn` citati **dentro** il testo delle voci, che sono
 *  quattro volte piu' numerosi delle prese — e il rumore e' **correlato al segnale**, perche' la riga
 *  che prende un numero e' proprio quella che ne cita di piu': il suo corpo e' la nota di provenienza,
 *  coi controlli positivi e negativi della misura.
 *
 *  ⚠️ Le decorazioni sono opzionali e in ordine tilde-fuori/grassetto-dentro: il registro non e'
 *  omogeneo. Le prime voci scrivono `| D-001 |` senza grassetto e le ritirate `| ~~**D-044**~~ |` —
 *  una regex ancorata su `**` ne perde alcune **in silenzio**, che e' il falso negativo che un gate
 *  non si puo' permettere. Una voce ritirata occupa comunque il suo numero. */
export const RIVENDICAZIONE = /^\|\s*(?:~~)?\s*(?:\*\*)?\s*(D-\d{3,})\s*(?:\*\*)?\s*(?:~~)?\s*\|/;

export interface Presa {
  id: string;
  riga: number;
}

export interface Collisione {
  id: string;
  righe: number[];
}

/** Le prese di numero in un testo di registro, con la riga in cui stanno. */
export function prese(text: string): Presa[] {
  const out: Presa[] = [];
  const righe = text.split(/\r?\n/);
  for (let i = 0; i < righe.length; i++) {
    const m = RIVENDICAZIONE.exec(righe[i]);
    if (m) out.push({ id: m[1], riga: i + 1 });
  }
  return out;
}

/** Controllo 1 — lo stesso numero preso due volte nello **stesso** albero: il difetto del merge. */
export function collisioniInAlbero(text: string): Collisione[] {
  const per = new Map<string, number[]>();
  for (const p of prese(text)) {
    const righe = per.get(p.id);
    if (righe) righe.push(p.riga);
    else per.set(p.id, [p.riga]);
  }
  const out: Collisione[] = [];
  for (const [id, righe] of per) if (righe.length > 1) out.push({ id, righe });
  out.sort((a, b) => a.id.localeCompare(b.id));
  return out;
}

/** I numeri che un diff **aggiunge**, cioe' teste `+` meno teste `−`.
 *
 *  ⛔ Non e' «le righe che cominciano per `+`». Una riga **modificata** produce una testa `+` e una
 *  testa `−` con lo stesso numero, e contare solo le `+` la riporta come presa: e' un falso positivo
 *  gia' vivo nel repository, su un ramo che riformula una voce esistente senza prenderne il numero.
 *  La sottrazione lo chiude. ⚠️ Se qualcuno «semplifica» questa funzione togliendola, il rumore torna
 *  senza che nulla diventi rosso. */
export function numeriAggiunti(diff: string): string[] {
  const piu = new Set<string>();
  const meno = new Set<string>();
  for (const riga of diff.split(/\r?\n/)) {
    if (riga.startsWith('+++') || riga.startsWith('---')) continue;
    const segno = riga[0];
    if (segno !== '+' && segno !== '-') continue;
    const m = RIVENDICAZIONE.exec(riga.slice(1));
    if (!m) continue;
    (segno === '+' ? piu : meno).add(m[1]);
  }
  for (const id of meno) piu.delete(id);
  return [...piu].sort();
}

/** Controllo 2 — lo stesso numero aggiunto da piu' di un ref: il difetto che accade oggi. */
export function collisioniFraRef(perRef: Map<string, string[]>): Map<string, string[]> {
  const chiRivendica = new Map<string, string[]>();
  for (const [ref, ids] of perRef) {
    for (const id of ids) {
      const chi = chiRivendica.get(id);
      if (chi) chi.push(ref);
      else chiRivendica.set(id, [ref]);
    }
  }
  const out = new Map<string, string[]>();
  for (const [id, chi] of chiRivendica) if (chi.length > 1) out.set(id, chi.sort());
  return out;
}

// ---------------------------------------------------------------------------------------------
// Comando
// ---------------------------------------------------------------------------------------------

/** `git`, con il buffer alzato: il registro supera il megabyte e il default di Node va in `ENOBUFS`. */
function git(...args: string[]): string {
  return execFileSync('git', args, {
    cwd: REPO_ROOT,
    encoding: 'utf8',
    maxBuffer: 64 * 1024 * 1024,
  });
}

function main() {
  const argv = process.argv.slice(2);
  const check = argv.includes('--check');

  const problemi: string[] = [];

  // --- Controllo 1: l'albero di lavoro --------------------------------------------------------
  const testo = readFileSync(join(REPO_ROOT, REGISTRO), 'utf8');
  const inAlbero = collisioniInAlbero(testo);
  const totalePrese = prese(testo).length;
  for (const c of inAlbero) {
    problemi.push(`${c.id}: rivendicato due volte nello stesso file, righe ${c.righe.join(' e ')}`);
  }

  // --- Controllo 2: i ref che possono ancora rivendicare ---------------------------------------
  // I ref gia' mergiati in `origin/main` non rivendicano piu' niente, e per loro `...` e' vuoto **per
  // costruzione**: `--no-merged` non e' un'ottimizzazione opinabile, e' un filtro esatto.
  let vivi: string[] = [];
  let baseSha = '(sconosciuto)';
  let erroreGit = '';
  try {
    baseSha = git('rev-parse', '--short', 'origin/main').trim();
    vivi = git(
      'for-each-ref', '--format=%(refname:short)',
      '--no-merged', 'origin/main', 'refs/remotes/origin',
    ).split(/\r?\n/).filter(Boolean).filter((r) => r !== 'origin/HEAD');
  } catch (e) {
    erroreGit = e instanceof Error ? e.message.split('\n')[0] : String(e);
  }

  // Freschezza: un ref locale il cui ramo non esiste piu' sul remoto e' un fantasma, e il suo numero
  // non e' rivendicato da nessuno. Senza questo confronto il primo reperto del gate e' un falso
  // positivo — misurato, uno su uno.
  let suRemoto: Set<string> | null = null;
  try {
    const out = git('ls-remote', '--heads', 'origin');
    suRemoto = new Set(
      out.split(/\r?\n/).filter(Boolean)
        .map((r) => r.split('\t')[1])
        .filter(Boolean)
        .map((r) => 'origin/' + r.replace(/^refs\/heads\//, '')),
    );
  } catch {
    suRemoto = null; // niente rete: si dichiara, non si finge
  }

  const fantasmi = suRemoto ? vivi.filter((r) => !suRemoto.has(r)) : [];
  const esaminati = suRemoto ? vivi.filter((r) => suRemoto.has(r)) : vivi;

  const perRef = new Map<string, string[]>();
  for (const ref of esaminati) {
    try {
      const diff = git('diff', `origin/main...${ref}`, '--', REGISTRO);
      const ids = numeriAggiunti(diff);
      if (ids.length) perRef.set(ref, ids);
    } catch {
      // un ref irraggiungibile non e' una collisione: si salta e lo dice la copertura
    }
  }

  const fraRef = collisioniFraRef(perRef);
  for (const [id, chi] of [...fraRef].sort((a, b) => a[0].localeCompare(b[0]))) {
    problemi.push(`${id}: rivendicato da ${chi.length} ref — ${chi.join(', ')}`);
  }

  // --- Copertura, sempre ------------------------------------------------------------------------
  console.error(
    `prese di numero lette: ${totalePrese} in ${REGISTRO} · ` +
      `ref vivi esaminati: ${esaminati.length}` +
      (fantasmi.length ? ` (${fantasmi.length} saltati: non esistono piu' sul remoto)` : '') +
      ` · base origin/main ${baseSha}`,
  );
  if (erroreGit) {
    console.error(`⛔ git non risponde (${erroreGit}): il confronto fra ref NON e' stato fatto`);
  } else if (suRemoto === null) {
    console.error(
      "⚠️ `git ls-remote` non ha risposto: i ref locali non sono stati confrontati col remoto, " +
        'quindi un ramo gia\' cancellato su GitHub puo\' comparire qui come rivendicazione viva',
    );
  }

  if (problemi.length === 0) {
    console.error('nessun numero D- e\' rivendicato due volte');
    return;
  }
  console.error(`\n${problemi.length} numeri rivendicati piu' di una volta:\n  ${problemi.join('\n  ')}`);
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
