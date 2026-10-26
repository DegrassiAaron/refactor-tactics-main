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

  Ritorna `$true` se il processo non c'e' piu' (anche perche' era gia' uscito), `$false` se NON l'ha chiuso --
  perche' la riga di comando non porta il marcatore, o perche' e' ancora vivo dopo `taskkill`. Non inghiotte
  l'esito: chi chiama decide che cosa farne.
#>
function ChiudiAlbero {
  param(
    [Parameter(Mandatory)] $Processo,
    [Parameter(Mandatory)] [string] $CommandLineContiene
  )

  $Processo.Refresh()
  if ($Processo.HasExited) { return $true }

  $cim = Get-CimInstance Win32_Process -Filter ('ProcessId = ' + $Processo.Id) -ErrorAction SilentlyContinue
  if (-not $cim) { return $true }   # uscito fra un controllo e l'altro

  if ([string]::IsNullOrEmpty($cim.CommandLine) -or
      $cim.CommandLine.IndexOf($CommandLineContiene, [StringComparison]::OrdinalIgnoreCase) -lt 0) {
    Write-Host ('[STOP] non chiudo il processo {0}: la sua riga di comando non porta il marcatore atteso.' -f $Processo.Id)
    return $false
  }

  # Nessun `2>&1`: in Windows PowerShell 5.1, con `$ErrorActionPreference = 'Stop'`, l'output su stderr di un
  # comando nativo rediretto diventa un errore terminante. L'esito si legge dallo STATO del processo.
  & taskkill.exe /PID $Processo.Id /T /F | Out-Null

  if (-not $Processo.WaitForExit(10000)) {
    Write-Host ('[STOP] il processo {0} e'' ancora vivo dopo taskkill.' -f $Processo.Id)
    return $false
  }
  return $true
}
