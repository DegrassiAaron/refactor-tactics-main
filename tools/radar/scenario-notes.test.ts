import { test } from 'node:test';
import assert from 'node:assert/strict';
import { segnala, raccogli } from './scenario-notes.ts';

/** Il caso reale che ha aperto `#2049`: la nota dice 98, il file asserisce 103. */
test('una nota che cita un totale diverso da cio che il file asserisce viene segnalata', () => {
  const scenario = {
    _nota: "la riduzione non vale: 120 - 22 = 98, non 113.",
    turns: [{ expect: [{ type: 'UnitHpEquals', unit: 'B1', value: 103 }] }],
  };

  const righe = segnala('X.json', scenario);

  assert.equal(righe.length, 1);
  assert.equal(righe[0]!.citato, 98);
  assert.equal(righe[0]!.atteso, 103);
  assert.equal(righe[0]!.delta, 5);
});

/** 🔑 Anti-vacuita': con la nota CORRETTA non deve segnalare niente, o segnalerebbe sempre. */
test('la stessa nota, corretta, non produce nessuna segnalazione', () => {
  const scenario = {
    _nota: "la riduzione non vale: 120 - (22 - 5 di scudo base) = 103.",
    turns: [{ expect: [{ type: 'UnitHpEquals', unit: 'B1', value: 103 }] }],
  };

  assert.equal(segnala('X.json', scenario).length, 0);
});

/**
 * 🔴 Il rumore, ed e' il difetto che ha prodotto tre falsi positivi VERI su questo corpus:
 * `D-074` letto come «74 HP».
 */
test('un numero di decisione, issue, checkpoint, data o cella non e un HP', () => {
  const scenario = {
    _nota: "non e' raggiungibile in partita (D-074, uscita (B) di #400), vedi CP 16.2 del 2026-08-16 a (q=-2,r=1).",
    turns: [{ expect: [{ type: 'UnitHpEquals', unit: 'A', value: 79 }] }],
  };

  assert.deepEqual(segnala('X.json', scenario), []);
});

/** Un numero che COINCIDE con un'attesa e' la nota che fa il suo mestiere: silenzio. */
test('un numero uguale a un attesa non viene segnalato', () => {
  const scenario = {
    _nota: 'Wraith resta a 90 pieni.',
    turns: [{ expect: [{ type: 'UnitHpEquals', unit: 'V1', value: 90 }] }],
  };

  assert.equal(segnala('X.json', scenario).length, 0);
});

/**
 * ⚠️ **Il buco dichiarato, misurato invece che promesso.** Un valore INTERMEDIO sta sopra il totale
 * finale, quindi la finestra stretta non lo vede — ed e' esattamente cosi' che
 * `Spec/Cover/TemporaryCoverExpires.json` teneva nascosti tre numeri sbagliati su quattro.
 * `--wide` lo trova. Se un giorno la finestra stretta cominciasse a vederlo, questo test lo direbbe.
 */
test('un valore intermedio sopra l attesa e invisibile alla finestra stretta e visibile a --wide', () => {
  const scenario = {
    _nota: 'T1 colpo pieno (21) -> 99.',
    turns: [{ expect: [{ type: 'UnitHpEquals', unit: 'B1', value: 76 }] }],
  };

  assert.equal(segnala('X.json', scenario, /*wide=*/ false).length, 0);
  const larga = segnala('X.json', scenario, /*wide=*/ true);
  assert.equal(larga.length, 1);
  assert.equal(larga[0]!.citato, 99);
  assert.equal(larga[0]!.delta, -23);
});

/** Uno scenario senza attese di HP non ha un metro: non si inventa. */
test('senza UnitHpEquals non si segnala niente', () => {
  const scenario = {
    _nota: 'Riktor scende a 110.',
    turns: [{ expect: [{ type: 'TurnsCompleted', value: 3 }] }],
  };

  assert.deepEqual(segnala('X.json', scenario), []);
});

/** Le note e le attese si raccolgono a qualunque profondita': gli scenari annidano turni e squadre. */
test('la raccolta scende nell albero invece di fermarsi al primo livello', () => {
  const { note, attese } = raccogli({
    a: { b: [{ _nota_x: 'testo', expect: [{ type: 'UnitHpEquals', value: 7 }] }] },
  });

  assert.deepEqual(note, [['_nota_x', 'testo']]);
  assert.deepEqual(attese, [7]);
});
