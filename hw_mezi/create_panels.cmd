@echo off

kikit panelize ^
--layout "grid; rows:2; cols:3; vspace:5mm; hspace:-4mm; rotation:90deg; hbackbone:5mm;" ^
--tabs "annotation" ^
--framing "frame; width:5mm; space:3mm; cuts:h" ^
--cuts "mousebites; drill:0.5mm; spacing: 1mm; offset:-0.5mm; prolong:0.5mm" ^
--source "tolerance:20mm" ^
--post "millradius:0.5mm; copperfill: true" ^
mezi.kicad_pcb panel.kicad_pcb
    
REM --tabs 'fixed; width: 3mm; height: 3mm; vcount: 1; hcount: 1' \
