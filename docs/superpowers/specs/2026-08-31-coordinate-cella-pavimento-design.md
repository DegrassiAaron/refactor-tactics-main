# Le coordinate della cella sul pavimento — design

**Data**: 2026-08-31 · **Issue**: #1920 · **Epic**: #1861 (Map Editor 0.1)
**Base**: `origin/main` @ `7c48ce63`

---

## 1. Il problema, e perché non è «manca una scritta»

Chi disegna una mappa ragiona in coordinate assiali. Il repository le usa ovunque — le fixture le nominano,
i referti dei test le stampano, i messaggi d'errore le citano — ma sul pavimento non compaiono. La
coordinata è **disponibile** (un tool la mette in un pannello) e non **presente**: per leggerla bisogna
smettere di fare quello che si stava facendo.

Su mappa multilivello il costo raddoppia, perché due celle con la stessa `(x,y)` esistono su layer diversi e
si distinguono solo dalla terza componente.

## 2. Il vincolo che decide la forma

🔴 **Le celle sono istanze della stessa mesh.** `ARTHexMapActor` monta sette
`InstancedStaticMeshComponent` — `Cells`, `Relief`, `Blockers`, `EdgeFeatures`, `SurfaceGlyphs[4]`,
`CellBorders`, `KnowledgeVolumes` — e un ISM ripete **una** geometria. Le coordinate invece cambiano per
cella: non possono stare nella mesh.

⚠️ E i tre `PerInstanceCustomData` che il materiale legge sono **già occupati** dal colore di superficie.

∴ qualunque soluzione deve produrre geometria (o disegno) **per cella**, e la domanda diventa dove.

## 3. Decisioni prese, con chi le ha prese

| Decisione | Scelta | Da chi |
|---|---|---|
| Ambito | **solo editor**, mai in partita | autore |
| Attivazione | **sempre**, su tutta la mappa aperta | autore |
| Contenuto | `(x, y, layer)` — la `z` **è il layer** | autore |
| Direzioni | tre, a `0° / 120° / 240°` | autore |
| Disposizione | **radiale, dal vertice verso il centro** | autore |
| Scala del layer | **metà** di `x` e `y` | autore |
| Meccanismo | segmenti (B), con la strada del font atlas (C) lasciata aperta | proposta, approvata |

⚠️ **«Parte del gray kit» è stato sciolto, non ignorato.** Il kit è fatto di asset di gioco
(`SM_Graybox_Cover_Low`, `Door_Panel`, `Surface_Water`…), generati da un commandlet e istanziati anche in
partita. Un segno che in partita non esiste non appartiene al kit: aggiungere undici mesh di cifre (`0`–`9`
e il meno) lo farebbe crescere per qualcosa che nessuna build di gioco monterà mai.

## 4. Architettura — due pezzi, e il confine fra loro

```text
Source/RefactorTactics/Map/RTHexLabelLibrary        ← REGOLA: geometria pura, nessun world
        │  GlyphSegments(TCHAR) -> segmenti in [0..1]²
        │  BuildCellLabel(Cell, Origin, HexSize, LayerHeight) -> 3 run posate nel mondo
        ▼
Source/RefactorTacticsEditor/...  (WITH_EDITOR)     ← GUSCIO: consuma le pose e traccia linee
```

**`GlyphSegments`** conosce la forma dei caratteri e nient'altro: dato `'4'` restituisce i segmenti che lo
disegnano dentro un quadrato unitario. Il set è chiuso: `0`–`9`, `,`, `-`.

**`BuildCellLabel`** conosce la cella e nient'altro: prende i sei vertici da
`URTHexLibrary::CellCorners` — che è già l'autorità della geometria esagonale e non va duplicata — sceglie i
**tre vertici alternati**, e dispone la terna lungo ciascun raggio, dal bordo verso il centro. Restituisce,
per ogni carattere, la sua posa nel mondo e la sua scala.

