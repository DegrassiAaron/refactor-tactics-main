# CLAUDE DESIGN 02 — Icon Manifest v0.1

## Come leggere lo scope

- **CORE** — creare per la v0.1 HUD.
- **CORE_IF_SHOWN** — creare se il widget/slot è visibile nella build v0.1; utile per il catalogo completo.
- **CONDITIONAL_V01** — design-ready, ma non deve implicare che la feature gameplay sia già attiva.
- **REFERENCE/FUTURE** — non deve bloccare il pacchetto v0.1.
- **LEGACY_CHECK** — produrre solo se il repository corrente conferma ancora il consumer.

---

# A. Universal / Skill Bar actions

| Scope | Semantic ID | Label | Categoria | Silhouette / grammatica | Accent |
|---|---|---|---|---|---|
| CORE | `UI.Icon.Action.BasicAttack` | Basic Attack | Action | reticle + impact dot; generic, no weapon silhouette | Attack |
| CORE | `UI.Icon.Action.Move` | Move | Movement | `●──•──•──►` | Movement |
| CORE | `UI.Icon.Action.Sprint` | Sprint | Movement | long-stride route + speed trails | Movement |
| CORE | `UI.Icon.Action.Dash` | Dash | Movement | `●──»──»──►` | Movement |
| CORE | `UI.Icon.Action.Brace` | Brace | Defense/Reaction | anchor/braced stance, NOT shield | Defense/Reaction |
| CORE | `UI.Icon.Action.Overwatch` | Overwatch | Reaction/Control | eye/reticle + facing sector | Reaction |
| CORE | `UI.Icon.Action.Interact` | Interact | Utility | hand/device contact + node | Utility |
| CORE | `UI.Icon.Action.Wait` | Wait | Utility | neutral hourglass/pause cycle | Neutral |
| CORE | `UI.Icon.Coordination.Ready` | Ready | Coordination | locked chevron/check state | Gold/Primary |
| CORE | `UI.Icon.Coordination.Unready` | Unready | Coordination | open/unlocked variant | Neutral |

### Compatibility checks

| Scope | Semantic ID | Note |
|---|---|---|
| LEGACY_CHECK | `UI.Icon.Action.Guard` | Alcuni documenti storici/repo possono ancora referenziarlo; non confondere con Brace. |
| LEGACY_CHECK | `UI.Icon.Action.Activate` | Produrre solo se esiste consumer v0.1 distinto da Interact. |
| CONDITIONAL_V01 | `UI.Icon.Action.Sneak` | Movement profile; produrre se viene esposto come scelta HUD. |

---

# B. Target primitives

| Scope | ID | Label | Silhouette |
|---|---|---|---|
| CORE | `UI.Icon.Target.Self` | Self | unit marker + inward focus |
| CORE | `UI.Icon.Target.Ally` | Ally | rounded unit marker + connection tabs |
| CORE | `UI.Icon.Target.Enemy` | Enemy | angular unit marker + reticle notch |
| CORE | `UI.Icon.Target.Cell` | Cell | clean 2D hex |
| CORE | `UI.Icon.Target.Object` | Object | anchored object block |
| CORE | `UI.Icon.Target.Direction` | Direction | directional chevron/sector anchor |
| CORE_IF_SHOWN | `UI.Icon.Target.Structure` | Structure | barrier/device anchor |
| CORE_IF_SHOWN | `UI.Icon.Target.Objective` | Objective | neutral objective diamond/hex core |

---

# C. Geometry primitives

| Scope | ID | Label | Silhouette |
|---|---|---|---|
| CORE | `UI.Icon.Geometry.Line` | Line | origin + segment + arrowhead |
| CORE | `UI.Icon.Geometry.Circle` | Circle AoE | ring + center point |
| CORE | `UI.Icon.Geometry.Cone` | Cone/Sector | origin + diverging edges + curved boundary |
| CORE | `UI.Icon.Geometry.Chain` | Chain | nodes with chained jumps; NOT a lone bolt |
| CORE_IF_SHOWN | `UI.Icon.Geometry.Arc` | Arc | controlled curved sector |
| CORE_IF_SHOWN | `UI.Icon.Geometry.Path` | Path | route with intermediate nodes |
| CORE_IF_SHOWN | `UI.Icon.Geometry.Edge` | Edge | two-cell boundary / transition edge |

---

# D. Effect primitives

