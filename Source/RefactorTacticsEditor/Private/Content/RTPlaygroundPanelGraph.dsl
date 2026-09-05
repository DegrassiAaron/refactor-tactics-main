; RTPlaygroundPanelGraph.dsl - il grafo di WBP_RT_GrayKitPlayground, in forma leggibile.
;
; PERCHE' ESISTE QUESTO FILE
; `RTBuildPlaygroundPanelCommandlet` genera lo SCHELETRO del widget (i 22 widget, i nomi stabili,
; le tre righe di DIAGNOSTICS prese dal modello) ma NON il grafo. Il grafo e' stato scritto con
; `editor_toolset.toolsets.blueprint.BlueprintTools.write_graph_dsl` via il server MCP del motore.
;
; ⚠️ Un `.uasset` non e' diffabile: il diff di una PR non puo' mostrare cosa e' cambiato dentro.
; Questo file E' quel diff. Rileggerlo con `read_graph_dsl` deve restituire lo stesso grafo
; (a meno dell'inlining: il decompilatore stampa piu' volte un nodo puro condiviso - misurato,
; `Break RTPlayground Map State` e' UN nodo solo pur comparendo tre volte nella rilettura).
;
; COME RIAPPLICARLO
;   graph = /Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground.WBP_RT_GrayKitPlayground:EventGraph
;   write_graph_dsl(graph, <questo file>) -> compile_blueprint(warnings_as_errors=true) -> save_assets
; Gli eventi vanno creati PRIMA, con `UMGToolSet.BindToEventProperty`: `write_graph_dsl` non li crea.
;   Cmb_Facing/OnSelectionChanged · Cmb_Station/OnSelectionChanged (/Script/UMG.ComboBoxString)
;   Btn_SelectFixture/OnClicked · Btn_ResetFixture/OnClicked · Btn_Focus/OnClicked
;   Btn_CamClose/OnClicked · Btn_CamTactical/OnClicked · Btn_CamOverview/OnClicked (/Script/UMG.Button)
;   Spn_BodyRadius/OnValueChanged · Spn_BodyHeight/OnValueChanged
;   Spn_FaceHeight/OnValueChanged · Spn_MarkerLength/OnValueChanged (/Script/UMG.SpinBox)
;   Chk_Labels/OnCheckStateChanged (/Script/UMG.CheckBox)
;
; ➕ **2026-09-05 — i cinque controlli che la DoD chiedeva e che non esistevano.** Fino a questa
; revisione il pannello cablava `Facing` e NIENT'ALTRO dei cinque parametri: `ApplyFixtureParameters`
; era una `UFUNCTION` con zero chiamanti, e `PanelGraphCallsTheModel` era **verde** perche' non la
; elencava fra le attese — certificava l'assenza invece di trovarla. I quattro `USpinBox` e
; `Chk_Labels` li costruisce il commandlet (`EnsureFixtureAndViewControls`, idempotente, applicabile
; con `-RefreshOptions` senza perdere questo grafo).
;
; 🔴 **`Select Fixture` e `Reset` RIEMPIONO i quattro campi, e non e' una comodita'.** Un `USpinBox`
; nasce a `0`: senza `PushFixtureParametersToSpinBoxes` la prima rotellata scriverebbe `0` su un corpo
; che vale `60` — il pannello sembrerebbe funzionare mentre cancella il fixture. Dopo `Reset` serve per
; la ragione simmetrica: l'attore e' gia' tornato ai default e i campi mostrerebbero i valori sporchi.
;
; 🔑 **NESSUN nodo `SpinBox|SetValue` o `SpinBox|GetValue` in questo grafo, ed e' deliberato.** Misurato
; il 2026-09-05: `USlider` e `USpinBox` dichiarano **entrambi** `SetValue(float)` e `GetValue()` con
; `Category="Behavior"` — stesso nome, stessa firma — e il DSL prende la prima. E' la trappola gia'
; pagata su `ComboBox|GetSelectedOption`. Percio' i quattro campi si riempiono con UNA chiamata C++ che
; riceve i widget come parametri (dove il tipo e' esatto), e ogni `OnValueChanged` scrive il SOLO campo
; che l'utente ha mosso con `ApplyFixtureParameter`, che prende l'enum invece di quattro `float`.
; ⚠️ Cosi' gli altri tre valori vengono dal FIXTURE, che e' la fonte di verita', e non dai widget: se un
; campo e l'attore divergessero, rileggere dai widget propagherebbe la divergenza sull'attore.
;
; ⛔ **Un solo toggle, non tre.** `Chk_Labels` c'e' perche' le etichette sono `ATextRenderActor` e la
; classe e' un aggancio stabile. Guida e bounds NON hanno un aggancio: misurato il 2026-09-05, la
; mappa non dichiara nessun `Tags`/`ComponentTags`/`Layers` e i suoi 25 attori si distinguono solo per
; `ActorLabel`; i bounds non hanno nemmeno un attore. Elencare qui `GKP_MetreGuide_PresentationOnly`
; sarebbe la prima copia in codice di un nome che vive solo nel `.umap` — `#1459`. Sono una
; integration request verso #1991, non lavoro di questo lato.
;
; I widget scritti devono essere VARIABILI, altrimenti il loro `Get<Nome>` non esiste. Dalla
; revisione del 2026-09-02 li rende tali il commandlet stesso (`MakeEveryWidgetAVariable`): non
; serve piu' passare da `ToggleWidgetAsVariable` a mano.
;
; ✅ Cablati dal 2026-09-02 gli otto controlli di allora; i cinque nuovi dal 2026-09-05 (vedi sopra).
; ⛔ **«Tutto cablato» non e' una frase da lasciare in piedi senza un conto**: dal 2026-09-02 al
; 2026-09-05 questa riga diceva TUTTO mentre cinque controlli della DoD non esistevano nemmeno come
; widget. Chi la leggeva concludeva che non ci fosse altro da fare. Il conto lo tiene
; `PanelDeclaredControlsExist`, che confronta i controlli con l'elenco della DoD invece di fidarsi.
;
; Il buco precedente - `Btn_Focus`, i tre preset di camera e
; `Cmb_Station` - nasceva da una premessa sbagliata: che servisse `ComboBox|GetSelectedOption`, che
; esiste in DUE varianti con lo stesso type_id e il DSL prende la prima. **Non serve**: entrambe le combo
; consegnano `SelectedItem` come PARAMETRO di `OnSelectionChanged`. Da li' `ParseStationOption` torna al
; numero, che vive nella variabile `SelectedStation`, e i quattro pulsanti la usano.
;
; ⚠️ La logica della camera e' RIPETUTA nei quattro eventi invece di stare in una `(fn ...)`: una
; funzione e' un grafo separato, e `write_graph_dsl` scrive UN grafo per volta. Se un giorno si
; accorpasse, va accorpata in tutti e quattro - il decompilatore non lo direbbe.
;
; ➕ Le OPZIONI delle due combo le riempie il commandlet in `DefaultOptions`, dal modello, e
; `Playground.PanelComboOptionsComeFromTheModel` lo verifica voce per voce.
;
; ⚠️ `(* forward braccio)` diventa un `vector*vector`, non un `vector*float`: il DSL non raggiunge il
; nodo scalare, e Blueprint promuove il float a `(a,a,a)`. Per una SCALATURA il risultato e' identico,
; ed e' voluto — non un caso da "correggere" leggendo la rilettura.

(event UserInterface|EventConstruct
  ; le tre righe di DIAGNOSTICS vengono dal modello, non sono riscritte qui
  (bind diag (RefactorTactics|Playground|DiagnosticsLines))
  (Widget|SetText(Text) :self (Variables|WBP_RT_GrayKitPlayground|GetTxt_DiagStation)
     :InText (Utilities|Array|Get(acopy) diag 0))
  (Widget|SetText(Text) :self (Variables|WBP_RT_GrayKitPlayground|GetTxt_DiagBounds)
     :InText (Utilities|Array|Get(acopy) diag 1))
  (Widget|SetText(Text) :self (Variables|WBP_RT_GrayKitPlayground|GetTxt_DiagActor)
     :InText (Utilities|Array|Get(acopy) diag 2))
  ; lo stato della mappa per ultimo: lo switch chiude il flusso exec
  (bind lvl (Game|GetCurrentLevelName :WorldContextObject self :bRemovePrefixString true))
  (bind (readiness mapName reason)
        (Utilities|Struct|BreakRTPlaygroundMapState (RefactorTactics|Playground|EvaluateMapState lvl)))
  (bind txtState (Variables|WBP_RT_GrayKitPlayground|GetTxt_MapState))
  (switch Utilities|FlowControl|Switch|SwitchonERTPlaygroundReadiness readiness
    (:Ready
      (Widget|SetText(Text) :self txtState :InText (Utilities|String|Append mapName "  -  Ready")))
    (:Error
      (Widget|SetText(Text) :self txtState
        :InText (Utilities|String|Append mapName (Utilities|String|Append "  -  " reason))))))

(event OnSelectionChanged(Cmb_Facing) (SelectedItem SelectionType)
  (bind (facing ok) (RefactorTactics|Playground|ParseFacingOption SelectedItem))
  (if ok
    (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                   :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
    (if (> (Utilities|Array|Length actors) 0)
      (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
                 :Object (Utilities|Array|Get(acopy) actors 0))
        (:then
          (RefactorTactics|Playground|ApplyFixtureFacing fx facing))
        (:CastFailed)))))

(event OnClicked(Btn_SelectFixture)
  (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                 :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
  (bind txtName (Variables|WBP_RT_GrayKitPlayground|GetTxt_FixtureName))
  (if (> (Utilities|Array|Length actors) 0)
    (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
               :Object (Utilities|Array|Get(acopy) actors 0))
      (:then
        (Widget|SetText(Text) :self txtName
          :InText (Utilities|GetDisplayName (Utilities|Array|Get(acopy) actors 0)))
        ; 🔴 I quattro campi si RIEMPIONO prima di poter essere toccati: uno `USpinBox` nasce a `0`, e
        ;    la prima rotellata su un campo non letto scriverebbe `0` sul corpo. Vedi la nota in testa.
        (RefactorTactics|Playground|PushFixtureParametersToSpinBoxes fx
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_BodyRadius)
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_BodyHeight)
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_FaceHeight)
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_MarkerLength))
      (:CastFailed))
    (else
      (Widget|SetText(Text) :self txtName :InText "nessun fixture nella mappa aperta"))))

