/*
The MIT License (MIT)
Copyright (c) 2021 NVIDIA
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

/*

This is a c++ port of the work by Matthias Müller-Fischer
source: https://matthias-research.github.io/pages/challenges/softBody.html

*/

#define PHYS_TODO 0

struct SoftBody;

// physics scene

#if PHYS_TODO

var grabber;

#endif

struct PhysicsParams
{
	float gravity[3] = { 0.f, -10.f, 0.f };
	float timeStep = 1.f / 60.f;
	int numSubsteps = 5;
	float friction = 1000.f;
	float worldBounds[6] = { -2.5,-1.0, -2.5, 2.5, 10.0, 2.5 };
};

struct PhysicsScene
{
	std::vector<SoftBody*> softBodies;
};

static PhysicsParams physicsParams;

static PhysicsScene physicsScene;

// ----- vector math -------------------------------------------------------------

static void vecSetZero(float * __restrict a,int anr)
{
	anr *= 3;
	a[anr++] = 0.f;
	a[anr++] = 0.f;
	a[anr]   = 0.f;
}

static void vecCopy(float * __restrict a,int anr, const float * __restrict b,int bnr)
{
	anr *= 3; bnr *= 3;
	a[anr++] = b[bnr++];
	a[anr++] = b[bnr++];
	a[anr]   = b[bnr];
}

static void vecAdd(float * __restrict a,int anr, const float * __restrict b,int bnr)
{
	anr *= 3; bnr *= 3;
	a[anr++] += b[bnr++];
	a[anr++] += b[bnr++];
	a[anr]   += b[bnr];
}

static void vecAdd(float * __restrict a,int anr, const float * __restrict b,int bnr, const float scale)
{
	anr *= 3; bnr *= 3;
	a[anr++] += b[bnr++] * scale;
	a[anr++] += b[bnr++] * scale;
	a[anr]   += b[bnr] * scale;
}

static void vecSetDiff(float * __restrict dst,int dnr, const float * __restrict a,int anr, const float * __restrict b,int bnr)
{
	dnr *= 3; anr *= 3; bnr *= 3;
	dst[dnr++] = (a[anr++] - b[bnr++]);
	dst[dnr++] = (a[anr++] - b[bnr++]);
	dst[dnr]   = (a[anr] - b[bnr]);
}

static void vecSetDiff(float * __restrict dst,int dnr, const float * __restrict a,int anr, const float * __restrict b,int bnr, const float scale)
{
	dnr *= 3; anr *= 3; bnr *= 3;
	dst[dnr++] = (a[anr++] - b[bnr++]) * scale;
	dst[dnr++] = (a[anr++] - b[bnr++]) * scale;
	dst[dnr]   = (a[anr] - b[bnr]) * scale;
}

static float vecLengthSquared(const float * __restrict a,int anr)
{
	anr *= 3;
	const float a0 = a[anr], a1 = a[anr + 1], a2 = a[anr + 2];
	return a0 * a0 + a1 * a1 + a2 * a2;
}

static float vecDistSquared(const float * __restrict a,int anr, const float * __restrict b,int bnr)
{
	anr *= 3; bnr *= 3;
	const float a0 = a[anr] - b[bnr], a1 = a[anr + 1] - b[bnr + 1], a2 = a[anr + 2] - b[bnr + 2];
	return a0 * a0 + a1 * a1 + a2 * a2;
}

static void vecSetCross(float * __restrict a,int anr, const float * __restrict b,int bnr, const float * __restrict c,int cnr)
{
	anr *= 3; bnr *= 3; cnr *= 3;
	a[anr++] = b[bnr + 1] * c[cnr + 2] - b[bnr + 2] * c[cnr + 1];
	a[anr++] = b[bnr + 2] * c[cnr + 0] - b[bnr + 0] * c[cnr + 2];
	a[anr]   = b[bnr + 0] * c[cnr + 1] - b[bnr + 1] * c[cnr + 0];
}

