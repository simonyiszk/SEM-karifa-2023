@echo off

kikit panelize ^
--layout "grid; rows:3; cols:2; vspace:3mm; hspace:3mm; alternation:none; rotation:0deg; hbackbone:5mm;" ^
--tabs "annotation" ^
--framing "frame; width:5mm; space:3mm; cuts:h" ^
--cuts "style:vcuts; cutcurves:true; layer:User.1" ^
--source "tolerance:20mm" ^
--post "millradius:0.5mm; copperfill: true" ^
pumpkin.kicad_pcb panel.kicad_pcb
    
