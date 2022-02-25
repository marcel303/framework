#include "framework.h"

#include "Quat.h"
#include "Vec3.h"

/*

This is a c++ port of the wonderful Javascript code samples
graciously shared by Matthias Müller (thank you!)

Original source materials:

- https://matthias-research.github.io/pages/challenges/challenges.html
- https://github.com/matthias-research/pages/blob/master/challenges/PBD.js

*/

static const float maxRotationPerSubstep = 0.5f;

// Pose  -----------------------------------------------------------

struct Pose
{
	Vec3 p;
	Quat q;
	
    void rotate(Vec3 & v) const
    {
		v = this->q.mul(v);
    }
    
    void invRotate(Vec3 & v) const
    {
        const Quat inv = q.calcConjugate();
        v = inv.mul(v);
    }
    
    void transform(Vec3 & v) const
    {
		rotate(v);
		v += this->p;
    }
    
    void invTransform(Vec3 & v) const
    {
        v -= this->p;
        this->invRotate(v);
    }
    
    void transformPose(Pose & pose) const
    {
		pose.q = this->q * pose.q;
        this->rotate(pose.p);
        pose.p += this->p;
    }
};

static Vec3 getQuatAxis0(const Quat & q)
{
	const float x2 = q.m_xyz[0] * 2.f;
    const float w2 = q.m_w      * 2.f;
    return Vec3(
		( q.m_w      * w2) + (q.m_xyz[0] * x2) - 1.f,
		(+q.m_xyz[2] * w2) + (q.m_xyz[1] * x2),
		(-q.m_xyz[1] * w2) + (q.m_xyz[2] * x2));
}

static Vec3 getQuatAxis1(const Quat & q)
{
	const float y2 = q.m_xyz[1] * 2.f;
    const float w2 = q.m_w      * 2.f;
    return Vec3(
		(-q.m_xyz[2] * w2) + (q.m_xyz[0] * y2),
		( q.m_w      * w2) + (q.m_xyz[1] * y2) - 1.f,
		(+q.m_xyz[0] * w2) + (q.m_xyz[2] * y2));
}

static Vec3 getQuatAxis2(const Quat & q)
{
	const float z2 = q.m_xyz[2] * 2.f;
	const float w2 = q.m_w      * 2.f;
	return Vec3(
		(+q.m_xyz[1] * w2) + (q.m_xyz[0] * z2),
		(-q.m_xyz[0] * w2) + (q.m_xyz[1] * z2),
		( q.m_w      * w2) + (q.m_xyz[2] * z2) - 1.f);
}

// Rigid body class  -----------------------------------------------------------

struct Mesh
{
	Vec3 position;
	Quat quaternion;
	Vec3 scale = Vec3(1.f);
	
	struct
	{
		struct Body * physicsBody = nullptr;
	} userData;
};

struct Body
{
	Pose pose;
	Pose prevPose;
	Pose origPose;
	
	Vec3 vel;
	Vec3 omega;
	
	float invMass = 0.f;
	Vec3 invInertia;
	
	Mesh * mesh = nullptr;
	
    Body(const Pose & pose, Mesh * mesh)
    {
        this->pose = pose;
        this->prevPose = pose;
        this->origPose = pose;
        
        this->vel = Vec3();
        this->omega = Vec3();
        
        this->invMass = 1.f;
        this->invInertia = Vec3(1.f);
        
        this->mesh = mesh;
        this->mesh->position = this->pose.p;
        this->mesh->quaternion = this->pose.q;
        this->mesh->userData.physicsBody = this;
    }

    void setBox(Vec3Arg size, const float density = 1.0)
    {
        float mass = size[0] * size[1] * size[2] * density;
        this->invMass = 1.f / mass;
        
        mass /= 12.f;
        this->invInertia.Set(
            1.f / (size[1] * size[1] + size[2] * size[2]) / mass,
            1.f / (size[2] * size[2] + size[0] * size[0]) / mass,
            1.f / (size[0] * size[0] + size[1] * size[1]) / mass);
    }

