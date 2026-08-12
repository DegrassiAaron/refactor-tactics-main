/** I sei assi del Profile Radar (#562, D-107), nell'ordine normativo di §2.1.
 *
 *  Tre di essi — `durability`, `control`, `support` — sono condivisi col Balance Radar: si calcolano
 *  qui una volta sola, o le due viste direbbero cose diverse sullo stesso eroe. */
import type { AbilityInput, HeroInput } from './parse-catalog.ts';
import { availabilityWeight, rate } from './rubric.ts';
import { powerRating } from './power.ts';
import { precisionRating } from './precision.ts';

/** Ancore assolute (D-112). Derivate da grandezze del gioco, non scelte perche' i conti tornassero:
 *  la salute non supera i 150, il movimento composto non supera gli 80. */
const DURABILITY_ANCHOR = 150;
const MOBILITY_ANCHOR = 80;
const CONTROL_ANCHOR = 30;
const SUPPORT_ANCHOR = 25;

/** Disponibilita' sulla scala leggibile (10 · 6.67 · 5 · 4), derivata dal peso intero. */
function availability(cooldown: number): number {
  return (20 * availabilityWeight(cooldown)) / 60;
}

/** Numero che segue una parola chiave nella cella `Effetto`: `Dash 3` -> 3, `scudo 15` -> 15. */
function amountAfter(effect: string, keyword: string): number {
  return Number(effect.match(new RegExp(`${keyword}\\s+(\\d+)`, 'i'))?.[1] ?? 0);
}

/** Vocabolario **chiuso** degli effetti che alimentano `control` — §2.1: push, pull, slow, zoning,
 *  denial, modifica cover e archi, blocco di rotte. Ogni voce conta **una volta** per abilita'. */
const CONTROL_EFFECTS = [
  /Push \d/,
  /`?Slow`?/,
  /`?Interrupt`?/,
  /stop del movimento/,
  /crea fumo/, // zoning
  /crea acqua/, // denial
  /crea una copertura/,
  /sposta o ruota una copertura/,
  /marca una cella/,
];

/** Vocabolario **chiuso** di `support` — §2.1: protezione alleati, sustain, buff, peel, setup per
 *  combo. ⚠️ `Wet` sta qui e non in `control`: non controlla nessuno, **abilita** il payoff elettrico. */
const SUPPORT_EFFECTS = [
  /cura \d/,
  /applica `Wet`/,
  /`Wet` ai nemici/,
  /intercetta un attacco diretto a un alleato/,
];

function countMatching(a: AbilityInput, patterns: RegExp[]): number {
  return patterns.filter((p) => p.test(a.effect)).length;
}

/** `Salute` piu' la mitigazione che l'eroe applica **a se stesso**.
 *
 *  ⚠️ La divisione per bersaglio non e' un dettaglio: `Bastion.Interposition` incassa per un
 *  ALLEATO, quindi va in `support`. Contarla qui renderebbe Bastion il piu' resistente due volte e
 *  svuoterebbe `support` del suo contributo piu' concreto. */
function durabilityRaw(hero: HeroInput): number {
  const selfMitigation = hero.abilities.reduce((sum, a) => {
    if (/a un alleato/.test(a.effect)) return sum; // protegge altri: e' support
    const amount = amountAfter(a.effect, 'scudo') + amountAfter(a.effect, 'riduce il danno di');
    return sum + (amount * availability(a.cooldown)) / 10;
  }, 0);

  return hero.health + selfMitigation;
}

function mobilityRaw(hero: HeroInput): number {
  return hero.abilities.reduce(
    (sum, a) =>
      sum +
      3 * amountAfter(a.effect, '`?Dash`?') +
      2 * amountAfter(a.effect, '`?Reposition`?') +
      (a.kind === 'charge' ? 2 : 0),
    10 * hero.movePoints,
  );
}

export interface ProfileAxes {
  offense: number;
  durability: number;
  mobility: number;
  control: number;
  support: number;
  information: number;
}

export function profileAxes(hero: HeroInput): ProfileAxes {
  const power = powerRating(hero);
  const precision = precisionRating(hero);

  const sumOver = (patterns: RegExp[]) =>
    hero.abilities.reduce((s, a) => s + countMatching(a, patterns) * availability(a.cooldown), 0);

  return {
    // Il Profile **aggrega** cio' che il Balance scompone. Un PRODOTTO e non una media: la precisione
    // di chi colpisce poco deve MODULARE l'offesa, non comprarla — con la media Bastion uscirebbe
    // offensivo quanto Flux, facendo 8 danni di attacco base contro 22.
    offense: Math.max(1, Math.min(10, Math.round(power * (0.5 + precision / 20)))),
    durability: rate(durabilityRaw(hero), DURABILITY_ANCHOR),
    mobility: rate(mobilityRaw(hero), MOBILITY_ANCHOR),
    control: rate(sumOver(CONTROL_EFFECTS), CONTROL_ANCHOR),
    support: rate(sumOver(SUPPORT_EFFECTS), SUPPORT_ANCHOR),
    // `information` ha un solo ingrediente e la Vista e' gia' sulla scala giusta (D-107): non c'e'
    // nulla da normalizzare, e fingere una formula prometterebbe piu' di quanto l'asse mantenga.
    information: Math.max(1, Math.min(10, hero.visionRange)),
  };
}
