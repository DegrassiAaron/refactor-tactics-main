import { test } from 'node:test';
import assert from 'node:assert/strict';
import { scanPaths, isHistorical, deadRefs, withParents, exemption } from './issue-refs.ts';

test('un esenzione vale solo col motivo, e il motivo si legge', () => {
  assert.equal(
    exemption('testo\n<!-- issue-refs: ignora — documenta lo svuotamento di docs/src/ -->\naltro'),
    'documenta lo svuotamento di docs/src/',
  );
  // senza motivo non e' un esenzione: un buco muto e indistinguibile da una dimenticanza
  assert.equal(exemption('<!-- issue-refs: ignora -->'), null);
  assert.equal(exemption('nessun marcatore qui'), null);
});

test('lo scan trova i percorsi del repository dentro inline code e ignora il resto', () => {
  assert.deepEqual(scanPaths('lo stato vive in `docs/roadmap/feature-registry.yaml`, non qui'), [
    'docs/roadmap/feature-registry.yaml',
  ]);
  // prosa senza backtick: e' la scelta dichiarata nel docstring, come per doc-links.ts
  assert.deepEqual(scanPaths('lo stato vive in docs/roadmap/feature-registry.yaml'), []);
  // parola qualsiasi dentro backtick
  assert.deepEqual(scanPaths('il campo `bInterrupted` diventa vero'), []);
  // radice che non appartiene al repository
  assert.deepEqual(scanPaths('vedi `/usr/local/bin/qualcosa`'), []);
});

test('da un comando si prende il percorso, non gli argomenti', () => {
  assert.deepEqual(scanPaths('- [ ] `python scripts/feature_registry.py validate` verde'), [
    'scripts/feature_registry.py',
  ]);
  assert.deepEqual(scanPaths('il gate `node tools/radar/doc-links.ts --check --with-archive`'), [
    'tools/radar/doc-links.ts',
  ]);
});

test('il suffisso di riga e la punteggiatura non entrano nel percorso', () => {
  assert.deepEqual(scanPaths('vedi `Source/RefactorTactics/Map/RTCellId.h:11`'), [
    'Source/RefactorTactics/Map/RTCellId.h',
  ]);
  assert.deepEqual(scanPaths('nel file `(docs/roadmap/roadmap-v0.1.md),`'), [
    'docs/roadmap/roadmap-v0.1.md',
  ]);
});

test('brace e glob non sono percorsi letterali e vengono lasciati stare', () => {
  assert.deepEqual(scanPaths('tocca `Source/RefactorTactics/UI/RTHUD.{h,cpp}`'), []);
  assert.deepEqual(scanPaths('il registro `docs/roadmap/feature-registry.*`'), []);
});

test('una riga che racconta la rimozione e storica, una che prescrive no', () => {
  assert.equal(isHistorical('> - `docs/roadmap/feature-registry.*` → **D-181** — esce dal repository'), true);
  assert.equal(isHistorical('lo stato vive ~~in `feature-registry.yaml`~~ → in `roadmap-v0.1.md`'), true);
  assert.equal(isHistorical('`feature-registry.yaml` non vive piu` qui'), true);
  assert.equal(isHistorical('> 🔁 **Rimisurato il 2026-08-31** — il registro non esiste'), true);
  assert.equal(isHistorical('- [ ] `python scripts/feature_registry.py validate` verde'), false);
  assert.equal(isHistorical('`RT-FEAT-UI-ICON` in `docs/roadmap/feature-registry.yaml`. Lo stato vive li'), false);
});

test('anche D-178 rende storica una riga, come D-181 e D-182', () => {
  // La forma corta — la decisione senza il verbo che la descrive — e' quella che il gate non
  // riconosceva: 4 righe di issue aperte la usano e la loro sola copertura era il blockquote.
  // Fuori da un blockquote la stessa frase diventerebbe un falso positivo.
  assert.equal(isHistorical('- `scripts/rt_shared_id.py` → **D-178** — lo sviluppo torna sequenziale'), true);
  assert.equal(isHistorical('`docs/roadmap/parallel-batch.yaml` non e nel writable di nessuna track (D-178)'), true);
  // D-179 e D-177 non sono decisioni di rimozione: il confine dell'elenco resta stretto
  assert.equal(isHistorical('- [ ] `docs/roadmap/parallel-batch.yaml` allineato a **D-179**'), false);
});

test('CANCELLATO si segnala, MAI ESISTITO no: e la distinzione che rende il gate usabile', () => {
  const vivi = new Set(['docs/roadmap/roadmap-v0.1.md']);
  const cancellati = new Set(['scripts/feature_registry.py']);
  const isAlive = (p: string) => vivi.has(p);
  const wasDeleted = (p: string) => cancellati.has(p);

  const body = [
    '- [ ] `python scripts/feature_registry.py validate` verde', // cancellato → segnala
    '- [ ] creare `docs/technical/systems/spec-nuova.md`', // mai esistito → deliverable
    'lo stato vive in `docs/roadmap/roadmap-v0.1.md`', // vivo → niente
  ].join('\n');

  const morti = deadRefs(body, 42, isAlive, wasDeleted);
  assert.deepEqual(
    morti.map((m) => m.path),
    ['scripts/feature_registry.py'],
  );
  assert.equal(morti[0].issue, 42);
  assert.equal(morti[0].line, 1);
});

test('una riga storica non viene segnalata anche se cita un percorso cancellato', () => {
  const isAlive = () => false;
  const wasDeleted = () => true;
  const body = '> `scripts/feature_registry.py` e uscito con **D-182** il 2026-08-21';
  assert.deepEqual(deadRefs(body, 7, isAlive, wasDeleted), []);
});

test('i corpi con CRLF danno lo stesso numero di riga di quelli con LF', () => {
  const isAlive = () => false;
  const wasDeleted = (p: string) => p === 'scripts/feature_registry.py';
  const righe = ['intestazione', '', 'testo', '', '- [ ] `scripts/feature_registry.py validate`'];

  const lf = deadRefs(righe.join('\n'), 1, isAlive, wasDeleted);
  const crlf = deadRefs(righe.join('\r\n'), 1, isAlive, wasDeleted);

  assert.equal(lf.length, 1);
  assert.equal(crlf.length, 1);
  // il difetto misurato su #703 il 2026-09-01: la riga indicata non era quella che si vede aprendo la issue
  assert.equal(crlf[0].line, lf[0].line);
  assert.equal(crlf[0].line, 5);
});

test('withParents rende citabile anche la cartella che contiene un percorso', () => {
  const s = withParents(['docs/src/design/icon/nota.md']);
  assert.equal(s.has('docs/src'), true);
  assert.equal(s.has('docs/src/design'), true);
  assert.equal(s.has('docs/src/design/icon/nota.md'), true);
  assert.equal(s.has('docs/altro'), false);
});
