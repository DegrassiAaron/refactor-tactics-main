import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import {
  compareActions,
  coreCatalogBody,
  parseActionCatalog,
  parseActionCpp,
  promisedCells,
  splitKnown,
} from './action-catalog.ts';

const GATE = fileURLToPath(new URL('./catalog-code.ts', import.meta.url));

/** Una tabella del catalogo, nella forma di §1: e' l'intestazione che dice quale cella e' quale. */
const HEADER = '| ActionId | Azione | Slot | Macro-fase | Cod. | Prio | Range | CD | Rumore | Fallback | Interr. |';
const RULE = '|---|---|---|---|---:|---:|---|---:|---:|---|---|';
const table = (...rows: string[]) => [HEADER, RULE, ...rows].join('\n');

/** Una chiamata a `ShippedAction` con la firma vera: id, fase, priorita', range, cooldown, fallback,
 *  effetti, e gli ultimi tre facoltativi. */
function call(
  id: string,
  phase: string,
  priority: number,
  range: number,
  cooldown: number,
  fallback: string,
  tail = '',
): string {
  return (
    `Catalog.Add(ShippedAction(TEXT("${id}"), ERTResolutionPhase::${phase}, /*Priority*/ ${priority},\n` +
    `    /*Range*/ ${range}, /*Cooldown*/ ${cooldown}, ERTActionFallback::${fallback}, {}${tail}));`
  );
}

const body = (...calls: string[]) =>
  `TArray<FRTActionDef> URTCatalogLibrary::GetCoreActionCatalog()\n{\n${calls.join('\n')}\n\treturn Catalog;\n}`;

function run(md: string, cpp: string) {
  const rows = parseActionCatalog(md);
  const parsed = parseActionCpp(coreCatalogBody(cpp));
  return compareActions(rows, parsed, promisedCells(md));
}

test('una divergenza in un campo numerico e rossa, e il referto porta i DUE valori', () => {
  const md = table('| `Action.Guard` | Guardia | Principale | **Prep** | 10 | 40 | self | 0 | 0 | `Fallback.Cancel` | no |');
  const cpp = body(call('Action.Guard', 'Preparation', 55, 0, 0, 'Cancel', ', ERTInterruptPolicy::None, ERTActionSlot::Main'));

  const r = run(md, cpp);
  assert.equal(r.divergences.length, 1);
  const d = r.divergences[0]!;
  assert.equal(d.actionId, 'Action.Guard');
  assert.equal(d.field, 'priority');
  assert.equal(d.catalog, 40);
  assert.equal(d.cpp, 55);
  // La riga del catalogo viene col referto: senza, chi legge la cerca a mano in un documento che porta una
  // tabella per famiglia di azioni.
  assert.equal(d.line, 3);
});

test('la fase si legge dal CODICE, non dalla macro-fase di Atlas', () => {
  // 🔴 Il difetto che questo pinna, misurato il 2026-09-18: confrontando la macro-fase, dodici azioni
  // risultavano divergenti perche' il catalogo scrive `Blast` dove il C++ dice `Control` o `Environment`.
  // `Blast` e' la macro-fase di Atlas e raccoglie piu' fasi del resolver: non e' il campo da confrontare.
  const md = table('| `Action.Push` | Spinta | Principale | Blast | 30 | 40 | 2 | 1 | 0 | `Fallback.Cancel` | sì |');
  const cpp = body(call('Action.Push', 'Control', 40, 2, 1, 'Cancel'));

  assert.deepEqual(run(md, cpp).divergences, []);
});

test('una cella `30/40` ammette due fasi, e il C++ ne sceglie una senza divergere', () => {
  const md = table('| `Action.Counter` | Contrattacco | Reazione | Blast | 30/40 | 20 | 0 | 2 | 0 | — | no |');
  const control = body(call('Action.Counter', 'Control', 20, 0, 2, 'Cancel', ', ERTInterruptPolicy::None, ERTActionSlot::Reaction'));
  const attack = body(call('Action.Counter', 'Attack', 20, 0, 2, 'Cancel', ', ERTInterruptPolicy::None, ERTActionSlot::Reaction'));
  const altrove = body(call('Action.Counter', 'Cleanup', 20, 0, 2, 'Cancel', ', ERTInterruptPolicy::None, ERTActionSlot::Reaction'));

  assert.deepEqual(run(md, control).divergences, []);
  assert.deepEqual(run(md, attack).divergences, []);
  // Una terza fase, che il catalogo NON ammette, resta una divergenza: l'appartenenza non e' un colabrodo.
  assert.equal(run(md, altrove).divergences.length, 1);
});

