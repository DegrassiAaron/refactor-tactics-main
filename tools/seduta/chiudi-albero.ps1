<#
.SYNOPSIS
  Chiude un processo E I SUOI FIGLI, in Windows PowerShell 5.1 come in PowerShell 7 (#3862).

.DESCRIPTION
  `giro-meccanico.ps1` chiudeva il gioco con `try { $p.Kill($true) } catch {}`. `Process.Kill(bool)` esiste
  solo in .NET Core 3+ (PowerShell 7): in Windows PowerShell 5.1 il metodo non ha quell'overload, l'eccezione
  era inghiottita dal `catch {}` e `RefactorTactics.exe` restava vivo a consumare CPU. Questo helper usa
  `taskkill /T /F`, che c'e' su entrambe le edizioni e chiude anche i figli.

  Non chiude "a occhi chiusi": prima verifica sulla RIGA DI COMANDO del processo che porti il marcatore atteso
  (nello script, il `-abslog` della propria run). Un PID si riusa, e Unreal e' uno per macchina: chiudere il
  processo di un'altra sessione e' peggio che lasciare vivo il proprio.

  Ritorna `$true` se non resta niente da chiudere, `$false` se qualcosa NON si e' chiuso o non e' verificabile
  che sia il proprio. Non inghiotte l'esito: chi chiama decide che cosa farne, e il helper dice il perche'.
#>
function ChiudiAlbero {
  param(
    [Parameter(Mandatory)] $Processo,
    [Parameter(Mandatory)] [string] $CommandLineContiene
  )

  # Il nome dell'eseguibile, preso PRIMA di tutto: a processo uscito non si legge piu' dall'oggetto.
  $nome = $null
  try { $nome = [IO.Path]::GetFileName([string]$Processo.StartInfo.FileName) } catch {}

  $Processo.Refresh()
  if (-not $Processo.HasExited) {
    $cim = Get-CimInstance Win32_Process -Filter ('ProcessId = ' + $Processo.Id) -ErrorAction SilentlyContinue
    if (-not $cim) {
      # WMI non risponde, o il processo e' uscito fra un controllo e l'altro: lo stato lo dice .NET, non WMI.
      $Processo.Refresh()
      if (-not $Processo.HasExited) {
        Write-Host ('[STOP] non riesco a leggere il processo {0} (WMI): non posso verificare che sia il proprio. Chiudilo a mano.' -f $Processo.Id)
        return $false
      }
    }
    else {
      if ([string]::IsNullOrEmpty($cim.CommandLine)) {
        Write-Host ('[STOP] la riga di comando del processo {0} e'' illeggibile (accesso negato?): non posso verificare che sia il proprio. Chiudilo a mano.' -f $Processo.Id)
        return $false
      }
      if ($cim.CommandLine.IndexOf($CommandLineContiene, [StringComparison]::OrdinalIgnoreCase) -lt 0) {
        Write-Host ('[STOP] non chiudo il processo {0}: la sua riga di comando non porta il marcatore atteso. Se e'' il tuo, chiudilo a mano.' -f $Processo.Id)
        return $false
      }

      # Nessun `2>&1`: in Windows PowerShell 5.1, con `$ErrorActionPreference = 'Stop'`, l'output su stderr di un
      # comando nativo rediretto diventa un errore terminante. L'esito si legge dallo STATO del processo.
      & taskkill.exe /PID $Processo.Id /T /F | Out-Null

      if (-not $Processo.WaitForExit(10000)) {
        Write-Host ('[STOP] il processo {0} e'' ancora vivo dopo taskkill. Chiudilo a mano.' -f $Processo.Id)
        return $false
      }
    }
  }

  # Gli ORFANI: `taskkill /T` raggiunge solo i discendenti con un padre ancora vivo. Se il padre e' uscito e un figlio
  # no, chiudere "il processo" ritornerebbe subito con il gioco ancora in esecuzione -- il difetto di #3862.
  #
  # ⛔ **Solo lo STESSO eseguibile**, non "tutto cio' che cita il marcatore": il marcatore e' il percorso del log
  # della run, e lo cita anche una shell che lo legge, un editor aperto sul log, una query di diagnostica. Quelli non
  # sono il gioco, e chiuderli sarebbe peggio del difetto. Senza il nome dell'eseguibile non si cerca.
  if ($nome) {
    $cercaOrfani = {
      @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object {
        $_.ProcessId -ne $PID -and $_.Name -ieq $nome -and $_.CommandLine -and
        $_.CommandLine.IndexOf($CommandLineContiene, [StringComparison]::OrdinalIgnoreCase) -ge 0 })
    }
    $orfani = & $cercaOrfani
    foreach ($o in $orfani) { & taskkill.exe /PID $o.ProcessId /T /F | Out-Null }
    if ($orfani.Count -gt 0) {
      Start-Sleep -Milliseconds 500
      $ancora = & $cercaOrfani
      if ($ancora.Count -gt 0) {
        Write-Host ('[STOP] resta vivo un processo {0} con il marcatore della run (PID {1}). Chiudilo a mano.' -f $nome, $ancora[0].ProcessId)
        return $false
      }
    }
  }
  return $true
}
