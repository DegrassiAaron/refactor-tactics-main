/** Il vocabolario **chiuso** degli effetti (#557, #559).
 *
 *  Ogni frammento che la rubrica sa leggere e' elencato qui. Un effetto che non compare **non vale
 *  zero**: fa fallire la generazione. Uno zero silenzioso e' il difetto peggiore di questa catena —
 *  un'abilita' nuova entrerebbe nei radar senza contribuire, e nessuno se ne accorgerebbe. */
import type { AbilityInput, HeroInput } from './parse-catalog.ts';

/** I frammenti riconosciuti, con la sede che li consuma. Aggiungere un'abilita' al catalogo che ne
 *  usi uno nuovo richiede di **estendere questa lista e la formula che lo pesa**, insieme. */
export const KNOWN_EFFECTS: { pattern: RegExp; axis: string }[] = [
  { pattern: /\d+ danni/, axis: 'power · precision' },
  { pattern: /\+\d+ su/, axis: 'precision (condizionalita\')' },
  { pattern: /propagazione/, axis: 'precision (non selettivo)' },
  { pattern: /`?Push`? \d/, axis: 'control' },
  { pattern: /`?Slow`?/, axis: 'control' },
  { pattern: /`?Interrupt`?/, axis: 'control' },
  { pattern: /stop del movimento/, axis: 'control' },
  { pattern: /crea fumo/, axis: 'control · precision' },
  { pattern: /crea acqua/, axis: 'control · precision' },
  { pattern: /crea una copertura/, axis: 'control' },
  { pattern: /sposta o ruota una copertura/, axis: 'control' },
  { pattern: /marca una cella/, axis: 'control' },
  { pattern: /cura \d/, axis: 'support' },
  { pattern: /applica `Wet`|`Wet` ai nemici/, axis: 'support (setup)' },
  { pattern: /intercetta un attacco diretto a un alleato/, axis: 'support (peel)' },
  { pattern: /scudo \d/, axis: 'durability' },
  { pattern: /riduce il danno di \d/, axis: 'durability' },
  { pattern: /`?Dash`? \d/, axis: 'mobility' },
  { pattern: /`?Reposition`? ?\d?/, axis: 'mobility' },
];

/** Frammenti che il catalogo scrive ma che **nessun asse consuma**, dichiarati perche' non e' una
 *  dimenticanza: la loro presenza non deve far fallire il gate. */
const IGNORED = [/range \d/, /raggio \d/, /`Action\.\w+`/, /\(\[D-\d+\]/, /attraversando/, /lungo il percorso/, /sui dispositivi/, /agli alleati/, /all'attaccante/];

/** Spezza la cella `Effetto` nei suoi frammenti e verifica che ognuno sia noto. */
export function assertKnownEffects(hero: HeroInput): void {
  for (const a of hero.abilities) {
    for (const frag of splitEffect(a)) {
      const known = KNOWN_EFFECTS.some((e) => e.pattern.test(frag)) || IGNORED.some((p) => p.test(frag));
      if (!known) {
        throw new Error(
          `${a.id}: effetto non nel vocabolario: "${frag}". ` +
            `Un effetto sconosciuto NON vale zero — aggiungilo a KNOWN_EFFECTS e alla formula che lo pesa.`,
        );
      }
    }
  }
}

/** I frammenti separati da virgola, ripuliti dal grassetto markdown. */
function splitEffect(a: AbilityInput): string[] {
  return a.effect
    .replace(/\*\*/g, '')
    .split(',')
    .map((x) => x.trim())
    .filter((x) => x.length > 0 && !/^è /.test(x));
}
