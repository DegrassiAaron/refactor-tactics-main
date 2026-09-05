import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  COLONNE,
  LICENZA_NON_VERIFICATA,
  MARCATORE_FINE,
  MARCATORE_INIZIO,
  coveringRow,
  findDeadRows,
  findUncovered,
  parseRegistry,
  splitRow,
  type RegistryRow,
} from './registry.ts';

/** Costruisce un registro minimo attorno alle righe date, cosi' che ogni test dichiari **solo** cio'
 *  che sta misurando. Le celle si passano gia' formattate: un test sulla larghezza sbagliata deve
 *  poter scrivere una riga sbagliata. */
function registro(...righe: string[]): string {
  return [
    '# Registro di prova',
    '',
    MARCATORE_INIZIO,
    '',
    `| ${COLONNE.join(' | ')} |`,
    `|${COLONNE.map(() => '---').join('|')}|`,
    ...righe,
    '',
    MARCATORE_FINE,
    '',
  ].join('\n');
}

/** Una riga valida, con il solo prefisso variabile. */
function riga(copre: string, licenza = 'Fab Standard License'): string {
  return `| Prova | \`${copre}\` | [Fab](https://fab.com/eula) | ${licenza} | listing n/d | 2026-09-05 | 5.8.1 | nessuna | BP_Unit_Prova |`;
}

test('legge una riga valida e ne estrae tutte le colonne', () => {
  const { rows, issues } = parseRegistry(registro(riga('/Game/FabAsset/Paragon/ParagonGadget/')));
  assert.deepEqual(issues, []);
  assert.equal(rows.length, 1);
  assert.equal(rows[0].famiglia, 'Prova');
  assert.equal(rows[0].copre, '/Game/FabAsset/Paragon/ParagonGadget/');
  assert.equal(rows[0].licenza, 'Fab Standard License');
  assert.equal(rows[0].acquisito, '2026-09-05');
  assert.equal(rows[0].ue, '5.8.1');
  assert.equal(rows[0].consumer, 'BP_Unit_Prova');
});

test('senza marcatori non legge niente e lo dice', () => {
  const { rows, issues } = parseRegistry('# Solo prosa\n\n| a | b |\n|---|---|\n| 1 | 2 |\n');
  assert.deepEqual(rows, []);
  assert.equal(issues.length, 1);
  assert.match(issues[0].perche, /marcatori/);
});

test("un'intestazione diversa ferma la lettura invece di leggere colonne sbagliate", () => {
  const md = [
    MARCATORE_INIZIO,
    '| Famiglia | Percorso | Fonte | Licenza | Versione | Acquisito | UE | Attribuzione | Consumer |',
    `|${COLONNE.map(() => '---').join('|')}|`,
    riga('/Game/FabAsset/Paragon/ParagonGadget/'),
    MARCATORE_FINE,
  ].join('\n');
  const { rows, issues } = parseRegistry(md);
  assert.deepEqual(rows, []);
  assert.equal(issues.length, 1);
  assert.match(issues[0].perche, /intestazione inattesa/);
});

test('una riga con una colonna in meno e’ un registro rotto, non un asset scoperto', () => {
  const monca = '| Prova | `Content/X/` | Fab | Fab Standard License | n/d | 2026-09-05 | 5.8.1 | nessuna |';
  const { rows, issues } = parseRegistry(registro(monca));
  assert.deepEqual(rows, []);
  assert.equal(issues.length, 1);
  assert.match(issues[0].perche, /8 celle invece di 9/);
});

test('il prefisso deve stare fra backtick', () => {
  const senza = '| Prova | Content/X/ | Fab | Fab Standard License | n/d | 2026-09-05 | 5.8.1 | nessuna | — |';
  const { rows, issues } = parseRegistry(registro(senza));
  assert.deepEqual(rows, []);
  assert.match(issues[0].perche, /fra backtick/);
});

test("una cartella senza '/' finale non e’ una copertura valida", () => {
  const { rows, issues } = parseRegistry(registro(riga('/Game/FabAsset/Paragon/ParagonGadget')));
  assert.deepEqual(rows, []);
  assert.match(issues[0].perche, /copertura valida/);
});

test('una copertura a file singolo copre se stessa e non i fratelli', () => {
  const { rows, issues } = parseRegistry(registro(riga('tools/Paragon_Skill_Icons_Downloader.zip')));
  assert.deepEqual(issues, []);
  assert.notEqual(coveringRow('tools/Paragon_Skill_Icons_Downloader.zip', rows), null);
  assert.equal(coveringRow('tools/Paragon_Skill_Icons_Downloader.zip.bak', rows), null);
  assert.equal(coveringRow('tools/altro.zip', rows), null);
});

test("una cartella senza '/' non copre per prefisso il fratello che ne estende il nome", () => {
  // `Content/Icon` senza barra assolverebbe `Content/Icons/`: la barra e' cio' che lo impedisce.
  const { rows } = parseRegistry(registro(riga('Content/Icons/')));
  assert.equal(coveringRow('Content/IconsAltro/X.svg', rows), null);
  assert.notEqual(coveringRow('Content/Icons/X.svg', rows), null);
});

