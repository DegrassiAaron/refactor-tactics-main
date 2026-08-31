# Asset map — quali asset servono, chi li produce, quali esistono

> `CURRENT` · **Creato**: 2026-08-13 · **Ultima misura**: 2026-08-31 (sopra `771eb9aa`) · **Owner**: questo file —
> è il **registro degli asset di contenuto** attesi dal progetto, release per release.
>
> **Cosa non è.** Non è l'owner di percorsi e naming: quello è
> [`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md), che è normativo e da cui questo file **deriva**
> ogni path. Non è l'owner dei principi di pipeline (presentazione-only, riferimenti soft con fallback,
> licenze): quello è [`spec-asset-pipeline.md`](../architecture/spec-asset-pipeline.md). Non è l'owner dello stato delle
> sedute in editor: quello è [`../../roadmap/editor-sessions.yaml`](../../roadmap/editor-sessions.yaml), reso in
> `../../roadmap/editormap.shortlist.md`.
>
> Nasce perché quelle tre fonti, insieme, **non rispondono a una domanda**: *quali asset servono e quanti ne
> mancano*. `convenzioni-contenuti-ue.md` §4 lo dichiara esplicitamente — «questo documento non è un
> tracker». Questo lo è.

---

## 1. Come si legge lo stato, e come si rimisura

Un asset ha tre stati possibili, e **due dei tre si misurano da soli**:

| Stato | Significa | Come si verifica |
|---|---|---|
| ✅ **committato** | è nel repository, chi clona lo ottiene | `git ls-files <path>` |
| 🟡 **su disco** | esiste nel progetto locale ma **non** è committato | esiste nel filesystem, non in `git ls-files` |
| ⏳ **assente** | non esiste ancora | nessuna delle due |

**La lista degli asset attesi non è un'opinione**: è l'allowlist di `.gitignore`. Il repository ignora tutto
`Content/**/*.uasset` e riammette per **path esplicito** ciò che deve entrare — quindi una riga `!Content/…`
è al tempo stesso il permesso e la dichiarazione d'intenti. Chi aggiunge un asset senza toccarla scopre che
`git add` non lo vede.

```bash
# stato di tutti gli asset attesi — si esegue dalla radice del repository
python - <<'PY'
import subprocess, os
root = subprocess.run(['git','rev-parse','--show-toplevel'],
                      capture_output=True, text=True).stdout.strip()
os.chdir(root)
allow = [l[1:].strip() for l in open('.gitignore', encoding='utf-8').read().splitlines()
         if l.startswith('!Content/') and not l.rstrip().endswith('/')]
# le righe glob sono FAMIGLIE aperte, non path: contarle fra gli attesi produce falsi ASSENTE
concrete = sorted(a for a in allow if '*' not in a)
globs    = sorted(a for a in allow if '*' in a)
# -z: i path con spazi o accenti non si spezzano e git non li quota
tracked = set(subprocess.run(['git','ls-files','-z','Content'],
                             capture_output=True, text=True).stdout.split('\0')) - {''}
for a in concrete:
    print(('OK      ' if a in tracked else ('DISCO   ' if os.path.exists(a) else 'ASSENTE ')) + a)
print(f"\n{len(concrete)} attesi · {sum(a in tracked for a in concrete)} committati"
      f" · {len(globs)} righe glob (famiglie aperte, non path):")
for g in globs:
    print('    ' + g)
# seconda direzione: un binario tracciato che .gitignore NON riammette. L'oracolo è
# `git check-ignore`, lo stesso di §6: exit 0 = ignorato, cioè muto a `git add`.
binari = sorted(t for t in tracked if t.endswith(('.uasset', '.umap')))
muti = [b for b in binari
        if subprocess.run(['git','check-ignore','-q',b]).returncode == 0]
print(f"binari tracciati ({len(binari)}) FUORI allowlist: {len(muti)}")
for m in muti:
    print('  ' + m)
PY
```

⚠️ La riga `tracciati FUORI allowlist` non c'era nella prima stesura, e la sua assenza aveva prodotto
un'affermazione falsa (§2). Un comando che itera solo la lista attesa non può scoprire ciò che la
lista non prevede: le due direzioni si controllano separatamente.

🔴 **Il comando qui sopra è stato riscritto il 2026-08-28, e la stesura precedente aveva due difetti
di cui il primo è il peggiore.**

1. **Non era eseguibile.** L'`f-string` finale era spezzata su due righe senza virgolette triple:
   `SyntaxError: unterminated f-string literal` su Python 3.12, cioè prima di stampare qualunque cosa.
   Un documento che dice *«esegui questo per sapere com'è messo il repository»* spediva uno script morto,
   e nessuna delle misure qui sotto poteva esserne uscita.
2. **Trattava le righe glob come path concreti.** `.gitignore` ha **4** righe d'allowlist con `**`
   (`RT/UI/**/*.uasset`, `RT/Maps/**/*.umap`, `RT/Maps/**/*.uasset`, `RT/World/Graybox/**/*.uasset`):
   sono **famiglie**, non file, e nessuna sarà mai in `git ls-files`. Contate fra gli attesi producevano
   **4 falsi `ASSENTE`** e gonfiavano il totale da 24 a 28. Nella direzione opposta l'errore si
   specchiava: i 5 asset che **solo** quelle righe riammettono — `WBP_RT_MainMenu`, `WBP_RT_MenuEntry`,
   `WBP_RT_SettingsPanel`, `L_Frontend.umap`, `DA_HexMap_Scratch_Basin.uasset` — finivano fra i
   «tracciati FUORI allowlist», cioè accusati di essere muti proprio mentre l'allowlist li ammetteva.

⚠️ **La direzione inversa ora usa `git check-ignore` invece dell'appartenenza a un insieme**, ed è la
correzione che rende il controllo robusto ai glob futuri: è lo stesso oracolo che §6 già dichiara, e
l'unico che sa leggere `.gitignore` come git lo legge. Il confronto per stringa non lo saprà mai.

**Misurato il 2026-08-28** su `HEAD` `d0e35814`: **24 attesi · 20 committati · 0 su disco · 4 assenti**,
più **4 righe glob** che aprono altrettante famiglie. I quattro assenti erano gli `ABP_*` dei personaggi.

🔴 **Il perimetro è cambiato il 2026-08-30, e il numero con lui** ([D-248](../../decisions/RT_PDR_00_Decision_Log.md),
[#1720](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1720)): i quattro `ABP_*` **non sono più attesi**, perché il grafo di
locomozione vive in C++ e non in un `.uasset`. ⚠️ **Non è una rimisurazione**: è la stessa misura del 28-08
con quattro righe uscite dagli «attesi» — **20 attesi · 20 committati · 0 assenti**, ri-derivato dalla
tabella e non riletto da un `HEAD` nuovo. Al loro posto U8 ne porta **dodici**, i montaggi `AM_*`, che
nessuno aveva ancora contato: il totale atteso torna a salire alla prossima misura vera.

✅ **La misura vera è arrivata il 2026-08-31** su `HEAD` `bf9bdb4d`, e conferma il numero ri-derivato:
**20 attesi · 20 committati · 0 su disco · 0 assenti**, più le 4 righe glob. Nessun asset dell'allowlist
manca dal repository.

🔴 **Ma per arrivarci è servito togliere quattro righe dal `.gitignore`, e il perché vale più del cosa.**
Eseguito *prima* di quella pulizia, il comando qui sopra rispondeva **24 attesi · 4 assenti**: continuava
a contare gli `ABP_*` fra gli attesi e a dichiararli mancanti, cioè **quattro asset assenti che D-248
vieta di produrre**. La decisione aveva corretto nove documenti, e non aveva tolto le righe che li
dichiaravano — perché il suo oracolo è `grep -rn "ABP_" docs/`, e **`.gitignore` non sta in `docs/`**.
Una decisione che rimuove un asset atteso tocca *quattro* posti, non tre: le tre righe di §6 **e**
l'allowlist che le riammetteva. Finché la riga resta, la misura non può dare il numero giusto, e la
distanza fra il totale dichiarato e quello riproducibile è esattamente la riga dimenticata.

➕ **Poche ore dopo il totale è risalito a 32, ed è una buona notizia.** I **dodici montaggi `AM_*`** hanno
preso la loro riga d'allowlist il **2026-08-31**, sopra `771eb9aa`: **32 attesi · 20 committati · 0 su
disco · 12 assenti**. Il numero peggiora e il repository sta meglio — è la differenza fra un lavoro **non
tracciato** e un lavoro **tracciato e non fatto**, e solo il secondo si può contare. ⚠️ **Le righe sono
state scritte prima che il primo montaggio esista**, che è l'ordine di §6: questi dodici non possono
ripetere la storia di `ABP_Gadget`, dove la riga c'era e mancava il gesto. I path non sono trascritti a
mano — sono **verificati uguali** per insiemi agli `artifacts` di U8 in `editor-sessions.yaml`, perché una
lista attesa che esiste già in un'altra fonte si confronta, non si ricopia.

*(Misura precedente, 2026-08-17 su `a4a393b6`: 21 attesi · 16 committati · 0 su disco · 5 assenti. Prima
ancora, 2026-08-13 su `515c5c88`: 17 attesi · 13 committati · 4 mancanti.)*

⚠️ **I quattro committati in più dal 17 agosto sono tre `WBP_RT_*` e una mappa**, non «i cinque di U24 meno
uno» come diceva questa riga fino al 31-08: `FallbackBanner` ed `ErrorModal` erano già committati il 17, e
i nuovi sono `LoadingScreen`, `FrontendRoot`, `ModalLayer` più `DA_Format_Scratch`. L'allowlist nel
frattempo ha riammesso anche gli ultimi due — righe proprie in `.gitignore`, da cercare **per nome** — che
§2.1 dava ancora come famiglia senza riga, ed è la correzione fatta lì.

🔴 **Il 🟡 non è durato quattro giorni, e non è stato committato: è stato cancellato.** Il 13 agosto
`ABP_Gadget.uasset` esisteva sul disco di chi sviluppa e non era nel repository. Oggi non esiste più
da nessuna parte, e nessun commit lo ha mai contenuto — `git log` non lo conosce. Un `.uasset` mai
aggiunto all'indice è **untracked**, e `git clean -fd` — il comando che questo stesso documento
raccomanda in §5 per non cancellare i pack — lo porta via senza chiedere: `-e Content/FabAsset`
protegge i 48 GB di terze parti, non il file da 73 KB che stavi costruendo dentro `Content/RT/`.

Questo **rafforza** la ragione per cui la cifra che conta è quella dei committati, e la sposta: il 🟡
non è soltanto *inutile a chi clona*, è **volatile anche per chi sviluppa**. La riga d'allowlist era
già lì dal 2026-08-11, quindi `git add` avrebbe funzionato in qualsiasi momento: mancava solo il
gesto. **Il momento di committare un asset è quando lo si salva la prima volta**, non quando è finito
— un anim BP a metà, committato, si riprende; lo stesso file non committato è un lavoro che nessun
`git` può restituire. È il perimetro della seduta **U8**, che dopo la cancellazione riparte da zero.

⚠️ **Nessun `.uasset` o `.umap` sta fuori dall'allowlist, e i file non binari che ci stanno sono
140** — rimisurato il 2026-08-30 su `fff33020`: `Content/.gitkeep`, i **due** di
`RT_UI_AssetPack_FromHUD/` e i **137** di `Content/Icons/`, arrivati dopo il 17 agosto. Quei 137 sono
**134 SVG** più tre file che non lo sono — `LEGGIMI.md`, `manifest.json` e `rticonehud20260826.zip`.
Non sono `.uasset`: le regole di `.gitignore` che li lasciano passare sono altre.

🔴 **Questa riga diceva `145`, `137 SVG` e «più il manifest», e sbagliava tutti e tre.** Non è un numero
invecchiato: sul commit che l'ha scritta — `505e5234`, 2026-08-28 — l'albero rispondeva già **140** e
**134**, e `Content/Icons/` non è più stata toccata da allora (un solo commit la popola, `48e591ae`).
L'errore è di metodo ed è riconoscibile: `137` è il **totale** di `Content/Icons/`, riusato come se
fosse il conto dei soli SVG e poi sommato di nuovo al manifest — così il `.md` e lo `.zip` sono
scomparsi e il totale ha guadagnato cinque unità che nessun addendo produce. Contare una colonna e
descriverla come un'altra è lo stesso difetto che il paragrafo qui sotto registra a proposito di «Tre
file tracciati». Rimisurato consumando il §8 di
[`CLAUDE_FIX_PROJECT_CONTENT_STRUCTURE.md`](../../archive/src/CLAUDE_FIX_PROJECT_CONTENT_STRUCTURE.md),
che chiedeva esattamente questo: *«rimisura, non conservare conteggi vecchi»*.

🔴 **Questa riga diceva «Tre file tracciati», ed era vera il 2026-08-17.** Il numero è cresciuto di
due ordini di grandezza senza che nessuna regola cambiasse, il che dice qualcosa sul controllo e non sul
repository: contare **file** in questa direzione misura soprattutto quanti `.svg` sono entrati. Ciò che
il documento vuole sapere è *se un binario è muto a `git add`*, e la risposta — **0 su 25** — è quella
che il comando di §1 ora isola.

⚠️ Vale comunque la lezione originale: la prima stesura dichiarava «nessun asset è tracciato fuori
dall'allowlist» perché il controllo escludeva `.md`, `.json` e `.gitkeep` — aveva filtrato via
esattamente i casi che lo smentivano.

---

## 2. v0.1 — misurata

I 32 path che il repository dichiara di volere. La colonna **Seduta** dice chi lo produce, secondo
`editor-sessions.yaml`; `—` significa che nessuna seduta lo rivendica (esisteva prima che le sedute
fossero un dato).

| Asset (sotto `Content/RT/`) | Famiglia | Seduta | Stato |
|---|---|:--:|---|
| `Characters/Gadget/Blueprints/BP_Unit_Gadget.uasset` | Unità giocabile | **U7** | ✅ committato |
| `Characters/Phase/Blueprints/BP_Unit_Phase.uasset` | Unità giocabile | **U7** | ✅ committato |
| `Characters/Riktor/Blueprints/BP_Unit_Riktor.uasset` | Unità giocabile | **U7** | ✅ committato |
| `Characters/Wraith/Blueprints/BP_Unit_Wraith.uasset` | Unità giocabile | **U7** | ✅ committato |
| ~~`Characters/<Pack>/Animation/ABP_<Pack>.uasset`~~ ×4 | Animazione | ~~**U8**~~ | ⛔ **non più attesi dal 2026-08-30** ([D-248](../../decisions/RT_PDR_00_Decision_Log.md)): il grafo di locomozione vive in C++ (`URTUnitAnimInstance`) e nessun `.uasset` di animazione va creato. *Erano «⏳ assente», e `ABP_Gadget` era «su disco» il 13-08, cancellato senza essere mai committato (§1)* |
| `Characters/Gadget/Animation/AM_Gadget_Attack.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Gadget/Animation/AM_Gadget_Hit.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Gadget/Animation/AM_Gadget_Death.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Phase/Animation/AM_Phase_Attack.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Phase/Animation/AM_Phase_Hit.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Phase/Animation/AM_Phase_Death.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Riktor/Animation/AM_Riktor_Attack.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Riktor/Animation/AM_Riktor_Hit.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Riktor/Animation/AM_Riktor_Death.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Wraith/Animation/AM_Wraith_Attack.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Wraith/Animation/AM_Wraith_Hit.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Wraith/Animation/AM_Wraith_Death.uasset` | Animazione | **U8** | ⏳ assente |
| `Characters/Shared/Materials/M_SelectionRing.uasset` | Condiviso | — | ✅ committato |
| `Characters/Shared/Materials/M_TeamRing.uasset` | Condiviso | — | ✅ committato |
| `Maps/Dev/L_HexArena/L_HexArena.umap` | Mappa | **U1** | ✅ committato |
| `Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.uasset` | Mappa | **U1** | ✅ committato |
| `Maps/Dev/L_DevSandbox/L_DevSandbox.umap` | Mappa | — | ✅ committato |
| `Maps/Dev/L_DevSandbox/Data/DA_HexMap_Sandbox.uasset` | Mappa | — | ✅ committato |
| `Maps/Dev/L_Prototype/L_Prototype.umap` | Mappa | — | ✅ committato |
| `Core/Framework/BP_GameMode.uasset` | Framework | — | ✅ committato |
| `Core/Grid/M_HexCell.uasset` | Materiale griglia | — | ✅ committato |
| `Art/GlobalMaterials/M_Global_Tint.uasset` | Materiale globale | — | ✅ committato |
| `UI/Framework/WBP_RT_FallbackBanner.uasset` | Frontend | **U24** | ✅ committato |
| `UI/Framework/WBP_RT_ErrorModal.uasset` | Frontend | **U24** | ✅ committato |
| `UI/Framework/WBP_RT_LoadingScreen.uasset` | Frontend | **U24** | ✅ committato |
| `UI/Framework/WBP_RT_FrontendRoot.uasset` | Frontend | **U24** | ✅ committato |
| `UI/Framework/WBP_RT_ModalLayer.uasset` | Frontend | **U24** | ✅ committato |
| `Maps/Dev/L_DevSandbox/Data/DA_Format_Scratch.uasset` | Mappa | — | ✅ committato |

**Quel che resta della v0.1 è una famiglia sola, e dal 2026-08-31 è in questa tabella: i dodici montaggi
`AM_*`.** Tutto il resto è committato. Il **frontend** di U24 si è chiuso — tutti e cinque i `WBP_RT_*`
sono nel repository, `LoadingScreen`, `FrontendRoot` e `ModalLayer` compresi — e i quattro `ABP_*` sono
usciti dagli attesi con [D-248](../../decisions/RT_PDR_00_Decision_Log.md). Restano `Cast`, `Hit` e
`Death` per quattro eroi: lavoro reale di
[#288](https://github.com/DegrassiAaron/refactor-tactics-main/issues/288) (`PIE-AS4b`), che ora ha una riga
d'allowlist **scritta prima del primo asset** — la riga 1 di §6 nell'ordine giusto, per una volta senza che
sia costato prima un file perso.

⚠️ **`M_HexCell.uasset` non è rivendicato da nessuna seduta.** È in allowlist e committato, ma
`editor-sessions.yaml` non lo nomina: come i cinque path storici marcati `—`, esiste senza che una
seduta ne risponda. Non è un difetto da correggere qui — è un buco della fonte, e si chiude lì.

### 2.1 Famiglie attese che non hanno una riga d'allowlist

**Tre** cose che la v0.1 richiede e per cui **`git add` tace**, perché nessuna riga di `.gitignore` le
riammette. È l'unico predicato vero di tutte e tre: il *percorso* ce l'hanno in due (le icone lo hanno
deciso, i sorgenti icona esistono già sul disco), la *seduta* in una (U21). Non sono dimenticanze di
questo file: sono buchi delle fonti, e vanno chiusi lì.

✅ **Erano quattro per mezza giornata, il 2026-08-31.** I dodici montaggi `AM_*` sono entrati qui la
mattina — venivano da §2, dove non potevano stare perché §2 elenca l'allowlist e loro non c'erano — e ne
sono usciti il pomeriggio con la loro riga. Erano l'unica famiglia ad avere **percorso e seduta** insieme,
la stessa posizione dei due `WBP_RT_*` poco prima: è la posizione da cui si esce scrivendo la riga, e
l'unica in cui la lacuna si chiude in un minuto invece che in una seduta.

🔴 **Erano cinque fino al 2026-08-28, e due si sono chiuse — verificato, non deciso qui.**

- **`WBP_RT_FrontendRoot` e `WBP_RT_ModalLayer`** hanno la loro riga d'allowlist in `.gitignore` — **cercala
  per nome**: al 2026-08-31 stava tredici righe più in basso di dove questa frase la dava, e la frase
  sbagliata era proprio quella che avverte di non fidarsi dei numeri di riga.
  Entrambi risultano **committati**. ⚠️ La riga citava *«righe 124–126 del `.gitignore`»* per gli altri
  tre, che oggi stanno a **168–170**: i numeri di riga di un file che cresce non sono un ancoraggio, e
  qui hanno retto un giorno più del contenuto.
- **Il kit graybox degli oggetti** ha percorso e riga: [`D-173`](../../decisions/RT_PDR_00_Decision_Log.md)
  ha chiuso `GBX-4` il **2026-08-18** fissando `/Game/RT/World/Graybox/` con
  `Cover/ · Doors/ · Surfaces/ · Volumes/`, e la riga in `.gitignore` è il glob `RT/World/Graybox/**/*.uasset`
  — citato per **contenuto** e non per numero, che questa stessa modifica ha spostato di quattro
  (`!Content/RT/World/Graybox/**/*.uasset`, pattern di cartella — *e il numero era `192` fino al
  2026-08-30: i numeri di riga di un file che cresce non sono un ancoraggio, come questa stessa
  sezione dichiara tre righe più su*). Oracolo verificato il 2026-08-30:
  `git check-ignore -q Content/RT/World/Graybox/Volumes/BP_Graybox_CellPlacementVolume.uasset` esce
  **`1`**. ✅ **E il kit ora esiste**: `git ls-files 'Content/RT/World/Graybox/*'` dà **7** dal
  **2026-08-28** — le sei `SM_Graybox_*` di §8.1 più `BP_Graybox_CellPlacementVolume`. È la prima
  famiglia delle quattro a passare dalla riga d'allowlist a un file versionato.
  ➕ **Dal 2026-08-30 si aggiunge `Materials/`** ([#1714](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1714)):
  `M_Graybox_Master` più sei `MI_Graybox_*`, una per mesh. Il pattern d'allowlist è di **cartella** e li
  copre già senza una riga nuova — ed è il caso in cui quella scelta paga, perché una famiglia che cresce
  non richiede un secondo intervento su `.gitignore`.
  ⚠️ **Sono generati e assegnati dallo stesso commandlet delle mesh**, non autorati: un materiale
  assegnato a mano in Editor si perderebbe alla prima rigenerazione.
  🔑 **Le sei mesh sono GENERATE, non autorate** ([`D-229`](../../decisions/RT_PDR_00_Decision_Log.md)):
  le produce `UnrealEditor-Cmd … -run=RTBuildGrayboxMeshes`, che ha un `-DryRun`. Conta per questo
  documento perché cambia cosa si fa davanti a un conflitto su uno di quei binari: **si rigenera**,
  non si fonde. La sorgente è il commandlet, l'`.uasset` è il suo output.
  ⚠️ **Esistere non è essere verificato**: le cinque voci `PIE-GBX-*` restano ⏳, e ora aspettano
  la seduta **U25** invece di un asset.

La terza riga rimasta era **l'unica con percorso e seduta insieme**, ed è per questo che è stata la
prima a mordere: percorso esatto e seduta che la rivendica. ⏱️ *Dal 2026-08-28 non è più «rimasta»:
il kit è versionato. La riga resta come descrizione di quale delle quattro famiglie si sblocca per
prima, e perché — percorso e produttore, non una sola delle due.*

> ⚠️ *Questa frase ha sbagliato **tre volte** e ogni correzione ne ha riletta una parte sola: prima il
> numero («Due» con tre righe), poi il predicato «né in una seduta» (falso per U21 e U25), poi «manca il
> percorso» (falso per le prime due righe). Il titolo della sezione portava lo stesso errore e non era
> stato toccato. Ora il predicato è quello che la tabella qui sotto verifica riga per riga.*

| Famiglia | Chi la richiede | Perché `git add` tace |
|---|---|---|
| **Icone dell'HUD** | E20 · E11 | L'**insieme** richiesto è derivato e cresce da solo — `URTIconLibrary::RequiredIconIds()` lo compone dalle fasi volontarie e dal catalogo azioni *realmente in codice*, gate `RTIconCatalogTests` — ma **path e naming sono già decisi**: `/Game/RT/UI/Icons/`, `T_UI_Icon_<Categoria>_<Nome>` ([`brief-icone-v01.md`](brief-icone-v01.md) §33–34). Manca solo la riga d'allowlist, ed è un problema concreto: chi importa le texture al path già deciso fa `git add`, git tace, e le icone restano locali |
| **Sorgenti icona già sul disco** | E20 | `Content/RT_UI_AssetPack_FromHUD/` contiene **30 PNG** (`icons/I_Guard.png`, `I_Overwatch.png`, …) più `buttons/`, `panels/`, `tiles/`, `warnings/` e un `manifest.json` con box e margini 9-slice. Di tutto il kit **il repository traccia due file**: `README.md` e `manifest.json`. I trenta PNG no. È la famiglia più vicina a essere pronta, e la sola che nessuna riga d'allowlist prevede |
| **Livello illuminato del graybox** | seduta **U21** | U21 dichiara di produrre «il livello illuminato **committato**», ma ha `artifacts: []`: nessuno sa quale file sarà, quindi non può entrare nell'allowlist prima della seduta |

---

## 3. v0.2 — derivata dalle epic, non dai file

Nessun asset della v0.2 esiste, e nessuno ha ancora un path: quello che segue è il **fabbisogno** che le epic
dichiarano, tradotto nei percorsi che le convenzioni impongono. Serve a dimensionare il lavoro, non a
committare niente.

| Famiglia | Epic | Quanti | Percorso previsto (§5 delle convenzioni) |
|---|:--:|--:|---|
| Unità giocabili dei 4 eroi nuovi | **E35** | 4 | `Characters/<Pack>/Blueprints/BP_Unit_<Pack>.uasset` |
| Animazioni dei 4 eroi nuovi | **E35** | 12 | `Characters/<Pack>/Animation/AM_<Pack>_*.uasset` — ⚠️ *diceva «4 · `ABP_<Pack>.uasset`»: un eroe nuovo non porta un anim BP, porta una **riga di dati** in `ClipsPerHero` più i suoi tre montaggi ([D-248](../../decisions/RT_PDR_00_Decision_Log.md))* |
| Mappa di classe Standard 3v3 | **E24** | 1 + dati | `Maps/<Categoria>/<Nome>/` con il suo `DA_HexMap_*` |
| Icone: catalogo completo | **E25** | derivato | ancora senza path — vedi §2.1 |
| Muri e porte come oggetti | **E23** | ignoto | l'epic definisce il **modello logico**; se servano mesh dedicate non è deciso |

**Gli eroi sono già scelti e già speccati**: Steel e Murdock (Sentinel Directorate), Aurora e Kwang
(Resonance), in [`../../characters/v0.2`](../../characters/v0.2/) — E35 li porta a runtime, non li inventa. Il
`<Pack>` è il **nome del pack Paragon**, non quello dell'eroe di gioco (§5b delle convenzioni) — e per i
quattro nuovi **è già dichiarato**: ogni scheda in `docs/characters/v0.2/` porta `Asset base: Paragon —
<Nome>` e `Hero_Key`. I percorsi sono quindi già risolvibili (`Characters/Steel/Blueprints/BP_Unit_Steel.uasset`
e simili), e le righe d'allowlist si possono scrivere **prima** che gli asset esistano: è la pratica già
adottata per U1, U7 e U8.

> ⚠️ **Oggi i dati dell'eroe non sono un asset committato, e la ragione va verificata prima di farne una
> regola.** In `Content/` non esiste nessun `DA_Hero_*`, e l'esempio in §5 delle convenzioni descrive dove
> *starebbe*, non un file che esiste. Ma [`roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md) descrive
> `URTHeroData` come `UPrimaryDataAsset` con asset `PDA_*` sotto `Content/RT/`: **le due fonti divergono**,
> e questo registro non è l'owner che può chiudere la divergenza.
> *(Una prima stesura citava `#375` a sostegno di «spedito da C++». È un'attribuzione sbagliata: #375 è il
> checksum di determinismo (CP 12.1). Il precedente vero riguarda `Format.Skirmish2v2`, non gli eroi.)*