test('il codice 20 si sdoppia, e a dire quale delle due e la macro-fase', () => {
  const dash = table('| `Action.Sprint` | Scatto | **Movimento** | **Dash** | 20 | 60 | 8 MP | 0 | 5 | `Fallback.Stop` | sì |');
  const move = table('| `Action.Sprint` | Scatto | **Movimento** | **Move** | 20 | 60 | 8 MP | 0 | 5 | `Fallback.Stop` | sì |');
  const cpp = body(call('Action.Sprint', 'FastMovement', 60, 8, 0, 'Stop', ', ERTInterruptPolicy::InterruptBeforeEffect, ERTActionSlot::Movement'));

  assert.deepEqual(run(dash, cpp).divergences, []);
  const diverso = run(move, cpp).divergences;
  assert.equal(diverso.length, 1);
  assert.equal(diverso[0]!.catalog, 'NormalMovement');
  assert.equal(diverso[0]!.cpp, 'FastMovement');
});

test('un ID da un lato solo produce DUE diagnosi distinte, non una sola', () => {
  const md = table('| `Action.Solo` | Solo nel catalogo | Principale | Blast | 40 | 10 | 1 | 0 | 0 | `Fallback.Cancel` | sì |');
  const cpp = body(call('Action.Ignota', 'Attack', 10, 1, 0, 'Cancel'));

  const r = run(md, cpp);
  assert.deepEqual(r.onlyInCatalog, ['Action.Solo']);
  assert.deepEqual(r.onlyInCpp, ['Action.Ignota']);
  // Non si riparano allo stesso modo: un impegno non mantenuto dal codice non e' un numero senza autorita'.
  assert.notDeepEqual(r.onlyInCatalog, r.onlyInCpp);
});

test('un azione BARRATA nel catalogo e ritirata, e la sua assenza dal C++ non e un difetto', () => {
  const md = table('| `Action.Activate` | ~~Attiva~~ | — | — | 40 | 70 | 1 | 0 | — | — | — |');
  const r = run(md, body());
  assert.deepEqual(r.retired, ['Action.Activate']);
  assert.deepEqual(r.onlyInCatalog, [], 'una ritirata non va segnalata come mancante: sarebbe rumore');
});

test('i DEFAULT della firma C++ contano quanto i valori espliciti', () => {
  // Una chiamata che si ferma agli effetti dichiara `InterruptBeforeEffect` e `Main`. Leggerli come
  // «assenti» renderebbe il gate cieco proprio sulle azioni scritte in forma breve.
  const corta = parseActionCpp(coreCatalogBody(body(call('Action.Breve', 'Attack', 10, 1, 0, 'Cancel'))));
  assert.equal(corta.get('Action.Breve')?.slot, 'Main');
  assert.equal(corta.get('Action.Breve')?.interruptible, true);

  const md = table('| `Action.Breve` | Breve | Principale | Blast | 40 | 10 | 1 | 0 | 0 | `Fallback.Cancel` | no |');
  const r = run(md, body(call('Action.Breve', 'Attack', 10, 1, 0, 'Cancel')));
  assert.equal(r.divergences.length, 1, 'il catalogo dice `no`, il default del C++ dice interrompibile');
  assert.equal(r.divergences[0]!.field, 'interruptible');
});

test('un numero che diventa un etichetta abbassa la copertura invece di passare in silenzio', () => {
  // 🔴 Il modo di guasto temuto: se `Prio` smettesse di essere un intero, il campo uscirebbe dal confronto
  // e il gate resterebbe VERDE proprio quando servirebbe. `Cod.`, `Prio` e `CD` sono le colonne che devono
  // sempre leggersi, e la loro copertura e' una soglia.
  const sano = table('| `Action.X` | X | Principale | Blast | 40 | 10 | 1 | 0 | 0 | `Fallback.Cancel` | sì |');
  const rotto = table('| `Action.X` | X | Principale | Blast | 40 | media | 1 | 0 | 0 | `Fallback.Cancel` | sì |');

  const a = run(sano, body()).coverage;
  assert.equal(a.strictRead, a.strictPromised);

  const b = run(rotto, body()).coverage;
  assert.equal(b.strictPromised, 3, 'Cod. · Prio · CD restano promessi dall intestazione');
  assert.equal(b.strictRead, 2, 'e uno non si legge piu');
  assert.ok(b.strictRead < b.strictPromised, 'la copertura scende, ed e il segnale che il gate e cieco');
});

