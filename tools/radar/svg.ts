/** Generatore SVG deterministico (#560, D-108).
 *
 *  L'insidia non è disegnare: è che le coordinate vengono da `sin`/`cos`, cioè da **float**, la cui
 *  resa decimale cambia con piattaforma e locale. Il gate `--check` confronta gli artefatti byte a
 *  byte, quindi ogni numero che finisce nel documento è arrotondato **esplicitamente** e formattato
 *  senza passare dal locale. */
import type { ProfileAxes } from './profile.ts';

export interface AxisSpec {
  key: string;
  label: string;
}

/** L'ordine è **normativo** (D-105, §2.3): cambiarlo cambia la forma del poligono a parità di
 *  valori, e due radar prodotti con ordini diversi non sono confrontabili. */
export const PROFILE_AXES: AxisSpec[] = [
  { key: 'offense', label: 'Offesa' },
  { key: 'durability', label: 'Durabilità' },
  { key: 'mobility', label: 'Mobilità' },
  { key: 'control', label: 'Controllo' },
  { key: 'support', label: 'Supporto' },
  { key: 'information', label: 'Informazione' },
];

/** Cinque assi, in **quest'ordine** (owner §2.2). Vale la stessa regola del Profile: cambiare
 *  l'ordine cambia la forma a parita' di valori, quindi due radar con ordini diversi non sono
 *  confrontabili. */
export const BALANCE_AXES: AxisSpec[] = [
  { key: 'precision', label: 'Precisione' },
  { key: 'power', label: 'Potenza' },
  { key: 'control', label: 'Controllo' },
  { key: 'support', label: 'Supporto' },
  { key: 'durability', label: 'Durabilità' },
];

const SIZE = 400;
const CENTER = SIZE / 2;
const RADIUS = 150;
const MAX_RATING = 10;

/** Decimali fissi, punto come separatore, **mai** `toLocaleString`: con locale italiano darebbe la
 *  virgola e l'SVG non sarebbe nemmeno valido. `toFixed` è già locale-independent, ma il vincolo va
 *  scritto perché la prossima persona non lo scopra dal gate rosso. */
function coord(n: number): string {
  return n.toFixed(2);
}

/** Vertice dell'asse `i` su `count`, a distanza `ratio` dal centro. Il primo raggio punta in alto. */
function vertex(i: number, count: number, ratio: number): [string, string] {
  const angle = (2 * Math.PI * i) / count - Math.PI / 2;
  return [coord(CENTER + RADIUS * ratio * Math.cos(angle)), coord(CENTER + RADIUS * ratio * Math.sin(angle))];
}

function escapeXml(s: string): string {
  return s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c]!);
}

/** Produce l'SVG, o **fallisce** se un asse non ha un valore.
 *
 *  Non esiste un radar «quasi completo» (owner §6): un asse mancante disegnato come `0` accuserebbe
 *  l'eroe di una debolezza che nessuno ha misurato. */
export function renderRadar(
  hero: string,
  role: string,
  axes: AxisSpec[],
  values: Record<string, number | undefined>,
): string {
  const missing = axes.filter((a) => typeof values[a.key] !== 'number').map((a) => a.key);
  if (missing.length > 0) {
    throw new Error(
      `${hero}: radar non generabile, assi senza valore: ${missing.join(', ')}. ` +
        `Un asse TBD non si disegna come 0 (owner §6).`,
    );
  }

  const grid = [2, 4, 6, 8, 10]
    .map((step) => {
      const points = axes.map((_, i) => vertex(i, axes.length, step / MAX_RATING).join(',')).join(' ');
      return `  <polygon class="grid" points="${points}" />`;
    })
    .join('\n');

  const shape = axes.map((a, i) => vertex(i, axes.length, values[a.key]! / MAX_RATING).join(',')).join(' ');

  const labels = axes
    .map((a, i) => {
      const [x, y] = vertex(i, axes.length, 1.18);
      return (
        `  <text class="axis" x="${x}" y="${y}">${escapeXml(a.label)}</text>\n` +
        `  <text class="value" x="${x}" y="${coord(Number(y) + 14)}">${values[a.key]}</text>`
      );
    })
    .join('\n');

  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${SIZE} ${SIZE}" role="img" aria-labelledby="t">
  <title id="t">${escapeXml(hero)} — ${escapeXml(role)} — Profile Radar</title>
  <style>
    .grid { fill: none; stroke: #d0d0d0; stroke-width: 1 }
    .shape { fill: #4a7fb5; fill-opacity: .35; stroke: #2b5c8a; stroke-width: 2 }
    .axis { font: 12px sans-serif; text-anchor: middle; fill: #333 }
    .value { font: bold 13px sans-serif; text-anchor: middle; fill: #111 }
  </style>
${grid}
  <polygon class="shape" points="${shape}" />
${labels}
</svg>
`;
}

/** Due profili sovrapposti sullo stesso set di assi.
 *
 *  Serve a rispondere «in cosa A e' avanti su B», che e' una delle due cose per cui il radar esiste.
 *  Le due forme devono restare **distinguibili**: colore diverso, riempimento piu' leggero e una
 *  legenda — due poligoni traslucidi senza etichetta non si leggono. */
export function renderCompare(
  a: { name: string; values: Record<string, number | undefined> },
  b: { name: string; values: Record<string, number | undefined> },
  axes: AxisSpec[],
): string {
  for (const side of [a, b]) {
    const missing = axes.filter((x) => typeof side.values[x.key] !== 'number').map((x) => x.key);
    if (missing.length > 0) {
      throw new Error(`${side.name}: radar non generabile, assi senza valore: ${missing.join(', ')}.`);
    }
  }

  const grid = [2, 4, 6, 8, 10]
    .map((step) => `  <polygon class="grid" points="${axes.map((_, i) => vertex(i, axes.length, step / MAX_RATING).join(',')).join(' ')}" />`)
    .join('\n');

  const shape = (side: typeof a, cls: string) =>
    `  <polygon class="shape ${cls}" points="${axes.map((x, i) => vertex(i, axes.length, side.values[x.key]! / MAX_RATING).join(',')).join(' ')}" />`;

  const labels = axes
    .map((x, i) => {
      const [px, py] = vertex(i, axes.length, 1.18);
      return `  <text class="axis" x="${px}" y="${py}">${escapeXml(x.label)}</text>`;
    })
    .join('\n');

  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${SIZE} ${SIZE}" role="img" aria-labelledby="t">
  <title id="t">${escapeXml(a.name)} vs ${escapeXml(b.name)} — confronto</title>
  <style>
    .grid { fill: none; stroke: #d0d0d0; stroke-width: 1 }
    .shape { fill-opacity: .25; stroke-width: 2 }
    .a { fill: #4a7fb5; stroke: #2b5c8a }
    .b { fill: #b5764a; stroke: #8a5a2b }
    .axis { font: 12px sans-serif; text-anchor: middle; fill: #333 }
    .legend { font: bold 12px sans-serif }
  </style>
${grid}
${shape(a, 'a')}
${shape(b, 'b')}
${labels}
  <rect class="a" x="12.00" y="12.00" width="12.00" height="12.00" />
  <text class="legend" x="30.00" y="22.00" fill="#2b5c8a">${escapeXml(a.name)}</text>
  <rect class="b" x="12.00" y="30.00" width="12.00" height="12.00" />
  <text class="legend" x="30.00" y="40.00" fill="#8a5a2b">${escapeXml(b.name)}</text>
</svg>
`;
}
