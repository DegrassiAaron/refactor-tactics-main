import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { join } from 'node:path';

import { REPO_ROOT } from './docs-corpus.ts';
import { git, isShallow } from './git.ts';
import {
  REGISTRO,
  prese,
  collisioniInAlbero,
  numeriAggiunti,
  collisioniFraRef,
  stessaPresaInGit,
} from './decision-ids.ts';

const VUOTO: ReadonlySet<string> = new Set();

/** Un diff finto con le sole teste che il gate legge. */
function diff(...righe: string[]): string {
  return ['--- a/' + REGISTRO, '+++ b/' + REGISTRO, ...righe].join('\n');
}

test('riconosce tutte e quattro le decorazioni con cui il registro scrive un numero', () => {
  // Il registro non e' omogeneo: le prime voci non hanno il grassetto, le ritirate sono barrate.
  // Una regex ancorata su `**` ne perde alcune IN SILENZIO, ed e' il falso negativo peggiore.
  const md = [
    '| D-001 | senza grassetto |',
    '| **D-182** | col grassetto |',
    '| ~~D-002~~ | barrata, senza grassetto |',
    '| ~~**D-044**~~ | barrata e in grassetto: il numero resta preso |',
  ].join('\n');

  assert.deepEqual(prese(md).map((p) => p.id), ['D-001', 'D-182', 'D-002', 'D-044']);
});

test('un numero CITATO nel corpo di una voce non e\' una presa', () => {
  // E' il rumore correlato al segnale: la riga che prende un numero e' proprio quella che ne cita di
  // piu', perche' il suo corpo e' la nota di provenienza coi controlli della misura. Senza il `|` di
  // chiusura della prima cella, questa riga sola produrrebbe cinque prese finte.
  const riga =
    '| **D-434** | Chiude ICON-TAX-1..6, supera [D-231](RT_PDR_00_Decision_Log.md) e ' +
    '[D-232], cita D-116 e D-264 | Accettata | Icone |';

  assert.deepEqual(prese(riga).map((p) => p.id), ['D-434']);
});

test('due righe con lo stesso numero sono una collisione, due diverse no', () => {
  const sano = prese('| **D-100** | a |\n| **D-101** | b |');
  assert.deepEqual(collisioniInAlbero(sano), []);

  const rotto = prese('| **D-100** | a |\n| **D-101** | b |\n| **D-100** | ancora, altrove |');
  assert.deepEqual(collisioniInAlbero(rotto), [{ id: 'D-100', righe: [1, 3] }]);
});

test('PROVA CHE SA FALLIRE — le tre collisioni storiche sono viste, e i loro genitori no', (t) => {
  // 🔴 Su un clone shallow gli oggetti vecchi non ci sono e `git show` fallisce con un messaggio che
  // non nomina ne' la causa ne' il rimedio. Si dichiara, invece di fallire in modo opaco.
  if (isShallow()) {
    t.skip('clone shallow: le fixture storiche non sono raggiungibili. Rimedio: git fetch --unshallow');
    return;
  }

  // La fixture e' gratis perche' e' in git. Tre forme diverse dello stesso difetto, tutte reali:
  //  · `a5041c57` — merge fra DUE RAMI SANI: ciascuno ha una riga `D-039`, il merge ne produce due.
  //    Git le fonde senza conflitto perche' atterrano in punti diversi della tabella;
  //  · `f1b2038c` — lo stesso merge ne crea TRE in un colpo (`D-041` `D-042` `D-043`), e NON `D-040`:
  //    quella riga era identica sui due lati, quindi git l'ha fusa in una sola;
  //  · `c4d5e6e8` — un commit NON-merge da una riga sola, che riusa un `D-091` gia' presente.
  const visti = (sha: string) =>
    collisioniInAlbero(prese(git(['show', `${sha}:${REGISTRO}`]))).map((c) => c.id);

  assert.deepEqual(visti('a5041c57'), ['D-039']);
  assert.deepEqual(visti('f1b2038c'), ['D-039', 'D-041', 'D-042', 'D-043']);
  assert.deepEqual(visti('c4d5e6e8'), ['D-091']);

  // I controlli NEGATIVI, che sono la meta' che conta: i genitori sono puliti, quindi il gate non e'
  // rosso per costruzione su qualunque albero storico.
  assert.deepEqual(visti('75b82645'), []); // ramo fix/282, prima del merge
  assert.deepEqual(visti('ea26c0f6'), []); // origin/main, prima del merge
  assert.deepEqual(visti('f640481a'), []); // il genitore del commit che riusa D-091
});

test('il registro di oggi e\' pulito, e la regex vede davvero qualcosa', () => {
  const presi = prese(readFileSync(join(REPO_ROOT, REGISTRO), 'utf8'));

  // Anti-vacuita': una regex rotta darebbe zero prese e zero collisioni, cioe' un verde identico a
  // quello vero. Senza questa riga il test sopravvive a una regex che non matcha piu' niente.
  assert.ok(presi.length > 400, 'la regex non trova piu\' le prese: verde per costruzione');
  assert.deepEqual(collisioniInAlbero(presi), []);
});