(event OnClicked(Btn_ResetFixture)
  (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                 :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
  (if (> (Utilities|Array|Length actors) 0)
    (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
               :Object (Utilities|Array|Get(acopy) actors 0))
      (:then
        (RefactorTactics|Playground|ResetFixture fx)
        ; 🔴 E i quattro campi si RILEGGONO dopo il reset. Senza questo il fixture torna ai default e i
        ;    campi restano sui valori vecchi: il pannello mostrerebbe `95` su un corpo che vale `60`, e
        ;    la rotellata successiva riscriverebbe `95` disfacendo il reset appena chiesto.
        (RefactorTactics|Playground|PushFixtureParametersToSpinBoxes fx
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_BodyRadius)
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_BodyHeight)
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_FaceHeight)
          (Variables|WBP_RT_GrayKitPlayground|GetSpn_MarkerLength))
      (:CastFailed))))

(event OnSelectionChanged(Cmb_Station) (SelectedItem SelectionType)
  ; L'evento porta gia' `SelectedItem`: non serve leggere la combo, che il DSL non sa disambiguare.
  ; ⛔ NON si scrive nelle tre righe di DIAGNOSTICS: quelle portano le dichiarazioni di D-304, e
  ;    sovrascriverle per dare un riscontro cancellerebbe cio' che il pannello esiste per mostrare.
  (bind (number parsed) (RefactorTactics|Playground|ParseStationOption SelectedItem))
  (if parsed
    (Variables|Default|SetSelectedStation number)))