---

## 4. v0.3 e oltre — quello che le fonti dicono davvero

⚠️ **Questa sezione è stata riscritta dopo il merge di #818 (D-136), che l'aveva smentita mentre
veniva scritta.** La prima stesura diceva «nel registry la v0.4 ha zero feature» e «sei feature
`future` non tracciate»: misure vere su `515c5c88`, false sull'albero in cui il documento è atterrato.
Il modello di release ora arriva alla **v1.0** e le release intermedie **v0.5–v0.8 esistono**.

| Release | Feature nel registry | Asset dichiarati | Perché |
|---|--:|---|---|
| **v0.3** *Informazione* | 5 | **nessuno** | percezione e bot: simulazione, non contenuto |
| **v0.4** *Operations* | **4** | 1 famiglia: mappa Operations (**E30**) | le altre epic sono formato e obiettivi |
| **v0.5 – v0.8** | 1 ciascuna | **non ancora esaminati** | release introdotte da D-136: il loro fabbisogno di asset non è stato misurato da questo registro |
| **v1.0** | — | **nessuno dichiarato** | — |
| `future` | **4** | acqua dinamica · strutture e crolli · verticalità · Control Center | le uniche rimaste senza release dopo D-136; le prime tre toccherebbero il mondo e richiederebbero asset ambientali |

