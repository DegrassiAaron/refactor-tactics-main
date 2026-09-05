/** Verifica che ogni asset del progetto abbia una riga nel registro di provenienza.
 *
 *  Uso:  node tools/asset-provenance/check.ts
 *
 *  Esce **1** se trova un asset scoperto, una riga morta o un registro illeggibile.
 *  Il registro e' `docs/technical/asset-licenze.md`, ed e' l'oggetto che `FR-ASSET-LIC-01`
 *  (`spec-asset-pipeline.md` §8) prescriveva senza che esistesse: fino al 2026-09-05 il verificatore
 *  scritto accanto al requisito **non poteva fallire**, che e' il difetto che `#1767` misura.
 *
 *  ⚠️ Gate **a mano**, come `tools/asset-refs/check.ts` e i due di `tools/radar/`: il repository non ha
 *  CI per scelta dichiarata (`D-182`, `AGENTS.md`). Si esegue prima di committare un asset.
 *
 *  ⛔ **Un verde significa «registrato», mai «consentito».** Questo controllo non puo' leggere un EULA:
 *  verifica che una **riga esista**, non che dica il vero. Il guardrail e' scritto in `#1767` a lettere
 *  maiuscole e va ripetuto qui, perche' un gate che tace su cosa non misura viene letto per piu' di
 *  quello che e'.
 *
 *  🔴 **Guarda due popolazioni, e una sola non basterebbe.**
 *
 *   - **P1 — versionata**: i file asset che `git ls-files` conosce sotto `Content/` e `tools/`.
 *   - **P2 — referenziata**: i package path citati dagli asset versionati che **nessun file versionato
 *     soddisfa**. E' precisamente cio' che `tools/asset-refs/check.ts` lascia passare oggi con
 *     `ALLOWED_PREFIXES = ['/Game/FabAsset/']` — la riga in cui il progetto dice «sta fuori dal
 *     repository e va bene cosi'» **senza dire sotto quale licenza**.
 *
 *  Un gate che guardasse solo P1 vedrebbe **365** file e **zero** dei 37.482 `.uasset` sotto
 *  `Content/FabAsset/`: sono ignorati da `.gitignore` per scelta, quindi andrebbe verde su ~15,8 GB di
 *  contenuto di terze parti non registrato. E' il difetto che `#1767` chiama *«un registro che esiste
 *  fuori dal versionamento non e' un registro, e' un file»*, visto dal lato del controllo.
 *
 *  ⚠️ **Cosa NON verifica**, dichiarato perche' non venga scoperto dopo:
 *
 *   - **la verita' di una riga.** Vedi sopra: esistenza, non contenuto;
 *   - **`docs/`.** Le immagini della documentazione (`docs/technical/img/*.png`, gli SVG generati di
 *     `docs/characters/radar/`) non sono nella popolazione. E' un **confine dichiarato**, non una
 *     dimenticanza: `FR-ASSET-LIC-01` parla degli asset **importati nel gioco**, e allargare qui
 *     significherebbe registrare ogni figura di ogni documento. Se si decide che serve, e' un'altra
 *     issue e una riga in piu' in `DIRECTORY`;
 *   - **i file senza estensione d'asset.** `README.md`, `manifest.json`, `.gitkeep` e i sorgenti dei
 *     tool non sono asset. Il conteggio degli scartati si stampa, cosi' il confine e' misurabile
 *     invece che creduto;
 *   - **i pack che nessuno ha ancora scaricato.** Un pack valutato e non acquisito non e' ne'
 *     versionato ne' referenziato: nessuna delle due popolazioni lo vede. */
