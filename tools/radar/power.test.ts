import { test } from 'node:test';
import assert from 'node:assert/strict';
import { parseHeroCatalog } from './parse-catalog.ts';
import { rate } from './rubric.ts';
import {
  abilityBands,
  abilityContributions,
  guaranteedDamage,
  contributionPerTurn,
  POWER_ANCHOR,
  POWER_WEIGHT_SCALE,
  powerRaw,
  powerRating,
} from './power.ts';

const HERO = new URL('../../docs/balance/RT_HeroCatalog_v0.1.md', import.meta.url);
const ACTION = new URL('../../docs/balance/RT_ActionCatalog_v0.1.md', import.meta.url);
const roster = () => parseHeroCatalog(HERO, ACTION);
const ability = (h: string, id: string) =>
  roster().find((x) => x.name === h)!.abilities.find((a) => a.id === id)!;

test('un bonus condizionato da uno stato non entra nel danno garantito', () => {
  // `24 danni, +8 su bersaglio Wet` -> 24, non 32.
  assert.equal(guaranteedDamage(ability('Aevik', 'Hero.Aevik.LinearDischarge')), 24);
});

test('un payoff condizionato da una previsione non entra affatto', () => {
  // Stessa regola, condizionante diverso: la tabella reazioni dichiara il trigger d'ingresso su
  // movimento, quindi i 16 danni valgono solo se hai indovinato dove andra' l'avversario.
  assert.equal(guaranteedDamage(ability('Ivrin', 'Hero.Ivrin.InterceptShot')), 0);
});

test('un danno incondizionato entra per intero', () => {
  assert.equal(guaranteedDamage(ability('Ivrin', 'Hero.Ivrin.PulseShot')), 21);
  assert.equal(guaranteedDamage(ability('Aevik', 'Hero.Aevik.ConductiveNode')), 20);
});

test('i quattro power_raw coincidono con quelli pubblicati in #603', () => {
  const atteso: Record<string, number> = { Aevik: 55.2, Ivrin: 31.0, Branth: 18.0, Muiren: 16.0 };
  for (const hero of roster()) {
    assert.equal(powerRaw(hero), atteso[hero.name], `${hero.name}`);
  }
});

test('i quattro rating power sono quelli pubblicati, e distinti', () => {
  const rating = Object.fromEntries(roster().map((h) => [h.name, powerRating(h)]));
  assert.deepEqual(rating, { Aevik: 6, Ivrin: 4, Branth: 3, Muiren: 2 });
});

test('la somma dei contributi ricostruisce ESATTAMENTE il power_raw PUBBLICATO', () => {
  // ⚠️ L'oracolo sono i valori di #603, **non** `powerRaw`: da quando `powerRaw` somma questi stessi
  // contributi, confrontare i due lati sarebbe una tautologia — entrambi si muoverebbero insieme, e
  // il test resterebbe verde anche con la derivazione sbagliata. Misurato: mutando `weighted` in un
  // valore gia' diviso, un confronto con `powerRaw` NON cade; questo si'.
  const pubblicato: Record<string, number> = { Aevik: 55.2, Ivrin: 31.0, Branth: 18.0, Muiren: 16.0 };
  for (const hero of roster()) {
    const somma = abilityContributions(hero).reduce((s, c) => s + c.weighted, 0);
    assert.equal(somma / POWER_WEIGHT_SCALE, pubblicato[hero.name], `${hero.name}`);
  }
});

test('il contributo per-abilita resta INTERO, come la rubrica tiene interi i pesi', () => {
  // ⚠️ Validato per MUTAZIONE: dividendo `weighted` per `POWER_WEIGHT_SCALE` dentro
  // `abilityContributions` questo test cade su tutte e venti le abilita, ed e' il difetto che
  // `D-108` rende rosso su una macchina e verde su un'altra.
  let quante = 0;
  for (const hero of roster()) {
    for (const c of abilityContributions(hero)) {
      assert.ok(Number.isInteger(c.availability), `${c.id}: availability ${c.availability}`);
      assert.ok(Number.isInteger(c.weighted), `${c.id}: weighted ${c.weighted}`);
      quante++;
    }
  }
  // Anti-vacuita': un roster vuoto renderebbe verdi le asserzioni qui sopra senza guardare niente.
  assert.equal(quante, 20, 'il catalogo dichiara venti abilita');
});