static void vecSetClamped(float * __restrict dst,int dnr, const float * __restrict a,int anr, const float * __restrict b,int bnr)
{
	dnr *= 3; anr *= 3; bnr *= 3;
	dst[dnr] = fmaxf(a[anr++], fminf(b[bnr++], dst[dnr])); dnr++;
	dst[dnr] = fmaxf(a[anr++], fminf(b[bnr++], dst[dnr])); dnr++;
	dst[dnr] = fmaxf(a[anr++], fminf(b[bnr++], dst[dnr])); dnr++;
}

// ----- matrix math ----------------------------------

static float matIJ(const float * __restrict A,const int anr, const int row, const int col)
{
	return A[9*anr + 3 * col + row];
}

static void matSetVecProduct(float * __restrict dst,const int dnr, const float * __restrict A,int anr, const float * __restrict b,int bnr)
{
	bnr *= 3; anr *= 3;
	const float b0 = b[bnr++];
	const float b1 = b[bnr++];
	const float b2 = b[bnr];
	vecSetZero(dst,dnr);
	vecAdd(dst,dnr, A,anr++, b0);
	vecAdd(dst,dnr, A,anr++, b1);
	vecAdd(dst,dnr, A,anr,   b2);
}

static void matSetMatProduct(float * __restrict Dst,int dnr, const float * __restrict A,const int anr, const float * __restrict B,int bnr)
{
	dnr *= 3; bnr *= 3;
	matSetVecProduct(Dst,dnr++, A,anr, B,bnr++);
	matSetVecProduct(Dst,dnr++, A,anr, B,bnr++);
	matSetVecProduct(Dst,dnr++, A,anr, B,bnr++);
}

static float matGetDeterminant(const float * __restrict A,int anr)
{
	anr *= 9;
	const float a11 = A[anr + 0], a12 = A[anr + 3], a13 = A[anr + 6];
	const float a21 = A[anr + 1], a22 = A[anr + 4], a23 = A[anr + 7];
	const float a31 = A[anr + 2], a32 = A[anr + 5], a33 = A[anr + 8];
	return a11*a22*a33 + a12*a23*a31 + a13*a21*a32 - a13*a22*a31 - a12*a21*a33 - a11*a23*a32;
}

static void matSetInverse(float * __restrict A,int anr)
{
	const float det = matGetDeterminant(A,anr);

	if (det == 0.f)
	{
		for (int i = 0; i < 9; i++)
			A[anr + i] = 0.f;
		return;
	}

	const float invDet = 1.f / det;
	
	anr *= 9;
	
	auto a11 = A[anr + 0], a12 = A[anr + 3], a13 = A[anr + 6];
	auto a21 = A[anr + 1], a22 = A[anr + 4], a23 = A[anr + 7];
	auto a31 = A[anr + 2], a32 = A[anr + 5], a33 = A[anr + 8];
	
	A[anr + 0] =  (a22 * a33 - a23 * a32) * invDet;
	A[anr + 3] = -(a12 * a33 - a13 * a32) * invDet;
	A[anr + 6] =  (a12 * a23 - a13 * a22) * invDet;
	A[anr + 1] = -(a21 * a33 - a23 * a31) * invDet;
	A[anr + 4] =  (a11 * a33 - a13 * a31) * invDet;
	A[anr + 7] = -(a11 * a23 - a13 * a21) * invDet;
	A[anr + 2] =  (a21 * a32 - a22 * a31) * invDet;
	A[anr + 5] = -(a11 * a32 - a12 * a31) * invDet;
	A[anr + 8] =  (a11 * a22 - a12 * a21) * invDet;
}

// ------------------------------------------------------------------

static float * copyFloatVec(const std::vector<float> & v)
{
	float * r = new float[v.size()];
	std::copy(v.begin(), v.end(), r);
	return r;
}

