@echo off

kikit panelize ^
--layout "grid; rows:3; cols:4; vspace:3mm; hspace:3mm; alternation:none; rotation:0deg; hbackbone:5mm;" ^
--tabs "annotation" ^
--framing "frame; width:5mm; space:3mm; cuts:h" ^
--cuts "mousebites; drill:0.5mm; spacing: 0.8mm; offset:-0.5mm; prolong:0.5mm" ^
--source "tolerance:20mm" ^
--post "millradius:0.5mm; copperfill: true" ^
hopehely.kicad_pcb panel.kicad_pcb
    
