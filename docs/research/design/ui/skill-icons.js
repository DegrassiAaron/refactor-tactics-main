(function(){
  function inject(){
    if(document.getElementById('rt-icon-defs'))return;
    var d=document.createElement('div');
    d.id='rt-icon-defs';d.style.display='none';
    d.innerHTML='<svg xmlns="http://www.w3.org/2000/svg">'+
    // PHASE MARKERS (solid)
    sym('mk-dodge','<path d="M12 4 21 20H3Z" fill="currentColor"/>')+
    sym('mk-blast','<path d="M12 2 22 12 12 22 2 12Z" fill="currentColor"/>')+
    sym('mk-move','<rect x="4.5" y="4.5" width="15" height="15" fill="currentColor"/>')+
    sym('mk-react','<path d="M13.5 2 5 14h5.5L9 22l10-13h-6.5Z" fill="currentColor"/>')+
    // TARGET
    sym('t-self',s('M4 8V4h4M20 8V4h-4M4 16v4h4M20 16v4h-4')+s('M7.5 17.5c0-2.6 2-4 4.5-4s4.5 1.4 4.5 4')+'<circle cx="12" cy="9.5" r="2.6" fill="none" stroke="currentColor" stroke-width="1.8"/>')+
    sym('t-unit','<circle cx="12" cy="7.5" r="3.4" fill="none" stroke="currentColor" stroke-width="1.8"/>'+s('M4.5 20c0-4.4 3.3-6.4 7.5-6.4s7.5 2 7.5 6.4'))+
    sym('t-cell','<use href="#rt-Information_CellOnly"></use>')+
    sym('t-direction',s('M3 12h15M13 6.5 19 12l-6 5.5'))+
    sym('t-path','<path d="M4 19c4.5 0 4-6.5 8-6.5 3.2 0 3.2-4.5 6.5-6" fill="none" stroke="currentColor" stroke-width="1.8" stroke-dasharray="3 2.6" stroke-linecap="round"/>'+s('M15.5 3.6 19.8 5.4 18.3 9.8'))+
    sym('b-ally','<use href="#rt-Identity_Ally"></use>')+
    sym('b-enemy','<use href="#rt-Identity_Enemy"></use>')+
    sym('b-any','<path d="M12 4v16M5.1 8l13.8 8M18.9 8 5.1 16" stroke="currentColor" stroke-width="2.2" fill="none" stroke-linecap="round"/>')+
    // SHAPE (filled cells = area)
    sym('s-single','<rect x="4.5" y="4.5" width="15" height="15" fill="none" stroke="currentColor" stroke-width="1.8"/><rect x="8.75" y="8.75" width="6.5" height="6.5" fill="currentColor"/>')+
    sym('s-line','<rect x="2.5" y="9.75" width="4.2" height="4.5" fill="currentColor"/><rect x="7.9" y="9.75" width="4.2" height="4.5" fill="currentColor"/><rect x="13.3" y="9.75" width="4.2" height="4.5" fill="currentColor"/><rect x="18.7" y="9.75" width="4.2" height="4.5" fill="currentColor"/>')+
    sym('s-ray','<rect x="2.5" y="9.75" width="4.2" height="4.5" fill="currentColor"/><rect x="7.9" y="9.75" width="4.2" height="4.5" fill="currentColor"/><rect x="13.3" y="9.75" width="4.2" height="4.5" fill="currentColor"/>'+s('M18.5 8.5 22 12l-3.5 3.5'))+
    sym('s-cone','<path d="M5.5 12 20 4.8v14.4Z" fill="currentColor"/><circle cx="3.2" cy="12" r="1.5" fill="currentColor"/>')+
    sym('s-circle','<circle cx="12" cy="12" r="7.2" fill="currentColor"/>')+
    sym('s-ring','<circle cx="12" cy="12" r="7.2" fill="none" stroke="currentColor" stroke-width="3.6"/>')+
    sym('s-arc','<path d="M4.5 15.5a8.5 8.5 0 0 1 15 0" fill="none" stroke="currentColor" stroke-width="3.4" stroke-linecap="round"/>')+
    sym('s-cross','<path d="M9.4 4h5.2v5.4H20v5.2h-5.4V20H9.4v-5.4H4V9.4h5.4Z" fill="currentColor"/>')+
    sym('s-rect','<rect x="4" y="7.5" width="16" height="9" fill="currentColor"/>')+
    sym('s-wall','<rect x="3.5" y="7" width="4.6" height="10" fill="currentColor"/><rect x="9.7" y="7" width="4.6" height="10" fill="currentColor"/><rect x="15.9" y="7" width="4.6" height="10" fill="currentColor"/>')+
    sym('s-chain',s('M4.5 17.5 12 9.5l7.5 7')+'<circle cx="4.5" cy="17.5" r="2.4" fill="currentColor"/><circle cx="12" cy="9.5" r="2.4" fill="currentColor"/><circle cx="19.5" cy="16.5" r="2.4" fill="currentColor"/>')+
    sym('s-region','<path d="M4 9.5h5.5V4H16v6h4v6.5h-6V21H7.5v-5H4Z" fill="currentColor"/>')+
    // DELIVERY
    sym('d-direct','<path d="M3 12h14M13 7.5l4.5 4.5-4.5 4.5" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/>')+
    sym('d-projectile','<circle cx="17.5" cy="12" r="3" fill="currentColor"/><path d="M2.5 12h2.6M8 12h2.6M12.6 12h1.2" stroke="currentColor" stroke-width="2" fill="none" stroke-linecap="round"/>')+
    sym('d-beam','<rect x="2.5" y="10.2" width="19" height="3.6" fill="currentColor"/><path d="M2.5 6.8h19M2.5 17.2h19" stroke="currentColor" stroke-width="1.1" fill="none" opacity="0.65"/>')+
    sym('d-lob','<path d="M3.5 18.5C6 7 18 7 20.3 15" fill="none" stroke="currentColor" stroke-width="1.8" stroke-dasharray="3 2.6" stroke-linecap="round"/><circle cx="20.5" cy="17.6" r="2.2" fill="currentColor"/>')+
    sym('d-ground',s('M2.5 19.5h19')+'<path d="M4 15h8.5" stroke="currentColor" stroke-width="1.8" fill="none" stroke-dasharray="3 2.4" stroke-linecap="round"/>'+s('M10.8 11.8 14.5 15l-3.7 3.2'))+
    // EFFECT
    sym('e-damage','<use href="#rt-Action_BasicAttack"></use>')+
    sym('e-shield','<use href="#rt-Action_Shield"></use>')+
    sym('e-armor','<rect x="4.5" y="5" width="15" height="14" rx="1" fill="none" stroke="currentColor" stroke-width="1.8"/><path d="M4.5 12h15" stroke="currentColor" stroke-width="1.4" fill="none"/><circle cx="8" cy="8.5" r="1" fill="currentColor"/><circle cx="16" cy="8.5" r="1" fill="currentColor"/><circle cx="8" cy="15.5" r="1" fill="currentColor"/><circle cx="16" cy="15.5" r="1" fill="currentColor"/>')+
    sym('e-heal','<use href="#rt-Action_Heal"></use>')+
    sym('e-push','<use href="#rt-Action_Push"></use>')+
    sym('e-pull','<use href="#rt-Action_Pull"></use>')+
    sym('e-displace',s('M4 8h11M11.5 4.5 15.5 8l-4 3.5')+s('M20 16H9M12.5 12.5 8.5 16l4 3.5'))+
    sym('e-wet','<use href="#rt-Status_Wet"></use>')+
    sym('e-electric','<use href="#rt-Status_Electrified"></use>')+
    sym('e-fire','<use href="#rt-Environment_Fire"></use>')+
    sym('e-status','<use href="#rt-Status_Marked"></use>')+
    sym('e-cover','<use href="#rt-Action_CreateCover"></use>')+
    sym('e-terrain','<use href="#rt-Action_CreateWater"></use>')+
    sym('e-graph','<use href="#rt-Action_ModifyArc"></use>')+
    sym('e-utility','<use href="#rt-Action_Interact"></use>')+
    // VALUES
    sym('v-range',s('M4 5.5v13M4 12h14.5M14 7.5l4.5 4.5-4.5 4.5'))+
    sym('v-radius','<circle cx="12" cy="12" r="8" fill="none" stroke="currentColor" stroke-width="1.8"/><circle cx="12" cy="12" r="1.5" fill="currentColor"/>'+s('M12 12h8'))+
    sym('v-duration',s('M6.5 3.5h11M6.5 20.5h11')+s('M7.5 3.5c0 4.5 3 5.5 3 8.5s-3 4-3 8.5M16.5 3.5c0 4.5-3 5.5-3 8.5s3 4 3 8.5'))+
    sym('v-cooldown','<use href="#rt-Warning_Cooldown"></use>')+
    sym('v-targets','<circle cx="7" cy="8" r="2.4" fill="none" stroke="currentColor" stroke-width="1.8"/><circle cx="17" cy="8" r="2.4" fill="none" stroke="currentColor" stroke-width="1.8"/><circle cx="12" cy="15.5" r="2.4" fill="none" stroke="currentColor" stroke-width="1.8"/>')+
    sym('v-hits','<path d="M8 4l1.4 4 4.1 1.5L9.4 11 8 15 6.6 11 2.5 9.5 6.6 8Z" fill="currentColor"/><path d="M16.5 9l1.2 3.4 3.8 1.1-3.8 1.3-1.2 3.7-1.2-3.7-3.4-1.3 3.4-1.1Z" fill="currentColor"/>')+
    // MODIFIERS
    sym('m-pierce','<path d="M9.5 4.5v15M14.5 4.5v15" stroke="currentColor" stroke-width="1.8" fill="none" stroke-linecap="round"/>'+s('M2 12h17.5M16.5 9 20.5 12l-4 3'))+
    sym('m-stopfirst',s('M2.5 12H13M10 9l3.5 3-3.5 3')+'<path d="M18 5v14" stroke="currentColor" stroke-width="3" fill="none" stroke-linecap="round"/>')+
    sym('m-hitall',s('M2 12h20')+'<circle cx="7" cy="12" r="2" fill="currentColor"/><circle cx="12.5" cy="12" r="2" fill="currentColor"/><circle cx="18" cy="12" r="2" fill="currentColor"/>')+
    sym('m-bounce',s('M2.5 20.5h19')+s('M3.5 18 9 8l4 8 5.5-9.5'))+
    sym('m-ignorecover','<rect x="10" y="14.5" width="5.5" height="6" fill="none" stroke="currentColor" stroke-width="1.8"/><path d="M3 16.5c1.6-8.5 13.5-9 16.3-2" fill="none" stroke="currentColor" stroke-width="1.8" stroke-dasharray="3 2.4" stroke-linecap="round"/><path d="M21.3 16.8l-3.9.3 1.7-3.5Z" fill="currentColor"/>')+
    sym('m-destroycover',s('M3 20.5h18')+'<rect x="6" y="9" width="12" height="11.5" fill="none" stroke="currentColor" stroke-width="1.8"/>'+s('M12 9l-2 4h4l-2 4.5'))+
    sym('m-friendly','<use href="#rt-Warning_FriendlyFire"></use>')+
    sym('m-ap','<rect x="8" y="6.5" width="8" height="11" fill="none" stroke="currentColor" stroke-width="1.8"/>'+s('M2 12h18.5M17.5 9.5 21 12l-3.5 2.5'))+
    sym('m-shred','<rect x="4.5" y="5" width="15" height="14" rx="1" fill="none" stroke="currentColor" stroke-width="1.8"/>'+s('M12 5l-2.3 4.6h4.6L11.5 19')+'<circle cx="7.5" cy="8" r="0.9" fill="currentColor"/><circle cx="16.5" cy="16" r="0.9" fill="currentColor"/>')+
    sym('m-eye','<use href="#rt-Action_Overwatch"></use>')+
    '</svg>';
    document.body.appendChild(d);
  }
  function sym(id,inner){return '<symbol id="'+id+'" viewBox="0 0 24 24">'+inner+'</symbol>';}
  function s(d){return '<path d="'+d+'" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>';}
  if(document.body){inject();}else{document.addEventListener('DOMContentLoaded',inject);}
})();
