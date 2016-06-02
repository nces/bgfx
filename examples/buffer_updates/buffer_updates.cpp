#include "common.h"
#include "bgfx_utils.h"

#include <array>
#include <random>
#include <cmath>
#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>

struct PosColorFlatVertex
{
	float m_x;
	float m_y;
	float m_z;
	float m_nx; // not currently used
	float m_ny; // not currently used
	float m_nz; // not currently used
	uint32_t m_abgr;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexDecl ms_decl;
};
bgfx::VertexDecl PosColorFlatVertex::ms_decl;

// Helper functions
float length(const PosColorFlatVertex& v)
{
	return sqrt(v.m_x*v.m_x + v.m_y*v.m_y + v.m_z*v.m_z);
}
PosColorFlatVertex normalize(const PosColorFlatVertex& v)
{
	float l = length(v);
	if (l <= std::numeric_limits<float>::epsilon())
	{
		l = 1.0f;
	}
	return {
		v.m_x / l, v.m_y / l, v.m_z / l, // position
		0,0,0,                           // normal
		v.m_abgr                         // alpha
	};
}
float lerp(float a, float b, float t)
{
	return (t * (b - a)) + a;
}
PosColorFlatVertex lerp(const PosColorFlatVertex& v0, const PosColorFlatVertex& v1, float t)
{
	return {
		lerp(v0.m_x, v1.m_x, t),
		lerp(v0.m_y, v1.m_y, t),
		lerp(v0.m_z, v1.m_z, t),
		v0.m_nx,
		v0.m_ny,
		v0.m_nz,
		v0.m_abgr
	};
}
PosColorFlatVertex operator+(const PosColorFlatVertex& v0, const PosColorFlatVertex& v1)
{
	return {
		v0.m_x + v1.m_x,
		v0.m_y + v1.m_y,
		v0.m_z + v1.m_z,
		v0.m_nx,
		v0.m_ny,
		v0.m_nz,
		v0.m_abgr
	};
}
std::ostream& operator<<(std::ostream& out, const PosColorFlatVertex& v)
{
	out << "(" << v.m_x << ", "
		<< v.m_y << ", "
		<< v.m_z << ", "
		<< v.m_nx << ", "
		<< v.m_ny << ", "
		<< v.m_nz;
	return out;
}

// Number of vertices to create
constexpr int CIRC_RES = 128;
//constexpr int CIRC_RES = std::numeric_limits<uint16_t>::max();

// Number of vertices to update
constexpr int UPDATE_EXTENT = CIRC_RES / 8;

// For random number generation
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<uint16_t> udist(0, CIRC_RES - 1);

// Geometry data
static std::array<PosColorFlatVertex, CIRC_RES + 1> geomData; // Vertex data
static std::array<uint16_t, CIRC_RES * 3> indData;            // Index data

// Geometry data
// NOTE: bgfx crashes when resizing a vector. Although bgfx::copy is used (which is basically just a call to alloc) to
//       copy the host data, bgfx appears to be somehow dependent on the host data, which is unexpected.
//
//       Another possibility is that this is somehow due to multi-threading. e.g., perhaps the underlying vector data
//       was deleted while bgfx was copying it.
//static std::vector<PosColorFlatVertex> updateData;
static std::array<PosColorFlatVertex, CIRC_RES + 1> updateData; // Updated geomtry to send to GPU
static std::array<uint16_t, UPDATE_EXTENT> updateDataInds;      // Indices of vertices being updated
static uint16_t lowerExtent, upperExtent;                       // Contiguous chunk that is being updated

// Initially copied from the 01-cubes example
class BufferUpdateTest : public entry::AppI
{
	// Generates a "dynamic vertex buffer" that will be continously updated by the CPU.
	void genVertBuf()
	{
		constexpr int   res = CIRC_RES;
		constexpr float dr  = (2 * M_PI) / res;

		// center vertex
		geomData[0].m_x    = 0.0f;
		geomData[0].m_y    = 0.0f;
		geomData[0].m_z    = 0.0f;
		geomData[0].m_nx   = 0.0f;
		geomData[0].m_ny   = 0.0f;
		geomData[0].m_nz   = 0.0f;
		geomData[0].m_abgr = 0xffff0000;

		float angle = 0.0f;
		for (int i = 1, j = 0; i < res + 1; ++i, angle += dr, j += 3)
		{
			geomData[i].m_x    = cos(angle);
			geomData[i].m_y    = sin(angle);
			geomData[i].m_z    = 0.0f;
			geomData[i].m_nx   = 0.0f;
			geomData[i].m_ny   = 0.0f;
			geomData[i].m_nz   = -1.0f;
			geomData[i].m_abgr = 0xff0000ff;

			indData[j]     = 0;
			indData[j + 1] = i;
			indData[j + 2] = (i == res) ? 1 : i + 1;
		}

		PosColorFlatVertex::init();

		m_vb = bgfx::createDynamicVertexBuffer(
				bgfx::copy(geomData.data(), geomData.size() * sizeof(PosColorFlatVertex)),
				PosColorFlatVertex::ms_decl);

		m_ib = bgfx::createDynamicIndexBuffer(bgfx::copy(indData.data(), indData.size() * sizeof(uint16_t)));
	}

