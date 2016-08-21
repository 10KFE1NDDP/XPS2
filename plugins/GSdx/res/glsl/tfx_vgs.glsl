//#version 420 // Keep it for text editor detection

#ifdef VERTEX_SHADER
layout(location = 0) in vec2  i_st;
layout(location = 2) in vec4  i_c;
layout(location = 3) in float i_q;
layout(location = 4) in uvec2 i_p;
layout(location = 5) in uint  i_z;
layout(location = 6) in uvec2 i_uv;
layout(location = 7) in vec4  i_f;

out SHADER
{
    vec4 t_float;
    vec4 t_int;
    vec4 c;
    flat vec4 fc;
} VSout;

const float exp_min32 = exp2(-32.0f);

void texture_coord()
{
    vec2 uv = vec2(i_uv);

    // Float coordinate
    VSout.t_float.xy = i_st;
    VSout.t_float.w  = i_q;

    // Integer coordinate => normalized
    VSout.t_int.xy = uv * TextureScale;
    // Integer coordinate => integral
    VSout.t_int.zw = uv;
}

void vs_main()
{
    highp uint z = i_z & DepthMask;

    // pos -= 0.05 (1/320 pixel) helps avoiding rounding problems (integral part of pos is usually 5 digits, 0.05 is about as low as we can go)
    // example: ceil(afterseveralvertextransformations(y = 133)) => 134 => line 133 stays empty
    // input granularity is 1/16 pixel, anything smaller than that won't step drawing up/left by one pixel
    // example: 133.0625 (133 + 1/16) should start from line 134, ceil(133.0625 - 0.05) still above 133
    vec4 p;

    p.xy = vec2(i_p) - vec2(0.05f, 0.05f);
    p.xy = p.xy * VertexScale - VertexOffset;
    p.w = 1.0f;
    p.z = float(z) * exp_min32;

    gl_Position = p;

    texture_coord();

    VSout.c = i_c;
    VSout.fc = i_c;
    VSout.t_float.z = i_f.x; // pack for with texture
}

#endif

#ifdef GEOMETRY_SHADER

in SHADER
{
    vec4 t_float;
    vec4 t_int;
    vec4 c;
    flat vec4 fc;
} GSin[];

out SHADER
{
    vec4 t_float;
    vec4 t_int;
    vec4 c;
    flat vec4 fc;
} GSout;

struct vertex
{
    vec4 t_float;
    vec4 t_int;
    vec4 c;
};

void out_vertex(in vertex v)
{
    GSout.t_float  = v.t_float;
    GSout.t_int    = v.t_int;
    GSout.c        = v.c;
    // Flat output
#if GS_POINT == 1
    GSout.fc       = GSin[0].fc;
#else
    GSout.fc       = GSin[1].fc;
#endif
    gl_PrimitiveID = gl_PrimitiveIDIn;
    EmitVertex();
}

#if GS_POINT == 1
layout(points) in;
#else
layout(lines) in;
#endif
layout(triangle_strip, max_vertices = 6) out;

#if GS_POINT == 1

void gs_main()
{
    // Transform a point to a NxN sprite
    vertex point = vertex(GSin[0].t_float, GSin[0].t_int, GSin[0].c);

    // Get new position
    vec4 lt_p = gl_in[0].gl_Position;
    vec4 rb_p = gl_in[0].gl_Position + vec4(PointSize.x, PointSize.y, 0.0f, 0.0f);
    vec4 lb_p = rb_p;
    vec4 rt_p = rb_p;
    lb_p.x    = lt_p.x;
    rt_p.y    = lt_p.y;

    // Triangle 1
    gl_Position = lt_p;
    out_vertex(point);

    gl_Position = lb_p;
    out_vertex(point);

    gl_Position = rt_p;
    out_vertex(point);
    EndPrimitive();

    // Triangle 2
    gl_Position = lb_p;
    out_vertex(point);

    gl_Position = rt_p;
    out_vertex(point);

    gl_Position = rb_p;
    out_vertex(point);
    EndPrimitive();
}

#elif GS_LINE == 1

void gs_main()
{
    // Transform a line to a thick line-sprite
    vertex right = vertex(GSin[1].t_float, GSin[1].t_int, GSin[1].c);
    vertex left  = vertex(GSin[0].t_float, GSin[0].t_int, GSin[0].c);
    vec4 lt_p = gl_in[0].gl_Position;
    vec4 rt_p = gl_in[1].gl_Position;

    // Potentially there is faster math
    vec2 line_vector = normalize(rt_p.xy - lt_p.xy);
    vec2 line_normal = vec2(line_vector.y, -line_vector.x);
    vec2 line_width  = line_normal * PointSize;

    vec4 lb_p = gl_in[0].gl_Position + vec4(line_width, 0.0f, 0.0f);
    vec4 rb_p = gl_in[1].gl_Position + vec4(line_width, 0.0f, 0.0f);

    // Triangle 1
    gl_Position = lt_p;
    out_vertex(left);

    gl_Position = lb_p;
    out_vertex(left);

    gl_Position = rt_p;
    out_vertex(right);
    EndPrimitive();

    // Triangle 2
    gl_Position = lb_p;
    out_vertex(left);

    gl_Position = rt_p;
    out_vertex(right);

    gl_Position = rb_p;
    out_vertex(right);
    EndPrimitive();
}

#else

void gs_main()
{
    // left top     => GSin[0];
    // right bottom => GSin[1];
    vertex rb = vertex(GSin[1].t_float, GSin[1].t_int, GSin[1].c);
    vertex lt = vertex(GSin[0].t_float, GSin[0].t_int, GSin[0].c);

    vec4 rb_p = gl_in[1].gl_Position;
    vec4 lb_p = rb_p;
    vec4 rt_p = rb_p;
    vec4 lt_p = gl_in[0].gl_Position;

    // flat depth
    lt_p.z = rb_p.z;
    // flat fog and texture perspective
    lt.t_float.zw = rb.t_float.zw;
    // flat color
    lt.c = rb.c;

    // Swap texture and position coordinate
    vertex lb    = rb;
    lb.t_float.x = lt.t_float.x;
    lb.t_int.x   = lt.t_int.x;
    lb.t_int.z   = lt.t_int.z;
    lb_p.x       = lt_p.x;

    vertex rt    = rb;
    rt_p.y       = lt_p.y;
    rt.t_float.y = lt.t_float.y;
    rt.t_int.y   = lt.t_int.y;
    rt.t_int.w   = lt.t_int.w;

    // Triangle 1
    gl_Position = lt_p;
    out_vertex(lt);

    gl_Position = lb_p;
    out_vertex(lb);

    gl_Position = rt_p;
    out_vertex(rt);
    EndPrimitive();

    // Triangle 2
    gl_Position = lb_p;
    out_vertex(lb);

    gl_Position = rt_p;
    out_vertex(rt);

    gl_Position = rb_p;
    out_vertex(rb);
    EndPrimitive();
}

#endif

#endif