| Scope | ID | Label | Silhouette |
|---|---|---|---|
| CORE | `UI.Icon.Effect.Damage` | Damage | abstract impact/crack |
| CORE | `UI.Icon.Effect.Heal` | Heal | soft recovery pulse/cross |
| CORE | `UI.Icon.Effect.Push` | Push | impulse + unit + outgoing arrow |
| CORE_IF_SHOWN | `UI.Icon.Effect.Pull` | Pull | inverse impulse |
| CORE | `UI.Icon.Effect.Shield` | Shield | geometric shield |
| CORE | `UI.Icon.Effect.Reposition` | Reposition | short origin→destination shift |
| CORE | `UI.Icon.Effect.Interrupt` | Interrupt | broken action/line |
| CORE_IF_SHOWN | `UI.Icon.Effect.StopMovement` | Stop Movement | path ending in hard stop |
| CORE | `UI.Icon.Effect.CreateCover` | Create Cover | plus/create + barrier, but avoid Heal cross collision |
| CORE | `UI.Icon.Effect.ModifyCover` | Modify Cover | barrier + rotate/shift cue |
| CORE_IF_SHOWN | `UI.Icon.Effect.Reveal` | Reveal | sensor pulse + visible target |
| CORE_IF_SHOWN | `UI.Icon.Effect.Cleanse` | Cleanse | remove-status ring/slash |
| CORE_IF_SHOWN | `UI.Icon.Effect.Mark` | Mark | target pin/diamond mark |
| CORE_IF_SHOWN | `UI.Icon.Effect.LockFacing` | Lock Facing | facing arrow + lock |

---

# E. Character Kit — Flux

Current v0.1 naming to support in slot art.

| Scope | Ability ID | Label | Slot-art grammar |
|---|---|---|---|
| CORE | `Flux.ArcPulse` | Arc Pulse | Basic Attack + Electric; short pulse/bolt impact |
| CORE | `Flux.LinearDischarge` | Linear Discharge | Line + Electric; clear linear discharge |
| CORE_IF_SHOWN | `Flux.ConductiveNode` | Conductive Node | Cell/Object + Conductive + Electric setup; node/network cue |
| CORE | `Flux.Overload` | Overload | Circle + Electric + Damage; radial surge |
| CORE | `Flux.ReactiveCapacitor` | Reactive Capacitor | Reaction + Shield + Electric return cue |

Do not create a lightning-bolt-only icon for every Flux ability. Differentiate geometry and function.

---

# F. Character Kit — Riva

| Scope | Ability ID | Label | Slot-art grammar |
|---|---|---|---|
| CORE | `Riva.PressureJet` | Pressure Jet | Line + Water + Push |
| CORE | `Riva.CircularTide` | Circular Tide | Circle + Water + Heal/Support |
| CORE | `Riva.FluidTrail` | Fluid Trail | Dash + Water trail |
| CORE | `Riva.MistVeil` | Mist Veil | Circle/Area + Water→Smoke/Obscure |
| CONDITIONAL_V01 | `Riva.FlowReaction` | Flow Reaction | Reaction + Reposition + Water |

---

# G. Character Kit — Bastion

| Scope | Ability ID | Label | Slot-art grammar |
|---|---|---|---|
| CORE | `Bastion.ImpactShot` | Impact Shot | Basic Attack + Kinetic impact |
| CORE_IF_SHOWN | `Bastion.KineticPanel` | Kinetic Panel | Create Cover + directional panel |
| CORE_IF_SHOWN | `Bastion.Reconfigure` | Reconfigure | Modify Cover + rotate/shift |
| CORE | `Bastion.Ram` | Ram | Charge/Dash + Push + Damage |
| CORE | `Bastion.Interposition` | Interposition | Reaction + Ally + redirect/intercept |

Brace action must not reuse Kinetic Panel or Interposition art.

---

# H. Character Kit — Vektor

| Scope | Ability ID | Label | Slot-art grammar |
|---|---|---|---|
| CORE | `Vektor.PulseShot` | Pulse Shot | Basic Attack + kinetic pulse |
| CONDITIONAL_V01 | `Vektor.InterceptShot` | Intercept Shot | Overwatch/Reaction + controlled cell/line + stop movement |
| CORE | `Vektor.PassingBlade` | Passing Blade | Dash + Line + Damage |
| CORE | `Vektor.Deflection` | Deflection | Reaction + deflect/impact redirect; not Shield pool |
| CORE_IF_SHOWN | `Vektor.Feint` | Feint | Prediction + Cell mark + Reposition |

---

# I. Environment / Surface / Terrain

