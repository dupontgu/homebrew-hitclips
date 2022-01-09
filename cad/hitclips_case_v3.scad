
pcb_width = 20;
pcb_thickness = 0.83;
depth_offset = 0.5;
sd_thickness = 1.65;
samd_thickness = 1.65 + depth_offset;
total_comp_thickness = pcb_thickness + sd_thickness + samd_thickness;
buffer_scale = 1.01;

// clearance on most players is 6mm, leave a bit of wiggle room
outer_depth = 5.63;
strap_hole_depth = outer_depth/2;
echo(str("thickness = ", total_comp_thickness));

module pcb() {
    color([0/255, 100/255, 0/255])
    linear_extrude(height=samd_thickness, center=false, convexity=10, twist=0) {
        square(size=[pcb_width, 16.002], center=true); 
    }

    pcb_thickness_buffer = 0.5;
    translate([0, 0, samd_thickness - (pcb_thickness_buffer / 2)])
    linear_extrude(height=pcb_thickness + pcb_thickness_buffer, center=false, convexity=10, twist=0) {
        import("edge_v2.dxf");    
    }

    // spacer for sd
    color([250/255, 0/255, 0/255])
    translate([0, 0, pcb_thickness + samd_thickness]) {
        linear_extrude(height=sd_thickness, center=false, convexity=10, twist=0) {
            square(size=[pcb_width, 16.002], center=true); 
        }
    }
}

module case() {
    linear_extrude(height=outer_depth, center=false, convexity=10, twist=0) {
        square(size=[22.8, 22.8], center=true); 
    }
}

module strap_hole() {
    difference() {
        hull() {
            translate([1.25, 3, 0])
            cylinder(r=2, h=strap_hole_depth, center=false, $fn=100);
            
            root_offset = 0.8;
            translate([0, 0, -root_offset])
            linear_extrude(height=strap_hole_depth + root_offset, center=false, convexity=10, twist=0) {
                square(size=[6.5, 1], center=true); 
            }
        }

        translate([1.25, 3, -outer_depth]) {
            cylinder(r=1, h=outer_depth*2, center=false, $fn=100);
        }
    }
}

module assembly() {
    translate([8, 10, strap_hole_depth]) strap_hole();
    difference() {
        case();
        translate([0, -0, -0.05]) {
            scale([buffer_scale, buffer_scale+0.01, buffer_scale]) {
                translate([0, -2.5, 0])
                pcb();

                // extra negative space next to the PCB
                translate([-(pcb_width/2), -12, 0])
                color([0/255, 0/255, 180/255])
                linear_extrude(height=total_comp_thickness, center=false, convexity=10, twist=0) {
                    square(size=[pcb_width, 21], center=false); 
                }
            }
        }
    }
}

assembly();



