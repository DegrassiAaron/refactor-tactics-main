import { test } from 'node:test';
import assert from 'node:assert/strict';
import { extractPackageRefs, toContentPaths, findUntrackedRefs } from './refs.ts';

/** Un `.uasset` non e' un formato di testo, ma i package path vivono nella name table come stringhe
 *  ASCII terminate da NUL: e' cosi' che `/Game/RT/Maps/Dev/L_DevSandbox/Data/DA_Format_Rotto` e'
 *  leggibile con `grep -a`. Il fixture riproduce quella forma, byte per byte. */
function nameTable(...names: string[]): Buffer {
  const parts = names.map((n) => Buffer.concat([Buffer.from([0x11, 0x00, 0x00, 0x00]), Buffer.from(n, 'ascii'), Buffer.from([0x00])]));
  return Buffer.concat(parts);
}

test('estrae i package path dai byte di un asset', () => {
  const bytes = nameTable('/Game/RT/Core/Framework/BP_GameMode', 'MapSource', '/Game/RT/Maps/Dev/Data/DA_Format_Rotto');
  assert.deepEqual(extractPackageRefs(bytes), [
    '/Game/RT/Core/Framework/BP_GameMode',
    '/Game/RT/Maps/Dev/Data/DA_Format_Rotto',
  ]);
});

test('lo stesso path citato due volte si conta una volta sola', () => {
  const bytes = nameTable('/Game/RT/A/X', '/Game/RT/A/X');
  assert.deepEqual(extractPackageRefs(bytes), ['/Game/RT/A/X']);
});

test('un path piu\' corto di un segmento non e\' un riferimento', () => {
  // `/Game` da solo e' il mount point, non un asset: segnalarlo produrrebbe rumore su ogni file.
  assert.deepEqual(extractPackageRefs(nameTable('/Game', '/Game/')), []);
});

test('un package puo\' essere un .uasset o un .umap, e vanno provati entrambi', () => {
  assert.deepEqual(toContentPaths('/Game/RT/Maps/Dev/L_HexArena/L_HexArena'), [
    'Content/RT/Maps/Dev/L_HexArena/L_HexArena.uasset',
    'Content/RT/Maps/Dev/L_HexArena/L_HexArena.umap',
  ]);
});

test('un riferimento a un file che git non ha e\' una segnalazione', () => {
  const tracked = new Set(['Content/RT/Core/Framework/BP_GameMode.uasset']);
  const findings = findUntrackedRefs(
    [{ asset: 'Content/RT/Core/Framework/BP_GameMode.uasset', refs: ['/Game/RT/Maps/Dev/Data/DA_Format_Rotto'] }],
    tracked,
    [],
  );
  assert.deepEqual(findings, [
    { asset: 'Content/RT/Core/Framework/BP_GameMode.uasset', ref: '/Game/RT/Maps/Dev/Data/DA_Format_Rotto' },
  ]);
});

test('un riferimento versionato non e\' una segnalazione', () => {
  const tracked = new Set(['Content/RT/Maps/Dev/L_HexArena/L_HexArena.umap']);
  const findings = findUntrackedRefs(
    [{ asset: 'Content/a.uasset', refs: ['/Game/RT/Maps/Dev/L_HexArena/L_HexArena'] }],
    tracked,
    [],
  );
  assert.deepEqual(findings, []);
});

test('un prefisso dichiarato nelle eccezioni non e\' una segnalazione', () => {
  // Le mesh Paragon vivono in `Content/FabAsset/`, 48 GB fuori dal repository per scelta dichiarata.
  // Senza l'eccezione il gate segnalerebbe quattro riferimenti legittimi e diventerebbe rumore.
  const findings = findUntrackedRefs(
    [{ asset: 'Content/RT/Characters/Gadget/Blueprints/BP_Unit_Gadget.uasset', refs: ['/Game/FabAsset/Paragon/ParagonGadget/Meshes/Gadget'] }],
    new Set(),
    ['/Game/FabAsset/'],
  );
  assert.deepEqual(findings, []);
});

test('un asset che cita se stesso non si segnala da solo', () => {
  // Ogni package porta il proprio nome nella name table: senza questa regola ogni asset non versionato
  // — nessuno, oggi — e ogni asset versionato letto da un albero parziale si accuserebbe da se'.
  const findings = findUntrackedRefs(
    [{ asset: 'Content/RT/Core/Framework/BP_GameMode.uasset', refs: ['/Game/RT/Core/Framework/BP_GameMode'] }],
    new Set(),
    [],
  );
  assert.deepEqual(findings, []);
});