| Scope | ID | Label | Note |
|---|---|---|---|
| CORE | `UI.Icon.Surface.Floor` | Floor | neutral baseline tile |
| CORE | `UI.Icon.Surface.Rough` | Rough | broken/rough ground |
| CORE | `UI.Icon.Surface.Water` | Water / Shallow Water | cell + water droplet/wave |
| CORE | `UI.Icon.Surface.Conductive` | Conductive | cell + conductive node/grid |
| CORE | `UI.Icon.Surface.Fire` | Fire | cell + flame |
| CORE | `UI.Icon.Surface.Smoke` | Smoke | cell + obscuring cloud |
| CORE | `UI.Icon.Surface.Ice` | Ice | cell + crystal/slip cue |
| CORE | `UI.Icon.Surface.HighGround` | High Ground | layer/elevation chevron |
| CORE | `UI.Icon.Environment.Water` | Water payload | droplet |
| CORE | `UI.Icon.Environment.Electric` | Electric payload | max-3-segment bolt |
| CORE | `UI.Icon.Environment.Fire` | Fire payload | flame |
| CORE | `UI.Icon.Environment.Ice` | Ice payload | crystal / angular frost |
| CORE | `UI.Icon.Environment.Smoke` | Smoke payload | cloud/opacity cue |
| CORE_IF_SHOWN | `UI.Icon.Environment.Steam` | Steam | vapor/water+heat; only if active in build |
| CORE | `UI.Icon.Environment.Cover` | Cover | directional barrier |
| CORE | `UI.Icon.Environment.Hazard` | Hazard | dedicated hazard shape + `!`/notch |
| CORE | `UI.Icon.Environment.Height` | Height | elevation stack/up arrow |

---

# J. Status icons

Status icon = unit/state framing, not surface icon recolored.

| Scope | ID | Label | Grammar |
|---|---|---|---|
| CORE | `UI.Icon.Status.Wet` | Wet | unit/status ring + droplet |
| CORE | `UI.Icon.Status.Electrified` | Electrified/Shocked | unit/status ring + electric payload |
| CORE | `UI.Icon.Status.Burning` | Burning | unit/status ring + flame |
| CORE | `UI.Icon.Status.Shielded` | Shielded | status ring + shield |
| CORE | `UI.Icon.Status.Guarded` | Guarded/Defended | defensive posture marker; verify label |
| CORE | `UI.Icon.Status.Stunned` | Stunned | interrupted focus / impact stars simplified |
| CORE | `UI.Icon.Status.Interrupted` | Interrupted | broken-action glyph |
| CORE_IF_SHOWN | `UI.Icon.Status.Slowed` | Slowed | movement ↓ |
| CORE_IF_SHOWN | `UI.Icon.Status.Suppressed` | Suppressed | threat/line + movement ↓ |
| CORE_IF_SHOWN | `UI.Icon.Status.Marked` | Marked | target mark |
| CORE_IF_SHOWN | `UI.Icon.Status.Rooted` | Rooted | unit + ground anchor |
| CORE_IF_SHOWN | `UI.Icon.Status.Anchored` | Anchored | unit + anchor/anti-push |
| CORE_IF_SHOWN | `UI.Icon.Status.Obscured` | Obscured | eye/visibility crossed by smoke veil |
| CORE | `UI.Icon.Status.LowHealth` | Low Health | HP/critical shape, not just red |
| CORE | `UI.Icon.Status.KO` | KO | disabled/down state |

---

# K. Reaction system icons/states

| Scope | ID | Label | Visual state |
|---|---|---|---|
| CORE | `UI.Icon.Reaction.Generic` | Reaction | broken ring + trigger notch |
| CORE | `UI.Icon.Reaction.Available` | Available | open armed ring |
| CORE | `UI.Icon.Reaction.Armed` | Armed | closed/charged trigger ring |
| CORE | `UI.Icon.Reaction.Opportunity` | Opportunity | ring + active target notch |
| CORE | `UI.Icon.Reaction.Commit` | Commit / Fire | commit target mark |
| CORE | `UI.Icon.Reaction.Hold` | Hold | open palm/retain marker |
| CORE | `UI.Icon.Reaction.Consumed` | Consumed | depleted ring |
| CORE | `UI.Icon.Reaction.Expired` | Expired | faded/broken timeout ring |
| CORE | `UI.Icon.Reaction.Invalidated` | Invalidated | reaction ring + slash |
| CORE_IF_SHOWN | `UI.Icon.Reaction.Timeout` | Timeout | deadline marker; UI timer number dynamic |
| CORE | `UI.Icon.Decision.FastReaction` | Fast Reaction | reaction ring + decision marker |
| CORE_IF_SHOWN | `UI.Icon.Decision.FastAction` | Fast Action | action continuation mark distinct from reaction |

