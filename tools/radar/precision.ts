/** L'asse `precision` del Balance Radar (#603): quanto del danno arriva **senza chiedere niente** —
 *  né setup, né posizionamento della squadra.
 *
 *  Senza RNG nel combattimento, «precisione» non può significare probabilità di andare a segno: qui
 *  è strutturale, e combina due componenti con pesi `1:2`. */
import type { AbilityInput, HeroInput } from './parse-catalog.ts';
import { availabilityWeight, rate } from './rubric.ts';
import { guaranteedDamage } from './power.ts';

/** `w₁ : w₂` = incondizionalità : selettività. Misurato sul roster, non scelto a intuito:
 *  l'incondizionalità varia da `0.77` a `1.00` mentre la selettività copre `0.00–1.00`, quindi
 *  pesare di più la componente che varia meno comprimerebbe tre eroi su quattro nello stesso intero. */
const W_UNCONDITIONAL = 1;
const W_SELECTIVE = 2;

/** La scala è già `0..1`, quindi l'ancora è `1`. */
const PRECISION_ANCHOR = 1;

/** Il danno che l'azione produrrebbe **se ogni condizione fosse soddisfatta**: garantito più il
 *  bonus su stato, e per un payoff interamente predittivo il danno dichiarato. */
function potentialDamage(a: AbilityInput): number {
  return (a.damage ?? 0) + (a.conditionalBonus ?? 0);
}

/** Un'abilità **senza danno** entra nel conto solo se il suo effetto si applica a un'area e produce
 *  sugli alleati lo **stesso** svantaggio che produce sui nemici — `MistVeil`, `FluidTrail`.
 *  `CircularTide` no: distingue, cura gli alleati. Le coperture nemmeno: proteggono chi vi sta dietro. */
function hitsAlliesToo(a: AbilityInput): boolean {
  return /crea fumo|crea acqua/.test(a.effect);
}

/** Le azioni che `precision` guarda: le offensive, più le senza-danno ad area simmetrica. */
function relevant(hero: HeroInput): AbilityInput[] {
  return hero.abilities.filter((a) => potentialDamage(a) > 0 || hitsAlliesToo(a));
}

/** Un'azione è selettiva quando colpisce **solo** chi scegli. `linea` e `AoE` no: il friendly fire
 *  è reale, e `Scenarios/Combat/FriendlyFire.json` lo dimostra su `Flux.Overload`. */
function isSelective(a: AbilityInput): boolean {
  if (hitsAlliesToo(a)) return false;
  // La **propagazione** non e' selettiva anche quando la forma lo sembra: `Flux.ConductiveNode` ha
  // tipo `cella` ma scarica sul grafo conduttivo e colpisce chiunque sia collegato, alleati
  // compresi. La forma dichiara dove parte il colpo, non chi raggiunge.
  if (/propagazione/.test(a.effect)) return false;
  return !/linea|AoE|dash/.test(a.kind);
}

/** Quota di danno **garantito** sul potenziale, pesata per disponibilità.
 *
 *  Pesata e non grezza per coerenza con `power` e `selectivity`: un'abilità a cooldown lungo pesa
 *  meno ovunque, o i tre assi direbbero cose diverse sullo stesso kit. */
export function unconditionality(hero: HeroInput): number {
  let guaranteed = 0;
  let potential = 0;

  for (const a of hero.abilities) {
    const w = availabilityWeight(a.cooldown);
    guaranteed += guaranteedDamage(a) * w;
    potential += potentialDamage(a) * w;
  }

  // Un eroe senza alcun danno non ha nulla di condizionale da dichiarare.
  return potential === 0 ? 1 : guaranteed / potential;
}

/** Quota della **disponibilità** delle azioni rilevanti che è selettiva.
 *
 *  Sulla disponibilità e non sul danno, perché un'abilità senza danno non ha danno da pesare — e
 *  perché il rischio di colpire un alleato dipende da quanto **spesso** usi l'azione. */
export function selectivity(hero: HeroInput): number {
  const acts = relevant(hero);
  if (acts.length === 0) return 1;

  const total = acts.reduce((n, a) => n + availabilityWeight(a.cooldown), 0);
  const selective = acts
    .filter(isSelective)
    .reduce((n, a) => n + availabilityWeight(a.cooldown), 0);

  return selective / total;
}

export function precisionRating(hero: HeroInput): number {
  const q =
    (W_UNCONDITIONAL * unconditionality(hero) + W_SELECTIVE * selectivity(hero)) /
    (W_UNCONDITIONAL + W_SELECTIVE);
  return rate(q, PRECISION_ANCHOR);
}
