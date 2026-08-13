import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync, rmSync } from 'node:fs';
import { parseHeroCatalog, readCatalogs } from './parse-catalog.ts';

const HERO_CATALOG = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION_CATALOG = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);

test('legge le quattro statistiche di Gadget dal catalogo', () => {
  const heroes = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG);
  const flux = heroes.find((h) => h.name === 'Gadget');

  assert.ok(flux, 'Gadget deve esistere nel catalogo');
  assert.equal(flux.health, 90);
  assert.equal(flux.movePoints, 5);
  assert.equal(flux.visionRange, 7);
  assert.equal(flux.pushResistance, 0);
});

test('legge le abilita di Gadget con danno e cooldown', () => {
  const flux = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG).find((h) => h.name === 'Gadget')!;

  assert.equal(flux.abilities.length, 5);

  const arcPulse = flux.abilities.find((a) => a.id === 'Flux.ArcPulse')!;
  assert.equal(arcPulse.damage, 22);
  assert.equal(arcPulse.cooldown, 0);
  assert.equal(arcPulse.kind, 'attacco base');
});

test('legge i quattro eroi del roster e le loro venti abilita', () => {
  const heroes = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG);

  assert.deepEqual(
    heroes.map((h) => h.name).sort(),
    // Ordinata: il confronto e' su `.sort()`. La lista era alfabetica sui nomi VECCHI
    // (`Bastion, Flux, Riva, Vektor`) e la rinomina di D-120 non la riordina — e' il caso in cui una
    // sostituzione meccanica produce un letterale corretto nei nomi e sbagliato nell'ordine.
    ['Gadget', 'Phase', 'Riktor', 'Wraith'],
  );
  assert.equal(
    heroes.reduce((n, h) => n + h.abilities.length, 0),
    20,
  );
});

test('una statistica rinominata fa fallire il parser nominando il campo', () => {
  const rotto = readFileSync(HERO_CATALOG, 'utf8').replace('| Salute | 120 |', '| HP | 120 |');
  const tmp = new URL('./catalogo-rotto.tmp.md', import.meta.url);
  writeFileSync(tmp, rotto);

  try {
    assert.throws(() => parseHeroCatalog(tmp, ACTION_CATALOG), /Riktor.*"Salute" assente/);
  } finally {
    rmSync(tmp);
  }
});

test('risolve il rinvio `e Action.X` sul catalogo azioni (D-115)', () => {
  const flux = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG).find((h) => h.name === 'Gadget')!;
  const node = flux.abilities.find((a) => a.id === 'Flux.ConductiveNode')!;

  // Il catalogo eroi non porta il numero: viene da `Action.Electrify` (20 danni).
  assert.equal(node.damage, 20);
  assert.equal(node.delegatesTo, 'Action.Electrify');
});

test('la copertura e dichiarata, e un eroe in meno la fa fallire', () => {
  const { heroes, coverage } = readCatalogs(HERO_CATALOG, ACTION_CATALOG, { expectHeroes: 4, expectAbilities: 20 });
  assert.equal(heroes.length, 4);
  assert.equal(coverage, '4/4 eroi · 20 abilita · 1 delega risolta');

  // Un eroe che sparisce non deve produrre un risultato piu' corto in silenzio. Il filtro delle
  // sezioni eroe guarda l'intestazione della tabella, non il titolo: e' quella che va rotta perche'
  // Riktor diventi invisibile al parser.
  // Il catalogo e' salvato con CRLF: la sostituzione non puo' assumere `\n`.
  const senzaRiktor = readFileSync(HERO_CATALOG, 'utf8').replace(
    /\| Statistica \| Valore \|(\r?\n\|---\|---:\|\r?\n\| Salute \| 120 \|)/,
    '| Attributo | Valore |$1',
  );
  const tmp = new URL('./catalogo-corto.tmp.md', import.meta.url);
  writeFileSync(tmp, senzaRiktor);

  try {
    assert.throws(
      () => readCatalogs(tmp, ACTION_CATALOG, { expectHeroes: 4, expectAbilities: 20 }),
      /letti 3 eroi, attesi 4/,
    );
  } finally {
    rmSync(tmp);
  }
});