**Il costo in asset resta quasi tutto nella v0.2**, ed è il roster: le quattro unità della v0.1 sono
costate quattro `BP_Unit` più quattro `ABP`, e il roster a 8 raddoppia quel lavoro in una release
sola. Dal lato contenuti il conto è lineare e noto: **8 asset**.

⚠️ **Le release v0.5–v0.8 sono un buco dichiarato di questo documento**, non del registry: sono nate
lo stesso giorno in cui è nato lui, e nessuno ha ancora chiesto quali asset richiedano.

> 🔵 **Aggiornamento 2026-08-17 — il buco si è ristretto da un lato solo, ed è quello di sopra.** Il
> consolidamento del Graybox Kit (`D-153`) ha misurato il fabbisogno di **oggetti di mappa** per tutte le
> release e la risposta è che **non atterra nella v0.5–v0.8**: da lì i temi canonici sono rete, GAS,
> dedicated server e hardening, e nessuno di essi è un tema di contenuto. Dei **sette** elementi `DEFER` del
> catalogo, **quattro** si appoggiano alla riga **`future`** qui sopra — `RT-FEAT-MAP-STRUCTURAL` (macerie,
> muri sfondati) e `RT-FEAT-MAP-VERTICALITY` (rampe, piattaforme) sono le due feature `IDEA` che li
> possiedono, e finché restano `IDEA` quegli asset non hanno un committente. Gli altri tre non c'entrano con
> `future` e hanno ciascuno una ragione propria: la **valvola** è fuori scope v0.1 dichiarato, generatore e
> serbatoio hazard sono proxy di elementi che nessuno produce ancora. **Non è un ritardo della pipeline**: è
> che sette voci su diciannove descrivono sistemi che il progetto non ha ancora deciso di costruire.
> La sola release che acquisisce un impegno nuovo è la **v1.0**, e non in asset: `E45` diventa il punto in
> cui il contratto di ingombro **si congela** perché l'arte finale possa sostituire il graybox senza
> cambiare le regole competitive.

