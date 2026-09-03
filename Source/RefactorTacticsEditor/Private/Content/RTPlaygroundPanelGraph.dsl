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
;   Cmb_Facing/OnSelectionChanged (/Script/UMG.ComboBoxString)
;   Btn_SelectFixture/OnClicked · Btn_ResetFixture/OnClicked (/Script/UMG.Button)
; I widget scritti devono essere VARIABILI, altrimenti il loro `Get<Nome>` non esiste. Dalla
; revisione del 2026-09-02 li rende tali il commandlet stesso (`MakeEveryWidgetAVariable`): non
; serve piu' passare da `ToggleWidgetAsVariable` a mano.
;
; ✅ TUTTO CABLATO dal 2026-09-02. Il buco precedente - `Btn_Focus`, i tre preset di camera e
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
    (Widget|SetText(Text) :self txtName
      :InText (Utilities|GetDisplayName (Utilities|Array|Get(acopy) actors 0)))
    (else
      (Widget|SetText(Text) :self txtName :InText "nessun fixture nella mappa aperta"))))

(event OnClicked(Btn_ResetFixture)
  (bind actors (Actor|GetAllActorsOfClass :WorldContextObject self
                 :ActorClass "/Script/RefactorTactics.RTGrayboxUnitFacingFixture"))
  (if (> (Utilities|Array|Length actors) 0)
    (bind fx (Utilities|Casting|CastToRTGrayboxUnitFacingFixture
               :Object (Utilities|Array|Get(acopy) actors 0))
      (:then
        (RefactorTactics|Playground|ResetFixture fx))
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