static float * allocFloatVec(const size_t n)
{
	float * result = new float[n];
	std::fill(result, result + n, 0.f);
	return result;
}

struct SoftBody
{
	int numParticles = 0;
	int numElems = 0;
	
	float * __restrict pos = nullptr;
	float * __restrict prevPos = nullptr;
	float * __restrict vel = nullptr;
	
	float * __restrict invMass = nullptr;
	float * __restrict invRestPose = nullptr;
	
	std::vector<int> tetIds;
	
	float volError = 0.f;
	
	float grabPos[3] = { };
	int grabId = -1;
	
	float devCompliance = 0.f;
	float volCompliance = 0.f;
		
	std::vector<float> visVerts;
	std::vector<int> visTriIds;
	int numVisVerts = 0;
	
	SoftBody(
		const std::vector<float> & vertices,
		const std::vector<int> & tetIds,
		const std::vector<int> & tetEdgeIds,
		const float density,
		const float devCompliance,
		const float volCompliance,
		const std::vector<float> & visVerts,
		const std::vector<int> & visTriIds)
	{
		// physics data 

		this->numParticles = vertices.size() / 3;
		this->numElems = tetIds.size() / 4;

		this->pos = copyFloatVec(vertices);
		this->prevPos = copyFloatVec(vertices);
		this->vel = allocFloatVec(3 * this->numParticles);
		
		this->invMass = allocFloatVec(this->numParticles);
		this->invRestPose = allocFloatVec(9 * this->numElems);
		
		this->tetIds = tetIds;
		
		this->volError = 0.f;

		this->grabId = -1;

		this->devCompliance = devCompliance;
		this->volCompliance = volCompliance;

		this->initPhysics(density);
	
		// visual embedded mesh

		this->visVerts = visVerts;
		this->visTriIds = visTriIds;
		this->numVisVerts = visVerts.size() / 4;
	}

	void initPhysics(const float density)
	{
		for (int i = 0; i < this->numParticles; i++)
			this->invMass[i] = 0.f;

		for (int i = 0; i < this->numElems; i++)
		{
			const auto id0 = this->tetIds[4 * i    ];
			const auto id1 = this->tetIds[4 * i + 1];
			const auto id2 = this->tetIds[4 * i + 2];
			const auto id3 = this->tetIds[4 * i + 3];

			vecSetDiff(this->invRestPose, 3 * i,     this->pos,id1, this->pos,id0);
			vecSetDiff(this->invRestPose, 3 * i + 1, this->pos,id2, this->pos,id0);
			vecSetDiff(this->invRestPose, 3 * i + 2, this->pos,id3, this->pos,id0);

			const float V = matGetDeterminant(this->invRestPose, i) / 6.f;

			matSetInverse(this->invRestPose, i);

			const float pm = V / 4.f * density;
			this->invMass[id0] += pm;
			this->invMass[id1] += pm;
			this->invMass[id2] += pm;
			this->invMass[id3] += pm;
		}

		for (int i = 0; i < this->numParticles; i++)
		{
			if (this->invMass[i] != 0.f)
				this->invMass[i] = 1.f / this->invMass[i];
		}

	}

	// ----------------- begin solver -----------------------------------------------------				