## 5. Da dove vengono gli asset

Import da Fab attraverso un **magazzino** e `Migrate` — procedura vigente dal 2026-08-10,
[`convenzioni-contenuti-ue.md`](convenzioni-contenuti-ue.md) §B.2a. In sintesi, perché la scorciatoia non
esiste: Fab non lascia scegliere la cartella di destinazione dentro `Content/`, quindi si installa il pack in
un progetto vuoto fuori dal repository, **lì** si rinomina, e da lì si migra solo ciò che serve.

⚠️ Due punti su cui si sbaglia, entrambi già pagati: `Migrate` **preserva il path virtuale** (rinominare dopo
non basta, va fatto nel magazzino), e non si portano `SoundCue` né `DialogueWave`.

Il **vault con junction** è storia (§B.2b, 2026-08-05 → 06): fu abbandonato perché il progetto finiva per
dipendere da un percorso esterno. Non va riproposto.

I pack scaricati vivono in `Content/FabAsset/` e `Content/Paragon*/`, **fuori dal repository per scelta**:
sono decine di GB e chi clona se li riscarica.

🔴 **`git clean -fdx` li cancella, e l'esclusione a una sola cartella non basta.** Il comando che gira
nel progetto — `git clean -fdx -e Content/FabAsset` — protegge la prima e **non** la seconda, mentre
`.gitignore` le dichiara entrambe ignorate (`/Content/FabAsset/`, `/Content/Paragon*/`). Servono
entrambe le esclusioni:

