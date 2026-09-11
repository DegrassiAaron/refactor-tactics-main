import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  MCP_SECTION,
  endpointFromMcpJson,
  settingsFromIni,
  effective,
  disagreements,
} from './check.ts';

const MCP_JSON = JSON.stringify({
  mcpServers: { 'unreal-mcp': { type: 'http', url: 'http://127.0.0.1:8765/mcp' } },
});

test("l'endpoint si legge da .mcp.json, host porta e path separati", () => {
  assert.deepEqual(endpointFromMcpJson(MCP_JSON), {
    host: '127.0.0.1',
    port: 8765,
    path: '/mcp',
  });
});

test('un .mcp.json senza quel server, illeggibile o senza url http vale null', () => {
  assert.equal(endpointFromMcpJson(JSON.stringify({ mcpServers: { altro: { url: 'http://x:1/y' } } })), null);
  assert.equal(endpointFromMcpJson('{ non json'), null);
  assert.equal(
    endpointFromMcpJson(JSON.stringify({ mcpServers: { 'unreal-mcp': { command: 'npx' } } })),
    null,
  );
});

test('della sezione MCP si leggono porta, path e accensione, e le altre sezioni non contaminano', () => {
  const ini = [
    '[/Script/UnrealEd.EditorEngine]',
    'ServerPortNumber=9999',
    `[${MCP_SECTION}]`,
    'bAutoStartServer=True',
    'ServerPortNumber=8766',
    'ServerUrlPath=/mcp',
    '[/Script/Qualcosa.Altro]',
    'ServerPortNumber=1234',
  ].join('\n');

  assert.deepEqual(settingsFromIni(ini), { port: 8766, path: '/mcp', autoStart: true });
});

test('una chiave duplicata vale per la SUA ultima occorrenza, come fa Unreal', () => {
  // Unreal riscrive per accodamento: prendere la prima leggerebbe un valore che il motore non usa.
  const ini = [`[${MCP_SECTION}]`, 'ServerPortNumber=8000', 'ServerPortNumber=8765'].join('\n');
  assert.equal(settingsFromIni(ini).port, 8765);
});

test('una sezione assente non e\' un errore: nessuna chiave, e il default versionato resta in vigore', () => {
  assert.deepEqual(settingsFromIni('[/Script/UnrealEd.EditorEngine]\nqualcosa=1'), {});
});

test("l'override per utente vince sul default versionato, chiave per chiave", () => {
  const saved = { port: 8766 };
  const projectDefault = { port: 8765, path: '/mcp', autoStart: false };

  // La porta viene da Saved, path e accensione dal default: la gerarchia e' per chiave, non per file.
  assert.deepEqual(effective(saved, projectDefault), {
    port: 8766,
    path: '/mcp',
    autoStart: false,
  });
});

test('un clone che NON ospita il ponte non ha disaccordi, anche con la porta diversa', () => {
  // E' il caso normale, ed e' la ragione per cui questo gate non e' rosso su ogni checkout che non
  // e' MAIN: una porta che non verra' mai aperta non puo' contraddire nessuno.
  const declared = endpointFromMcpJson(MCP_JSON);
  assert.deepEqual(disagreements(declared, { port: 8766, path: '/mcp', autoStart: false }), []);
});

test('un clone che ospita con la porta derivata produce il disaccordo, e lo nomina', () => {
  const declared = endpointFromMcpJson(MCP_JSON);
  const problems = disagreements(declared, { port: 8766, path: '/mcp', autoStart: true });

  assert.equal(problems.length, 1);
  assert.match(problems[0], /porta/);
  assert.match(problems[0], /8765/);
  assert.match(problems[0], /8766/);
});

test('anche il path viene confrontato: un endpoint giusto sulla rotta sbagliata tace uguale', () => {
  const declared = endpointFromMcpJson(MCP_JSON);
  const problems = disagreements(declared, { port: 8765, path: '/rpc', autoStart: true });

  assert.equal(problems.length, 1);
  assert.match(problems[0], /path/);
});

test('un clone che ospita e concorda su entrambi non ha disaccordi', () => {
  const declared = endpointFromMcpJson(MCP_JSON);
  assert.deepEqual(disagreements(declared, { port: 8765, path: '/mcp', autoStart: true }), []);
});

test('un .mcp.json che non dichiara nulla e\' un difetto a se', () => {
  const problems = disagreements(null, { port: 8765, autoStart: true });
  assert.equal(problems.length, 1);
  assert.match(problems[0], /\.mcp\.json/);
});
