$input a_position, a_color0, a_tangent, a_bitangent
$output v_color0, v_dist

/*
 * Copyright 2011-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

// We need to hijack some of bgfx's fixed attributes
#define a_pos1 a_tangent
#define a_pos2 a_bitangent

//TODO figure out how to set custom uniforms in bgfx
#define u_WIN_SCALE u_viewRect.zw

#include "../common/common.sh"

// taken from http://www.imm.dtu.dk/~janba/Wireframe/
void main()
{
    // We store the vertex id (0,1, or 2) in the w coord of the vertex
    // which then has to be restored to w=1.
    float swizz = a_position.w;
    vec4 pos = a_position;
    pos.w = 1.0;
    gl_Position   = u_modelViewProj * pos;
    vec4 pos1Proj = u_modelViewProj * vec4(a_pos1, 1.0);
    vec4 pos2Proj = u_modelViewProj * vec4(a_pos2, 1.0);

    // Compute screen space coordinates of each vertex
    vec2 p0 = u_WIN_SCALE * gl_Position.xy / gl_Position.w;
    vec2 p1 = u_WIN_SCALE * pos1Proj.xy / pos1Proj.w;
    vec2 p2 = u_WIN_SCALE * pos2Proj.xy / pos2Proj.w;
    
    // Compute each edge of the triangle
    vec2 v0 = p2 - p1;
    vec2 v1 = p2 - p0;
    vec2 v2 = p1 - p0;

    // Compute 2D area of triangle.
    float area = abs(v1.x*v2.y - v1.y*v2.x);

    // ---
    // The swizz variable tells us which of the three vertices
    // we are dealing with. The ugly comparisons would not be needed if
    // swizz was an int.
    // Computed distance from vertex to line in 2D coords
    if(swizz<0.1)
    {
       v_dist = vec3(area/length(v0),0,0);
    }
    else if(swizz<1.1)
    {
       v_dist = vec3(0,area/length(v1),0);
    }
    else
    {
       v_dist = vec3(0,0,area/length(v2));
    }

    // ----
    // Quick fix to defy perspective correction
    v_dist *= gl_Position.w;

	v_color0 = a_color0;
}
