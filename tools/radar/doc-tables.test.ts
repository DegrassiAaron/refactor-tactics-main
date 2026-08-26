import { test } from 'node:test';
import assert from 'node:assert/strict';
import { findBrokenRows, tableBlocks, isComparable } from './doc-tables.ts';

test('una riga con una cella in meno rompe la tabella e viene segnalata', () => {
  // Caso reale: `OPEN_DECISIONS.md` BAS-5 fondeva domanda e risposta perche' mancava una pipe.
  const md = [
    '| ID | Domanda | Risposta |',
    '|---|---|---|',
    '| `A-1` | prima domanda | prima risposta |',
    '| `A-2` | seconda domanda e la risposta tutta attaccata |',
  ].join('\n');

  const broken = findBrokenRows(md);

  assert.equal(broken.length, 1);
  assert.equal(broken[0]!.line, 4);
  assert.equal(broken[0]!.cells, 2);      // `A-2` piu' la cella fusa
  assert.equal(broken[0]!.expected, 3);   // ID, Domanda, Risposta
});

test('una pipe ESCAPATA non e un separatore, e la riga non e rotta', () => {
  // `Attack \| Ability \| Overwatch` in `ECO-1`: il markdown le rende come testo. Un contatore che le
  // conta come separatori produce un falso positivo — ed e' l'errore che questo test pinna.
  const md = [
    '| ID | Domanda | Risposta |',
    '|---|---|---|',
    '| `B-1` | copre `Attack \\| Ability \\| Overwatch` | e non dice altro |',
  ].join('\n');

  assert.deepEqual(findBrokenRows(md), []);
});

test('una pipe NON escapata dentro codice inline esplode la riga, ed e il secondo dei due tipi', () => {
  // Il caso che ha motivato il controllo: `if (!IsValid(X) || !X->IsAlive())` porta un `||` che il
  // Markdown legge come due separatori. Senza questo test il tipo `piu' celle` non era pinnato da
  // nessuna parte, e il docstring lo dichiara da sempre.
  const md = [
    '| ID | Nota |',
    '|---|---|',
    '| `C-1` | guardia `if (!IsValid(X) || !X->IsAlive())` |',
    '| `C-2` | riga sana |',
  ].join('\n');

  const broken = findBrokenRows(md);

  assert.equal(broken.length, 1);
  assert.equal(broken[0]!.line, 3);
  // Quattro, non tre: `||` sono DUE separatori, quindi la riga da due celle ne mostra quattro — la
  // guardia si spezza in `if (!IsValid(X) `, una cella vuota, ` !X->IsAlive())`.
  assert.equal(broken[0]!.cells, 4);
  assert.equal(broken[0]!.expected, 2);
});

test('la larghezza attesa e quella della MAGGIORANZA, non quella dell intestazione', () => {
  // Se ad avere la pipe di troppo fosse l'intestazione, ancorarsi a lei segnalerebbe tutte le righe
  // buone. Qui l'intestazione e' l'unica rotta, e deve essere l'unica segnalata.
  const md = [
    '| ID | Nota | e una cella | di troppo |',
    '|---|---|',
    '| `D-1` | prima |',
    '| `D-2` | seconda |',
  ].join('\n');

  const broken = findBrokenRows(md);

  assert.deepEqual(broken.map((b) => b.line), [1]);
  assert.equal(broken[0]!.expected, 2);
});

test('LIMITE DICHIARATO: se il difetto e maggioritario, a essere segnalate sono le righe sane', () => {
  // Il prezzo della regola di maggioranza, e sta nel docstring perche' non venga scoperto dopo: tre
  // righe rotte su cinque diventano la norma, e il controllo accusa intestazione e separatore.
  const md = [
    '| A | B |',
    '|---|---|',
    '| a | x||y |',
    '| b | x||y |',
    '| c | x||y |',
  ].join('\n');

  assert.deepEqual(findBrokenRows(md).map((b) => b.line), [1, 2]);
});

test('LIMITE DICHIARATO: un blocco di meno di tre righe non viene confrontato', () => {
  // E' il caso della riga staccata dalla sua tabella da una riga vuota (D-192, D-193): due righe con
  // larghezze diverse sarebbero 1 a 1, e dire quale sia quella giusta e' indovinare.
  const md = ['| A | B | C |', '| a | b |'].join('\n');

  assert.deepEqual(findBrokenRows(md), []);

  const blocks = tableBlocks(md.split('\n'));
  assert.equal(blocks.length, 1);
  assert.equal(isComparable(blocks[0]!), false);
});

test('LIMITE DICHIARATO: una riga indentata spezza il blocco e zittisce la tabella intera', () => {
  // GFM accetta fino a tre spazi davanti a una riga di tabella; qui la riga non comincia con `|`,
  // quindi il blocco si spezza in frammenti che la regola delle tre righe scarta. La riga indentata e'
  // genuinamente rotta — due celle invece di tre — e nessuno la vede.
  const md = ['| ID | A | B |', '|---|---|---|', '  | x | y |', '| q | r | s |'].join('\n');

  assert.deepEqual(findBrokenRows(md), []);
  assert.equal(tableBlocks(md.split('\n')).filter(isComparable).length, 0);
});

test('FALSO POSITIVO NOTO: la pipe finale che GFM rende facoltativa', () => {
  // `| q | r | s` rende tre celle, il contatore ne vede due. Oggi non capita nel corpus, e il giorno in
  // cui capitera' questo test dice che e' il gate a sbagliare, non il documento.
  const md = ['| ID | A | B |', '|---|---|---|', '| x | y | z |', '| q | r | s'].join('\n');

  const broken = findBrokenRows(md);

  assert.deepEqual(broken.map((b) => b.line), [4]);
  assert.equal(broken[0]!.cells, 2);
});

test('un corpus CRLF da lo stesso esito di uno LF', () => {
  // Ogni documento che il gate legge e' CRLF (`.gitattributes`: `*.md text`), mentre le fixture sono
  // scritte con `\n`. D-192 registra che l'estensione fu resa inerte proprio da questa cecita': la
  // maschera dei fence non matchava `\r` e le fixture LF restavano verdi.
  const rows = ['| ID | A | B |', '|---|---|---|', '| x | y |', '| q | r | s |'];

  const lf = findBrokenRows(rows.join('\n'));
  const crlf = findBrokenRows(rows.join('\r\n'));

  assert.deepEqual(lf.map((b) => [b.line, b.cells, b.expected]), [[3, 2, 3]]);
  assert.deepEqual(
    crlf.map((b) => [b.line, b.cells, b.expected]),
    lf.map((b) => [b.line, b.cells, b.expected]),
  );
});

test('la copertura distingue i blocchi confrontati da quelli scartati', () => {
  // Il numero che si pubblica come copertura deve essere quello dei blocchi CONFRONTATI: contare anche
  // i troppo corti dichiarava 1539 tabelle esaminate quando a essere confrontate erano 1521.
  const md = [
    '| A | B |',
    '|---|---|',
    '| a | b |',
    '',
    '| orfana | staccata |',
    '',
    '| C | D |',
    '|---|---|',
    '| c | d |',
  ].join('\n');

  const blocks = tableBlocks(md.split('\n'));

  assert.equal(blocks.length, 3);
  assert.equal(blocks.filter(isComparable).length, 2);
  assert.equal(blocks.filter((b) => !isComparable(b)).length, 1);
});
