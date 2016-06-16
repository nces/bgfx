$input v_color0, v_dist

/*
 * Copyright 2011-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "../common/common.sh"

//TODO figure out how to set custom uniforms in bgfx
//uniform vec4 u_WIRE_COL; // edge color
const vec4 u_WIRE_COL = vec4(1.0,1.0,1.0,1.0); // edge color

// taken from http://www.imm.dtu.dk/~janba/Wireframe/
void main()
{
    // Undo perspective correction.
    vec3 dist_vec = v_dist * gl_FragCoord.w;

    // Compute the shortest distance to the edge
    float d = min(dist_vec.x,min(dist_vec.y,dist_vec.z));

    // Compute line intensity and then fragment color
    float I = exp2(-1.0*d*d);
    gl_FragColor = I*u_WIRE_COL + (1.0 - I)*v_color0;
}