	// Boiler plate from other examples
	void init(int _argc, char** _argv) BX_OVERRIDE
	{
		Args args(_argc, _argv);

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

		genVertBuf();

		// Create program from shaders.
		// loadProgram is a utility function
		m_program = loadProgram("vs_buffer_updates", "fs_buffer_updates");

		m_timeOffset = bx::getHPCounter();
	}

	virtual int shutdown() BX_OVERRIDE
	{
		// Cleanup.
		bgfx::destroyDynamicIndexBuffer(m_ib);
		bgfx::destroyDynamicVertexBuffer(m_vb);
		bgfx::destroyProgram(m_program);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	template<typename T>
	T clamp(const T& v, const T& l, const T& h)
	{
		if (v < l) { return l; }
		if (v > h) { return h; }
		return v;
	}
	
	float getAnimTime(float t)
	{
		t = clamp(t, 0.f, 1.f);
		if (t > 0.5)
		{
			return -2*t + 2;
		}
		else
		{
			return 2*t;
		}
	}

	// Updates a "chunk" of vertices over a 1s interval
	void performSubUpdates()
	{
		static const float animDur = 1.0f; // Animation length in seconds
		float currAnimTime = m_currTime - m_startAnimTime; // Current time in the animation

		if (m_animating)
		{
			if (currAnimTime > animDur)
			{
				m_animating = false;
			}
			else
			{
				float t = getAnimTime(currAnimTime);

				// Update only the changing vertices
				for (uint16_t ind : updateDataInds)
				{
					PosColorFlatVertex& p = geomData[ind];
					PosColorFlatVertex& dir = p; // already normalized
					PosColorFlatVertex np = p + dir;
					updateData[ind - lowerExtent] = lerp(p, np, t);
				}

				// Have to upload a contiguous chunk, which is [min(updateDataInds), max(updateDataInds)]
				bgfx::updateDynamicVertexBuffer(m_vb, lowerExtent, bgfx::copy(updateData.data(), (upperExtent - lowerExtent) * sizeof(PosColorFlatVertex)));
			}
		}
		else
		{
			// Start a new update
			// Choose UPDATE_EXTENT random vertices to update, and store the random indices into updateDataInds
			m_startAnimTime = m_currTime;
			m_animating = true;
			uint16_t tmpInd = udist(gen);
			updateDataInds[0] = tmpInd;
			lowerExtent = tmpInd; // will be min(updateDataInds)
			upperExtent = tmpInd; // will be max(updateDataInds)
			for (int i = 1; i < updateDataInds.size(); ++i)
			{
				tmpInd = udist(gen);
				updateDataInds[i] = tmpInd;
				if      (tmpInd < lowerExtent) { lowerExtent = tmpInd; }
				else if (tmpInd > upperExtent) { upperExtent = tmpInd; }
			}

			// This vector resize causes bgfx to crash
			//updateData.resize(upperExtent - lowerExtent);
			//updateData.assign(geomData.begin() + lowerExtent, geomData.begin() + upperExtent);

			// Copy the vertices that we are going to be updating
			std::copy(geomData.begin() + lowerExtent, geomData.begin() + upperExtent, updateData.begin());
		}
	}

	// Boilerplate from other examples
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

			m_currTime = (float)( (now-m_timeOffset)/double(bx::getHPFrequency() ) );

			// Use debug font to print information about this example.
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/buffer_updates");
			bgfx::dbgTextPrintf(0, 2, 0x6f, "Buffer updates test.");
			bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

			float at[3]  = { 0.0f, 0.0f,   0.0f };
			float eye[3] = { 0.0f, 0.0f, -3.0f };

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

			performSubUpdates();

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			// Submit 11x11 cubes.
			float mtx[16] = {
				1,  0,  0,  0,
				0,  1,  0,  0,
				0,  0,  1,  0,
				0,  0,  0,  1,
			};
			//bx::mtxRotateXY(mtx, time + 0.21f, time + 0.37f);
			//bx::mtxRotateXY(mtx, time + xx*0.21f, time + yy*0.37f);
			//mtx[12] = -15.0f + float(xx)*3.0f;
			//mtx[13] = -15.0f + float(yy)*3.0f;
			//mtx[14] = 0.0f;

			// Set model matrix for rendering.
			//bgfx::setTransform(mtx);

			// Set vertex and index buffer.
			bgfx::setVertexBuffer(m_vb);
			bgfx::setIndexBuffer(m_ib);

			// Set render states.
			bgfx::setState(BGFX_STATE_DEFAULT);

			// Submit primitive for rendering to view 0.
			bgfx::submit(0, m_program);

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
	bgfx::DynamicVertexBufferHandle m_vb;
	bgfx::DynamicIndexBufferHandle m_ib;
	bgfx::ProgramHandle m_program;
	int64_t m_timeOffset;
	float m_currTime;
	bool m_animating = false;
	float m_startAnimTime;
};

ENTRY_IMPLEMENT_MAIN(BufferUpdateTest);
