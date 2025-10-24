  kikit panelize \
    --layout 'grid; rows: 2; cols: 3; vspace: 4.5mm; hspace: 0mm; alternation: rows; rotation: 270deg; hbackbone: 5mm;' \
    --tabs 'annotation' \
    --framing 'frame; width: 5mm; space: 3mm; cuts: h' \
    --source 'tolerance: 20mm' \
    --cuts 'mousebites; drill: 0.5mm; spacing: 1mm; offset: 0.2mm; prolong: 0.5mm' \
    --post 'millradius: 0.5mm; copperfill: true' \
    hullocsillag.kicad_pcb panel.kicad_pcb
    
#--tabs 'fixed; width: 3mm; height: 3mm; vcount: 1; hcount: 1' \