test('due righe con lo stesso prefisso sono un difetto del registro', () => {
  const { rows, issues } = parseRegistry(
    registro(riga('Content/Icons/'), riga('Content/Icons/')),
  );
  assert.equal(rows.length, 2);
  assert.equal(issues.length, 1);
  assert.match(issues[0].perche, /duplicato/);
});

test('una pipe escapata dentro una cella non spezza la riga', () => {
  const conPipe =
    '| Prova | `Content/X/` | [Fab](https://fab.com/eula) | Fab Standard License \\| Tier Personal | n/d | 2026-09-05 | 5.8.1 | nessuna | — |';
  const { rows, issues } = parseRegistry(registro(conPipe));
  assert.deepEqual(issues, []);
  assert.equal(rows.length, 1);
  assert.equal(rows[0].licenza, 'Fab Standard License | Tier Personal');
});

test('splitRow non conta la pipe escapata come separatore', () => {
  assert.deepEqual(splitRow('| a \\| b | c |'), ['a | b', 'c']);
});

test('un asset sotto il prefisso e’ coperto, uno fuori no', () => {
  const { rows } = parseRegistry(registro(riga('/Game/FabAsset/Paragon/ParagonGadget/')));
  assert.notEqual(
    coveringRow('/Game/FabAsset/Paragon/ParagonGadget/Characters/Heroes/Gadget/Meshes/Gadget', rows),
    null,
  );
  assert.equal(coveringRow('/Game/FabAsset/Paragon/ParagonWraith/Meshes/Wraith', rows), null);
});

test('fra prefissi sovrapposti vince il piu’ specifico, in qualunque ordine siano scritti', () => {
  const larga = riga('Content/Icons/');
  const stretta = '| Stretta | `Content/Icons/Frames/` | [x](https://e.example) | CC0 | n/d | 2026-09-05 | 5.8.1 | nessuna | HUD |';

  for (const ordine of [[larga, stretta], [stretta, larga]]) {
    const { rows, issues } = parseRegistry(registro(...ordine));
    assert.deepEqual(issues, []);
    const vincente = coveringRow('Content/Icons/Frames/F_Panel.svg', rows);
    assert.equal(vincente?.famiglia, 'Stretta', `ordine: ${ordine === undefined ? '' : ordine[0].slice(2, 9)}`);
    assert.equal(coveringRow('Content/Icons/Icons/I_Move.svg', rows)?.famiglia, 'Prova');
  }
});

test('findUncovered restituisce solo cio’ che nessuna riga copre', () => {
  const { rows } = parseRegistry(registro(riga('Content/RT/')));
  const trovati = findUncovered(
    [
      { path: 'Content/RT/UI/W.uasset', origine: 'versionato' },
      { path: 'Content/Icons/I.svg', origine: 'versionato' },
    ],
    rows,
  );
  assert.deepEqual(trovati.map((u) => u.path), ['Content/Icons/I.svg']);
});

test('una riga il cui prefisso non copre piu’ niente e’ morta', () => {
  const { rows } = parseRegistry(registro(riga('Content/RT/'), riga('Content/Sparito/')));
  const morte = findDeadRows(rows, ['Content/RT/UI/W.uasset']);
  assert.deepEqual(morte.map((r) => r.copre), ['Content/Sparito/']);
});

test('un pack presente solo sul disco tiene viva la sua riga', () => {
  // Nessun package versionato cita ParagonBoris: senza il terzo insieme di path la sua riga
  // risulterebbe morta, e il registro perderebbe proprio cio' che deve descrivere.
  const { rows } = parseRegistry(registro(riga('/Game/FabAsset/Paragon/ParagonBoris/')));
  assert.deepEqual(findDeadRows(rows, []), rows);
  assert.deepEqual(findDeadRows(rows, ['/Game/FabAsset/Paragon/ParagonBoris/Characters/X']), []);
});

test('NON VERIFICATA e’ una licenza registrata, non un difetto', () => {
  const { rows, issues } = parseRegistry(
    registro(riga('Content/RT_UI_AssetPack_FromHUD/', LICENZA_NON_VERIFICATA)),
  );
  assert.deepEqual(issues, []);
  assert.equal(rows[0].licenza, LICENZA_NON_VERIFICATA);
  assert.equal(coveringRow('Content/RT_UI_AssetPack_FromHUD/manifest.json', rows)?.licenza, LICENZA_NON_VERIFICATA);
});

test('il registro reale del repository si legge senza difetti', async () => {
  const { readFileSync } = await import('node:fs');
  const { fileURLToPath } = await import('node:url');
  const path = fileURLToPath(new URL('../../docs/technical/asset-licenze.md', import.meta.url));
  const { rows, issues } = parseRegistry(readFileSync(path, 'utf8'));
  assert.deepEqual(issues, [], 'il registro versionato non deve avere righe illeggibili');
  assert.ok(rows.length > 0, 'il registro versionato non deve essere vuoto');
  const nonVerificate = rows.filter((r: RegistryRow) => r.licenza === LICENZA_NON_VERIFICATA);
  assert.ok(
    nonVerificate.length > 0,
    'il DoD di #1767 chiede che il caso «licenza non verificata» sia scritto, non omesso',
  );
});