test('una riga MODIFICATA non e\' una presa: `+` e `-` sullo stesso numero si annullano', () => {
  // E' il falso positivo gia' vivo nel repository: un ramo che riformula una voce esistente produce
  // `+D-309` e `-D-309`. Contare le sole righe `+` lo riporterebbe come rivendicazione.
  const d = diff(
    '-| **D-309** | il testo vecchio |',
    '+| **D-309** | il testo riscritto |',
    '+| **D-433** | una presa vera |',
  );
  assert.deepEqual([...numeriAggiunti(d)], [['D-433', 1]]);
});

test('il conteggio e\' NETTO, non un insieme: riscrivere una riga e aggiungerne una seconda e\' una presa', () => {
  // Con due insiemi `+`/`−` questo caso si annullerebbe e il ramo non rivendicherebbe nulla: un falso
  // negativo che nasconde proprio una doppia riga in arrivo.
  const d = diff(
    '-| **D-420** | vecchio |',
    '+| **D-420** | riscritto |',
    '+| **D-420** | e una SECONDA riga con lo stesso numero |',
  );
  assert.deepEqual([...numeriAggiunti(d)], [['D-420', 1]]);
});

test('le intestazioni del diff non sono prese, e un diff vuoto non aggiunge niente', () => {
  assert.equal(numeriAggiunti('').size, 0);
  assert.equal(numeriAggiunti(diff('@@ -1 +1 @@', ' contesto invariato')).size, 0);
});

test('lo stesso numero su due ref e\' una collisione; numeri diversi no', () => {
  const collide = new Map([
    ['origin/issue/100-a', new Map([['D-433', 1]])],
    ['origin/issue/200-b', new Map([['D-433', 1], ['D-434', 1]])],
  ]);
  assert.deepEqual(collisioniFraRef(collide, VUOTO), [
    'D-433: rivendicato da 2 ref — origin/issue/100-a, origin/issue/200-b',
  ]);

  const sano = new Map([
    ['origin/issue/100-a', new Map([['D-433', 1]])],
    ['origin/issue/200-b', new Map([['D-434', 1]])],
  ]);
  assert.deepEqual(collisioniFraRef(sano, VUOTO), []);
});

test('PROVA CHE SA FALLIRE — il caso che ha motivato il gate: un numero GIA\' preso in main', () => {
  // 🔴 E' il caso `D-433`: prenotato, rilasciato, preso da un altro ramo e mergiato, mentre il primo
  // ramo continuava a portarlo. A rivendicarlo resta UN ref solo, e in albero compare UNA volta sola:
  // un gate che pretendesse due ref, o che guardasse solo l'albero, sarebbe verde.
  const unRefSolo = new Map([['origin/issue/1500-confine', new Map([['D-433', 1]])]]);

  assert.deepEqual(collisioniFraRef(unRefSolo, VUOTO), [], 'senza main non c\'e\' nulla da vedere');
  assert.deepEqual(collisioniFraRef(unRefSolo, new Set(['D-433'])), [
    "D-433: gia' preso in origin/main, e origin/issue/1500-confine lo rivendica di nuovo",
  ]);
});

test('PROVA CHE SA FALLIRE — due rami sani, ciascuno verde da solo', () => {
  // Il difetto che accade davvero non lascia traccia in nessun albero: su ciascun ramo il file e'
  // internamente sano. E' la ragione per cui il controllo in albero da solo sarebbe vero ma scaduto.
  const unRamo = prese('| **D-430** | la voce del primo ramo |');
  assert.deepEqual(collisioniInAlbero(unRamo), [], 'ogni ramo e\' sano da solo');

  const perRef = new Map([
    ['origin/issue/1317-integrity', numeriAggiunti(diff('+| **D-430** | la voce del primo ramo |'))],
    ['origin/issue/1498-il-morto', numeriAggiunti(diff('+| **D-430** | la voce del secondo ramo |'))],
  ]);
  assert.deepEqual(collisioniFraRef(perRef, VUOTO), [
    'D-430: rivendicato da 2 ref — origin/issue/1317-integrity, origin/issue/1498-il-morto',
  ]);
});

test('un ramo che aggiunge lo stesso numero due volte e\' segnalato da solo', () => {
  const perRef = new Map([['origin/issue/900-doppia', new Map([['D-500', 2]])]]);
  assert.deepEqual(collisioniFraRef(perRef, VUOTO), [
    'D-500: origin/issue/900-doppia lo aggiunge 2 volte nello stesso ramo',
  ]);
});