	void solveElem(const int elemNr, const float dt) 
	{
		const float * __restrict ir = this->invRestPose;

		// tr(F) = 3

		const auto id0 = this->tetIds[4 * elemNr    ];
		const auto id1 = this->tetIds[4 * elemNr + 1];
		const auto id2 = this->tetIds[4 * elemNr + 2];
		const auto id3 = this->tetIds[4 * elemNr + 3];

		float P[3*3];
		float F[3*3];
		float g[4*3];
		
		float dF[3*3];
		
		vecSetDiff(P,0, this->pos,id1, this->pos,id0);
		vecSetDiff(P,1, this->pos,id2, this->pos,id0);
		vecSetDiff(P,2, this->pos,id3, this->pos,id0);

		matSetMatProduct(F,0, P,0, this->invRestPose,elemNr);
		
		vecSetZero(g,1);
		vecAdd(g,1, F,0, 2.0 * matIJ(ir,elemNr, 0,0));
		vecAdd(g,1, F,1, 2.0 * matIJ(ir,elemNr, 0,1));
		vecAdd(g,1, F,2, 2.0 * matIJ(ir,elemNr, 0,2));

		vecSetZero(g,2);
		vecAdd(g,2, F,0, 2.0 * matIJ(ir,elemNr, 1,0));
		vecAdd(g,2, F,1, 2.0 * matIJ(ir,elemNr, 1,1));
		vecAdd(g,2, F,2, 2.0 * matIJ(ir,elemNr, 1,2));

		vecSetZero(g,3);
		vecAdd(g,3, F,0, 2.0 * matIJ(ir,elemNr, 2,0));
		vecAdd(g,3, F,1, 2.0 * matIJ(ir,elemNr, 2,1));
		vecAdd(g,3, F,2, 2.0 * matIJ(ir,elemNr, 2,2));

		float C = vecLengthSquared(F,0) + vecLengthSquared(F,1) + vecLengthSquared(F,2) - 3.f;

		this->applyToElem(elemNr, g, C, this->devCompliance, dt);
		
		// det F = 1

		vecSetDiff(P,0, this->pos,id1, this->pos,id0);
		vecSetDiff(P,1, this->pos,id2, this->pos,id0);
		vecSetDiff(P,2, this->pos,id3, this->pos,id0);

		matSetMatProduct(F,0, P,0, this->invRestPose,elemNr);

		vecSetCross(dF,0, F,1, F,2);
		vecSetCross(dF,1, F,2, F,0);
		vecSetCross(dF,2, F,0, F,1);

		vecSetZero(g,1);
		vecAdd(g,1, dF,0, matIJ(ir,elemNr, 0,0));
		vecAdd(g,1, dF,1, matIJ(ir,elemNr, 0,1));
		vecAdd(g,1, dF,2, matIJ(ir,elemNr, 0,2));

		vecSetZero(g,2);
		vecAdd(g,2, dF,0, matIJ(ir,elemNr, 1,0));
		vecAdd(g,2, dF,1, matIJ(ir,elemNr, 1,1));
		vecAdd(g,2, dF,2, matIJ(ir,elemNr, 1,2));

		vecSetZero(g,3);
		vecAdd(g,3, dF,0, matIJ(ir,elemNr, 2,0));
		vecAdd(g,3, dF,1, matIJ(ir,elemNr, 2,1));
		vecAdd(g,3, dF,2, matIJ(ir,elemNr, 2,2));

		C = matGetDeterminant(F,0) - 1.0;
		this->volError += C;
		
		this->applyToElem(elemNr, g, C, this->volCompliance, dt);
	}

	void applyToElem(const int elemNr, float * __restrict g, const float C, const float compliance, const float dt)
	{
		if (C == 0.f)
			return;

		vecSetZero(g,0);
		vecAdd(g,0, g,1, -1.f);
		vecAdd(g,0, g,2, -1.f);
		vecAdd(g,0, g,3, -1.f);

		float w = 0.f;
		for (int i = 0; i < 4; i++)
		{
			const int id = this->tetIds[4 * elemNr + i];
			w += vecLengthSquared(g,i) * this->invMass[id];
		}

		if (w == 0.f)
			return;
		const float alpha = compliance / dt / dt;
		const float dlambda = -C / (w + alpha);

		for (int i = 0; i < 4; i++)
		{
			const int id = this->tetIds[4 * elemNr + i];
			vecAdd(this->pos,id, g,i, dlambda * this->invMass[id]);
		}
	}