🔑 **Il confine è dove sta il valore.** `BuildCellLabel` dice *dove va cosa*, mai *come si disegna*. Il
guscio d'oggi traccia linee; un guscio futuro potrebbe posare quad con un font atlas (approccio C) e
consumerebbe **le stesse run**, senza che una riga di geometria cambi. È la stessa separazione di
`RTHexProbe` (#711) e `RTHexLos` (#1755): la regola nel runtime, la presentazione nell'editor.

## 5. Il contenimento non è una raccomandazione: è ciò che dimensiona le cifre

Il criterio *«senza uscire dall'esagono»* diventa un test:

> ogni estremo di ogni segmento, di tutte e tre le run, cade dentro il poligono dei sei vertici.

⚠️ **E fissa la dimensione massima delle cifre.** Non la scegliamo a occhio: la si deriva dal vincolo. Un
esagono ha larghezza variabile lungo il raggio — larga al centro, nulla al vertice — quindi una terna
disposta radialmente **si assottiglia verso lo spigolo**, e il test è ciò che impedisce di accorgersene
solo a schermo.

⚠️ La terna più lunga possibile va prevista: coordinate negative a due cifre, cioè `-10,-10,1` — dieci
caratteri. Se la dimensione è tarata su `0,0,0` la mappa grande sborda, e nessuno se ne accorge finché non
apre una mappa grande.

## 6. Testabilità — cosa è misurato e cosa no

**Misurato** (automation, senza world):

- le tre run sono a `120°` esatti l'una dall'altra, verificato sulle **pose** e non a occhio;
- la terna corre dal vertice al centro: la prima cifra è più lontana dal centro dell'ultima;
- il layer ha **metà** della scala di `x` e `y`;
- ogni segmento è **dentro** l'esagono, sul caso peggiore (`-10,-10,1`);
- il segno meno compare per le coordinate negative;
- il set di caratteri è chiuso: un carattere fuori set non produce segmenti inventati.

**Non misurato, e dichiarato:**

⛔ **Quanto sia leggibile una cifra a segmenti su una cella.** È un giudizio d'autore, come la leggibilità
del ventaglio della sonda (#711): una voce `PIE-*` collocata in una seduta, non un test. Ed è la sola cosa
che può far dichiarare fallita questa feature pur essendo tutto verde.

⛔ **Che il componente non esista nella build di gioco** si verifica per **assenza** — `WITH_EDITOR` e nessun
riferimento fuori dal modulo editor — non con un test che gira nell'editor.

## 7. Guardrail

- ⛔ Nessun asset nuovo nel gray kit (§3).
- ⛔ Nessun ottavo ISM sull'`ARTHexMapActor`: quelle sette famiglie sono canali di lettura **della partita**.
- ⛔ Nessuna seconda fonte per i vertici dell'esagono: `CellCorners` è l'autorità.
- ⛔ Nessun interruttore. La richiesta è «sempre visibili»; una checkbox non richiesta è una decisione presa
  di passaggio.
- ⛔ Nessun font, nessuna texture, nessun atlas in questa passata: sono l'approccio C, che questa
  architettura lascia possibile e non anticipa.

## 8. Rischi

| Rischio | Perché | Come lo si vede |
|---|---|---|
| Illeggibilità a segmenti | un font a linee su una cella piccola può risultare confuso | voce PIE, seduta |
| Costo di disegno | ogni cella produce tre run di ~10 caratteri; su mappe grandi sono molte linee per frame | si misura in seduta, come per la sonda (#711) — dove un costo per-hover non misurato ha bloccato l'Editor |
| Rumore visivo | coordinate su ogni cella competono con superficie, bordo e glifo | giudizio d'autore |

⚠️ Il secondo rischio ha un precedente **di questa settimana**: la sonda di movimento è stata consegnata
verde e in seduta bloccava il viewport, perché il costo per evento non era misurato da nessun test. Qui il
costo è per frame e su tutte le celle: va guardato presto, non alla fine.