```bash
git clean -fdx -e Content/FabAsset -e 'Content/Paragon*'
```

*(Oggi `Content/Paragon*/` non esiste sul disco di chi sviluppa — i pack sono stati consolidati sotto
`FabAsset/` — quindi il difetto non morde. Morde alla prima reimportazione che segue la regola
`.gitignore` invece del comando. La forma monca vive anche in `convenzioni-contenuti-ue.md` §B: là è
un difetto preesistente che questo registro non può correggere da solo, ed è segnalato qui perché è
lo stesso comando.)*

---

## 6. Come si aggiunge un asset

Tre righe, in quest'ordine. Saltarne una produce un difetto silenzioso, e per ognuna è già successo:

1. **`.gitignore`** — la riga `!Content/RT/…` con il path esatto. Senza, `git add` tace e l'asset resta
   locale. **I casi aperti stanno in §2.1**, e questa riga non ne nomina più nessuno: è la regola, non
   l'elenco.
   *(⚠️ Questa riga ha citato un caso vivo tre volte e tre volte è invecchiata — `ABP_Gadget` fino al
   17-08, `WBP_RT_FrontendRoot` e `WBP_RT_ModalLayer` fino al 31-08, i dodici `AM_*` per mezza giornata.
   Ogni volta l'esempio è caduto **perché il caso si era chiuso**, cioè per la ragione più desiderabile
   possibile: un documento che si aggiorna solo quando le cose peggiorano non lo si aggiorna mai. Il
   rimedio non è scegliere un esempio più stabile — è **non metterne**, e puntare alla sezione che i casi
   li elenca già e li tiene aggiornati per mestiere. ⛔ `ABP_Gadget` era anche l'esempio **sbagliato**: la
   sua riga d'allowlist esisteva dall'11 agosto, quindi non era rimasto locale per una regola mancante ma
   per un `git add` mai eseguito. Due cause diverse, lo stesso sintomo, e solo la seconda si è portata via
   il file.)*