test('due abilita di EROI DIVERSI con lo stesso peso valgono lo stesso, ed e la domanda di #2566', () => {
  // `ConductiveNode` (Aevik), `Ram` (Branth) e `PassingBlade` (Ivrin) pesano tutte 6000: 20 danni
  // garantiti con cooldown 2. La metrica deve dirlo senza sapere di chi sono.
  const tutte = roster().flatMap(abilityContributions);
  const seimila = tutte.filter((c) => c.weighted === 6000).map((c) => c.id);
  assert.deepEqual(seimila.sort(), [
    'Hero.Aevik.ConductiveNode',
    'Hero.Branth.Ram',
    'Hero.Ivrin.PassingBlade',
  ]);
  for (const id of seimila) {
    const c = tutte.find((x) => x.id === id)!;
    assert.equal(contributionPerTurn(c), 10, id);
  }
});

test('l ancora dell EROE non si applica all abilita: comprimerebbe venti valori in tre', () => {
  // La derivazione di questa scelta, misurata invece che asserita. `rate(raw, POWER_ANCHOR)` e'
  // tarata su «uccide un eroe medio in un turno»: applicata a una singola abilita' restituisce 1, 2
  // o 3 e basta, quindi NON risponde a «questa abilita' vale quanto quest'altra». La metrica di
  // confronto resta il contributo nella stessa unita' di `power_raw` — nessuna scala nuova (#2579).
  const tutte = roster().flatMap(abilityContributions);
  assert.equal(tutte.length, 20);
  const distinti = new Set(tutte.map((c) => rate(contributionPerTurn(c), POWER_ANCHOR)));
  assert.equal(distinti.size, 3, `la scala 1..10 distingue solo ${distinti.size} valori su 20 abilita`);
  // Mentre il contributo per turno ne distingue molti di piu', ed e' la ragione della scelta.
  assert.ok(new Set(tutte.map(contributionPerTurn)).size > distinti.size);
});

test('ogni banda e DERIVATA dal roster e nomina le abilita che la producono', () => {
  const bande = abilityBands(roster());
  const base = bande.find((b) => b.kind === 'attacco base')!;
  // Calcolata a mano dal catalogo: ImpactShot 8/turno, PulseShot 21, ArcPulse 22.
  assert.deepEqual({ min: base.min, max: base.max }, { min: 8, max: 22 });
  assert.deepEqual(base.from.slice().sort(), [
    'Hero.Aevik.ArcPulse',
    'Hero.Branth.ImpactShot',
    'Hero.Ivrin.PulseShot',
  ]);
});

test('una banda senza abilita non entra: ogni banda ne nomina almeno una', () => {
  const bande = abilityBands(roster());
  assert.ok(bande.length > 0, 'il roster produce almeno una banda');
  for (const b of bande) {
    assert.ok(b.from.length > 0, `${b.kind}: banda senza derivazione`);
  }
  // Anti-vacuita': le bande coprono TUTTE le venti abilita, nessuna categoria si perde per strada.
  const coperte = bande.reduce((s, b) => s + b.from.length, 0);
  assert.equal(coperte, 20);
});

test('la banda di una categoria contiene il contributo di ogni sua abilita', () => {
  const tutte = roster().flatMap(abilityContributions);
  for (const b of abilityBands(roster())) {
    for (const id of b.from) {
      const v = contributionPerTurn(tutte.find((c) => c.id === id)!);
      assert.ok(v >= b.min && v <= b.max, `${id} (${v}) fuori dalla banda ${b.kind} [${b.min}, ${b.max}]`);
    }
  }
});
