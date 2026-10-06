import { test } from 'node:test';
import assert from 'node:assert/strict';
import { compare, cppPairs, docRows } from './action-descriptions.ts';

/** Le forme sono copiate dai due file veri — la tabella C++ e il documento — non inventate. */
const CPP = [
  'const TArray<TPair<FName, FString>>& RTActionDescriptions::All()',
  '{',
  '\tstatic const TArray<TPair<FName, FString>> Voci = {',
  '\t\t{ TEXT("Action.Wait"),      TEXT("Non agisci in questo turno.") },',
  '\t\t{ TEXT("Hero.Aevik.LinearDischarge"),   TEXT("Una scarica in linea retta, piu\' dannosa su chi e\' bagnato.") },',
  '\t};',
  '\treturn Voci;',
  '}',
  '// una coppia fuori dal corpo non conta',
  '{ TEXT("Action.Fuori"), TEXT("non letta") }',
].join('\n');

const NOMI = [
  'FText HeroActionDisplayName(const FName& Id)',
  '{',
  '\tstatic const TMap<FName, FString> Names = {',
  '\t\t{ TEXT("Hero.Aevik.LinearDischarge"),    TEXT("Scarica lineare") },',
  '\t};',
].join('\n');

const DOC = [
  '| Azione | Nome | Frase | Da rivedere |',
  '|---|---|---|---|',
  '| `Action.Wait` | Attesa | Non agisci in questo turno. |  |',
  "| `Hero.Aevik.LinearDischarge` | Scarica lineare | Una scarica in linea retta, piu' dannosa su chi e' bagnato. |  |",
].join('\r\n');

const nomiDi = () => [...cppPairs(NOMI, 'FText HeroActionDisplayName('), { id: 'Action.Wait', testo: 'Attesa' }];

test('le coppie si leggono solo dentro il corpo della funzione', () => {
  const voci = cppPairs(CPP, 'RTActionDescriptions::All()');
  assert.deepEqual(voci.map((v) => v.id), ['Action.Wait', 'Hero.Aevik.LinearDischarge']);
  assert.equal(voci[1].testo, "Una scarica in linea retta, piu' dannosa su chi e' bagnato.");
});

test('una funzione che non esiste piu da zero voci, non un errore silenzioso altrove', () => {
  assert.deepEqual(cppPairs(CPP, 'FuoriDalFile('), []);
});

test('le righe del documento si leggono anche con fine riga CRLF', () => {
  const righe = docRows(DOC);
  assert.deepEqual(righe.map((r) => r.id), ['Action.Wait', 'Hero.Aevik.LinearDischarge']);
  assert.equal(righe[0].nome, 'Attesa');
  assert.equal(righe[0].line, 3);
});

test('due copie uguali concordano', () => {
  assert.deepEqual(compare(docRows(DOC), cppPairs(CPP, 'RTActionDescriptions::All()'), nomiDi()), []);
});

test('una frase cambiata da un lato solo e una divergenza, con entrambi i valori', () => {
  const doc = DOC.replace('Non agisci in questo turno.', 'Aspetti.');
  const d = compare(docRows(doc), cppPairs(CPP, 'RTActionDescriptions::All()'), nomiDi());
  assert.deepEqual(d, [{ id: 'Action.Wait', campo: 'frase', doc: 'Aspetti.', cpp: 'Non agisci in questo turno.' }]);
});

test('anche il nome e una copia, e diverge', () => {
  const doc = DOC.replace('| Scarica lineare |', '| Scarica dritta |');
  const d = compare(docRows(doc), cppPairs(CPP, 'RTActionDescriptions::All()'), nomiDi());
  assert.deepEqual(d.map((x) => x.campo), ['nome']);
});

test('una frase senza riga, una riga senza frase e una riga doppia sono tre divergenze di presenza', () => {
  const senzaRiga = DOC.split('\r\n').slice(0, 3).join('\r\n');
  assert.deepEqual(
    compare(docRows(senzaRiga), cppPairs(CPP, 'RTActionDescriptions::All()'), nomiDi()).map((x) => x.id),
    ['Hero.Aevik.LinearDischarge'],
  );
  const conOrfana = `${DOC}\r\n| \`Action.Orfana\` | Orfana | Una frase. |  |`;
  assert.deepEqual(
    compare(docRows(conOrfana), cppPairs(CPP, 'RTActionDescriptions::All()'), nomiDi()).map((x) => x.cpp),
    ['nessuna frase'],
  );
  const doppia = `${DOC}\r\n| \`Action.Wait\` | Attesa | Non agisci in questo turno. |  |`;
  assert.equal(compare(docRows(doppia), cppPairs(CPP, 'RTActionDescriptions::All()'), nomiDi()).length, 1);
});