2. **`editor-sessions.yaml`** — l'asset va fra gli `artifacts` della seduta che lo produce, **come
   stringa di path e nient'altro**. Senza, nessuna vista sa che quell'asset è atteso: è il caso di
   **U21**.
   🔴 **Non scrivere `tracked`**: non è un campo del sorgente, è un **derivato** che
   `scripts/feature_registry.py` calcola confrontando il path con `git ls-files` e scrive solo in
   `project-graph.json`. Trasformare le voci in mapping `{path:…, tracked:…}` fa confrontare dict
   contro stringhe dentro `tracked_artifacts()` e `session_state()`: l'artefatto non risulta mai
   `done`, la seduta resta bloccata, **e nessun gate fallisce**.
3. **questo file** — la riga nella tabella della sua release.

⚠️ **Un rename tocca tutte e tre.** Le convenzioni lo dichiarano già per il caso `<CharacterId>`: gli otto
path degli artefatti sono elencati **per esteso** nell'allowlist, quindi un rename li rende muti senza che
niente fallisca.

🔴 **E poi c'è il gesto che nessuna delle tre righe copre: `git add` alla prima salvata.** Le tre righe
rendono l'asset *committabile*; non lo committano. Fra il momento in cui l'editor scrive il `.uasset` e
il momento in cui qualcuno lo aggiunge all'indice, quel file è **untracked** — e un `git clean -fd`, che
in questo progetto si esegue di routine per non toccare i pack, lo cancella senza traccia recuperabile.
Non aspettare che l'asset sia finito: un lavoro a metà committato è un lavoro a metà, un lavoro a metà
non committato è un lavoro perso. `ABP_Gadget` è costato una seduta per impararlo.

---

## 7. Quello che questa map non sa

- **Non conosce gli asset non committabili.** I pack Paragon importati stanno fuori dal repository: qui si
  vede il `BP_Unit` che li usa, non le mesh e le animazioni sorgente da cui dipende.
- **Non misura le dipendenze.** Che `BP_Unit_Gadget` referenzi una `SkeletalMesh` di un pack è vero e non
  verificabile da qui: lo dice l'editor, e il fallback al cilindro è ciò che tiene in piedi il gioco quando
  il riferimento soft non risolve (`spec-asset-pipeline.md`).
- **Non sostituisce l'allowlist.** Se le due divergono, **vince `.gitignore`**: è il gate che il repository
  esegue davvero. Il comando di §1 se ne accorge **in entrambe le direzioni** — attesi che mancano, e
  tracciati che nessuno ha dichiarato — ma solo dalla sua seconda stesura: la prima iterava la sola
  allowlist, e una lista che interroga sé stessa non può trovare ciò che non prevede.
- **Le righe di §3 e §4 non sono impegni.** Sono fabbisogno derivato dalle epic; diventano impegni quando
  entrano in una seduta e nell'allowlist.