    void applyRotation(Vec3Arg rot, float scale = 1.f)
    {
        // safety clamping. This happens very rarely if the solver
        // wants to turn the body by more than 30 degrees in the
        // orders of milliseconds

        const float maxPhi = .5f;
        
        const float phi = rot.CalcSize();
        if (phi * scale > maxRotationPerSubstep)
            scale = maxRotationPerSubstep / phi;
            
        Quat dq = Quat(
			rot[0] * scale,
			rot[1] * scale,
			rot[2] * scale, 0.f);
        dq *= this->pose.q;
        
        this->pose.q = Quat(
			this->pose.q.m_xyz[0] + .5f * dq.m_xyz[0],
			this->pose.q.m_xyz[1] + .5f * dq.m_xyz[1],
			this->pose.q.m_xyz[2] + .5f * dq.m_xyz[2],
			this->pose.q.m_w      + .5f * dq.m_w);
        this->pose.q.normalize();
    }

    void integrate(const float dt, Vec3Arg gravity)
    {
        this->prevPose = this->pose;
        
        this->vel += gravity * dt;
        
        this->pose.p += this->vel * dt;
        
        this->applyRotation(this->omega, dt);
    }

    void update(const float dt)
    {
        this->vel = (this->pose.p - this->prevPose.p) / dt;
        
	#if 1
        this->prevPose.q = this->prevPose.q.calcConjugate();
        Quat dq = this->pose.q * this->prevPose.q;
	#else
        Quat dq = this->pose.q * this->prevPose.q.calcConjugate(); // fixme : this is more correct?
	#endif
	
        this->omega.Set(
			dq.m_xyz[0] * 2.f / dt,
			dq.m_xyz[1] * 2.f / dt,
			dq.m_xyz[2] * 2.f / dt);
        if (dq.m_w < 0.f)
            this->omega = -this->omega;

        // this->omega.multiplyScalar(1.0 - 1.0 * dt);
        // this->vel.multiplyScalar(1.0 - 1.0 * dt);

        this->mesh->position = this->pose.p;
        this->mesh->quaternion = this->pose.q;
    }

    Vec3 getVelocityAt(Vec3Arg pos) const
    {
        Vec3 vel = pos - this->pose.p;
        vel = vel % this->omega;
        vel = this->vel - vel;
        return vel;
    }

    float getInverseMass(Vec3Arg normal, const Vec3 * pos = nullptr) const
    {
        Vec3 n;
        if (pos == nullptr)
            n = normal;
        else
        {
            n = *pos - this->pose.p;
            n = n % normal;
        }
        
        this->pose.invRotate(n);
        
        float w =
            n[0] * n[0] * this->invInertia[0] +
            n[1] * n[1] * this->invInertia[1] +
            n[2] * n[2] * this->invInertia[2];
        
        if (pos != nullptr)
            w += this->invMass;
            
        return w;
    }

    void applyCorrection(Vec3Arg corr, const Vec3 * pos = nullptr, const bool velocityLevel = false)
    {
        Vec3 dq;
        if (pos == nullptr)
            dq = corr;
        else
        {
            if (velocityLevel)
                this->vel += corr * this->invMass;
            else
                this->pose.p += corr * this->invMass;
                
            dq = *pos - this->pose.p;
            dq = dq % corr;
        }
        
        this->pose.invRotate(dq);
        
        dq.Set(
			this->invInertia[0] * dq[0],
			this->invInertia[1] * dq[1],
			this->invInertia[2] * dq[2]);
        this->pose.rotate(dq);
        
        if (velocityLevel)
            this->omega += dq;
        else
            this->applyRotation(dq);
    }
};

// ------------------------------------------------------------------------------------

static void applyBodyPairCorrection(
	Body * body0,
	Body * body1,
	Vec3Arg corr,
	const float compliance,
	const float dt,
	Vec3 * pos0 = nullptr,
	Vec3 * pos1 = nullptr,
    const bool velocityLevel = false)
{
    const float C = corr.CalcSize();
    if (C == 0.f)
        return;

// fixme : source calculated normalized, but gives me a glitchy result. check port for errors
#if 0
    Vec3 normal = corr;
#else
    Vec3 normal =
		corr.CalcSize() < 1e-6f
		? Vec3()
		: corr.CalcNormalized();
#endif

    const float w0 = body0 ? body0->getInverseMass(normal, pos0) : 0.f;
    const float w1 = body1 ? body1->getInverseMass(normal, pos1) : 0.f;

    const float w = w0 + w1;
    if (w == 0.f)
        return;

    const float lambda = -C / (w + compliance / dt / dt);
    
    normal *= -lambda;
    
    if (body0) body0->applyCorrection( normal, pos0, velocityLevel);
    if (body1) body1->applyCorrection(-normal, pos1, velocityLevel);
}

