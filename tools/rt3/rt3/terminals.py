"""Terminali che RT3 possiede: identita' del processo, titolo, tempo trascorso.

Modulo quasi PURO. L'unica cosa che tocca il sistema e' leggere l'istante di avvio di un
processo, e serve proprio a non ucciderne uno sbagliato.

## Perche' il PID non basta

🔴 Windows riusa i PID. Un numero che ieri era la nostra PowerShell oggi puo' essere
l'editor di qualcun altro, e un `taskkill` su quel numero chiuderebbe il lavoro di una
persona. L'identita' di un processo qui e' **la coppia**:

    ProcessId + ProcessStartedAt

Se l'istante di avvio non coincide con quello registrato, quel processo NON e' il nostro:
il terminale diventa `LOST` e non si termina niente.

⛔ Non si chiude mai un processo cercato per titolo della finestra, per nome
(`powershell.exe`) o per SessionId. Sono tutti attributi che un processo estraneo puo'
avere per caso o per scelta di qualcun altro.
"""

import os
import subprocess

#: Chi possiede il ciclo di vita di una finestra.
#:
#: 🔴 La distinzione che impedisce a RT3 di chiudere il terminale di una persona. Una
#: PowerShell aperta a mano, dentro cui qualcuno esegue `rt3 session start`, resta sua:
#: RT3 la vede, la usa, e non la chiude mai.
TERMINAL_OWNERSHIP = ("RT3_MANAGED",)

#: Stato di un terminale gestito.
#:
#:     STARTING  il processo e' stato chiesto, non ancora confermato
#:     ACTIVE    esiste, ed e' identificato senza ambiguita'
#:     CLOSING   gli e' stato chiesto di chiudersi
#:     CLOSED    e' finito, e sappiamo com'e' finito
#:     LOST      non e' piu' identificabile: non si tocca
TERMINAL_STATES = ("STARTING", "ACTIVE", "CLOSING", "CLOSED", "LOST")

#: Shell supportate. `pwsh` non si assume presente: si sceglie solo se c'e' davvero.
SHELL_TYPES = ("powershell", "pwsh")


def check_terminal_state(value):
    if value not in TERMINAL_STATES:
        raise ValueError(
            "stato di terminale non valido: {!r}. Ammessi: {}.".format(
                value, ", ".join(TERMINAL_STATES)
            )
        )
    return value


# ---------------------------------------------------------------------------
# Identita' di processo
# ---------------------------------------------------------------------------


def process_started_at(pid):
    """L'istante di avvio del processo, o `None` se non esiste / non e' leggibile.

    ⚠️ `None` significa «non lo so», e chi chiama deve trattarlo come «non identificato».
    Non significa «il processo non c'e'»: un processo di un altro utente puo' esistere ed
    essere illeggibile, e ucciderlo sarebbe peggio che lasciarlo vivere.
    """
    if not pid:
        return None
    if os.name == "nt":
        return _started_at_windows(int(pid))
    return _started_at_posix(int(pid))


