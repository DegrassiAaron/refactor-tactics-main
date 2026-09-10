/** Verifica che il ponte MCP dichiarato e il ponte configurato siano lo stesso ponte.
 *
 *  Uso:  node tools/mcp/check.ts [--check]
 *
 *  Il problema che chiude: l'endpoint del bridge vive in **due** posti che devono concordare, e uno
 *  dei due non e' versionato.
 *
 *    .mcp.json                                          versionato, lo legge ogni sessione Claude Code
 *    Saved/Config/<Piattaforma>/EditorPerProjectUserSettings.ini    per utente, gitignorato
 *
 *  Quando divergono il sintomo non e' un errore, e' **l'assenza**: il server `unreal-mcp` semplicemente
 *  non compare, senza una riga. E' successo almeno tre volte — `ff2488f7` il 2026-08-28, un `8000`
 *  conservato in un `.bak-pre-mcp-fix`, e il 2026-09-10 MAIN su `8766` mentre `.mcp.json` chiedeva
 *  `8765` (#2849). Ogni volta la diagnosi e' ripartita da zero, perche' nessun comando la diceva.
 *
 *  ⚠️ **Fallisce solo se QUESTO clone ospita il ponte.** Il bridge e' uno per macchina e lo accende un
 *  clone solo: dove `bAutoStartServer` e' spento — il default versionato, e il caso normale — una porta
 *  diversa non e' un difetto, e' una riga che non verra' mai usata. Un gate che gridasse anche li'
 *  sarebbe rosso su ogni checkout che non e' MAIN, e verrebbe spento al terzo falso positivo.
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perche' non venga scoperto dopo:
 *   - che il server sia **vivo**. Questo confronta due file; se il ponte risponde lo dice
 *     `Invoke-WebRequest http://127.0.0.1:<porta>/mcp` (`405` su GET = sano). La diagnosi completa
 *     sta in `docs/technical/tooling/brief-mcp-developer-bridge.md` §9.2;
 *   - la porta di un ALTRO clone: legge solo il proprio albero, e non ha modo di sapere quale clone
 *     ospiti oggi;
 *   - l'override da riga di comando `-ModelContextProtocolPort=N`, che vince sui settings e non lascia
 *     traccia su disco.
 *
 *  La copertura si stampa **sempre**, anche in verde: un gate che non dice quanto ha guardato non e'
 *  distinguibile da uno che non guarda (#576). */
import { readFileSync, existsSync, readdirSync } from 'node:fs';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { join } from 'node:path';

const REPO_ROOT = fileURLToPath(new URL('../../', import.meta.url));

/** La sezione che Unreal legge. Un nome diverso viene ignorato **in silenzio**, quindi il default
 *  sicuro resterebbe in vigore senza un errore: e' scritto anche in
 *  `Config/DefaultEditorPerProjectUserSettings.ini`, e vale la pena non riscriverlo a memoria. */
export const MCP_SECTION = '/Script/ModelContextProtocolEngine.ModelContextProtocolSettings';

/** L'endpoint come `.mcp.json` lo dichiara. */
export interface Endpoint {
  host: string;
  port: number;
  path: string;
}

/** Porta, path e accensione come i settings li configurano. `undefined` = la chiave non c'e'. */
export interface McpSettings {
  port?: number;
  path?: string;
  autoStart?: boolean;
}

/** Legge l'endpoint di un server da `.mcp.json`. `null` se il server non c'e' o non ha un url http. */
export function endpointFromMcpJson(text: string, server = 'unreal-mcp'): Endpoint | null {
  let parsed: unknown;
  try {
    parsed = JSON.parse(text);
  } catch {
    return null;
  }
  const servers = (parsed as { mcpServers?: Record<string, { url?: string }> })?.mcpServers;
  const url = servers?.[server]?.url;
  if (typeof url !== 'string') return null;

  const m = /^https?:\/\/([^/:]+):(\d+)(\/.*)?$/.exec(url);
  if (!m) return null;
  return { host: m[1], port: Number(m[2]), path: m[3] ?? '/' };
}

/** Legge la sezione MCP di un `EditorPerProjectUserSettings.ini`.
 *
 *  ⚠️ Prende l'**ultima** occorrenza di ogni chiave: Unreal riscrive questi file per accodamento, e una
 *  chiave duplicata non e' un errore di sintassi — vince l'ultima, e un parser che prendesse la prima
 *  leggerebbe un valore che il motore non usa. */