(event OnClicked(Btn_Focus)
  (bind (station found) (RefactorTactics|Playground|FindStation (Variables|Default|GetSelectedStation)))
  (if found
    (bind (num sname minW maxW centre live) (Utilities|Struct|BreakRTPlaygroundStationInfo station))
    (bind arms (RefactorTactics|Playground|CameraPresetArmLengths))
    (bind rot (Math|Rotator|MakeRotator :Pitch (RefactorTactics|Playground|CameraPresetPitch) :Yaw 0.0 :Roll 0.0))
    ; ⛔ NON `centro + Z*braccio`: quella e' una picchiata, e mostra i tetti. La camera sta su un BRACCIO —
    ;    indietro lungo il proprio forward — quindi `centro - forward * braccio`, ed e' il pitch a
    ;    decidere quanto di quell'arretramento diventa quota.
    (Development|Editor|SetLevelViewportCameraInfo
      :self (EditorSubsystems|GetUnrealEditorSubsystem)
      :CameraLocation (- centre (* (Math|Vector|GetForwardVector rot) (Utilities|Array|Get(acopy) arms 1)))
      :CameraRotation rot)))

(event OnClicked(Btn_CamClose)
  (bind (station found) (RefactorTactics|Playground|FindStation (Variables|Default|GetSelectedStation)))
  (if found
    (bind (num sname minW maxW centre live) (Utilities|Struct|BreakRTPlaygroundStationInfo station))
    (bind arms (RefactorTactics|Playground|CameraPresetArmLengths))
    (bind rot (Math|Rotator|MakeRotator :Pitch (RefactorTactics|Playground|CameraPresetPitch) :Yaw 0.0 :Roll 0.0))
    ; ⛔ NON `centro + Z*braccio`: quella e' una picchiata, e mostra i tetti. La camera sta su un BRACCIO —
    ;    indietro lungo il proprio forward — quindi `centro - forward * braccio`, ed e' il pitch a
    ;    decidere quanto di quell'arretramento diventa quota.
    (Development|Editor|SetLevelViewportCameraInfo
      :self (EditorSubsystems|GetUnrealEditorSubsystem)
      :CameraLocation (- centre (* (Math|Vector|GetForwardVector rot) (Utilities|Array|Get(acopy) arms 0)))
      :CameraRotation rot)))

