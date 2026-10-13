# Il piano si legge dal bordo — brainstorm del 2026-10-12

> `SNAPSHOT` · fotografia del 2026-10-12: vale finché è l'ultima misura del suo oggetto.
>
> - **Statuto**: brief di una sessione `/sc:brainstorm` con l'autore. Le scelte qui sotto sono sue. Diventano
>   [D-508](../../decisions/RT_PDR_00_Decision_Log.md) al merge di [#3704](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3704).
> - **Viene da**: la seduta PIE `U84` su [#3679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3679) e [D-501](../../decisions/RT_PDR_00_Decision_Log.md), cioè i piani rispetto a quello attivo.
> - **Issue**: [#3704](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3704) per le celle e i fantasmi,
>   [#3705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3705) per le unità,
>   [#3706](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3706) per muri, coperture e porte. Tutte in v0.1.

---

## 1. Da dove viene

Nella seduta `U84` l'autore ha guardato `Visual.Map.LayerStack` dal vivo, con le leve di #3679 cambiate in console.

| Che cosa | Verdetto dell'autore |
|---|---|
| piani sotto, desaturazione 0,7 e scurimento 0,55 per piano | *«sotto va bene»* |
| piani sopra, opacità 0,2 | *«sopra troppo leggero, si capisce poco»* |
| il cambio di piano | *«andando page up non mi sembra si aggiornino i piani superiori»* |

L'ultima frase è stata **misurata**, e i dati si aggiornano. Dal bridge, sulla board viva, col piano attivo 1: le 19 celle del mezzanino sono piene, le 61 del piano terra attenuate, e le 7 della cima hanno trasparenza 0,8. È la percezione che non regge: un piano sopra traslucido, appoggiato su un piano dello stesso grigio, non si distingue, a nessuna opacità.

> 🔴 **Premessa corretta in #3704, lo stesso giorno.** I dati si aggiornavano, ma **i piani sopra non erano traslucidi affatto**: la maschera di `M_HexCell` era collegata e ignorata (`UseConstant` acceso sul pin `Opacity Mask`). L'autore aveva ragione su tutte e due le frasi: a schermo i piani sopra non cambiavano, e nessuna opacità si è mai vista, né 0,2 né 0,35. La regola dei bordi non dipende da questa premessa: l'autore ha giudicato i colori dei bordi, e poi, sull'asset riparato, l'opacità.

## 2. La regola

> **Ogni cosa dice a che piano sta col suo bordo.**

| Relazione col piano attivo | Bordo | In scala di grigi (§47-bis.1) |
|---|---|---|
| il piano attivo | **nero** | scuro |
| un piano sopra | **bianco** | chiaro |
| un piano sotto | **viola** (§32 `Violet`) | medio |

- **Lo stesso colore a ogni distanza**: la distanza la dice già lo scurimento dei piani sotto.
- **Si aggiunge** al trattamento di D-501: i piani sopra restano traslucidi e quelli sotto scuriti e desaturati.
- **Una mappa a un piano** resta tutta nera, com'è oggi.
- Il piano attivo nero vuol dire che il bordo «marcato» di #3679 (+6 stop, cioè chiaro) **si inverte**: torna il bordo di prima.

## 3. A che cosa si applica

| Che cosa | Come porta il bordo | Issue | Note |
|---|---|---|---|
| celle | il bordo della griglia (`CellBorders`), **opaco su ogni piano** | #3704 | un piano sopra diventa un riempimento traslucido con un contorno bianco netto |
| glifi di superficie | **seguono la cella**: nessun colore proprio | #3704 | il piano lo dice già il bordo della cella |
| fantasmi del piano | un **contorno** del colore del piano; il riempimento resta il colore della certezza | #3704 | |
| unità | il **contorno del modello**, che **si vede anche attraverso il piano attivo** quando questo copre l'unità | #3705 | l'anello resta della squadra; solo le unità che la squadra vede. «Attraverso il piano» l'ha deciso l'autore in seduta, il 2026-10-12: col mezzanino attivo, l'unità sul pavimento sotto di esso spariva |
| muri, coperture, porte | la **tinta del volume** | #3706 | oggi usano il materiale di default delle mesh: serve un materiale con dati per istanza |

## 4. Scartato, e perché

- **Il rosso sotto.** È il colore della squadra avversaria (`Team1Color`) e delle celle bloccate (`BlockedCellColor`). L'ambra stava vicino al giallo delle celle che bloccano la vista e del contorno dell'hover; l'azzurro spento vicino al blu della squadra amica.
- **Il bordo della targa** e **un secondo anello** per le unità: il contorno del modello si legge meglio. Costa di più, e il costo lo scrive #3705.
- **Un filo sullo spigolo alto** per i muri: la tinta del volume si legge anche da lontano.
- **Sfumare il bordo con la distanza**: lo fa già lo scurimento.

## 5. Vincoli che restano

- **Sola presentazione.** Nessuna `FRTCellId` cambia, e il picking resta quello di D-255.
- **Il velo viene prima.** Una cella mai vista resta non disegnata, bordo compreso, e un'unità non vista non ha contorno: il contorno non deve rivelare né un nemico né la sua quota.
- ⚠️ **Il viola c'è già** nella superficie `Void` (150,40,150) e nell'accento delle reazioni dell'HUD. In PIE va verificato che un bordo viola non si legga come `Void`.

## 6. Intensità già scelte, e quelle da scegliere

| Leva | Valore | Stato |
|---|---|---|
| `rt.Map.Layers.LowerDesaturation` | 0,7 per piano | scelta in `U84`; diventa il default con #3704 |
| `rt.Map.Layers.LowerDarkening` | 0,55 per piano | scelta in `U84`; diventa il default con #3704 |
| `rt.Map.Layers.UpperOpacity` | 0,35 | scelta dall'autore il 2026-10-12, **su piani traslucidi** e coi bordi bianchi. Il giudizio di prima («0,2 troppo leggero») era stato dato con la maschera inerte |
| `rt.Map.Layers.ActiveBorderStops` | superata dalla regola | si toglie con #3704 |
| `rt.Map.Layers.UpperExposure` | 3 stop | ➕ aggiunta da #3674, dopo questo brief: i piani sopra sono anche schiariti. Scelta dall'autore la sera del 2026-10-12 su `Visual.Map.MultiLevel`, dopo aver provato 1,5 e 4, e tenuta dopo aver visto `Visual.Map.LayerStack` |

## 7. Ordine

1. **#3704**, che porta la regola e D-508. Il suo verdetto PIE chiude #3679 e, se la piattaforma si distingue a 120° e 180°, #3674.
   ➕ *2026-10-12, sera:* con #3704 su `main` la piattaforma non si distingueva ancora. L'ha chiusa #3674, schiarendo
   i piani sopra.
2. **#3705** e **#3706**, indipendenti fra loro, dopo #3704.
   ➕ *2026-10-13:* #3705 è attuata con un post-process su custom depth; il contorno di chi sta sotto il piano
   attivo passa attraverso la geometria. Aspetta il giudizio dell'autore (`PIE-CAM-CONTORNO-UNITA`).
   ➕ *2026-10-13:* #3706 è attuata con un materiale generato, `M_HexVolume`, e sette dati per istanza. Il
   rilievo del costo resta fuori. Aspetta il giudizio dell'autore (`PIE-CAM-TINTA-VOLUMI`).
   ➕ *2026-10-13, più tardi:* l'autore ha giudicato #3706 sulle tavole delle catture. La forza della tinta
   resta 0,25, e la issue è chiusa.