def _started_at_windows(pid):
    """Istante di avvio via Win32, senza dipendenze esterne.

    `PROCESS_QUERY_LIMITED_INFORMATION` e non `PROCESS_QUERY_INFORMATION`: serve solo a
    leggere, e chiedere di piu' fallirebbe su processi che pure potremmo interrogare.
    """
    import ctypes
    from ctypes import wintypes

    PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.OpenProcess.restype = wintypes.HANDLE
    kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]

    handle = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not handle:
        return None
    try:
        # 🔴 `OpenProcess` che riesce NON significa «il processo e' vivo». Windows tiene
        # in piedi l'oggetto processo finche' resta un handle aperto, quindi un processo
        # gia' terminato resta interrogabile: misurato nello smoke, dove RT3 dichiarava
        # «il processo resiste» mentre la finestra era sparita da un pezzo.
        #
        # `GetExitCodeProcess` distingue i due casi con i diritti che gia' abbiamo.
        # `WaitForSingleObject` sarebbe piu' netto ma vuole anche SYNCHRONIZE, e
        # chiedere piu' diritti del necessario fa fallire l'apertura su processi che
        # pure potremmo interrogare. `STILL_ACTIVE` (259) e' teoricamente ambiguo con un
        # exit code 259: per la domanda «e' vivo?» l'ambiguita' e' accettabile, e la
        # lettura sbagliata sarebbe conservativa (lo crederemmo vivo, e non lo tocchiamo).
        STILL_ACTIVE = 259
        codice = wintypes.DWORD()
        if kernel32.GetExitCodeProcess(handle, ctypes.byref(codice)):
            if codice.value != STILL_ACTIVE:
                return None
        creation = wintypes.FILETIME()
        exit_t = wintypes.FILETIME()
        kernel_t = wintypes.FILETIME()
        user_t = wintypes.FILETIME()
        ok = kernel32.GetProcessTimes(
            handle, ctypes.byref(creation), ctypes.byref(exit_t),
            ctypes.byref(kernel_t), ctypes.byref(user_t),
        )
        if not ok:
            return None
        # FILETIME: 100 ns dal 1601-01-01. Si converte in ISO-8601 UTC al secondo, la
        # stessa risoluzione di ogni altro timestamp RT3.
        ticks = (creation.dwHighDateTime << 32) | creation.dwLowDateTime
        if ticks == 0:
            return None
        import datetime

        epoch = datetime.datetime(1601, 1, 1, tzinfo=datetime.timezone.utc)
        istante = epoch + datetime.timedelta(microseconds=ticks // 10)
        return istante.replace(microsecond=0).isoformat().replace("+00:00", "Z")
    finally:
        kernel32.CloseHandle(handle)


def _started_at_posix(pid):
    """Su POSIX basta `ps`. Non e' la piattaforma di riferimento di RT3, ma il modulo
    resta testabile fuori da Windows."""
    try:
        out = subprocess.run(
            ["ps", "-o", "lstart=", "-p", str(pid)],
            capture_output=True, text=True, timeout=5,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    testo = (out.stdout or "").strip()
    return testo or None


def identity_matches(registrato, osservato):
    """Il processo osservato e' QUELLO che avevamo registrato?

    🔴 Il valore di ritorno e' a tre stati e non due, e la differenza conta:

        True    e' lui: si puo' chiudere
        False   e' un altro processo con lo stesso PID: NON si tocca
        None    non identificabile: NON si tocca

    Trattare `None` come `True` significherebbe uccidere un processo di cui non sappiamo
    nulla. Trattarlo come `False` e' invece corretto qui: si rinuncia a chiudere.
    """
    if registrato is None or osservato is None:
        return None
    return registrato == osservato


def verify(terminal, ora_di_avvio=None):
    """Lo stato REALE di un terminale registrato, verificato contro il sistema.

    Ritorna `(stato, motivo)`. Non muta niente: e' una lettura.
    """
    stato = terminal.get("state")
    if stato in ("CLOSED", "LOST"):
        return stato, None
    pid = terminal.get("process_id")
    if not pid:
        return "LOST", "nessun PID registrato"
    osservato = ora_di_avvio if ora_di_avvio is not None else process_started_at(pid)
    if osservato is None:
        return "LOST", "il processo {} non esiste piu'".format(pid)
    esito = identity_matches(terminal.get("process_started_at"), osservato)
    if esito is False:
        return "LOST", (
            "il PID {} e' stato riusato: avviato {}, registrato {}".format(
                pid, osservato, terminal.get("process_started_at")
            )
        )
    if esito is None:
        return "LOST", "identita' del processo {} non verificabile".format(pid)
    return "ACTIVE", None


# ---------------------------------------------------------------------------
# Presentazione
# ---------------------------------------------------------------------------


def elapsed_ms(started_at, now):
    """Millisecondi fra due istanti ISO-8601. `None` se uno dei due manca o non e' valido.

    🔴 La durata NON si persiste mai: si ricava. Salvarla creerebbe due verita' - gli
    istanti e il loro intervallo - di cui la seconda invecchia a ogni secondo.
    """
    a, b = _parse_iso(started_at), _parse_iso(now)
    if a is None or b is None:
        return None
    delta = int((b - a).total_seconds() * 1000)
    return max(0, delta)


def _parse_iso(valore):
    if not valore:
        return None
    import datetime

    testo = str(valore).strip()
    if testo.endswith("Z"):
        testo = testo[:-1] + "+00:00"
    try:
        return datetime.datetime.fromisoformat(testo)
    except ValueError:
        return None


def format_elapsed(ms):
    """`00:37:26`, oppure `1d 03:17:09` oltre le ventiquattr'ore.

    ⚠️ Solo per gli occhi. Il valore leggibile da una macchina resta `elapsedMs`.
    """
    if ms is None:
        return "--:--:--"
    secondi = max(0, int(ms) // 1000)
    giorni, resto = divmod(secondi, 86400)
    ore, resto = divmod(resto, 3600)
    minuti, sec = divmod(resto, 60)
    if giorni:
        return "{}d {:02d}:{:02d}:{:02d}".format(giorni, ore, minuti, sec)
    return "{:02d}:{:02d}:{:02d}".format(ore, minuti, sec)


def format_short_elapsed(ms):
    """`+00:37` - la forma compatta per il titolo di finestra e il prompt."""
    if ms is None:
        return "+--:--"
    secondi = max(0, int(ms) // 1000)
    ore, resto = divmod(secondi, 3600)
    minuti = resto // 60
    return "+{:02d}:{:02d}".format(ore, minuti)


#: Caratteri che in un titolo di finestra non hanno senso e in un comando sarebbero
#: rumore. La sanificazione non e' difesa dall'injection - quella la da' l'argument list
#: di `Popen` - ma evita che un valore strano renda illeggibile la barra del titolo.
_TITOLO_VIETATI = "\r\n\t\"'`$;|&<>"


def sanitize_title_part(valore, massimo=48):
    if valore is None:
        return ""
    testo = "".join(" " if c in _TITOLO_VIETATI else c for c in str(valore))
    testo = " ".join(testo.split())
    return testo[:massimo]


def window_title(session, elapsed=None):
    """Il titolo della finestra, dal runtime reale.

    ⛔ Epic e task non si scrivono a mano da nessuna parte: vengono da qui, e se mancano
    il titolo lo dice invece di inventarli.

        RT3 | DEV-MAIN-1937-1 | 1936 | WRITER | +00:07
        RT3 | EDITOR-MAIN | EPIC-1937 | +00:18
        RT3 | VALIDATOR-1937 | WAITING | +00:12
    """
    ruolo = (session or {}).get("role")
    pezzi = ["RT3", sanitize_title_part((session or {}).get("session_id")) or "?"]
    task = sanitize_title_part((session or {}).get("task_id"))
    epic = sanitize_title_part((session or {}).get("epic_id"))
    if ruolo == "DEV":
        pezzi.append(task or "no task")
        pezzi.append(sanitize_title_part((session or {}).get("write_mode")) or "READ_ONLY")
    elif ruolo == "EDITOR":
        pezzi.append(epic or task or "no epic")
    else:
        pezzi.append(task or epic or "WAITING")
    pezzi.append(elapsed if elapsed is not None else format_short_elapsed(None))
    return " | ".join(p for p in pezzi if p)


# ---------------------------------------------------------------------------
# Chiusura
# ---------------------------------------------------------------------------


def terminate_process(pid, registrato, forza_dopo=3.0):
    """Chiude il processo SOLO se e' ancora quello registrato.

    ⛔ Nessuna ricerca per titolo, per nome o per SessionId. Il PID da solo non basta:
    prima di toccare qualcosa si riverifica l'istante di avvio, perche' un PID riusato
    appartiene a un programma di qualcun altro.

    Prima si chiede, poi si insiste. `taskkill` senza `/F` manda `WM_CLOSE` alle finestre
    del processo, che e' l'uscita pulita di una console; `/F` arriva solo se dopo qualche
    secondo il processo e' ancora li'.
    """
    if not pid:
        return {"terminated": False, "detail": "nessun PID registrato"}
    osservato = process_started_at(pid)
    esito = identity_matches(registrato, osservato)
    if esito is not True:
        return {
            "terminated": False,
            "detail": (
                "identita' non confermata per il PID {}: registrato {}, osservato {}. "
                "Non terminato.".format(pid, registrato, osservato)
            ),
        }

    import time

    if os.name == "nt":
        _run(["taskkill", "/PID", str(int(pid))])
    else:
        try:
            os.kill(int(pid), 15)
        except OSError:
            pass

    scadenza = time.time() + forza_dopo
    while time.time() < scadenza:
        if process_started_at(pid) is None:
            return {"terminated": True, "detail": "chiuso"}
        time.sleep(0.2)

    # Ancora vivo: si insiste, ma solo dopo aver RIVERIFICATO che sia sempre lui.
    if identity_matches(registrato, process_started_at(pid)) is not True:
        return {"terminated": False, "detail": "identita' cambiata durante la chiusura"}
    if os.name == "nt":
        _run(["taskkill", "/PID", str(int(pid)), "/T", "/F"])
    else:
        try:
            os.kill(int(pid), 9)
        except OSError:
            pass

    # ⚠️ Anche dopo `/F` la morte non e' istantanea: per qualche istante `OpenProcess`
    # riesce ancora su un processo che sta terminando. Una lettura sola qui diceva «il
    # processo resiste» mentre la finestra era gia' sparita - misurato nello smoke - e
    # marcava il terminale LOST, generando un ACTION_REQUIRED per un problema inesistente.
    scadenza = time.time() + forza_dopo
    while time.time() < scadenza:
        if process_started_at(pid) is None:
            return {"terminated": True, "detail": "chiuso a forza"}
        time.sleep(0.2)
    return {"terminated": False, "detail": "il processo resiste"}


def _run(argv):
    """Sempre argument list, mai una stringa: il quoting di Windows e' un campo minato,
    e un path con apostrofi o parentesi diventerebbe un comando diverso."""
    try:
        return subprocess.run(argv, capture_output=True, text=True, timeout=15)
    except (OSError, subprocess.SubprocessError):
        return None