(event OnClicked(Btn_CamTactical)
  (bind (station found) (RefactorTactics|Playground|FindStation (Variables|Default|GetSelectedStation)))
  (if found
    (bind (num sname minW maxW centre live) (Utilities|Struct|BreakRTPlaygroundStationInfo station))
    (bind arms (RefactorTactics|Playground|CameraPresetArmLengths))
    (bind rot (Math|Rotator|MakeRotator :Pitch (RefactorTactics|Playground|CameraPresetPitch) :Yaw 0.0 :Roll 0.0))
    ; ⛔ NON `centro + Z*braccio`: quella e' una picchiata, e mostra i tetti. La camera sta su un BRACCIO —
    ;    indietro lungo il proprio forward — quindi `centro - forward * braccio`, ed e' il pitch a
    ;    decidere quanto di quell'arretramento diventa quota.
    (Development|Editor|SetLevelViewportCameraInfo
      :self (EditorSubsystems|GetUnrealEditorSubsystem)
      :CameraLocation (- centre (* (Math|Vector|GetForwardVector rot) (Utilities|Array|Get(acopy) arms 1)))
      :CameraRotation rot)))

(event OnClicked(Btn_CamOverview)
  (bind (station found) (RefactorTactics|Playground|FindStation (Variables|Default|GetSelectedStation)))
  (if found
    (bind (num sname minW maxW centre live) (Utilities|Struct|BreakRTPlaygroundStationInfo station))
    (bind arms (RefactorTactics|Playground|CameraPresetArmLengths))
    (bind rot (Math|Rotator|MakeRotator :Pitch (RefactorTactics|Playground|CameraPresetPitch) :Yaw 0.0 :Roll 0.0))
    ; ⛔ NON `centro + Z*braccio`: quella e' una picchiata, e mostra i tetti. La camera sta su un BRACCIO —
    ;    indietro lungo il proprio forward — quindi `centro - forward * braccio`, ed e' il pitch a
    ;    decidere quanto di quell'arretramento diventa quota.
    (Development|Editor|SetLevelViewportCameraInfo
      :self (EditorSubsystems|GetUnrealEditorSubsystem)
      :CameraLocation (- centre (* (Math|Vector|GetForwardVector rot) (Utilities|Array|Get(acopy) arms 2)))
      :CameraRotation rot)))

