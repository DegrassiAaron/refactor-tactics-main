/** La rubrica: infrastruttura e regole comuni a ogni asse (#557).
 *
 *  Qui vivono la scala, le ancore e il peso del cooldown — cio' che vale per **tutti** gli assi.
 *  Le formule dei singoli assi stanno altrove (#562, #603): questo modulo non sa cosa sia `offense`. */

/** Denominatore comune dei pesi di disponibilita'. `10/(1+CD/2)` == `20/(2+CD)`, e i denominatori
 *  possibili sono {2,3,4,5}: il loro mcm e' 60, quindi ogni peso e' un intero esatto. */
const AVAILABILITY_LCM = 60;

/** Cooldown massimo dichiarato dal catalogo eroi. Oltre, il peso non e' definito. */
const MAX_COOLDOWN = 3;

/** Peso intero della disponibilita' di un'azione, dato il suo cooldown.
 *
 *  Il valore e' `AVAILABILITY_LCM / (2 + CD)`, cioe' la forma intera di `10/(1+CD/2)` fissata in
 *  #557. Restare interi non e' pedanteria: il gate `--check` di D-108 confronta artefatti byte a
 *  byte, e un arrotondamento in virgola mobile lo renderebbe rosso su una macchina e verde su
 *  un'altra. */
export function availabilityWeight(cooldown: number): number {
  if (!Number.isInteger(cooldown) || cooldown < 0 || cooldown > MAX_COOLDOWN) {
    throw new Error(
      `cooldown ${cooldown} fuori scala: il catalogo dichiara 0..${MAX_COOLDOWN}. ` +
        `Un cooldown nuovo richiede di estendere la rubrica, non di inventargli un peso.`,
    );
  }
  return AVAILABILITY_LCM / (2 + cooldown);
}

/** Porta un `raw` sulla scala `1..10` rispetto a un'**ancora assoluta**.
 *
 *  `raw == 0` vale `1`, `raw == anchor` vale `10`, e oltre l'ancora si satura. L'ancora non dipende
 *  dal roster (D-112): un eroe nuovo non deve spostare i rating di quelli che esistono gia', che e'
 *  esattamente cio' che una normalizzazione min-max farebbe. */
export function rate(raw: number, anchor: number): number {
  if (anchor <= 0) throw new Error(`ancora ${anchor} non positiva: la scala sarebbe degenere`);
  const scaled = 1 + Math.round((9 * raw) / anchor);
  return Math.max(1, Math.min(10, scaled));
}

/** Un asse che la rubrica non sa produrre. **Non e' zero** e non lo diventa mai (owner §6):
 *  disegnare `0` produrrebbe lo stesso rientro di una debolezza voluta, cioe' una forma che accusa
 *  l'eroe di una carenza che nessuno ha misurato. */
export const TBD = Symbol('TBD');
export type AxisValue = number | typeof TBD;

export interface ResolvedAxes {
  values: Record<string, AxisValue>;
  complete: boolean;
  /** Gli assi `TBD`, in ordine di dichiarazione. */
  missing: string[];
  /** I valori se sono tutti noti; altrimenti fallisce nominando l'eroe e gli assi mancanti. */
  orThrow(hero: string): Record<string, number>;
}

/** Raccoglie gli assi calcolati e decide se il radar e' generabile.
 *
 *  La regola che costa qualcosa e' qui: **un solo asse `TBD` impedisce l'intero radar**. Non esiste
 *  un radar «quasi completo», perche' una forma incompleta e' indistinguibile da una forma vera. */
export function resolveAxes(values: Record<string, AxisValue>): ResolvedAxes {
  const missing = Object.keys(values).filter((k) => values[k] === TBD);

  return {
    values,
    complete: missing.length === 0,
    missing,
    orThrow(hero: string): Record<string, number> {
      if (missing.length > 0) {
        throw new Error(
          `${hero}: radar non generabile, assi senza valore: ${missing.join(', ')}. ` +
            `Un asse TBD non si disegna come 0 (owner §6).`,
        );
      }
      return values as Record<string, number>;
    },
  };
}