// ------------------------------------------------------------------------------------------------

static void limitAngle(
	Body * body0,
	Body * body1,
	Vec3Arg n,
	Vec3Arg a,
	Vec3Arg b,
	const float minAngle,
	const float maxAngle,
	const float compliance,
	const float dt,
	const float maxCorr = float(M_PI))
{
    // the key function to handle all angular joint limits
    const Vec3 c = a % b; // todo : check cross works the same as THREE.crossVectors

    float phi = asinf(c * n);
    if (a * b < 0.f)
        phi = float(M_PI) - phi;

    if (phi > +float(M_PI))
        phi -= float(2.0 * M_PI);
    if (phi < -float(M_PI))
        phi += float(2.0 * M_PI);

    if (phi < minAngle || phi > maxAngle)
    {
        phi = fminf(fmaxf(minAngle, phi), maxAngle);

        Quat q;
        q.fromAxisAngle(n, phi);

        Vec3 omega = a;
        omega = q.mul(omega);
        omega = omega % b;

        phi = omega.CalcSize();
        if (phi > maxCorr)
            omega *= maxCorr / phi;

        applyBodyPairCorrection(body0, body1, omega, compliance, dt);
    }
}

// Joint class  -----------------------------------------------------------

enum JointType
{
    SPHERICAL,
    HINGE,
    FIXED
};

struct Joint
{
	Body * body0 = nullptr;
	Body * body1 = nullptr;
	
	Pose localPose0;
	Pose localPose1;
	Pose globalPose0;
	Pose globalPose1;
	
	JointType type = (JointType)0;
	
	float compliance = 0.f;
	float rotDamping = 0.f;
	float posDamping = 0.f;
	
	bool hasSwingLimits = false;
	float minSwingAngle = -float(2.0 * M_PI);
	float maxSwingAngle = +float(2.0 * M_PI);
	float swingLimitsCompliance = 0.f;
	
	bool hasTwistLimits = false;
	float minTwistAngle = -float(2.0 * M_PI);
	float maxTwistAngle = +float(2.0 * M_PI);
	float twistLimitCompliance = 0.f;
	
    Joint(const JointType type, Body * body0, Body * body1, Pose & localPose0, Pose & localPose1)
    {
        this->body0 = body0;
        this->body1 = body1;
        this->localPose0 = localPose0;
        this->localPose1 = localPose1;
        this->globalPose0 = localPose0;
        this->globalPose1 = localPose1;

        this->type = type;
        this->compliance = 0.f;
        this->rotDamping = 0.f;
        this->posDamping = 0.f;
        this->hasSwingLimits = false;
        this->minSwingAngle = -float(2.0 * M_PI);
        this->maxSwingAngle = +float(2.0 * M_PI);
        this->swingLimitsCompliance = 0.f;
        this->hasTwistLimits = false;
        this->minTwistAngle = -float(2.0 * M_PI);
        this->maxTwistAngle = +float(2.0 * M_PI);
        this->twistLimitCompliance = 0.f;
    }

    void updateGlobalPoses()
    {
        this->globalPose0 = this->localPose0;
        if (this->body0)
            this->body0->pose.transformPose(this->globalPose0);
            
        this->globalPose1 = this->localPose1;
        if (this->body1)
            this->body1->pose.transformPose(this->globalPose1);
    }

