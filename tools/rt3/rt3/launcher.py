"""Aprire la finestra: scelta della shell, script di avvio, spawn.

Vive nel CLIENT e non nel daemon. Il daemon e' un processo staccato senza console: una
finestra che nascesse da li' non apparterrebbe alla sessione interattiva in cui la
persona sta guardando. Il control plane possiede il REGISTRO e l'identita' dei processi
- cioe' cio' che decide se qualcosa si puo' chiudere - e questo basta a non avere un
secondo coordinator.

⛔ Nessun comando viene costruito concatenando stringhe. Gli argomenti passano per
`Popen` come lista, e i valori che finiscono nello script sono citati con le regole di
PowerShell. Un path con apostrofi o parentesi deve arrivare intatto, non diventare un
comando diverso.
"""

import os
import shutil
import subprocess

#: 🔴 `pwsh` PRIMA di `powershell`, e l'ordine non e' una preferenza estetica.
#:
#: Gli script RT del repository usano sintassi PowerShell 7. Windows PowerShell 5.1 non
#: li parsa: misurato il 2026-09-07 su `scripts/rt-suite.ps1` - **30 errori** con il
#: parser 5.1, **zero** con il 7.6.5. E `rt-lease.ps1` carica quello script come engine
#: guard, quindi da una finestra 5.1 ogni build muore con `ENGINE_GUARD_UNAVAILABLE`
#: prima di cominciare.
#:
#: La prima stesura sceglieva `powershell` per primo, ragionando che su Windows c'e'
#: sempre. E' vero, e irrilevante: una finestra che non puo' compilare non e' una
#: finestra di lavoro. `pwsh` resta non assunto - se manca si ripiega, e il fallback e'
#: il motivo per cui la lista ha ancora due voci.
SHELL_PREFERITE = ("pwsh", "powershell")


def resolve_shell(preferita=None):
    """`(nome, path)` della shell da usare. Solleva se non ce n'e' nessuna.

    ⚠️ Deterministico: la stessa macchina deve dare sempre la stessa risposta, altrimenti
    due finestre della stessa wave girerebbero su due shell diverse senza che nessuno
    l'abbia deciso.
    """
    candidati = [preferita] if preferita else list(SHELL_PREFERITE)
    for nome in candidati:
        if not nome:
            continue
        exe = shutil.which(nome) or shutil.which(nome + ".exe")
        if exe:
            return nome, exe
    from .errors import TerminalSpawnFailed

    raise TerminalSpawnFailed(
        "nessuna shell disponibile fra {}. Su Windows `powershell` fa parte del "
        "sistema: se manca anche quello, il PATH e' rotto.".format(
            ", ".join(SHELL_PREFERITE))
    )


def ps_quote(valore):
    """Cita per PowerShell in singoli apici, raddoppiando gli apici interni.

    E' la regola completa: dentro '...' PowerShell non interpreta niente, tranne '' che
    e' un apice letterale. Un path come `D:\\Progetti\\O'Brien (test)\\x` sopravvive.
    """
    return "'" + str(valore if valore is not None else "").replace("'", "''") + "'"


