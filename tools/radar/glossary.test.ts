import { test } from 'node:test';
import assert from 'node:assert/strict';
import { confronta } from './glossary.ts';

/** Le forme sono copiate dai due file veri — la tabella C++ e il documento — non inventate. */
const CPP = [
  '#include "UI/RTGlossary.h"',
  '// Le voci della v0.1.',
  'const TArray<TPair<FName, FString>>& RTGlossary::Names()',
  '{',
  '\tstatic const TArray<TPair<FName, FString>> Voci = {',
  '\t\t// Stati',
  '\t\t{ TEXT("Status.Burning"),     TEXT("In fiamme") },',
  '\t\t{ TEXT("Terrain.Ice"),          TEXT("Ghiaccio") },',
  '\t};',
  '\treturn Voci;',
  '}',
  '',
  'const TArray<TPair<FName, FString>>& RTGlossary::Effects()',
  '{',
  '\tstatic const TArray<TPair<FName, FString>> Voci = {',
  '\t\t{ TEXT("Status.Burning"),     TEXT("Subisci danno alla fine di ogni turno, finche\' le fiamme durano o l\'acqua le spegne.") },',
  '\t\t{ TEXT("Terrain.Ice"),          TEXT("Chi finisce qui il movimento con passi ancora da spendere scivola oltre, nella direzione in cui andava.") },',
  '\t};',
  '\treturn Voci;',
  '}',
  '',
  'FText RTGlossary::NameFor(const FName& Key)',
  '{',
  '\treturn GlossaryLookup(Names(), Key);',
  '}',
].join('\n');

const DOC = [
  '## Stati',
  '',
  '| Voce | Nome | Effetto | Da rivedere |',
  '|---|---|---|---|',
  "| `Status.Burning` | In fiamme | Subisci danno alla fine di ogni turno, finche' le fiamme durano o l'acqua le spegne. |  |",
  '',
  '## Superfici',
  '',
  '| Voce | Nome | Effetto | Da rivedere |',
  '|---|---|---|---|',
  '| `Terrain.Ice` | Ghiaccio | Chi finisce qui il movimento con passi ancora da spendere scivola oltre, nella direzione in cui andava. |  |',
].join('\r\n');

test('due copie uguali concordano, e la copertura si conta su entrambi i lati', () => {
  const e = confronta(DOC, CPP);
  assert.deepEqual(e.divergenze, []);
  assert.deepEqual([e.righe, e.nomi, e.effetti], [2, 2, 2]);
});

test('un effetto cambiato nel solo documento e una divergenza, con entrambi i valori', () => {
  const e = confronta(DOC.replace('scivola oltre', 'scivola'), CPP);
  assert.deepEqual(e.divergenze, [
    {
      id: 'Terrain.Ice',
      campo: 'frase',
      doc: 'Chi finisce qui il movimento con passi ancora da spendere scivola, nella direzione in cui andava.',
      cpp: 'Chi finisce qui il movimento con passi ancora da spendere scivola oltre, nella direzione in cui andava.',
    },
  ]);
});

test('un nome cambiato nel solo documento e una divergenza', () => {
  const e = confronta(DOC.replace('| Ghiaccio |', '| Lastra di ghiaccio |'), CPP);
  assert.deepEqual(e.divergenze.map((d) => [d.id, d.campo]), [['Terrain.Ice', 'nome']]);
});

test('una voce che manca dal documento e una divergenza di presenza', () => {
  const senzaGhiaccio = DOC.split('\r\n').slice(0, 5).join('\r\n');
  const e = confronta(senzaGhiaccio, CPP);
  assert.deepEqual(e.divergenze.map((d) => [d.id, d.campo]), [['Terrain.Ice', 'presenza']]);
});

test('una funzione rinominata da zero voci da quel lato: chi chiama lo tratta come errore', () => {
  const e = confronta(DOC, CPP.replace('RTGlossary::Effects()', 'RTGlossary::Effetti()'));
  assert.equal(e.effetti, 0);
  assert.equal(e.nomi, 2);
});

test('un commento che nomina la funzione prima della definizione non sposta la lettura', () => {
  const conCommento = CPP.replace(
    '// Le voci della v0.1.',
    '// Le voci della v0.1: vedi RTGlossary::Effects() e RTGlossary::Names() qui sotto.',
  );
  const e = confronta(DOC, conCommento);
  assert.deepEqual(e.divergenze, []);
  assert.deepEqual([e.nomi, e.effetti], [2, 2]);
});