	void simulate(const float dt, const float * gravity)
	{
		// XPBD prediction

		for (int i = 0; i < this->numParticles; i++)
		{
			vecAdd(this->vel,i, gravity,0, dt);
			vecCopy(this->prevPos,i, this->pos,i);
			vecAdd(this->pos,i, this->vel,i, dt);
		}

		// solve

		this->volError = 0.f;
		for (int i = 0; i < this->numElems; i++)
			this->solveElem(i, dt);
		this->volError /= this->numElems;

		// ground collision

		for (int i = 0; i < this->numParticles; i++)
		{
			vecSetClamped(this->pos,i, physicsParams.worldBounds,0,
				physicsParams.worldBounds,1);

			if (this->pos[3 * i + 1] < 0.f)
			{
				this->pos[3 * i + 1] = 0.f;

				// simple friction
				float F[3*3];
				vecSetDiff(F,0, prevPos,i, this->pos,i);

				this->pos[3 * i    ] += F[0] * fminf(1.f, dt * physicsParams.friction);
				this->pos[3 * i + 2] += F[2] * fminf(1.f, dt * physicsParams.friction);

				// this->pos[3 * i] = this->prevPos[3 * i];
				// this->pos[3 * i + 2] = this->prevPos[3 * i + 2];
			}

		}

		if (this->grabId >= 0)
		{
			vecCopy(this->pos,this->grabId, this->grabPos,0);
		}

		// XPBD velocity update

		for (int i = 0; i < this->numParticles; i++)
			vecSetDiff(this->vel,i, this->pos,i, this->prevPos,i, 1.f / dt);

	#if PHYS_TODO
		if (!grabber.physicsObject) 
			controls.enabled = true;
	#endif
	}

	// ----------------- end solver -----------------------------------------------------

	void drawVisMesh() const
	{
		float * __restrict positions = new float[this->numVisVerts * 3];

		int nr = 0;

		for (int i = 0; i < this->numVisVerts; i++)
		{
			int tetNr = this->visVerts[nr++] * 4;
			
			const float b0 = this->visVerts[nr++];
			const float b1 = this->visVerts[nr++];
			const float b2 = this->visVerts[nr++];
			const float b3 = 1.f - b0 - b1 - b2;

			const int id0 = this->tetIds[tetNr++];
			const int id1 = this->tetIds[tetNr++];
			const int id2 = this->tetIds[tetNr++];
			const int id3 = this->tetIds[tetNr++];

			vecSetZero(positions,i);

			vecAdd(positions,i, this->pos,id0, b0);
			vecAdd(positions,i, this->pos,id1, b1);
			vecAdd(positions,i, this->pos,id2, b2);
			vecAdd(positions,i, this->pos,id3, b3);
		}

		gxBegin(GX_TRIANGLES);
		{
			const int numTriangles = this->visTriIds.size() / 3;
			const int * __restrict vertexIndices = this->visTriIds.data();
			
			for (int i = 0; i < numTriangles; ++i)
			{
				const auto vertexIndex1 = vertexIndices[0];
				const auto vertexIndex2 = vertexIndices[1];
				const auto vertexIndex3 = vertexIndices[2];
				
				const auto * __restrict vertex1 = positions + vertexIndex1 * 3;
				const auto * __restrict vertex2 = positions + vertexIndex2 * 3;
				const auto * __restrict vertex3 = positions + vertexIndex3 * 3;
				
				const Vec3 a(vertex1[0], vertex1[1], vertex1[2]);
				const Vec3 b(vertex2[0], vertex2[1], vertex2[2]);
				const Vec3 c(vertex3[0], vertex3[1], vertex3[2]);
				
				const Vec3 normal = ((b - a) % (c - a)).CalcNormalized();
				
				gxNormal3fv(&normal[0]);
				gxVertex3fv(vertex1);
				gxVertex3fv(vertex2);
				gxVertex3fv(vertex3);
				
				vertexIndices += 3;
			}
		}
		gxEnd();
		
		delete [] positions;
		positions = nullptr;
	}