test('l\'ordine del referto non dipende dal locale della macchina', () => {
  // `localeCompare` darebbe un ordine diverso a seconda dell'ICU installato, e due referti dello
  // stesso stato smetterebbero di essere confrontabili fra sessioni.
  const perRef = new Map([
    ['origin/b', new Map([['D-300', 1], ['D-100', 1]])],
    ['origin/a', new Map([['D-300', 1], ['D-100', 1]])],
  ]);
  const out = collisioniFraRef(perRef, VUOTO);
  assert.deepEqual(out, [...out].sort((x, y) => (x < y ? -1 : x > y ? 1 : 0)));
  assert.ok(out[0].startsWith('D-100'));
});

// ---------------------------------------------------------------------------------------------
// #3480 — una presa CONDIVISA da branch impilati non e' due prese
// ---------------------------------------------------------------------------------------------

/** Un predicato finto: condividono la presa solo le coppie elencate, in qualunque ordine. */
function condivise(...coppie: [string, string][]): (id: string, a: string, b: string) => boolean {
  return (_id, a, b) => coppie.some(([x, y]) => (x === a && y === b) || (x === b && y === a));
}

test('PROVA CHE SA FALLIRE — due ref IMPILATI che condividono la presa non collidono, e la presa e\' riportata', () => {
  // E' il caso misurato il 2026-10-04: `issue/3469` creato sopra `issue/3468`, entrambi con D-454.
  const perRef = new Map([
    ['origin/issue/3468', new Map([['D-454', 1]])],
    ['origin/issue/3469', new Map([['D-454', 1]])],
  ]);
  const riportate: string[] = [];
  assert.deepEqual(
    collisioniFraRef(perRef, VUOTO, condivise(['origin/issue/3468', 'origin/issue/3469']), riportate),
    [],
  );
  assert.deepEqual(riportate, ['D-454: presa condivisa da origin/issue/3468, origin/issue/3469']);

  // Controllo positivo: senza il predicato e' la collisione di prima. Il caso sopra e' verde perche'
  // il predicato dice «condivisa», non perche' la funzione abbia smesso di contare.
  assert.deepEqual(collisioniFraRef(perRef, VUOTO), [
    'D-454: rivendicato da 2 ref — origin/issue/3468, origin/issue/3469',
  ]);
});

test('tre ref, due condividono e uno no: e\' una collisione, e il referto li nomina tutti', () => {
  const perRef = new Map([
    ['origin/a', new Map([['D-500', 1]])],
    ['origin/b', new Map([['D-500', 1]])],
    ['origin/c', new Map([['D-500', 1]])],
  ]);
  assert.deepEqual(collisioniFraRef(perRef, VUOTO, condivise(['origin/a', 'origin/b'])), [
    'D-500: rivendicato da 3 ref — origin/a, origin/b, origin/c',
  ]);
});

test('la condivisione si chiude per transitivita\': a~b e b~c sono UNA presa', () => {
  const perRef = new Map([
    ['origin/a', new Map([['D-501', 1]])],
    ['origin/b', new Map([['D-501', 1]])],
    ['origin/c', new Map([['D-501', 1]])],
  ]);
  assert.deepEqual(
    collisioniFraRef(perRef, VUOTO, condivise(['origin/a', 'origin/b'], ['origin/b', 'origin/c'])),
    [],
  );
});

test('una presa condivisa di un numero GIA\' preso in main resta una collisione', () => {
  const perRef = new Map([
    ['origin/a', new Map([['D-433', 1]])],
    ['origin/b', new Map([['D-433', 1]])],
  ]);
  assert.deepEqual(collisioniFraRef(perRef, new Set(['D-433']), condivise(['origin/a', 'origin/b'])), [
    "D-433: gia' preso in origin/main, e origin/a, origin/b lo rivendicano di nuovo",
  ]);
});

test('il predicato REALE, su commit veri: #3469 sopra #3468 condivide D-454, la base no', (t) => {
  // ⚠️ Commit della storia di `main` dal 2026-10-04: `06b76a716` e' il main di allora, `abe656ed7` la testa
  // di #3468 (che aggiunge D-454..456), `01f5e5dc6` la testa di #3469, costruita sopra. Permanenti, perche'
  // mergiati; su un clone shallow non ci sono, e il test lo dichiara invece di passare.
  if (isShallow()) {
    t.skip('clone shallow: la storia che il test legge non c\'e\'');
    return;
  }
  const base = '06b76a716';
  assert.equal(stessaPresaInGit(base, 'D-454', 'abe656ed7', '01f5e5dc6'), true,
    'branch impilati: il merge-base e\' #3468, che aggiunge D-454');
  assert.equal(stessaPresaInGit(base, 'D-454', '06b76a716', 'abe656ed7'), false,
    'controllo negativo: il merge-base e\' la base, che D-454 non ce l\'ha');
  assert.equal(stessaPresaInGit(base, 'D-999', 'abe656ed7', '01f5e5dc6'), false,
    'un numero che il merge-base non aggiunge non e\' condiviso');
  assert.equal(stessaPresaInGit(base, 'D-454', 'abe656ed7', 'non-un-ref-vero'), false,
    'se git non risponde il predicato dice FALSO: il dubbio non diventa un verde');
});
