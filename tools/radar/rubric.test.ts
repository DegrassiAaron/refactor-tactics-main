import { test } from 'node:test';
import assert from 'node:assert/strict';
import { availabilityWeight, rate, TBD, resolveAxes } from './rubric.ts';

test('la disponibilita del cooldown e un intero esatto, senza float', () => {
  // `10/(1+CD/2)` == `20/(2+CD)`: i denominatori sono {2,3,4,5} e il loro mcm e' 60, quindi i pesi
  // si esprimono come interi. Serve a D-108: il determinismo non deve dipendere dagli arrotondamenti
  // in virgola mobile di una macchina.
  assert.equal(availabilityWeight(0), 30);
  assert.equal(availabilityWeight(1), 20);
  assert.equal(availabilityWeight(2), 15);
  assert.equal(availabilityWeight(3), 12);
});

test('un cooldown fuori scala e un errore, non un peso inventato', () => {
  assert.throws(() => availabilityWeight(4), /cooldown 4 fuori scala/);
  assert.throws(() => availabilityWeight(-1), /cooldown -1 fuori scala/);
});

test('la scala e assoluta e satura agli estremi', () => {
  // `rate(raw, anchor)`: `raw == anchor` vale 10, `raw == 0` vale 1. Ancore ASSOLUTE (D-112):
  // non dipendono dal roster, quindi un eroe nuovo non muove i rating di quelli esistenti.
  assert.equal(rate(0, 100), 1);
  assert.equal(rate(100, 100), 10);
  assert.equal(rate(55.2, 100), 6);
  assert.equal(rate(16, 100), 2);

  // Oltre l'ancora non si sfora: un eroe piu' forte dell'atteso resta 10, non 12.
  assert.equal(rate(180, 100), 10);
});

test('TBD si propaga e non diventa mai zero', () => {
  // Owner §6: un asse che la rubrica non sa produrre e' `TBD`, e `TBD` NON si disegna come `0`.
  // Uno zero direbbe «questo eroe e' pessimo qui»; il dato dice «non lo sappiamo».
  const assi = resolveAxes({ offense: 6, durability: TBD, mobility: 7 });

  assert.equal(assi.complete, false);
  assert.deepEqual(assi.missing, ['durability']);
  assert.equal(assi.values.durability, TBD);
  assert.notEqual(assi.values.durability, 0);
});

test('un radar con un asse TBD non si genera, e l errore nomina l asse', () => {
  assert.throws(
    () => resolveAxes({ offense: 6, durability: TBD, mobility: TBD }).orThrow('Flux'),
    /Flux.*durability, mobility/,
  );
});

test('quando tutti gli assi hanno un valore, orThrow li restituisce', () => {
  const valori = resolveAxes({ offense: 6, durability: 7 }).orThrow('Flux');
  assert.deepEqual(valori, { offense: 6, durability: 7 });
});