---

# L. Information / Perception

These assets are useful even when some advanced consumers are deferred. Do not imply exact enemy position unless knowledge allows it.

| Scope | ID | Label | Grammar |
|---|---|---|---|
| CORE_IF_SHOWN | `UI.Icon.Intel.Visible` | Visible | full eye/visual contact |
| CONDITIONAL_V01 | `UI.Icon.Intel.Detected` | Detected | sensor/contact marker |
| CONDITIONAL_V01 | `UI.Icon.Intel.Heard` | Heard / Sound Contact | sound-wave contact; no enemy silhouette at exact cell |
| CONDITIONAL_V01 | `UI.Icon.Intel.Approximate` | Approximate Area | `?` + area/contact boundary |
| CORE_IF_SHOWN | `UI.Icon.Intel.Unknown` | Unknown | unknown/contact void mark |
| CORE_IF_SHOWN | `UI.Icon.Intel.LastKnown` | Last Known | historical pin + fade/clock-tail, not current contact |
| CORE | `UI.Icon.Intel.Targeted` | Targeted | target brackets |
| CORE | `UI.Icon.Intel.Focus` | Focus | focus brackets / magnified marker |
| CORE | `UI.Icon.Intel.Facing` | Facing | 6-direction facing anchor |

---

# M. Coordination / Team planning

| Scope | ID | Label |
|---|---|---|
| CORE | `UI.Icon.Coordination.Ready` | Ready |
| CORE | `UI.Icon.Coordination.Unready` | Unready |
| CORE | `UI.Icon.Coordination.Editing` | Editing |
| CORE_IF_SHOWN | `UI.Icon.Coordination.Locked` | Locked |
| CORE | `UI.Icon.Coordination.AllyIntent` | Ally Intent |
| CORE | `UI.Icon.Coordination.Ping` | Ping / Attention |
| CORE | `UI.Icon.Coordination.Conflict` | Conflict |
| CORE_IF_SHOWN | `UI.Icon.Coordination.SharedTarget` | Shared Target |
| CORE_IF_SHOWN | `UI.Icon.Coordination.Label` | Tactical Label |

Team identity is separate from faction identity.

---

# N. Certainty / validity modifiers

| Scope | ID | Label | Renderer style |
|---|---|---|---|
| CORE | `UI.Style.Certainty.Confirmed` | Confirmed | solid |
| CORE | `UI.Style.Certainty.Predicted` | Predicted | dashed/hollow |
| CORE | `UI.Style.Certainty.Uncertain` | Uncertain | dotted/fade + `?` |
| CORE | `UI.Style.Validity.Invalid` | Invalid | slash/cross-hatch/`⊘` |

These must exist as masks/pattern assets where useful, not as full duplicate icons for every semantic ID.

---

# O. Warning icons

| Scope | ID | Label |
|---|---|---|
| CORE | `UI.Icon.Warning.Info` | Info |
| CORE | `UI.Icon.Warning.Warning` | Warning |
| CORE | `UI.Icon.Warning.Critical` | Critical |
| CORE | `UI.Icon.Warning.FriendlyFire` | Friendly Fire |
| CORE | `UI.Icon.Warning.Collision` | Ally Collision |
| CORE | `UI.Icon.Warning.InsufficientResource` | Insufficient Resource |
| CORE | `UI.Icon.Warning.InvalidTarget` | Invalid Target |
| CORE | `UI.Icon.Warning.InvalidPath` | Invalid Path |
| CORE | `UI.Icon.Warning.Cooldown` | Cooldown |
| CORE | `UI.Icon.Warning.IntentNotCommitted` | Intent Not Committed |
| CORE | `UI.Icon.Warning.PathInvalidated` | Path Invalidated |
| CORE | `UI.Icon.Warning.UncertainOutcome` | Uncertain Outcome |
| CORE | `UI.Icon.Warning.Hazard` | Hazard |
| CORE_IF_SHOWN | `UI.Icon.Warning.PlanChanged` | Plan Changed |
| CORE_IF_SHOWN | `UI.Icon.Warning.MissingPlan` | Missing Plan |
| CORE_IF_SHOWN | `UI.Icon.Warning.TargetMayMove` | Target May Move |

Every warning should support:

- icon-only;
- compact chip;
- full warning row.

---

# P. Objective states

| Scope | ID | Label |
|---|---|---|
| CORE | `UI.Icon.Objective.Neutral` | Neutral |
| CORE | `UI.Icon.Objective.Capture` | Capture |
| CORE | `UI.Icon.Objective.Contested` | Contested |
| CORE | `UI.Icon.Objective.Owned` | Owned |
| CORE_IF_SHOWN | `UI.Icon.Objective.Locked` | Locked |
| CORE | `UI.Icon.Objective.Completed` | Completed |
| CORE_IF_SHOWN | `UI.Icon.Objective.Progress` | Progress |
| CORE_IF_SHOWN | `UI.Icon.Objective.Timer` | Objective Timer |
| CORE | `UI.Icon.Result.Success` | Mission Success |
| CORE | `UI.Icon.Result.Failure` | Mission Failure |

---

# Q. Map / Interaction icons

| Scope | ID | Label |
|---|---|---|
| CORE | `UI.Icon.Map.Cover` | Cover |
| CORE_IF_SHOWN | `UI.Icon.Map.Door` | Door |
| CORE_IF_SHOWN | `UI.Icon.Map.Switch` | Switch |
| CORE_IF_SHOWN | `UI.Icon.Map.Trigger` | Trigger |
| CORE_IF_SHOWN | `UI.Icon.Map.Bridge` | Bridge |
| CONDITIONAL_V01 | `UI.Icon.Map.Tunnel` | Tunnel |
| CONDITIONAL_V01 | `UI.Icon.Map.Elevator` | Elevator |
| CORE_IF_SHOWN | `UI.Icon.Map.LayerUp` | Layer Up |
| CORE_IF_SHOWN | `UI.Icon.Map.LayerDown` | Layer Down |
| CORE | `UI.Icon.Map.Relay` | Relay / Objective device |
| CORE_IF_SHOWN | `UI.Icon.Map.Structure` | Structure |
| CORE_IF_SHOWN | `UI.Icon.Map.ConductiveNode` | Conductive Node |

---

# R. Tactical markers

| Scope | ID | Label |
|---|---|---|
| CORE | `UI.Marker.Confirmed` | Confirmed marker |
| CORE | `UI.Marker.Predicted` | Predicted marker |
| CORE | `UI.Marker.Uncertain` | Uncertain marker |
| CORE | `UI.Marker.Waypoint` | Waypoint |
| CORE | `UI.Marker.Destination` | Destination |
| CORE_IF_SHOWN | `UI.Marker.LastContact` | Last Contact |
| CONDITIONAL_V01 | `UI.Marker.SoundContact` | Sound Contact |
| CORE | `UI.Marker.Targeted` | Targeted |
| CORE | `UI.Marker.FocusPing` | Focus/Ping |
| CORE | `UI.Marker.Cover` | Cover relation |
| CORE | `UI.Marker.FacingAnchor` | Facing anchor |

---

# S. Phase / timing / decision icons

| Scope | ID | Label |
|---|---|---|
| CORE | `UI.Icon.Phase.Planning` | Planning |
| CORE | `UI.Icon.Phase.Prep` | Prep |
| CORE | `UI.Icon.Phase.Dash` | Dash Phase |
| CORE | `UI.Icon.Phase.Blast` | Blast |
| CORE | `UI.Icon.Phase.Move` | Move Phase |
| CORE | `UI.Icon.Phase.Cleanup` | Cleanup |
| CORE | `UI.Icon.Boundary.Phase` | Phase Boundary |
| CORE | `UI.Icon.Boundary.Decision` | Decision Boundary |
| CORE | `UI.Icon.Decision.FastReaction` | Fast Reaction |
| CORE_IF_SHOWN | `UI.Icon.Decision.FastAction` | Fast Action |
| REFERENCE/FUTURE | `UI.Icon.Timing.DelayedAction` | Delayed Action |
| REFERENCE/FUTURE | `UI.Icon.Timing.PredictiveAction` | Predictive Action |
| REFERENCE/FUTURE | `UI.Icon.Timing.Trap` | Trap |

---

# T. Equipped item / loadout icons

Produce these if loadout/equipment is visible in v0.1 HUD, character inspect, pre-match or tooltip. They can also be useful for Wiki consistency.

## Weapon variants

