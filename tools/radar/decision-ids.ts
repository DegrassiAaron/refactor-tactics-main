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
 *  **Tre controlli, e il primo da solo sarebbe scaduto.**
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
 *   3. **Contro `origin/main`** — un numero **gia' preso** e riproposto da un ramo solo. 🔴 E' il caso
 *      di `D-433`, cioe' esattamente quello che ha motivato questo gate: un numero prenotato, poi
 *      rilasciato, poi preso da un altro ramo e mergiato, mentre il primo ramo continuava a portarlo.
 *      Con due soli controlli il gate **non lo avrebbe visto**, perche' a rivendicarlo resta un ref
 *      solo e in albero compare una volta sola.
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
 *     vede i ref che esistono in `refs/remotes/origin`, e nient'altro. ⚠️ E vede solo i ref
 *     `refs/heads/*` del remoto: un ref fetchato altrove (`refs/pull/*`) risulta un fantasma;
 *   - **la sezione `## Note`**: e' la cronaca delle prese, non il registro, e non viene letta;
 *   - **le rivendicazioni fuori dal diff del registro** — un numero annunciato solo nel corpo di una
 *     issue o di una PR e' invisibile qui;
 *   - **una riga che rivendica piu' numeri insieme** (`| **D-435 · D-436** |`). Oggi in tabella sono
 *     zero, ma il registro annota prese a gruppo: se un giorno una riga cosi' compare, questo gate la
 *     legge come una presa sola e perde le altre. E' il falso negativo da sorvegliare.
 *
 *  ⛔ **`NOT RUN` non e' `PASS`** (CLAUDE.md §6). Se `git` non risponde, se `ls-remote` non porta dati,
 *  o se il clone e' shallow, il gate **si ferma prima del verdetto** e lo dichiara: un verde che non e'
 *  stato misurato e' peggio di nessun gate, perche' viene creduto.
 *
 *  ⛔ **Freschezza.** Un ref remoto locale puo' essere stantio: il ramo e' stato cancellato su GitHub e
 *  il suo numero non e' piu' rivendicato da nessuno. Non e' un dettaglio — alla prima stesura di questo
 *  gate l'**unico** reperto prodotto offline era esattamente questo, cioe' un falso positivo su uno su
 *  uno. Percio' `git ls-remote` fa parte del gate, e i ref scartati si stampano **per nome**: un'esenzione
 *  silenziosa e' un buco che nessuno rivede.
 *
 *  La copertura si stampa **sempre**, anche in verde: un gate che non dice quanto ha guardato non e'
 *  distinguibile da uno che non guarda (#576). E conta cio' che ha **letto**, non cio' che ha elencato. */

import { readFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';
import { join } from 'node:path';

import { REPO_ROOT } from './docs-corpus.ts';
import { git } from './git.ts';

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
export function collisioniInAlbero(presi: Presa[]): Collisione[] {
  const per = new Map<string, number[]>();
  for (const p of presi) {
    const righe = per.get(p.id);
    if (righe) righe.push(p.riga);
    else per.set(p.id, [p.riga]);
  }
  const out: Collisione[] = [];
  for (const [id, righe] of per) if (righe.length > 1) out.push({ id, righe });
  // Confronto fra code unit, non `localeCompare`: quello darebbe un ordine diverso a seconda del
  // locale della macchina, e due referti dello stesso stato smetterebbero di essere confrontabili.
  out.sort((a, b) => (a.id < b.id ? -1 : a.id > b.id ? 1 : 0));
  return out;
}

/** Quante volte un diff **aggiunge** ciascun numero: teste `+` meno teste `−`, per id.
 *
 *  ⛔ Non e' «le righe che cominciano per `+`». Una riga **modificata** produce una testa `+` e una
 *  testa `−` con lo stesso numero, e contare solo le `+` la riporta come presa: e' un falso positivo
 *  gia' vivo nel repository, su un ramo che riformula una voce esistente senza prenderne il numero.
 *
 *  ⚠️ E il conteggio e' **netto per numero, non un insieme**: un ramo che riscrive una riga `D-X` e
 *  nello stesso cambiamento ne aggiunge una **seconda** con lo stesso numero ha `+2 −1`, cioe' una
 *  presa. Con due insiemi si annullerebbero e il ramo non rivendicherebbe nulla — un falso negativo
 *  che nasconde proprio una doppia riga in arrivo. */
export function numeriAggiunti(diff: string): Map<string, number> {
  const netto = new Map<string, number>();
  for (const riga of diff.split(/\r?\n/)) {
    if (riga.startsWith('+++') || riga.startsWith('---')) continue;
    const segno = riga[0];
    if (segno !== '+' && segno !== '-') continue;
    const m = RIVENDICAZIONE.exec(riga.slice(1));
    if (!m) continue;
    netto.set(m[1], (netto.get(m[1]) ?? 0) + (segno === '+' ? 1 : -1));
  }
  for (const [id, n] of [...netto]) if (n <= 0) netto.delete(id);
  return netto;
}

/** Controlli 2 e 3 — chi rivendica un numero che non e' suo da solo.
 *
 *  Tre forme, e la terza e' quella che ha motivato il gate:
 *   - due ref diversi aggiungono lo stesso numero;
 *   - un ref lo aggiunge **piu' di una volta** (una doppia riga in arrivo);
 *   - un ref aggiunge un numero **gia' preso in `origin/main`**. 🔴 Con un ref solo a rivendicarlo,
 *     un controllo che pretenda due ref non lo vede — ed e' il caso `D-433`. */
export function collisioniFraRef(
  perRef: Map<string, Map<string, number>>,
  giaInMain: ReadonlySet<string>,
): string[] {
  const chiRivendica = new Map<string, string[]>();
  const doppioNelRef: string[] = [];
  for (const [ref, numeri] of perRef) {
    for (const [id, n] of numeri) {
      const chi = chiRivendica.get(id);
      if (chi) chi.push(ref);
      else chiRivendica.set(id, [ref]);
      if (n > 1) doppioNelRef.push(`${id}: ${ref} lo aggiunge ${n} volte nello stesso ramo`);
    }
  }
  const out: string[] = [];
  for (const [id, chi] of chiRivendica) {
    const ordinati = [...chi].sort();
    if (ordinati.length > 1) {
      out.push(`${id}: rivendicato da ${ordinati.length} ref — ${ordinati.join(', ')}`);
    } else if (giaInMain.has(id)) {
      out.push(`${id}: gia' preso in origin/main, e ${ordinati[0]} lo rivendica di nuovo`);
    }
  }
  out.push(...doppioNelRef);
  out.sort((a, b) => (a < b ? -1 : a > b ? 1 : 0));
  return out;
}

// ---------------------------------------------------------------------------------------------
// Comando
// ---------------------------------------------------------------------------------------------

function main(): void {
  const argv = process.argv.slice(2);
  const check = argv.includes('--check');

  // --- Controllo 1: l'albero di lavoro --------------------------------------------------------
  const testo = readFileSync(join(REPO_ROOT, REGISTRO), 'utf8');
  const presi = prese(testo);
  const problemi: string[] = [];
  for (const c of collisioniInAlbero(presi)) {
    problemi.push(`${c.id}: rivendicato due volte nello stesso file, righe ${c.righe.join(' e ')}`);
  }

  // --- Prerequisiti dei controlli 2 e 3, e nessuno di essi puo' diventare un verde --------------
  let baseSha: string;
  let vivi: string[];
  let inMain: Set<string>;
  try {
    baseSha = git(['rev-parse', '--short', 'origin/main']).trim();
    inMain = new Set(prese(git(['show', `origin/main:${REGISTRO}`])).map((p) => p.id));
    vivi = git([
      'for-each-ref', '--format=%(refname:short)',
      '--no-merged', 'origin/main', 'refs/remotes/origin',
    ]).split(/\r?\n/).filter(Boolean).filter((r) => r !== 'origin/HEAD');
  } catch (e) {
    const causa = e instanceof Error ? e.message.split('\n')[0] : String(e);
    stampaAlbero(presi, problemi, check, `NOT RUN sul confronto fra ref: git non risponde (${causa})`);
    return;
  }

  // Un `ls-remote` che riesce e non porta dati non e' un verde: renderebbe fantasma ogni ref e il
  // gate passerebbe qualunque cosa. E' lo stesso difetto che `issue-refs.ts` dichiara sulla fetch.
  let suRemoto: Set<string> | null = null;
  try {
    const out = git(['ls-remote', '--heads', 'origin']);
    const nomi = out.split(/\r?\n/).filter(Boolean)
      .map((r) => r.split('\t')[1]).filter(Boolean)
      .map((r) => 'origin/' + r.replace(/^refs\/heads\//, ''));
    if (nomi.length > 0) suRemoto = new Set(nomi);
  } catch {
    suRemoto = null;
  }
  if (suRemoto === null) {
    stampaAlbero(presi, problemi, check,
      'NOT RUN sul confronto fra ref: `git ls-remote` non ha portato dati (rete assente, remoto\n' +
      '         irraggiungibile, o nessun ramo restituito). Senza, un ramo gia\' cancellato su GitHub\n' +
      '         comparirebbe come rivendicazione viva, e un verde qui non sarebbe stato misurato.');
    return;
  }

  const fantasmi = vivi.filter((r) => !suRemoto.has(r));
  const daLeggere = vivi.filter((r) => suRemoto.has(r));

  const perRef = new Map<string, Map<string, number>>();
  const letti: string[] = [];
  const illeggibili: string[] = [];
  for (const ref of daLeggere) {
    try {
      // `-U0`: senza contesto. Le righe di questa tabella arrivano a decine di migliaia di caratteri,
      // e tre righe di contesto per ref sono decine di KB trasferiti e riletti per niente.
      const diff = git(['diff', '-U0', `origin/main...${ref}`, '--', REGISTRO]);
      const numeri = numeriAggiunti(diff);
      if (numeri.size) perRef.set(ref, numeri);
      letti.push(ref);
    } catch {
      illeggibili.push(ref);
    }
  }

  problemi.push(...collisioniFraRef(perRef, inMain));

  // --- Copertura, sempre, e conta cio' che ha LETTO --------------------------------------------
  console.error(
    `prese di numero lette: ${presi.length} in ${REGISTRO} · ` +
      `numeri gia' presi in origin/main ${baseSha}: ${inMain.size} · ` +
      `ref vivi letti: ${letti.length} su ${vivi.length}`,
  );
  if (fantasmi.length) {
    console.error(`  saltati, non esistono piu' sul remoto: ${fantasmi.join(', ')}`);
  }
  if (illeggibili.length) {
    console.error(`  ⚠️ ILLEGGIBILI, non misurati: ${illeggibili.join(', ')}`);
  }

  verdetto(problemi, check);
}

/** Il referto quando i controlli 2 e 3 non si sono potuti fare: si dichiara, e non si dice verde. */
function stampaAlbero(presi: Presa[], problemi: string[], check: boolean, motivo: string): void {
  console.error(`prese di numero lette: ${presi.length} in ${REGISTRO} · confronto fra ref: NON FATTO`);
  console.error(`⛔ ${motivo}\n         \`NOT RUN\` non e' \`PASS\` — CLAUDE.md §6.`);
  if (problemi.length === 0) {
    console.error("in albero nessun numero e' rivendicato due volte; il resto non e' stato misurato");
    return; // exit 0: NOT RUN non blocca chi lavora offline, ma non e' un verde
  }
  verdetto(problemi, check);
}

function verdetto(problemi: string[], check: boolean): void {
  if (problemi.length === 0) {
    console.error("nessun numero D- e' rivendicato due volte");
    return;
  }
  console.error(`\n${problemi.length} numeri rivendicati piu' di una volta:\n  ${problemi.join('\n  ')}`);
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