	void startGrab(Vec3Arg pos)
	{
		const float * p = &pos[0];
		float minD2 = std::numeric_limits<float>::max();
		this->grabId = -1;
		for (int i = 0; i < this->numParticles; i++)
		{
			const float d2 = vecDistSquared(p,0, this->pos,i);
			if (d2 < minD2) {
				minD2 = d2;
				this->grabId = i;
			}
		}
		vecCopy(this->grabPos,0, p,0);
	}

	void moveGrabbed(Vec3Arg pos)
	{
		const float * p = &pos[0];
		vecCopy(this->grabPos,0, p,0);
	}

	void endGrab() 
	{
		this->grabId = -1;
	}
};

// ------------------------------------------------------------------
#include "99-softBody-dragon.cpp"

void initPhysics() 
{
	const float density = 1.f;
	const float devCompliance = 0.f;
	const float volCompliance = 0.f;
					;
	SoftBody * dragon = new SoftBody(
		dragonTetVerts,
		dragonTetIds,
		dragonTetEdgeIds,
		density,
		devCompliance,
		volCompliance,
		dragonAttachedVerts,
		dragonAttachedTriIds);

	physicsScene.softBodies.push_back(dragon);
}

// ------------------------------------------------------------------
void simulate() 
{
	const float dt = physicsParams.timeStep / physicsParams.numSubsteps;

	for (int step = 0; step < physicsParams.numSubsteps; step++)
	{
		for (int i = 0; i < physicsScene.softBodies.size(); i++)
		{
			physicsScene.softBodies[i]->simulate(dt, physicsParams.gravity);
		}
	}
}

// ---------------------------------------------------------------------

#if PHYS_TODO

class Grabber
{
	constructor(scene, renderer, camera)
	{
		this->scene = scene;
		this->renderer = renderer;
		this->camera = camera;
		this->mousePos = new THREE.Vector2();
		this->raycaster = new THREE.Raycaster();
		this->raycaster.layers.set(1);
//					this->raycaster.params.Mesh.threshold = 3;
		this->raycaster.params.Line.threshold = 0.1;
		this->grabDistance = 0.0;
		this->active = false;
		this->physicsObject = null;
	}
	updateRaycaster(x, y) {
		var rect = this->renderer.domElement.getBoundingClientRect();
		this->mousePos.x = ((x - rect.left) / rect.width ) * 2 - 1;
		this->mousePos.y = -((y - rect.top) / rect.height ) * 2 + 1;
		this->raycaster.setFromCamera( this->mousePos, camera );
	}
	start(x, y) {
		this->physicsObject = null;
		this->updateRaycaster(x, y);
		var intersects = this->raycaster.intersectObjects( scene.children );
		if (intersects.length > 0) {
			var obj = intersects[0].object.userData;
			if (obj instanceof SoftBody) {
				this->physicsObject = obj;
				this->grabDistance = intersects[0].distance;
				let hit = this->raycaster.ray.origin.clone();
				hit.addScaledVector(this->raycaster.ray.direction, this->grabDistance);
				this->physicsObject.startGrab(hit);
				this->active = true;
			}
			if (paused)
				run();						
		}
	}
	move(x, y) {
		if (this->active) {
			this->updateRaycaster(x, y);
			let hit = this->raycaster.ray.origin.clone();
			hit.addScaledVector(this->raycaster.ray.direction, this->grabDistance);
			if (this->physicsObject != null)
				this->physicsObject.moveGrabbed(hit);
		}
	}
	end() {
		if (this->active) {
			if (this->physicsObject != null) {
				this->physicsObject.endGrab();
				this->physicsObject = null;
			}
			this->active = false;
		}
		this->physicsObject = null;
	}
}

#endif

// ------------------------- callbacks ---------------------------------------------

#if PHYS_TODO

