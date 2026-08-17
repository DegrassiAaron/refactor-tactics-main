/** L'asse `power` del Balance Radar (#603): quanto danno l'eroe produce per turno, tenendo conto di
 *  quanto spesso puo' farlo. Non burst — un colpo enorme ogni tre turni non vale quanto uno ripetibile. */
import type { AbilityInput, HeroInput } from './parse-catalog.ts';
import { availabilityWeight, rate } from './rubric.ts';

/** Ancora assoluta: `raw 100` = «uccide un eroe medio in un turno». Non e' scelta a mano — la salute
 *  del roster e' 90/95/120/90, media 98.75, e `power_raw` e' gia' danno per turno. */
const POWER_ANCHOR = 100;

/** Denominatore dei pesi interi di `availabilityWeight`, piu' il fattore 10 con cui `power_raw` e'
 *  espresso in "danno per turno". Vedi `rubric.ts`. */
const WEIGHT_SCALE = 60 * 10;

/** Un payoff e' **condizionale** quando non arriva per il solo fatto di usare l'azione. Il catalogo
 *  lo dichiara in due sedi diverse, e la regola e' **una sola** (#557):
 *
 *  - nella cella `Effetto`, come bonus su uno stato — `+8 su bersaglio Wet`;
 *  - nella colonna `Tipo`, come azione **predittiva** — l'intero payoff di `InterceptShot` dipende
 *    dall'aver indovinato dove andra' l'avversario, quindi zero e' il danno che ci si puo' aspettare
 *    senza chiedere niente.
 *
 *  Trattarle come due meccanismi separati produrrebbe due strade che possono divergere.
 *
 *  ⚠️ **Fino al 2026-08-17 questa funzione faceva match su una FRASE IN PROSA** della tabella delle
 *  reazioni — `/trigger d'ingresso su movimento/` — e il difetto era che nessuno poteva saperlo: chi
 *  avesse riscritto quella cella per correggerne il contenuto (e c'era da correggerlo: dichiarava
 *  `InterceptShot` una reazione rinviata a E14, falso dal 2026-08-10) avrebbe **mosso un rating** senza
 *  toccare un numero. Ora legge la colonna `Tipo`, che e' un campo del catalogo con un vocabolario
 *  chiuso: correggere la prosa attorno non cambia piu' i radar. Vedi `#1080`. */
function isPredictive(ability: AbilityInput): boolean {
  return ability.kind === 'predittiva';
}

/** Il danno che l'azione produce **senza chiedere niente**. Zero quando l'intero payoff e' condizionale.
 *
 *  Il `damage` che arriva dal parser e' gia' il primo numero della cella, quindi il `+8` condizionale
 *  non vi rientra: quel bonus vale come **condizionalita'** in `precision`, mai come danno. */
export function guaranteedDamage(ability: AbilityInput): number {
  if (ability.damage === null) return 0;
  return isPredictive(ability) ? 0 : ability.damage;
}

/** Danno per turno dell'eroe. Aritmetica intera fino all'ultima divisione (vedi `rubric.ts`). */
export function powerRaw(hero: HeroInput): number {
  const scaled = hero.abilities.reduce(
    (sum, a) => sum + guaranteedDamage(a) * 20 * availabilityWeight(a.cooldown),
    0,
  );
  return scaled / WEIGHT_SCALE;
}

export function powerRating(hero: HeroInput): number {
  return rate(powerRaw(hero), POWER_ANCHOR);
}
