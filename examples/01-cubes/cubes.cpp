/*
 * Copyright 2011-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "common.h"
#include "bgfx_utils.h"

struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
    float m_w; // vertex ID

    // first neighbor
	float m_x1;
	float m_y1;
	float m_z1;

    // second neighbor
	float m_x2;
	float m_y2;
	float m_z2;

	uint32_t m_abgr; // color

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position,  4, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Tangent,   3, bgfx::AttribType::Float)       // First neighbor
			.add(bgfx::Attrib::Bitangent, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true) // Second neighbor
			.end();
	};

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosColorVertex::ms_decl;

static PosColorVertex s_cubeVertices[8] =
{
	{-1.0f,  1.0f,  1.0f, 0, 0,0,0, 0,0,0, 0xff000000 },
	{ 1.0f,  1.0f,  1.0f, 0, 0,0,0, 0,0,0, 0xff0000ff },
	{-1.0f, -1.0f,  1.0f, 0, 0,0,0, 0,0,0, 0xff00ff00 },
	{ 1.0f, -1.0f,  1.0f, 0, 0,0,0, 0,0,0, 0xff00ffff },
	{-1.0f,  1.0f, -1.0f, 0, 0,0,0, 0,0,0, 0xffff0000 },
	{ 1.0f,  1.0f, -1.0f, 0, 0,0,0, 0,0,0, 0xffff00ff },
	{-1.0f, -1.0f, -1.0f, 0, 0,0,0, 0,0,0, 0xffffff00 },
	{ 1.0f, -1.0f, -1.0f, 0, 0,0,0, 0,0,0, 0xffffffff },
};

static PosColorVertex s_cubeVerticesN0[36];

static const uint16_t s_cubeIndices[36] =
{
	0, 1, 2, // 0
	1, 3, 2,
	4, 6, 5, // 2
	5, 6, 7,
	0, 2, 4, // 4
	4, 2, 6,
	1, 5, 3, // 6
	5, 7, 3,
	0, 4, 1, // 8
	4, 5, 1,
	2, 3, 6, // 10
	6, 3, 7,
};
static uint16_t s_cubeIndices2[36];

class ExampleCubes : public entry::AppI
{
	void init(int _argc, char** _argv) BX_OVERRIDE
	{
		Args args(_argc, _argv);

        // initialize vertex buffers
        for (int i = 0; i < 36; i += 3)
        {
            uint16_t ind  = s_cubeIndices[i];
            uint16_t ind1 = s_cubeIndices[i + 1];
            uint16_t ind2 = s_cubeIndices[i + 2];

            // P0
            s_cubeVerticesN0[i]      = s_cubeVertices[ind];
            s_cubeVerticesN0[i].m_w  = 0;
            // neighbor 1
            s_cubeVerticesN0[i].m_x1 = s_cubeVertices[ind1].m_x;
            s_cubeVerticesN0[i].m_y1 = s_cubeVertices[ind1].m_y;
            s_cubeVerticesN0[i].m_z1 = s_cubeVertices[ind1].m_z;
            // neighbor 2
            s_cubeVerticesN0[i].m_x2 = s_cubeVertices[ind2].m_x;
            s_cubeVerticesN0[i].m_y2 = s_cubeVertices[ind2].m_y;
            s_cubeVerticesN0[i].m_z2 = s_cubeVertices[ind2].m_z;

            // P1
            s_cubeVerticesN0[i + 1]      = s_cubeVertices[ind1];
            s_cubeVerticesN0[i + 1].m_w  = 1;
            // neighbor 1
            s_cubeVerticesN0[i + 1].m_x1 = s_cubeVertices[ind].m_x;
            s_cubeVerticesN0[i + 1].m_y1 = s_cubeVertices[ind].m_y;
            s_cubeVerticesN0[i + 1].m_z1 = s_cubeVertices[ind].m_z;
            // neighbor 2
            s_cubeVerticesN0[i + 1].m_x2 = s_cubeVertices[ind2].m_x;
            s_cubeVerticesN0[i + 1].m_y2 = s_cubeVertices[ind2].m_y;
            s_cubeVerticesN0[i + 1].m_z2 = s_cubeVertices[ind2].m_z;

            // P2
            s_cubeVerticesN0[i + 2]      = s_cubeVertices[ind2];
            s_cubeVerticesN0[i + 2].m_w  = 2;
            // neighbor 1
            s_cubeVerticesN0[i + 2].m_x1 = s_cubeVertices[ind].m_x;
            s_cubeVerticesN0[i + 2].m_y1 = s_cubeVertices[ind].m_y;
            s_cubeVerticesN0[i + 2].m_z1 = s_cubeVertices[ind].m_z;
            // neighbor 2
            s_cubeVerticesN0[i + 2].m_x2 = s_cubeVertices[ind1].m_x;
            s_cubeVerticesN0[i + 2].m_y2 = s_cubeVertices[ind1].m_y;
            s_cubeVerticesN0[i + 2].m_z2 = s_cubeVertices[ind1].m_z;

            s_cubeIndices2[i    ] = i;
            s_cubeIndices2[i + 1] = i + 1;
            s_cubeIndices2[i + 2] = i + 2;
        }

		m_width  = 1280;
		m_height = 720;
		m_debug  = BGFX_DEBUG_TEXT;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::init(args.m_type, args.m_pciId);
		bgfx::reset(m_width, m_height, m_reset);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
				);

		// Create vertex stream declaration.
		PosColorVertex::init();

		// Create static vertex buffer.
		m_vbh = bgfx::createVertexBuffer(
				// Static data can be passed with bgfx::makeRef
				bgfx::makeRef(s_cubeVerticesN0, sizeof(s_cubeVerticesN0) )
				, PosColorVertex::ms_decl
				);

		// Create static index buffer.
		m_ibh = bgfx::createIndexBuffer(
				// Static data can be passed with bgfx::makeRef
				bgfx::makeRef(s_cubeIndices2, sizeof(s_cubeIndices) )
				);

		// Create program from shaders.
		m_program = loadProgram("vs_cubes", "fs_cubes");

        // the predefined u_viewRect uniform will contain this information
        //m_winScaleH = bgfx::createUniform("u_WIN_SCALE", bgfx::UniformType::Vec4);
        //float winScaleVec[4] = { m_width, m_height, 0,0, }; // This should be a vec2, but apparently that doesn't exist in BGFX...
        //bgfx::setUniform(m_winScaleH, winScaleVec);

        // bgfx::setUniform appears to be benign, so this value is currently being set directly in the shader
        m_wireColH = bgfx::createUniform("u_WIRE_COL", bgfx::UniformType::Vec4);
        float wireCol[4] = { 1.0, 1.0, 1.0, 1.0 };
        bgfx::setUniform(m_wireColH, wireCol);

		m_timeOffset = bx::getHPCounter();
	}

	virtual int shutdown() BX_OVERRIDE
	{
		// Cleanup.
		bgfx::destroyIndexBuffer(m_ibh);
		bgfx::destroyVertexBuffer(m_vbh);
		bgfx::destroyProgram(m_program);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() BX_OVERRIDE
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset) )
		{
			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency() );
			const double toMs = 1000.0/freq;

			float time = (float)( (now-m_timeOffset)/double(bx::getHPFrequency() ) );

			// Use debug font to print information about this example.
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/01-cube");
			bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Rendering simple static mesh.");
			bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

			float at[3]  = { 0.0f, 0.0f,   0.0f };
			float eye[3] = { 0.0f, 0.0f, -35.0f };

			// Set view and projection matrix for view 0.
			const bgfx::HMD* hmd = bgfx::getHMD();
			if (NULL != hmd && 0 != (hmd->flags & BGFX_HMD_RENDERING) )
			{
				float view[16];
				bx::mtxQuatTranslationHMD(view, hmd->eye[0].rotation, eye);
				bgfx::setViewTransform(0, view, hmd->eye[0].projection, BGFX_VIEW_STEREO, hmd->eye[1].projection);

				// Set view 0 default viewport.
				//
				// Use HMD's width/height since HMD's internal frame buffer size
				// might be much larger than window size.
				bgfx::setViewRect(0, 0, 0, hmd->width, hmd->height);
			}
			else
			{
				float view[16];
				bx::mtxLookAt(view, eye, at);

				float proj[16];
				bx::mtxProj(proj, 60.0f, float(m_width)/float(m_height), 0.1f, 100.0f);
				bgfx::setViewTransform(0, view, proj);

				// Set view 0 default viewport.
				bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );
			}

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

#define DRAW_GRID
#ifdef DRAW_GRID
			// Submit 11x11 cubes.
			for (uint32_t yy = 0; yy < 11; ++yy)
			{
				for (uint32_t xx = 0; xx < 11; ++xx)
				{
					float mtx[16];
					bx::mtxRotateXY(mtx, time + xx*0.21f, time + yy*0.37f);
					mtx[12] = -15.0f + float(xx)*3.0f;
					mtx[13] = -15.0f + float(yy)*3.0f;
					mtx[14] = 0.0f;

					// Set model matrix for rendering.
					bgfx::setTransform(mtx);

					// Set vertex and index buffer.
					bgfx::setVertexBuffer(m_vbh);
					bgfx::setIndexBuffer(m_ibh);

					// Set render states.
					bgfx::setState(BGFX_STATE_DEFAULT);

					// Submit primitive for rendering to view 0.
					bgfx::submit(0, m_program);
				}
			}
#else
			float mtx[16];
			//bx::mtxScale(mtx, 5.0f, 5.0f, 5.0f);
			bx::mtxRotateXY(mtx, time, time);
//			mtx[12] = -15.0f + float(xx)*3.0f;
//			mtx[13] = -15.0f + float(yy)*3.0f;
			mtx[14] = 0.0f;

			// Set model matrix for rendering.
			bgfx::setTransform(mtx);

			bgfx::setVertexBuffer(m_vbh);
			bgfx::setIndexBuffer(m_ibh, 0, 12);
			bgfx::setState(BGFX_STATE_DEFAULT);
			bgfx::submit(0, m_program);
#endif


			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;
    bgfx::UniformHandle m_winScaleH;
    bgfx::UniformHandle m_wireColH;
	bgfx::ProgramHandle m_program;
	int64_t m_timeOffset;
};

ENTRY_IMPLEMENT_MAIN(ExampleCubes);