function onPointer( evt ) 
{
//				event.preventDefault();
	if (evt.type == "pointerdown") {
		grabber.start(evt.clientX, evt.clientY);
		if (grabber.physicsObject) {
			controls.saveState();
			controls.enabled = false;
		}
		mouseDown = true;
	}
	else if (evt.type == "pointermove" && mouseDown) {
		if (grabber.active)
			grabber.move(evt.clientX, evt.clientY);
	}
	else if (evt.type == "pointerup"/* || evt.type == "pointerout"*/) {
		if (grabber.physicsObject) {
			grabber.end();
			controls.reset();
			controls = new THREE.OrbitControls(camera, renderer.domElement);
			controls.zoomSpeed = 2.0;
			controls.panSpeed = 0.4;
		}
		mouseDown = false;
	}
	if (!grabber.physicsObject) {
		controls.enabled = true;
	}
}

#endif

#include "Timer.h"

int timeFrames = 0;
int timeSum = 0;

static bool paused = true;

static void animate() 
{		
	const uint64_t startTime = g_TimerRT.TimeUS_get();

	if (!paused)
		simulate();

	const uint64_t endTime = g_TimerRT.TimeUS_get();
	timeSum += endTime - startTime; 
	timeFrames++;

	if (timeFrames >= 10)
	{
		timeSum /= timeFrames;
		printf("time: %.2fms\n", timeSum / 1000.f);
		timeFrames = 0;
		timeSum = 0;
	}

	float volError = 0.f;
	for (int i = 0; i < physicsScene.softBodies.size(); i++)
		volError += physicsScene.softBodies[i]->volError;
	volError /= physicsScene.softBodies.size();
	logDebug("error: %.2f", volError);
}

static void run()
{
	paused = !paused;
}

static void squash()
{
	for (int i = 0; i < physicsScene.softBodies.size(); i++)
	{
		auto & s =	*physicsScene.softBodies[i];

		for (int j = 0; j < s.numParticles; j++)
		{
			s.pos[3 * j    ] += random<float>(-.5f, +.5f) * .3f;
			s.pos[3 * j + 1] = .5f;
			s.pos[3 * j + 2] += random<float>(-.5f, +.5f) * .3f;
		}
	}

	if (!paused)
		run();
}

static bool showTetMesh = true;

static void onTetMesh()
{
	showTetMesh = !showTetMesh;
}

#if PHYS_TODO

document.getElementById("stepsSlider").oninput = function() {
	document.getElementById("numSteps").innerHTML = this->value;
	physicsParams.numSubsteps = this->value;
}

#endif

// ---- models --------------------------------------------------------

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	framework.enableDepthBuffer = true;

	if (!framework.init(800, 600))
		return -1;
	
	initPhysics();

	Camera3d camera;

	camera.position.Set(0, 2, -2);
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		if (keyboard.wentDown(SDLK_SPACE))
			run();
		
		if (keyboard.wentDown(SDLK_s))
			squash();
			
		if (keyboard.wentDown(SDLK_t))
			onTetMesh();
			
		animate();

		camera.tick(framework.timeStep, true);

		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			camera.pushViewMatrix();
			{
				// render ground plane
				
				gxPushMatrix();
				{
					gxScalef(4, 4, 4);
					
					pushLineSmooth(false);
					setColor(100, 100, 100);
					drawGrid3dLine(40, 40, 0, 2, true);
					popLineSmooth();
				}
				gxPopMatrix();
				
				// render meshes
				
				for (auto * body : physicsScene.softBodies)
				{
					setColor(colorGreen);
					gxBegin(GX_LINES);
					{
						for (auto & vertexIndex : dragonTetEdgeIds)
						{
							gxVertex3fv(body->pos + vertexIndex * 3);
						}
					}
					gxEnd();
					
					if (showTetMesh)
					{
						pushShaderOutputs("n");
						{
							setColor(colorWhite);
							body->drawVisMesh();
						}
						popShaderOutputs();
					}
				}
			}
			camera.popViewMatrix();
			
			popDepthTest();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