import { readFileSync, existsSync, readdirSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { copre, findDeadRows, findUncovered, parseRegistry } from './registry.ts';
import { extractPackageRefs, toContentPaths } from '../asset-refs/refs.ts';

const REPO_ROOT = fileURLToPath(new URL('../../', import.meta.url));
const REGISTRO = 'docs/technical/asset-licenze.md';

/** Le directory della popolazione P1. Allargarle e' una decisione, non una svista da correggere. */
const DIRECTORY = ['Content', 'tools'];

/** Cio' che conta come asset. Un file fuori da questo elenco non e' scoperto: e' **fuori scopo**, e i
 *  due esiti vanno tenuti distinti perche' hanno rimedi opposti — uno si registra, l'altro si ignora. */
const ESTENSIONI_ASSET = [
  'uasset', 'umap', 'svg', 'png', 'jpg', 'jpeg', 'tga', 'bmp', 'wav', 'ogg', 'mp3',
  'fbx', 'obj', 'psd', 'exr', 'hdr', 'ttf', 'otf', 'zip',
];

/**
 * I riferimenti che **non sono contenuto**, misurati uno per uno, con la ragione.
 *
 * ⚠️ **Questo elenco e' il gemello di `KNOWN_EXCEPTIONS` in `tools/asset-refs/check.ts`, e la
 * duplicazione e' deliberata.** I due gate rispondono a domande diverse — *«il riferimento risolve?»*
 * contro *«il riferimento ha una provenienza?»* — e fondere gli elenchi legherebbe la seconda risposta
 * a una costante che appartiene alla prima. Il costo e' che possono divergere; il rimedio e' che
 * entrambi dichiarano morta un'eccezione che non serve piu', quindi la divergenza diventa rossa invece
 * che silenziosa.
 */
const RESIDUI_DICHIARATI = [
  {
    ref: '/Game/Maps/L_Prototype',
    perche:
      `Metadato, non dipendenza — #1280. Sta nella coda del package di L_DevSandbox, dove vivono `
      + `thumbnail e asset registry data, ed e' il path da cui la sandbox fu duplicata quando `
      + `L_Prototype stava ancora in /Game/Maps/. Non e' contenuto di terze parti e non ha una `
      + `provenienza da registrare. L'analisi completa e la sua verifica stanno in `
      + `tools/asset-refs/check.ts.`,
  },
];

function git(...args: string[]): string[] {
  return execFileSync('git', args, { encoding: 'utf8', cwd: REPO_ROOT })
    .split('\n')
    .map((l) => l.trim())
    .filter(Boolean);
}

// ---------------------------------------------------------------- il registro

const registroPath = `${REPO_ROOT}${REGISTRO}`;
if (!existsSync(registroPath)) {
  console.error(`${REGISTRO} non esiste. E' il registro che FR-ASSET-LIC-01 prescrive.`);
  process.exit(1);
}
const { rows, issues } = parseRegistry(readFileSync(registroPath, 'utf8'));

// ---------------------------------------------------------------- P1: versionata

const versionati = git('ls-files', ...DIRECTORY);
const estensione = (p: string) => p.slice(p.lastIndexOf('.') + 1).toLowerCase();
const asset = versionati.filter((p) => p.includes('.') && ESTENSIONI_ASSET.includes(estensione(p)));
const scartati = versionati.length - asset.length;

// ---------------------------------------------------------------- P2: referenziata

const packages = git('ls-files', 'Content/*.uasset', 'Content/*.umap');
const packageSet = new Set(packages);
const residui = new Set(RESIDUI_DICHIARATI.map((r) => r.ref));
const residuiVivi = new Set<string>();

const referenziati: { path: string; origine: string }[] = [];
const visti = new Set<string>();
for (const pkg of packages) {
  for (const ref of extractPackageRefs(readFileSync(`${REPO_ROOT}${pkg}`))) {
    if (toContentPaths(ref).some((c) => packageSet.has(c))) continue;
    if (residui.has(ref)) { residuiVivi.add(ref); continue; }
    if (visti.has(ref)) continue;
    visti.add(ref);
    referenziati.push({ path: ref, origine: pkg });
  }
}

// ---------------------------------------------------------------- P3: presente sul disco

/** I path che tengono viva una riga senza essere ne' versionati ne' referenziati: i pack scaricati.
 *  Servono solo alla diagnosi di riga morta — non sono una popolazione da coprire, perche' un file sul
 *  disco che nessuno cita non arriva a nessun giocatore. */
const suDisco: string[] = [];
for (const r of rows) {
  if (!r.copre.startsWith('/Game/')) continue;
  const dir = `${REPO_ROOT}Content/${r.copre.slice('/Game/'.length)}`;
  if (existsSync(dir) && readdirSync(dir).length > 0) suDisco.push(r.copre);
}

/** Le righe che descrivono un posto **non raggiungibile da questo disco**: la cartella non c'e'.
 *  Su un clone pulito `Content/FabAsset/` non esiste — i pack si riscaricano — quindi dichiararle
 *  morte renderebbe il gate rosso per il motivo sbagliato. Si contano e si dicono, non si accusano. */
const nonVerificabili = rows.filter((r) => {
  if (!r.copre.startsWith('/Game/')) return false;
  if (suDisco.includes(r.copre)) return false;
  const coperta = [...asset, ...referenziati.map((x) => x.path)].some((p) => copre(r.copre, p));
  return !coperta;
});

/** I path che tengono viva una riga.
 *
 *  🔴 **Piu' larghi della popolazione da coprire, e di proposito.** Qui entrano **tutti** i file
 *  versionati, non i soli asset: `Content/RT_UI_AssetPack_FromHUD/` versiona il suo `README.md` e il
 *  suo `manifest.json` e **nessuna** delle immagini che descrive, e la sua riga e' viva lo stesso —
 *  la famiglia c'e', il progetto la conosce, e il DoD di `#1767` chiede proprio che un'assenza sia
 *  **motivata nel registro** invece che tacere. Usare la sola popolazione degli asset la
 *  dichiarerebbe morta, cioe' chiederebbe di cancellare la riga che documenta il caso peggiore. */
const vivificanti = [...versionati, ...referenziati.map((x) => x.path), ...suDisco];

// ---------------------------------------------------------------- verdetto

const popolazione = [
  ...asset.map((path) => ({ path, origine: 'versionato' })),
  ...referenziati,
];
const scoperti = findUncovered(popolazione, rows);
const morte = findDeadRows(rows.filter((r) => !nonVerificabili.includes(r)), vivificanti);
const residuiMorti = RESIDUI_DICHIARATI.filter((r) => !residuiVivi.has(r.ref));

for (const i of issues) {
  console.error(`${REGISTRO}:${i.riga} REGISTRO ILLEGGIBILE — ${i.perche}`);
  if (i.testo) console.error(`  ${i.testo}`);
}

for (const s of scoperti) {
  console.error(`${s.path}\n  -> nessuna riga del registro lo copre  (${s.origine})`);
}

for (const r of morte) {
  console.error(
    `RIGA MORTA: ${REGISTRO}:${r.riga}\n`
    + `  -> il prefisso ${r.copre} non copre piu' niente. Toglila, o spiega perche' resta.`,
  );
}

for (const r of residuiMorti) {
  console.error(
    `RESIDUO MORTO: ${r.ref} non e' piu' citato da nessun package versionato.\n`
    + `  Toglilo da RESIDUI_DICHIARATI.  (era: ${r.perche})`,
  );
}

const nonVerificate = rows.filter((r) => r.licenza === 'NON VERIFICATA').length;

console.log(
  `${rows.length} righe di registro; ${asset.length} asset versionati (${scartati} file non-asset `
  + `scartati) e ${referenziati.length} riferimenti non soddisfatti; ${scoperti.length} scoperti, `
  + `${morte.length} righe morte`
  + (nonVerificabili.length > 0
    ? `; ${nonVerificabili.length} righe non verificabili su questo disco (i pack non sono qui)`
    : '')
  + `; ${nonVerificate} righe con licenza NON VERIFICATA.`,
);
console.log(
  'Un verde dice che ogni asset ha una riga. NON dice che la licenza sia rispettata: '
  + 'nessun controllo automatico puo' + "'" + ' leggerla.',
);

const rotto = issues.length + scoperti.length + morte.length + residuiMorti.length;
process.exit(rotto === 0 ? 0 : 1);