test('il range non e una soglia, perche il catalogo vi scrive legittimamente MP e prosa', () => {
  const md = table('| `Action.Move` | Movimento | **Movimento** | **Move** | 20 | 50 | 5 MP | 0 | — | `Fallback.Stop` | sì |');
  const r = run(md, body(call('Action.Move', 'NormalMovement', 50, 5, 0, 'Stop', ', ERTInterruptPolicy::InterruptBeforeEffect, ERTActionSlot::Movement')));

  assert.deepEqual(r.divergences, [], '`5 MP` non si confronta con `5`: sono grandezze diverse');
  assert.ok(r.coverage.catalogUnreadable > 0, 'e la cella non confrontabile viene CONTATA, non taciuta');
  assert.equal(r.coverage.strictRead, r.coverage.strictPromised, 'ma non abbassa la soglia');
});

test('una divergenza DICHIARATA non e nuova, e una dichiarazione senza difetto e stantia', () => {
  const md = table('| `Action.Guard` | Guardia | Principale | **Prep** | 10 | 40 | self | 0 | 0 | `Fallback.Cancel` | no |');
  const cpp = body(call('Action.Guard', 'Preparation', 55, 0, 0, 'Cancel', ', ERTInterruptPolicy::None, ERTActionSlot::Main'));
  const cmp = run(md, cpp);

  const noto = splitKnown(cmp, [
    { actionId: 'Action.Guard', field: 'priority', reason: 'divergenza aperta: #9999 possiede la scelta' },
  ]);
  assert.deepEqual(noto.unexpectedDivergences, [], 'una divergenza dichiarata non e una scoperta');
  assert.equal(noto.expected.length, 1);
  assert.deepEqual(noto.stale, []);

  // ⚠️ E quando il difetto sparisce, la dichiarazione deve sparire con lui: un elenco che sopravvive al
  // proprio difetto e' il posto in cui le divergenze vanno a dormire.
  const stantia = splitKnown(run(md, body(call('Action.Guard', 'Preparation', 40, 0, 0, 'Cancel', ', ERTInterruptPolicy::None, ERTActionSlot::Main'))), [
    { actionId: 'Action.Guard', field: 'priority', reason: 'divergenza aperta: #9999 possiede la scelta' },
  ]);
  assert.equal(stantia.stale.length, 1);
});

test('il gate non prescrive quale lato correggere', () => {
  // E' la scelta piu' importante che questo confronto eredita da `catalog-code.ts`: il 2026-08-10 il codice
  // aveva ragione e il catalogo era indietro (D-075). Un gate che indicasse il lato da riparare avrebbe
  // annullato la decisione.
  const src = readFileSync(GATE, 'utf8');
  const sezione = src.slice(src.indexOf('function checkActions'), src.indexOf('function main('));
  assert.ok(sezione.length > 0, 'la sezione azioni del gate deve esistere');
  assert.match(sezione, /Quale lato sia giusto NON lo dice questo gate/);
  for (const imperativo of [/\ballinea\b/i, /\bcorreggi\b/i, /\baggiorna il (catalogo|codice)\b/i, /\bripara\b/i]) {
    assert.doesNotMatch(sezione, imperativo, `il referto non deve dire cosa fare: ${imperativo}`);
  }
});

test('sul catalogo e sul C++ VERI il parser legge tutte le righe e tutte le voci', () => {
  // La controprova che le fixture qui sopra non stanno misurando un mondo inventato: gli stessi parser,
  // sui due file reali, non devono perdere per strada nessuna riga ne' nessuna chiamata.
  const md = readFileSync(fileURLToPath(new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url)), 'utf8');
  const cpp = readFileSync(fileURLToPath(new URL('../../Source/RefactorTactics/Ability/RTCatalogLibrary.cpp', import.meta.url)), 'utf8');
  const core = coreCatalogBody(cpp);

  const rows = parseActionCatalog(md);
  const actions = parseActionCpp(core);
  assert.equal(rows.length, (md.match(/^\|\s*`Action\./gm) ?? []).length);
  assert.equal(actions.size, (core.match(/ShippedAction\(/g) ?? []).length);

  const cov = compareActions(rows, actions, promisedCells(md)).coverage;
  assert.equal(cov.strictRead, cov.strictPromised, '`Cod.`, `Prio` e `CD` si leggono su ogni riga');
  assert.equal(cov.catalogFields + cov.catalogUnreadable, cov.catalogPromised);
});