#: Lo script che la nuova finestra esegue. Fa quattro cose e nessuna di piu': dice a
#: `rt3` chi e', imposta il titolo, ridefinisce il prompt e stampa lo status iniziale.
#:
#: 🔴 Il tempo trascorso si calcola QUI, in locale, da `StartedAt`. Il prompt non
#: interroga RT3 a ogni Enter: una roadmap ricalcolata a ogni invio renderebbe la shell
#: lenta proprio mentre si lavora.
#:
#: ⛔ Nessun agente viene avviato: niente `claude`, niente `codex`. La finestra si apre
#: pronta, e chi la usa decide cosa eseguirci.
_SCRIPT = """# Generato da RT3 - non modificare a mano: viene riscritto a ogni launch.
$ErrorActionPreference = 'Continue'
$env:RT3_SESSION_ID = {session_id}
$env:PYTHONPATH = {pythonpath}
$env:PYTHONIOENCODING = 'utf-8'
{rt3_home}
$Global:RT3SessionId = {session_id}
$Global:RT3TerminalId = {terminal_id}
$Global:RT3StartedAt = [DateTime]::Parse({started_at}).ToUniversalTime()
$Global:RT3TitleBase = {title_base}
$Global:RT3Python = {python}
$Global:RT3Package = {package}

function Global:RT3-Elapsed {{
    $d = [DateTime]::UtcNow - $Global:RT3StartedAt
    if ($d.Ticks -lt 0) {{ $d = [TimeSpan]::Zero }}
    if ($d.Days -gt 0) {{
        return ('{{0}}d {{1:00}}:{{2:00}}:{{3:00}}' -f $d.Days, $d.Hours, $d.Minutes, $d.Seconds)
    }}
    return ('{{0:00}}:{{1:00}}:{{2:00}}' -f [int]$d.TotalHours, $d.Minutes, $d.Seconds)
}}

function Global:RT3-Short {{
    $d = [DateTime]::UtcNow - $Global:RT3StartedAt
    if ($d.Ticks -lt 0) {{ $d = [TimeSpan]::Zero }}
    return ('+{{0:00}}:{{1:00}}' -f [int]$d.TotalHours, $d.Minutes)
}}

function Global:RT3-Title {{
    $Host.UI.RawUI.WindowTitle = "$Global:RT3TitleBase | $(RT3-Short)"
}}

# `rt3` senza dover ricordare il PYTHONPATH: e' la stessa installazione che ha aperto
# questa finestra, non quella del checkout in cui ci si trova.
function Global:rt3 {{
    & $Global:RT3Python -m rt3 @args
}}

# L'uscita PULITA: ferma la sessione, rilascia i lease, stampa il riepilogo e chiude.
# E' il percorso preferito rispetto alla X della finestra, che RT3 puo' solo constatare.
function Global:rt3-finish {{
    & $Global:RT3Python -m rt3 --session $Global:RT3SessionId terminal finish
    exit
}}
Set-Alias -Name Stop-RT3Session -Value Global:rt3-finish -Scope Global

function Global:prompt {{
    RT3-Title
    $p = "[$Global:RT3SessionId]{prompt_extra}[$(RT3-Short)]"
    Write-Host $p -ForegroundColor DarkCyan
    "PS $($executionContext.SessionState.Path.CurrentLocation)$('>' * ($nestedPromptLevel + 1)) "
}}

RT3-Title
Write-Host ''
Write-Host '[RT3 SESSION READY]' -ForegroundColor Green
Write-Host ''
& $Global:RT3Python -m rt3 --session $Global:RT3SessionId status {status_args}
Write-Host ''
Write-Host ('StartedAt: ' + $Global:RT3StartedAt.ToString('yyyy-MM-dd HH:mm:ss') + 'Z')
Write-Host ('Elapsed:   ' + (RT3-Elapsed))
Write-Host ''
Write-Host 'Chiudi con `rt3-finish` (ferma la sessione, rilascia i lease e chiude).' -ForegroundColor DarkGray
Write-Host ''
"""


def render_script(session, terminal_id, title_base, python_executable, package_root,
                  started_at, roadmap_id=None, rt3_home=None):
    """Il testo dello script di avvio. Funzione pura: si puo' ispezionare in un test.

    ⚠️ `rt3_home` si propaga quando c'e'. Senza, la finestra parlerebbe con il control
    plane di default mentre chi l'ha aperta ne usava un altro: due store diversi che si
    credono lo stesso, che e' il modo piu' rapido per rendere incomprensibile uno smoke.
    """
    extra = ""
    if session.get("task_id"):
        extra += "[" + str(session["task_id"]) + "]"
    if session.get("write_mode") == "WRITER":
        extra += "[WRITER]"
    status_args = "--id " + ps_quote(roadmap_id) if roadmap_id else ""
    home = "$env:RT3_HOME = " + ps_quote(rt3_home) if rt3_home else ""
    return _SCRIPT.format(
        rt3_home=home,
        session_id=ps_quote(session.get("session_id")),
        terminal_id=ps_quote(terminal_id),
        pythonpath=ps_quote(package_root),
        python=ps_quote(python_executable),
        package=ps_quote(package_root),
        started_at=ps_quote(started_at),
        title_base=ps_quote(title_base),
        prompt_extra=extra,
        status_args=status_args,
    )


def write_script(path, testo):
    directory = os.path.dirname(os.path.abspath(path))
    if directory:
        os.makedirs(directory, exist_ok=True)
    # UTF-8 con BOM: PowerShell 5.1 legge come ANSI un file senza BOM, e i caratteri
    # non ASCII del titolo diventerebbero illeggibili.
    with open(path, "wb") as handle:
        handle.write(b"\xef\xbb\xbf" + testo.replace("\n", "\r\n").encode("utf-8"))
    return path


def spawn(shell_exe, script_path, cwd, env=None):
    """Apre la finestra. Ritorna il `Popen`.

    `CREATE_NEW_CONSOLE`: serve una finestra VISIBILE, quindi non `DETACHED_PROCESS`
    come per `rt3d`, che invece non deve averne nessuna.
    """
    argv = [
        shell_exe,
        "-NoExit",
        "-ExecutionPolicy", "Bypass",
        "-NoProfile",
        "-File", script_path,
    ]
    creationflags = 0
    if os.name == "nt":
        CREATE_NEW_CONSOLE = 0x00000010
        creationflags = CREATE_NEW_CONSOLE
    return subprocess.Popen(
        argv,
        cwd=cwd,
        env=env or dict(os.environ),
        creationflags=creationflags,
        close_fds=True,
    )
