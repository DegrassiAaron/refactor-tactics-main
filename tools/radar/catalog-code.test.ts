import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  parseCpp,
  parseCatalogSections,
  parseSummaryTable,
  parsePerceptionTable,
  parseHeaderDocstrings,
  compare,
} from './catalog-code.ts';

test('le stat base si leggono dai literal C++, per eroe', () => {
  const cpp = [
    'Gadget->MaxHealth = 90;',
    'Gadget->MovePoints = 5;',
    'Gadget->VisionRange = 7;',
    'Gadget->PushResistance = 0;',
    'Gadget->Affinity = TEXT("Affinity.Electricity");',
    'Gadget->Weakness = TEXT("Affinity.Water");',
  ].join('\n');

  const heroes = parseCpp(cpp);

  assert.deepEqual(heroes.get('Gadget'), {
    health: 90,
    movePoints: 5,
    visionRange: 7,
    pushResistance: 0,
    affinity: 'Affinity.Electricity',
    weakness: 'Affinity.Water',
  });
});

test('le schede del catalogo danno i sei campi, con l affinita tradotta dall italiano', () => {
  // Forma reale: l'affinita' e' una PAROLA italiana, la debolezza porta gia' l'identificatore fra
  // backtick. E' l'asimmetria che un parser ingenuo non vede.
  const md = [
    '## 1. Gadget — il controller',
    '',
    '| Statistica | Valore |',
    '|---|---|',
    '| Salute | 90 |',
    '| Movimento | 5 MP |',
    '| Range visivo | 7 — *era 6, alzata da [D-073](x.md)* |',
    '| Resistenza Push | 0 |',
    '| Affinità | elettricità |',
    '| Debolezza | acqua (`Affinity.Water`) — decisa in CP 6.2 |',
  ].join('\n');

  const heroes = parseCatalogSections(md);

  assert.deepEqual(heroes.get('Gadget'), {
    health: 90,
    movePoints: 5,
    visionRange: 7,
    pushResistance: 0,
    affinity: 'Affinity.Electricity',
    weakness: 'Affinity.Water',
  });
});

test('la tabella di confronto §5 e una terza fonte, e puo divergere dalle schede', () => {
  // Ripete i valori delle schede. Nella PR #572 erano sbagliate entrambe, ma possono divergere fra
  // loro: e' il motivo per cui il confronto ha tre lati e non due.
  const md = [
    '## 5. Confronto rapido',
    '',
    '| Eroe | HP | MP | Vista | Push res. | Affinità | Identità in una riga |',
    '|---|---:|---:|---:|---:|---|---|',
    '| Gadget | 90 | 5 | 7 | 0 | elettricità | fragile, vede lontano |',
    '| Branth | 120 | 4 | 5 | 0 | strutture | cambia la mappa, lento |',
  ].join('\n');

  const rows = parseSummaryTable(md);

  assert.deepEqual(rows.get('Gadget'), {
    health: 90,
    movePoints: 5,
    visionRange: 7,
    pushResistance: 0,
    affinity: 'Affinity.Electricity',
  });
  assert.equal(rows.get('Branth')!.health, 120);
});

const SEI = (over = {}) => ({
  health: 90, movePoints: 5, visionRange: 7, pushResistance: 0,
  affinity: 'Affinity.Electricity', weakness: 'Affinity.Water', ...over,
});

test('una divergenza nomina eroe, campo e i valori di ogni fonte che lo dichiara', () => {
  const sections = new Map([['Gadget', SEI()]]);
  const summary = new Map([['Gadget', SEI()]]);
  const cpp = new Map([['Gadget', SEI({ health: 100 })]]);

  const { divergences } = compare(sections, summary, cpp);

  assert.equal(divergences.length, 1);
  const d = divergences[0]!;
  assert.equal(d.hero, 'Gadget');
  assert.equal(d.field, 'health');
  assert.deepEqual(d.values, { schede: 90, 'tabella §5': 90, 'C++': 100 });
});

test('il gate NON dice quale lato correggere: riporta i valori e si ferma', () => {
  // D-075 e' il precedente: il 2026-08-10 il codice aveva ragione e il catalogo era indietro. Un gate
  // che avesse indicato il codice come lato da riparare avrebbe annullato la decisione.
  const sections = new Map([['Branth', SEI({ pushResistance: 1 })]]);
  const summary = new Map([['Branth', SEI({ pushResistance: 1 })]]);
  const cpp = new Map([['Branth', SEI({ pushResistance: 0 })]]);

  const { divergences } = compare(sections, summary, cpp);
  const testo = JSON.stringify(divergences);

  assert.match(testo, /pushResistance/);
  assert.doesNotMatch(testo, /corregg|aggiorna|allinea/i);
});

test('la copertura conta le estrazioni per lato, e un campo assente non e uno zero', () => {
  const sections = new Map([['Gadget', SEI()]]);
  const summary = new Map([['Gadget', { health: 90 }]]);
  const cpp = new Map([['Gadget', SEI()]]);

  const { coverage } = compare(sections, summary, cpp);

  assert.equal(coverage.sections, 6);
  assert.equal(coverage.summary, 1);
  assert.equal(coverage.cpp, 6);
});