| Scope | ID | Label |
|---|---|---|
| CORE_IF_SHOWN | `UI.Icon.Weapon.Precision` | Precision |
| CORE_IF_SHOWN | `UI.Icon.Weapon.Impact` | Impact |
| CORE_IF_SHOWN | `UI.Icon.Weapon.Overcharge` | Overcharge |
| CORE_IF_SHOWN | `UI.Icon.Weapon.Split` | Split |
| CORE_IF_SHOWN | `UI.Icon.Weapon.Suppressive` | Suppressive |
| CORE_IF_SHOWN | `UI.Icon.Weapon.Environmental` | Environmental |

## Gadgets

| Scope | ID | Label |
|---|---|---|
| CORE_IF_SHOWN | `UI.Icon.Gadget.Medkit` | Medkit |
| CORE_IF_SHOWN | `UI.Icon.Gadget.BreachCharge` | Breach Charge |
| CORE_IF_SHOWN | `UI.Icon.Gadget.Sprinkler` | Sprinkler |
| CORE_IF_SHOWN | `UI.Icon.Gadget.Insulator` | Insulator |
| CORE_IF_SHOWN | `UI.Icon.Gadget.SmokeEmitter` | Smoke Emitter |
| CORE_IF_SHOWN | `UI.Icon.Gadget.PortableCover` | Portable Cover |
| CORE_IF_SHOWN | `UI.Icon.Gadget.Sensor` | Sensor |
| CORE_IF_SHOWN | `UI.Icon.Gadget.Anchor` | Anchor |

## Generic reaction modules

| Scope | ID | Label |
|---|---|---|
| CORE_IF_SHOWN | `UI.Icon.Module.EmergencyDash` | Emergency Dash |
| CORE_IF_SHOWN | `UI.Icon.Module.ReactiveShield` | Reactive Shield |
| CORE_IF_SHOWN | `UI.Icon.Module.CounterShot` | Counter Shot |
| CORE_IF_SHOWN | `UI.Icon.Module.AllyIntercept` | Ally Intercept |
| CORE_IF_SHOWN | `UI.Icon.Module.HazardEscape` | Hazard Escape |
| CORE_IF_SHOWN | `UI.Icon.Module.Cleanse` | Cleanse |
| CORE_IF_SHOWN | `UI.Icon.Module.Anchor` | Reaction Anchor |

---

# U. Faction / Role identity

The repository currently treats Team and Faction as separate visual concepts. Do not use faction color to indicate enemy/ally.

| Scope | ID | Label |
|---|---|---|
| CORE_IF_SHOWN | `UI.Icon.Faction.Conflux` | Conflux |
| CORE_IF_SHOWN | `UI.Icon.Faction.Constrine` | Constrine |
| CORE_IF_SHOWN | `UI.Icon.Role.Controller` | Controller |
| CORE_IF_SHOWN | `UI.Icon.Role.Support` | Support |
| CORE_IF_SHOWN | `UI.Icon.Role.Guardian` | Guardian |
| CORE_IF_SHOWN | `UI.Icon.Role.Striker` | Striker |

Faction badges are secondary in combat HUD and should not appear permanently above every world unit.

---

# V. HUD micro-icons and static symbols

| Scope | ID | Label |
|---|---|---|
| CORE | `UI.Icon.Stat.Health` | Health |
| CORE_IF_SHOWN | `UI.Icon.Stat.Shield` | Shield resource |
| CORE | `UI.Icon.Stat.Resource` | Hero resource |
| CORE | `UI.Icon.Stat.Cooldown` | Cooldown |
| CORE | `UI.Icon.Stat.Charge` | Charge |
| CORE | `UI.Icon.Stat.Range` | Range |
| CORE | `UI.Icon.Stat.Duration` | Duration |
| CORE | `UI.Icon.Stat.Cost` | Cost |
| CORE_IF_SHOWN | `UI.Icon.Stat.Noise` | Noise |
| CORE_IF_SHOWN | `UI.Icon.Stat.Vision` | Vision |
| CORE_IF_SHOWN | `UI.Icon.Stat.Detection` | Detection |
| CORE | `UI.Icon.UI.Undo` | Undo |
| CORE | `UI.Icon.UI.Why` | WHY? / Explain |
| CORE_IF_SHOWN | `UI.Icon.UI.CombatLog` | Combat Log |
| CORE_IF_SHOWN | `UI.Icon.UI.TacticalView` | Tactical View |
| CORE_IF_SHOWN | `UI.Icon.UI.Settings` | Settings |
| CORE_IF_SHOWN | `UI.Icon.UI.Team` | Team / Roster |

Numbers remain dynamic text; do not rasterize numeric values.