export function settingsFromIni(text: string): McpSettings {
  const out: McpSettings = {};
  let inSection = false;

  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    if (line.startsWith('[')) {
      inSection = line === `[${MCP_SECTION}]`;
      continue;
    }
    if (!inSection || !line || line.startsWith(';')) continue;

    const eq = line.indexOf('=');
    if (eq < 0) continue;
    const key = line.slice(0, eq).trim();
    const value = line.slice(eq + 1).trim();

    if (key === 'ServerPortNumber' && /^\d+$/.test(value)) out.port = Number(value);
    else if (key === 'ServerUrlPath') out.path = value;
    else if (key === 'bAutoStartServer') out.autoStart = /^true$/i.test(value);
  }
  return out;
}

/** Il valore che il motore usera' davvero: l'override per utente se c'e', altrimenti il default
 *  versionato del progetto. E' la gerarchia di Unreal — `Config/Default*.ini` fa da base e
 *  `Saved/Config/<Piattaforma>/*.ini` la sovrascrive — e modellarla e' cio' che rende questo gate
 *  corretto dopo che il default e' stato dichiarato. */
export function effective(saved: McpSettings, projectDefault: McpSettings): McpSettings {
  return {
    port: saved.port ?? projectDefault.port,
    path: saved.path ?? projectDefault.path,
    autoStart: saved.autoStart ?? projectDefault.autoStart,
  };
}

/** I disaccordi fra cio' che si dichiara e cio' che si configura.
 *
 *  Vuoto quando questo clone non ospita il ponte: li' una porta diversa non verra' mai usata. */
export function disagreements(declared: Endpoint | null, eff: McpSettings): string[] {
  if (!declared) return ['.mcp.json non dichiara un endpoint http per `unreal-mcp`'];
  if (eff.autoStart !== true) return [];

  const problems: string[] = [];
  if (eff.port !== undefined && eff.port !== declared.port) {
    problems.push(
      `porta: .mcp.json chiede ${declared.port}, i settings servono ${eff.port} — ` +
        'il ponte sarebbe vivo e irraggiungibile insieme',
    );
  }
  if (eff.path !== undefined && eff.path !== declared.path) {
    problems.push(`path: .mcp.json chiede \`${declared.path}\`, i settings servono \`${eff.path}\``);
  }
  return problems;
}

/** Il primo `Saved/Config/<Piattaforma>/EditorPerProjectUserSettings.ini` che esiste, o `null`. */
function savedSettingsPath(root: string): string | null {
  const dir = join(root, 'Saved', 'Config');
  if (!existsSync(dir)) return null;
  for (const platform of readdirSync(dir)) {
    const candidate = join(dir, platform, 'EditorPerProjectUserSettings.ini');
    if (existsSync(candidate)) return candidate;
  }
  return null;
}

function main(): void {
  const check = process.argv.includes('--check');

  const mcpJson = join(REPO_ROOT, '.mcp.json');
  const declared = existsSync(mcpJson)
    ? endpointFromMcpJson(readFileSync(mcpJson, 'utf8'))
    : null;

  const defaultIni = join(REPO_ROOT, 'Config', 'DefaultEditorPerProjectUserSettings.ini');
  const projectDefault = existsSync(defaultIni)
    ? settingsFromIni(readFileSync(defaultIni, 'utf8'))
    : {};

  const savedIni = savedSettingsPath(REPO_ROOT);
  const saved = savedIni ? settingsFromIni(readFileSync(savedIni, 'utf8')) : {};

  const eff = effective(saved, projectDefault);

  console.error(
    `.mcp.json: ${declared ? `${declared.host}:${declared.port}${declared.path}` : 'assente'} | ` +
      `settings: porta ${eff.port ?? 'non dichiarata'}, path ${eff.path ?? 'non dichiarato'}, ` +
      `bAutoStartServer ${eff.autoStart ?? 'non dichiarato'} | ` +
      `override per utente: ${savedIni ? 'presente' : 'nessuno'}`,
  );

  if (eff.autoStart !== true) {
    console.error(
      'questo clone NON ospita il ponte (bAutoStartServer spento): niente da far concordare.',
    );
    return;
  }

  const problems = disagreements(declared, eff);
  if (problems.length === 0) {
    console.error('questo clone ospita il ponte, e i due lati concordano');
    return;
  }

  console.error(`\n${problems.length} disaccordi:\n  ${problems.join('\n  ')}`);
  console.error(
    '\nLa porta canonica e 8765, quella di MAIN: vedi Config/DefaultEditorPerProjectUserSettings.ini.\n' +
      "⚠️ Si corregge il SETTING, mai .mcp.json — e' versionato, e piegarlo alla porta di una macchina\n" +
      "e' la direzione che ha gia' prodotto ff2488f7. A Editor CHIUSO: Unreal riscrive quel file allo\n" +
      'shutdown, quindi una modifica applicata mentre gira si perde.',
  );
  if (check) process.exit(1);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