; ============================================================================
; I QUATTRO PARAMETRI NUMERICI  —  #1993, criterio «i cinque parametri si leggono e si modificano»
; ============================================================================
; ⚠️ La ricerca del fixture e' RIPETUTA in tutti e quattro, come nei quattro eventi di camera qui
; sopra e per la stessa ragione: una `(fn ...)` e' un grafo separato, e `write_graph_dsl` ne scrive
; UNO per volta. Se un giorno si accorpasse, va accorpata in tutti e quattro — il decompilatore non
; lo direbbe.
;
; 🔑 Ogni evento passa il PROPRIO valore nuovo (`InValue`) e RILEGGE gli altri tre dai loro spinbox.
;    Cosi' `ApplyFixtureParameters`, che scrive tutti e quattro insieme, non azzera i vicini — ed e'
;    lei a fare `RerunConstructionScripts`, che e' cio' che fa muovere il marker senza PIE.

(event OnValueChanged(Spn_BodyRadius) (InValue)
  (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                 :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
  (if (> (Utilities|Array|Length actors) 0)
    (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
               :Object (Utilities|Array|Get(acopy) actors 0))
      (:then
        ; ⛔ L'enum va QUOTATO: la reference del DSL dice che una parola non quotata e' un riferimento a
        ;    variabile, e `BodyRadius` nudo darebbe *«Undefined variable»*.
        (RefactorTactics|Playground|ApplyFixtureParameter fx "BodyRadius" InValue))
      (:CastFailed))))

(event OnValueChanged(Spn_BodyHeight) (InValue)
  (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                 :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
  (if (> (Utilities|Array|Length actors) 0)
    (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
               :Object (Utilities|Array|Get(acopy) actors 0))
      (:then
        ; ⛔ L'enum va QUOTATO: la reference del DSL dice che una parola non quotata e' un riferimento a
        ;    variabile, e `BodyRadius` nudo darebbe *«Undefined variable»*.
        (RefactorTactics|Playground|ApplyFixtureParameter fx "BodyHeight" InValue))
      (:CastFailed))))

(event OnValueChanged(Spn_FaceHeight) (InValue)
  (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                 :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
  (if (> (Utilities|Array|Length actors) 0)
    (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
               :Object (Utilities|Array|Get(acopy) actors 0))
      (:then
        ; ⛔ L'enum va QUOTATO: la reference del DSL dice che una parola non quotata e' un riferimento a
        ;    variabile, e `BodyRadius` nudo darebbe *«Undefined variable»*.
        (RefactorTactics|Playground|ApplyFixtureParameter fx "FaceHeight" InValue))
      (:CastFailed))))

(event OnValueChanged(Spn_MarkerLength) (InValue)
  (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                 :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
  (if (> (Utilities|Array|Length actors) 0)
    (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
               :Object (Utilities|Array|Get(acopy) actors 0))
      (:then
        ; ⛔ L'enum va QUOTATO: la reference del DSL dice che una parola non quotata e' un riferimento a
        ;    variabile, e `BodyRadius` nudo darebbe *«Undefined variable»*.
        (RefactorTactics|Playground|ApplyFixtureParameter fx "MarkerLength" InValue))
      (:CastFailed))))

; ============================================================================
; IL TOGGLE DELLE ETICHETTE  —  #1993, criterio «i tre toggle»  (uno solo: vedi la nota in testa)
; ============================================================================
; ⛔ Il grafo NON cerca gli attori: chiede al modello, che li trova per CLASSE. Un
;    `GetAllActorsOfClass` di `TextRenderActor` qui dentro sarebbe la stessa logica scritta due volte,
;    e la seconda copia non ha test.
(event OnCheckStateChanged(Chk_Labels) (bIsChecked)
  (RefactorTactics|Playground|SetStationLabelsVisible :WorldContextObject self :bVisible bIsChecked))
