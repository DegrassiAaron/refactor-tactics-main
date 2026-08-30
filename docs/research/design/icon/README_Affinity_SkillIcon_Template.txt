REFACTORTACTICS — AFFINITY SKILL ICON TEMPLATE v0.1
===================================================

CONTENUTO
- RT_Affinity_PrimitiveGlyph_Template_24.svg
  Master per creare i glyph/primitive semanticamente riutilizzabili.
- RT_Affinity_SkillIcon_Template_64.svg
  Template per comporre una skill icon modulare.
- preview_primitive_24.png / preview_skill_64.png
  Preview rapida.

APERTURA IN AFFINITY DESIGNER
1. Apri il file SVG in Affinity Designer.
2. Salvalo subito come .afdesign.
3. Nel pannello Layers mantieni i gruppi nominati.
4. Duplica il documento o il gruppo della skill per ogni nuova icona.
5. Nascondi 00_GUIDES_DO_NOT_EXPORT prima dell'export.

REGOLE DEL MASTER 24x24
- visual bounds preferiti: 20x20;
- core mass: circa 18x18;
- stroke tipico: circa 2 px;
- silhouette-first;
- master monocromatico e tintabile;
- niente glow, testo o cooldown dentro il glyph;
- variante ottica 16 px solo se il downscale perde leggibilità.

MAPPA DEL TEMPLATE SKILL 64x64
- 10_PHASE_MARKER                = quando agisce
- 20_TARGET_SLOT_TOP_LEFT        = chi/cosa seleziona
- 30_SHAPE_SLOT_TOP_RIGHT        = area/geometria
- 40_DELIVERY_SLOT_LEFT          = come raggiunge il target
- 50_PRIMARY_EFFECT_CORE         = effetto principale
- 55_ELEMENT_OR_STATUS_ACCENT    = eventuale elemento/status
- 60_VALUE_RANGE_SLOT_RIGHT      = range/size/valore con simbolo
- 70_MODIFIER_SLOT_BOTTOM_LEFT   = modifier della skill
- 75_SECONDARY_EFFECT_SLOT       = effetto secondario/durata
- 80_CERTAINTY_OVERLAY           = Confirmed / Predicted / Uncertain / Invalid

PHASE MARKER
Il template contiene alternative separate:
- Blast      = rombo
- Dodge      = triangolo
- Move       = quadrato
- Reaction   = broken arc + trigger dot

Mantieni UNA sola variante visibile. Il colore di fase può essere applicato in Affinity
o a runtime in UI, ma non deve essere l'unico canale: il marker geometrico resta.

CERTAINTY
- Confirmed = bordo solido
- Predicted = tratteggiato
- Uncertain = puntinato + ?
- Invalid   = slash/segno dedicato
Non affidarti al solo colore.

WORKFLOW CONSIGLIATO PER UNA NUOVA SKILL
1. Parti dalle primitive 24x24 già esistenti.
2. Duplica RT_Affinity_SkillIcon_Template_64.afdesign.
3. Rinomina il gruppo/documento con lo Skill ID stabile.
4. Attiva il Phase Marker corretto.
5. Inserisci Target + Shape.
6. Inserisci Delivery solo se necessaria a distinguere la skill.
7. Disegna/aggancia il Primary Effect al centro.
8. Aggiungi al massimo i modifier realmente competitivi.
9. Controlla 64, 48 e 32 px.
10. Controlla grayscale.
11. Esporta SVG master + PNG 64/48/32 se richiesto.

NAMING PROPOSTO
UI_Skill_<CharacterOrGeneric>_<SkillId>
Esempio:
UI_Skill_Generic_RailShot
UI_Skill_Generic_Barrier

NOTA IMPORTANTE
Questo file è deliberatamente modulare: non creare un'icona totalmente nuova per ogni
combinazione se la stessa informazione può essere composta da primitive riutilizzabili.
