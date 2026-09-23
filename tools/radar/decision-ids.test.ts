import { test } from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { join } from 'node:path';

import { REPO_ROOT } from './docs-corpus.ts';
import {
  REGISTRO,
  prese,
  collisioniInAlbero,
  numeriAggiunti,
  collisioniFraRef,
} from './decision-ids.ts';

function mostra(sha: string): string {
  return execFileSync('git', ['show', `${sha}:${REGISTRO}`], {
    cwd: REPO_ROOT,
    encoding: 'utf8',
    maxBuffer: 64 * 1024 * 1024,
  });
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
  const sano = '| **D-100** | a |\n| **D-101** | b |';
  assert.deepEqual(collisioniInAlbero(sano), []);

  const rotto = '| **D-100** | a |\n| **D-101** | b |\n| **D-100** | ancora, altrove |';
  assert.deepEqual(collisioniInAlbero(rotto), [{ id: 'D-100', righe: [1, 3] }]);
});

test('PROVA CHE SA FALLIRE — le tre collisioni storiche sono viste, e i loro genitori no', () => {
  // La fixture e' gratis perche' e' in git. Tre forme diverse dello stesso difetto, tutte reali:
  //  · `a5041c57` — merge fra DUE RAMI SANI: ciascuno ha una riga `D-039`, il merge ne produce due.
  //    Git le fonde senza conflitto perche' atterrano in punti diversi della tabella;
  //  · `f1b2038c` — lo stesso merge ne crea TRE in un colpo (`D-041` `D-042` `D-043`), e NON `D-040`:
  //    quella riga era identica sui due lati, quindi git l'ha fusa in una sola;
  //  · `c4d5e6e8` — un commit NON-merge da una riga sola, che riusa un `D-091` gia' presente.
  const visti = (sha: string) => collisioniInAlbero(mostra(sha)).map((c) => c.id);

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
  const testo = readFileSync(join(REPO_ROOT, REGISTRO), 'utf8');

  // Anti-vacuita': una regex rotta darebbe zero prese e zero collisioni, cioe' un verde identico a
  // quello vero. Senza questa riga il test sopravvive a una regex che non matcha piu' niente.
  assert.ok(prese(testo).length > 400, 'la regex non trova piu\' le prese: verde per costruzione');
  assert.deepEqual(collisioniInAlbero(testo), []);
});

test('una riga MODIFICATA non e\' una presa: `+` e `-` sullo stesso numero si annullano', () => {
  // E' il falso positivo gia' vivo nel repository: un ramo che riformula una voce esistente produce
  // `+D-309` e `-D-309`. Contare le sole righe `+` lo riporterebbe come rivendicazione.
  const diff = [
    '--- a/docs/decisions/RT_PDR_00_Decision_Log.md',
    '+++ b/docs/decisions/RT_PDR_00_Decision_Log.md',
    '-| **D-309** | il testo vecchio |',
    '+| **D-309** | il testo riscritto |',
    '+| **D-433** | una presa vera |',
  ].join('\n');

  assert.deepEqual(numeriAggiunti(diff), ['D-433']);
});

test('le intestazioni del diff non sono prese, e un diff vuoto non aggiunge niente', () => {
  assert.deepEqual(numeriAggiunti(''), []);
  assert.deepEqual(
    numeriAggiunti('--- a/f\n+++ b/f\n@@ -1 +1 @@\n contesto invariato'),
    [],
  );
});

test('lo stesso numero su due ref e\' una collisione; numeri diversi no', () => {
  const collide = new Map([
    ['origin/issue/100-a', ['D-433']],
    ['origin/issue/200-b', ['D-433', 'D-434']],
  ]);
  const esito = collisioniFraRef(collide);
  assert.deepEqual([...esito.keys()], ['D-433']);
  assert.deepEqual(esito.get('D-433'), ['origin/issue/100-a', 'origin/issue/200-b']);

  const sano = new Map([
    ['origin/issue/100-a', ['D-433']],
    ['origin/issue/200-b', ['D-434']],
  ]);
  assert.equal(collisioniFraRef(sano).size, 0);
});

test('PROVA CHE SA FALLIRE — il caso che oggi nessun albero contiene: due rami, lo stesso numero', () => {
  // Il difetto che accade davvero non lascia traccia in nessun albero: su ciascun ramo il file e'
  // internamente sano. E' la ragione per cui il controllo 1 da solo sarebbe vero ma scaduto, e questo
  // test e' l'unico posto dove quel caso si puo' esercitare senza inventare due rami veri.
  const dueRamiSani = '| **D-430** | la voce del primo ramo |';
  assert.deepEqual(collisioniInAlbero(dueRamiSani), [], 'ogni ramo e\' sano da solo');

  const perRef = new Map([
    ['origin/issue/1317-integrity', numeriAggiunti('+| **D-430** | la voce del primo ramo |')],
    ['origin/issue/1498-il-morto', numeriAggiunti('+| **D-430** | la voce del secondo ramo |')],
  ]);
  assert.deepEqual([...collisioniFraRef(perRef).keys()], ['D-430']);
});