test('nella sezione §5 c e piu di una tabella, e si legge solo quella delle statistiche', () => {
  // Caso REALE: §5.1 elenca le risorse firma con le stesse colonne di larghezza — `| Eroe | Vista |
  // Ruolo | Risorsa firma | Ricarica su | Cap |` — e ha un `Eroe` e degli interi come la prima.
  // Letta per errore, sovrascriveva i valori buoni con quelli della tabella sbagliata.
  const md = [
    '## 5. Confronto rapido',
    '',
    '| Eroe | HP | MP | Vista | Push res. | Affinità | Identità in una riga |',
    '|---|---:|---:|---:|---:|---|---|',
    '| Gadget | 90 | 5 | 7 | 0 | elettricità | fragile |',
    '',
    '### 5.1 Percezione e risorsa firma',
    '',
    '| Eroe | Vista | Ruolo | Risorsa firma | Ricarica su | Cap |',
    '|---|---:|---|---|---|---:|',
    '| Gadget | 7 | Controller | Carica Conduttiva | interazione elettrica | 4 |',
  ].join('\n');

  const rows = parseSummaryTable(md);

  assert.equal(rows.size, 1);
  assert.deepEqual(rows.get('Gadget'), {
    health: 90, movePoints: 5, visionRange: 7, pushResistance: 0, affinity: 'Affinity.Electricity',
  });
});

test('la soglia d udito vive in §5.1, una quarta tabella con una sola colonna da confrontare', () => {
  // #686: e' un numero di bilanciamento che viveva SOLO in C++. Sta in `5.1 Percezione e risorsa firma`,
  // non nelle schede ne' nel confronto rapido — quindi serve un quarto lato, non una colonna in piu'.
  const md = [
    '### 5.1 Percezione e risorsa firma',
    '',
    '| Eroe | Vista | Soglia d\'udito | Ruolo | Risorsa firma | Ricarica su | Cap |',
    '|---|---:|---:|---|---|---|---:|',
    "| Gadget | 7 | 5 | Controller | Carica Conduttiva | interazione elettrica | 4 |",
    "| Phase | 5 | 3 | Support | Riserva Idrica | interazione con acqua | 4 |",
  ].join('\n');

  const rows = parsePerceptionTable(md);

  assert.equal(rows.get('Gadget')!.hearingThreshold, 5);
  assert.equal(rows.get('Phase')!.hearingThreshold, 3);
  // La `Vista` c'e' gia' nelle schede e nel §5: qui si legge solo cio' che questa tabella possiede da sola.
  assert.equal(rows.get('Gadget')!.visionRange, undefined);
});


test('le docstring dell header sono il quinto lato, e si leggono anche a capo', () => {
  // Forma REALE del blocco Doxygen: ogni riga comincia con ` * `, e un campo puo' spezzarsi a capo
  // (`resistenza` / `push 0`). Senza normalizzare il prefisso, quel campo non si leggerebbe e la
  // copertura cadrebbe per un a-capo invece che per un difetto.
  const h = [
    '\t/**',
    "\t * Costruisce **Gadget**, tecnico della conduzione (catalogo eroi v0.1): 90 HP, 5 MP, **vista 7** (era 6,",
    "\t * alzata da D-073 / #131: l'unico del roster che vede oltre il raggio 6), resistenza",
    "\t * push 0, affinita' elettricita', debolezza acqua.",
    '\t */',
    '\tstatic URTHeroData* MakeGadget();',
  ].join('\n');

  assert.deepEqual(parseHeaderDocstrings(h).get('Gadget'), {
    health: 90,
    movePoints: 5,
    visionRange: 7,
    pushResistance: 0,
  });
});

test('vince il valore CORRENTE, non la cifra della nota storica che lo segue', () => {
  // ⚠️ E' il caso che rende il parser non ingenuo. Le docstring corrette portano accanto il valore
  // vecchio (`era 6`, `era 100`) e i riferimenti alle decisioni (`D-069`, `#131`): tutte cifre, tutte
  // dopo quella buona. Un parser che prendesse l'ultima occorrenza dichiarerebbe il difetto appena
  // riparato.
  const h = [
    "\t * Costruisce **Wraith**, duellante predittivo: **90 HP** (era 100, abbassata da D-069 / #131),",
    "\t * **6 MP** (il piu' mobile), vista 6, resistenza push 0.",
  ].join('\n');

  const w = parseHeaderDocstrings(h).get('Wraith')!;
  assert.equal(w.health, 90);
  assert.equal(w.movePoints, 6);
  assert.equal(w.visionRange, 6);
});

test('una docstring che contraddice i literal fa divergere il quinto lato', () => {
  // Il difetto misurato su `4fdb01a0`: quattro lati verdi, la docstring falsa. Senza questo lato la
  // divergenza non esisteva per nessuno.
  const cpp = parseCpp('Branth->PushResistance = 0;');
  const header = parseHeaderDocstrings(
    "\t * Costruisce **Branth**, architetto del campo: 120 HP, 4 MP, vista 5, **resistenza push 1**.",
  );

  const { divergences } = compare(new Map(), new Map(), cpp, new Map(), header);
  const d = divergences.find((x) => x.field === 'pushResistance')!;

  assert.ok(d, 'la divergenza sulla resistenza push deve emergere');
  assert.equal(d.hero, 'Branth');
  // Il gate NON prescrive quale lato correggere: stampa i due valori e si ferma.
  assert.deepEqual(d.values, { 'C++': 0, 'docstring .h': 1 });
});

test('un blocco che non e un eroe non entra fra le fonti', () => {
  // L'header nomina anche helper e tipi. Un `Costruisce **qualcosa**` che non sia un nome d'eroe non
  // deve produrre una quinta fonte fantasma con zero campi, che abbasserebbe la copertura senza motivo.
  const h = "\t * Costruisce **il** roster completo della v0.1, nell'ordine del catalogo.";
  assert.equal(parseHeaderDocstrings(h).size, 0);
});