    void solvePos(const float dt)
    {
        this->updateGlobalPoses();

        // orientation

        if (this->type == JointType::FIXED)
        {
        #if 0
            Quat & q = globalPose0.q;
		#else
			Quat q = globalPose0.q; // fixme : this is more correct?
		#endif
            q = q.calcConjugate();
            q = globalPose1.q * q;
            
            Vec3 omega(
				2.f * q.m_xyz[0],
				2.f * q.m_xyz[1],
				2.f * q.m_xyz[2]);
            if (q.m_w < 0.f) // fixme : this reads omega.w in the original source code..
                omega = -omega;
                
            applyBodyPairCorrection(body0, body1, omega, this->compliance, dt);
        }

        if (this->type == JointType::HINGE)
        {
            // align axes
                  Vec3 a0 = getQuatAxis0(this->globalPose0.q);
            const Vec3 b0 = getQuatAxis1(this->globalPose0.q);
            const Vec3 c0 = getQuatAxis2(this->globalPose0.q);
            const Vec3 a1 = getQuatAxis0(this->globalPose1.q);
            
            a0 = a0 % a1;
            
            applyBodyPairCorrection(this->body0, this->body1, a0, 0.f, dt);

            // limits
            if (this->hasSwingLimits)
            {
                this->updateGlobalPoses();
                
                const Vec3 n  = getQuatAxis0(this->globalPose0.q);
                const Vec3 b0 = getQuatAxis1(this->globalPose0.q);
                const Vec3 b1 = getQuatAxis1(this->globalPose1.q);
                
                limitAngle(
					this->body0,
					this->body1,
					n, b0, b1,
                    this->minSwingAngle,
                    this->maxSwingAngle,
                    this->swingLimitsCompliance,
                    dt);
            }
        }

        if (this->type == JointType::SPHERICAL)
        {
            // swing limits
            if (this->hasSwingLimits)
            {
                this->updateGlobalPoses();
                
                const Vec3 a0 = getQuatAxis0(this->globalPose0.q);
                const Vec3 a1 = getQuatAxis0(this->globalPose1.q);
                
                const Vec3 n = (a0 % a1).CalcNormalized();
                
                limitAngle(
					this->body0,
					this->body1,
					n, a0, a1,
                    this->minSwingAngle,
                    this->maxSwingAngle,
                    this->swingLimitsCompliance,
                    dt);
            }
            
            // twist limits
            if (this->hasTwistLimits)
            {
                this->updateGlobalPoses();
                
                const Vec3 n0 = getQuatAxis0(this->globalPose0.q);
                const Vec3 n1 = getQuatAxis0(this->globalPose1.q);
                
                const Vec3 n = (n0 + n1).CalcNormalized();
                
                Vec3 a0 = getQuatAxis1(this->globalPose0.q);
                a0 += n * (-n * a0);
                a0.Normalize();
                
                Vec3 a1 = getQuatAxis1(this->globalPose1.q);
                a1 += n * (-n * a1);
                a1.Normalize();

                // handling gimbal lock problem
			// fixme : what is intended here ? should always be multiplied by dt or not ?
                //const float maxCorr = (n0 * n1 > -.5f ? float(2.0 * M_PI) : 1.f) * dt;
                const float maxCorr = n0 * n1 > -.5f ? float(2.0 * M_PI) : 1.f * dt;
               
                limitAngle(
					this->body0,
					this->body1,
					n, a0, a1,
                    this->minTwistAngle,
                    this->maxTwistAngle,
                    this->twistLimitCompliance,
                    dt,
                    maxCorr);
            }
        }

        // position
        
        // simple attachment

        this->updateGlobalPoses();
        
        const Vec3 corr = this->globalPose1.p - this->globalPose0.p;
        
        applyBodyPairCorrection(
			this->body0,
			this->body1,
			corr,
			this->compliance,
			dt,
            &this->globalPose0.p,
            &this->globalPose1.p);
    }

    void solveVel(const float dt)
    {
        // Gauss-Seidel lets us make damping unconditionally stable in a
        // very simple way. We clamp the correction for each constraint
        // to the magnitude of the currect velocity making sure that
        // we never subtract more than there actually is.

        if (this->rotDamping > 0.f)
        {
            Vec3 omega;
            if (this->body0)
                omega -= this->body0->omega;
            if (this->body1)
                omega += this->body1->omega;
            
            omega *= fminf(1.f, this->rotDamping * dt);
            
            applyBodyPairCorrection(
				this->body0,
				this->body1,
				omega,
				0.f,
				dt,
				nullptr,
				nullptr,
				true);
        }
        
        if (this->posDamping > 0.f)
        {
            this->updateGlobalPoses();
            
            Vec3 vel;
            if (this->body0)
                vel -= this->body0->getVelocityAt(this->globalPose0.p);
            if (this->body1)
                vel += this->body1->getVelocityAt(this->globalPose1.p);
            
            vel *= fminf(1.f, this->posDamping * dt);
            
            applyBodyPairCorrection(
				this->body0,
				this->body1,
				vel,
				0.f,
				dt,
				&this->globalPose0.p,
				&this->globalPose1.p,
				true);
        }
    }
};

