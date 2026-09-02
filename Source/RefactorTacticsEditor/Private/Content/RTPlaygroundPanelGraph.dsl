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
; ⛔ COSA NON E' CABLATO, E PERCHE'
; - `Btn_Focus` e i tre preset di camera. Sono cablabili - `FRTPlaygroundStationInfo::CentreWorld`
;   piu' `Development|Editor|SetLevelViewportCameraInfo` - ma servono la station selezionata, che si
;   legge con `ComboBox|GetSelectedOption`: esiste in DUE varianti con lo STESSO type_id
;   (`UComboBoxKey` e `UComboBoxString`) e il DSL prende la prima, quindi non e' raggiungibile da qui.
;   `create_node` saprebbe disambiguare (`declaring_class`), `write_graph_dsl` no.
; - `Cmb_Station/OnSelectionChanged`, che pero' NON ha quel problema: l'evento porta `SelectedItem`
;   come parametro. E' semplicemente non ancora scritto.
;
; ➕ Le OPZIONI delle due combo non sono piu' un buco: le riempie il commandlet in `DefaultOptions`,
; dal modello, e `Playground.PanelComboOptionsComeFromTheModel` lo verifica voce per voce.

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