test('anche un abilita persa fa fallire la copertura, non solo un eroe', () => {
  // Una nota in coda alla cella dell'id rende la riga invisibile al parser — e' la forma che
  // `Action.Sprint` usa gia' nel catalogo azioni, quindi non e' un caso inventato. Senza un
  // atteso sulle abilita', Gadget ne perderebbe una e il roster resterebbe di quattro.
  const conNota = readFileSync(HERO_CATALOG, 'utf8').replace(
    '| `Flux.Overload` |',
    '| `Flux.Overload` *(vedi §7)* |',
  );
  const tmp = new URL('./catalogo-abilita.tmp.md', import.meta.url);
  writeFileSync(tmp, conNota);

  try {
    assert.throws(
      () => readCatalogs(tmp, ACTION_CATALOG, { expectHeroes: 4, expectAbilities: 20 }),
      /letta 19 abilita, attese 20/,
    );
  } finally {
    rmSync(tmp);
  }
});

test('citare un Action.X senza `e` NON delega: si tengono i numeri inline', () => {
  // Il rischio che D-115 nomina: un'abilita' che RIUSA la semantica di un'azione core tenendo il
  // proprio danno. Nel catalogo di oggi quelle righe stanno in una tabella con meno colonne, quindi
  // il filtro le scarta comunque — questo caso le mette dove il filtro NON protegge, cioe' dentro
  // la tabella delle abilita'. Senza la precisione del pattern, `damage` diventerebbe 20.
  const sintetico = readFileSync(HERO_CATALOG, 'utf8').replace(
    '| `Flux.ArcPulse` | Impulso ad arco | attacco base | 22 danni, range 4 | 0 |',
    '| `Flux.ArcPulse` | Impulso ad arco | attacco base | 22 danni, riusa `Action.Electrify` | 0 |',
  );
  const tmp = new URL('./catalogo-riuso.tmp.md', import.meta.url);
  writeFileSync(tmp, sintetico);

  try {
    const arcPulse = parseHeroCatalog(tmp, ACTION_CATALOG)
      .find((h) => h.name === 'Gadget')!
      .abilities.find((a) => a.id === 'Flux.ArcPulse')!;

    assert.equal(arcPulse.delegatesTo, null, 'citare non e delegare');
    assert.equal(arcPulse.damage, 22, 'il danno resta quello inline, non quello dell azione core');
  } finally {
    rmSync(tmp);
  }
});

test('la tabella reazioni si unisce all abilita, senza duplicarla', () => {
  const heroes = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG);
  const flux = heroes.find((h) => h.name === 'Gadget')!;

  // `ReactiveCapacitor` e' dichiarato DUE volte: tabella reazioni e tabella abilita'. Deve restare
  // una sola voce, con i dati uniti — non due abilita' omonime.
  const capacitors = flux.abilities.filter((a) => a.id === 'Flux.ReactiveCapacitor');
  assert.equal(capacitors.length, 1);
  assert.equal(flux.abilities.length, 5);

  assert.deepEqual(capacitors[0].reaction, {
    coreSemantics: 'Action.Counter',
    status: 'ready',
    note: 'scudo 15 **e** 10 danni all\'attaccante',
  });
});

test('una reazione rinviata a E14 dichiara status deferred e nessuna semantica core', () => {
  const vektor = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG).find((h) => h.name === 'Wraith')!;
  const intercept = vektor.abilities.find((a) => a.id === 'Vektor.InterceptShot')!;

  assert.equal(intercept.reaction?.status, 'deferred');
  assert.equal(intercept.reaction?.coreSemantics, null);
  // La nota resta grezza: e' da qui che la rubrica ricava la condizionalita' predittiva.
  assert.match(intercept.reaction!.note, /trigger d'ingresso su movimento/);
});

test('un abilita che non e una reazione ha reaction null', () => {
  const flux = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG).find((h) => h.name === 'Gadget')!;
  assert.equal(flux.abilities.find((a) => a.id === 'Flux.ArcPulse')!.reaction, null);
});

test('il bonus condizionale esce come numero, separato dal danno garantito', () => {
  const flux = parseHeroCatalog(HERO_CATALOG, ACTION_CATALOG).find((h) => h.name === 'Gadget')!;

  // `24 danni, **+8 su bersaglio Wet**` -> garantito 24, condizionale 8.
  const linear = flux.abilities.find((a) => a.id === 'Flux.LinearDischarge')!;
  assert.equal(linear.damage, 24);
  assert.equal(linear.conditionalBonus, 8);

  // Chi non ha bonus condizionali lo dichiara con null, non con zero.
  assert.equal(flux.abilities.find((a) => a.id === 'Flux.ArcPulse')!.conditionalBonus, null);
});