// Simulate -----------------------------------------------------------

static void simulate(
	Body ** bodies,
	const int numBodies,
	Joint ** joints,
	const int numJoints,
	const float timeStep,
	const int numSubsteps,
	Vec3Arg gravity)
{
    const float dt = timeStep / numSubsteps;

    for (auto i = 0; i < numSubsteps; i++)
    {
        for (auto j = 0; j < numBodies; j++)
            bodies[j]->integrate(dt, gravity);

        for (auto j = 0; j < numJoints; j++)
            joints[j]->solvePos(dt);

        for (auto j = 0; j < numBodies; j++)
            bodies[j]->update(dt);

        for (auto j = 0; j < numJoints; j++)
            joints[j]->solveVel(dt);
    }
}

//

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	// physics scene

	Vec3 gravity = Vec3(0.0, -10.0, 0.0);
	int numSubsteps = 40;

	int numObjects = 100;
	Vec3 objectsSize = Vec3(0.02, 0.04, 0.02);
	Vec3 lastObjectsSize = Vec3(0.2, 0.04, 0.2);

	float rotDamping = 1000.0;
	float posDamping = 1000.0;

	std::vector<Body*> bodies;
	std::vector<Joint*> joints;

	// create objects  -----------------------------------------------------------

	auto createObjects = [&]()
	{
		
		Vec3 pos = Vec3(0.0, (numObjects * objectsSize[1] + lastObjectsSize[1]) * 1.4 + 0.2, 0.0);
		Pose pose;
		Body * lastBody = nullptr;
		Pose jointPose0;
		Pose jointPose1;
		jointPose0.q.fromAxisAngle(Vec3(0.0, 0.0, 1.0), float(0.5 * M_PI));
		jointPose1.q.fromAxisAngle(Vec3(0.0, 0.0, 1.0), float(0.5 * M_PI));
		auto lastSize = objectsSize;
						
		for (int i = 0; i < numObjects; i++)
		{

			auto size = i < numObjects - 1 ? objectsSize : lastObjectsSize;

			// graphics

			Mesh * boxVis = new Mesh;
			boxVis->scale = size;

			// physics

			pose.p.Set(pos[0], pos[1] - i * objectsSize[1], pos[2]);

			auto * boxBody = new Body(pose, boxVis);
			boxBody->setBox(size);
			bodies.push_back(boxBody);
								
			auto s = i % 2 == 0 ? -0.5 : 0.5;
			jointPose0.p.Set(s * size[0], 0.5 * size[1], s * size[2]);
			jointPose1.p.Set( s * lastSize[0], -0.5 * lastSize[1], s * lastSize[2]);

			if (!lastBody) {
				jointPose1 = jointPose0;
				jointPose1.p += pose.p;
			}

			auto * joint = new Joint(JointType::SPHERICAL, boxBody, lastBody, jointPose0, jointPose1);
			joint->rotDamping = rotDamping;
			joint->posDamping = posDamping;
			joints.push_back(joint);
			
			lastBody = boxBody;
			lastSize = size;
		}
	};

	createObjects();
	
	Camera3d camera;
	camera.position.Set(0, 4, -4);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
			
		//const float timeStep = fmaxf(1.f / 60.f, framework.timeStep);
		const float timeStep = 1.f / 60.f;
		
		simulate(bodies.data(), bodies.size(), joints.data(), joints.size(), timeStep, numSubsteps, gravity);
		
		camera.tick(timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			camera.pushViewMatrix();
			{
				// todo : render meshes
				
				gxPushMatrix();
				{
					gxScalef(4, 4, 4);
					
					pushLineSmooth(false);
					setColor(100, 100, 100);
					drawGrid3dLine(40, 40, 0, 2, true);
					popLineSmooth();
				}
				gxPopMatrix();
				
				pushShaderOutputs("n");
				for (auto * body : bodies)
				{
					auto * mesh = body->mesh;
					
					gxPushMatrix();
					{
						gxTranslatef(mesh->position[0], mesh->position[1], mesh->position[2]);
						gxMultMatrixf(mesh->quaternion.toMatrix().m_v);
					
						setColor(colorWhite);
						fillCube(Vec3(), mesh->scale);
					}
					gxPopMatrix();
				}
				popShaderOutputs();
			}
			camera.popViewMatrix();
			
			popDepthTest();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
