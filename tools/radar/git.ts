/** `git` per i gate di `tools/radar/`, in un posto solo.
 *
 *  Sta qui e non dentro un gate per la ragione che `docs-corpus.ts` documenta per il corpus dei
 *  documenti: due definizioni identiche dello stesso helper **divergono in silenzio**. Chi indurisse
 *  la chiamata in un gate — un timeout, uno `stdio`, una variabile d'ambiente — lascerebbe l'altro
 *  com'era, e nulla lo direbbe.
 *
 *  Le tre durezze non sono estetiche, e ognuna chiude un modo di fallire misurato:
 *
 *   - **`stdio` con stderr ignorato.** Senza, `git` scrive i propri `fatal:` direttamente sullo stderr
 *     del gate, **prima** del referto: offline, un gate che voleva dichiarare `NOT RUN` con garbo
 *     sembra invece andato in crash.
 *   - **`GIT_TERMINAL_PROMPT=0`.** Un comando di rete su un repository privato puo' fermarsi su una
 *     richiesta di credenziali. Su Windows, con un helper grafico, il gate resterebbe appeso a tempo
 *     indeterminato invece di dichiarare il caso offline.
 *   - **`timeout`.** La rete puo' non rispondere senza fallire. Un gate appeso non e' distinguibile da
 *     uno lento, e chi lo lancia lo interrompe: e' il modo in cui un gate smette di essere lanciato.
 *
 *  `maxBuffer` alzato perche' il Decision Log supera il megabyte e il default di Node va in `ENOBUFS`. */

import { execFileSync } from 'node:child_process';

import { REPO_ROOT } from './docs-corpus.ts';

export interface GitOpts {
  /** Millisecondi oltre i quali il comando si considera non risposto. Default: 30 s. */
  timeout?: number;
}

export function git(args: string[], opts: GitOpts = {}): string {
  return execFileSync('git', args, {
    cwd: REPO_ROOT,
    encoding: 'utf8',
    maxBuffer: 64 * 1024 * 1024,
    timeout: opts.timeout ?? 30_000,
    // stdin chiuso e stderr scartato: il gate racconta i propri fallimenti con le proprie parole.
    stdio: ['ignore', 'pipe', 'ignore'],
    env: { ...process.env, GIT_TERMINAL_PROMPT: '0' },
  });
}

/** Vero se il clone non ha la storia completa.
 *
 *  🔴 E' il falso verde piu' silenzioso dei gate che leggono la storia: su un clone shallow gli oggetti
 *  vecchi non ci sono, una fixture storica non si apre e un controllo sul passato non trova nulla —
 *  senza che niente diventi rosso. Si dichiara `NOT RUN`, col rimedio: `git fetch --unshallow`. */
export function isShallow(): boolean {
  try {
    return git(['rev-parse', '--is-shallow-repository']).trim() === 'true';
  } catch {
    return false;
  }
}
