/** I cinque assi del Balance Radar (owner §2.2): la vista interna, vicina alle matrici di tuning.
 *
 *  Tre di essi — `control`, `support`, `durability` — sono gli **stessi** del Profile: si riusano i
 *  valori gia' calcolati invece di riderivarli, o le due viste potrebbero divergere sullo stesso eroe. */
import type { HeroInput } from './parse-catalog.ts';
import { powerRating } from './power.ts';
import { precisionRating } from './precision.ts';
import { profileAxes } from './profile.ts';

export interface BalanceAxes {
  precision: number;
  power: number;
  control: number;
  support: number;
  durability: number;
}

export function balanceAxes(hero: HeroInput): BalanceAxes {
  const { control, support, durability } = profileAxes(hero);
  return { precision: precisionRating(hero), power: powerRating(hero), control, support, durability };
}
